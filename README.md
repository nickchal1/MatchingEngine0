# MatchingEngine0

A C++20 limit order book / matching engine, built from scratch as a learning project focused on systems-level performance work — not just correctness, but understanding and quantifying *why* a given implementation is fast or slow, and iterating on that with real benchmark evidence.

This document covers the current architecture, known limitations, the benchmark suite, how to build and run everything, and the roadmap for what comes next.

---

## 1. Architecture / Technical Spec

### Core types

```
OrderId   = uint64_t   // no arithmetic performed on it, unsigned is safe
Price     = int32_t    // fixed-point tick integer, no floating point
Quantity  = int64_t    // can get large for cheap/penny stocks
Side      = enum class { Buy, Sell }
```

Prices and quantities are deliberately integer types, not floats — this avoids floating-point comparison bugs in the matching logic, which is a common correctness pitfall in naive order book implementations.

- **`Order`** — `{ id, quantity, price, side }`. A plain aggregate struct (no invariants beyond field values, so no need for a class with an API).
- **`Trade`** — `{ makerId, takerId, executionQuantity, executionPrice, takerSide }`. Represents one fill. `executionPrice` is always the *maker's* (resting order's) price — i.e., the aggressor gets price improvement, matching standard exchange convention.
- **`SubmissionResult`** — `{ orderId, trades }`. Returned from every `addOrder` call; `trades` is empty if the order rested without matching.

### `OrderBook` internals (current / v0)

```cpp
std::map<Price, std::vector<OrderId>> m_bids;
std::map<Price, std::vector<OrderId>> m_asks;
std::unordered_map<OrderId, Order>    m_idToOrder;
```

- Price levels are kept in a `std::map`, giving sorted access to best bid (`m_bids.rbegin()`) and best ask (`m_asks.begin()`) in O(log n) where n = number of distinct price levels.
- Within a price level, orders are stored in a `std::vector<OrderId>`, appended via `push_back` (so vector order == arrival order == FIFO / time priority). The front of the vector (`[0]`) is always the oldest resting order at that level.
- `m_idToOrder` gives O(1) average lookup from an external `OrderId` to the full `Order` by value.

### Matching algorithm

Standard price-time priority: an incoming order walks the opposite side's price levels from best to worst, and within each level, matches strictly in arrival order (oldest first). Matching stops once the incoming order is fully filled or no more price levels cross.

### Public API

```cpp
std::optional<SubmissionResult> addOrder(Quantity, Price, Side);
bool cancelOrder(OrderId);
void printOrderBook() const;
std::optional<Price> bestBid() const;
std::optional<Price> bestAsk() const;
std::optional<Order> findOrder(OrderId) const;
```

### Design decisions worth noting

- **Explicit initialization discipline**: every `Order` field is explicitly assigned on construction rather than relying on default member initializers, to avoid any risk of garbage/uninitialized state.
- **Price improvement on fills**: trades always execute at the resting order's price, never the aggressor's — verified correct in code review.
- **No self-trade prevention, no participant/client concept** — orders are anonymous internal IDs with no notion of "whose" order it is. Fine for a single-process benchmarking/matching core; would need addressing before any multi-client / networked version.

---

## 2. Current Limitations (v0)

- Single-threaded, no concurrency.
- Limit orders only — no market orders, IOC/FOK, or order modify/replace.
- No self-trade prevention.
- No networking, no market data feed, no persistence/audit log.
- **`cancelOrder` and the match-loop's full-fill path are both O(k) in the depth of the price level being touched.** This is the central finding of the benchmark suite below, and the primary target of the v1 rewrite (see Roadmap).

---

## 3. Build & Run

Requires CMake 3.20+ and a C++20 compiler.

```bash
cd MatchingEngine0
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

This produces three targets:

| Target              | Source                            | Purpose                                   |
|---------------------|------------------------------------|--------------------------------------------|
| `matching_engine`   | `main.cpp`                         | Small interactive demo of the order book   |
| `orderbook_tests`   | `tests/OrderBookTests.cpp`         | GoogleTest correctness suite               |
| `latency_benchmark` | `benchmarks/OrderBookBenchmark.cpp`| The full performance benchmark suite       |

Run the demo:
```bash
./build/matching_engine
```

Run the tests:
```bash
ctest --test-dir build
```

Run the benchmark suite (this is what produces the results in Section 4 — expect it to take a while, since it sweeps multiple operations across multiple book depths):
```bash
./build/latency_benchmark
```

**Build type matters for benchmarking.** The benchmark suite must be run as a `Release` build (`-O3 -DNDEBUG`) — a `Debug`/unoptimized build will produce numbers that don't reflect real performance characteristics, and comparisons across implementations (v0 vs. v1 vs. v2) are only meaningful if every build being compared uses the same optimization level.

---

## 4. Benchmark Suite

### Methodology

All timing uses `std::chrono::steady_clock` (monotonic, immune to wall-clock adjustments). Two methodology points worth flagging up front, since they shaped how every benchmark here is built:

1. **The clock's actual resolution is ~42ns on this hardware, not 1ns.** `std::chrono` reports nanosecond-typed values, but the underlying hardware counter (on this Apple Silicon Mac, an ARM generic timer ticking at ~24MHz) can't resolve anything finer than ~41.7ns per tick. This was confirmed directly by probing `mach_timebase_info` and `clock_getres`. Any operation faster than this floor will alias to the same reported value regardless of its true cost — this is why the cheapest operations in the results below cluster at exactly 42ns.
2. **Every benchmark holds book depth pinned at a fixed value across all its samples**, rather than letting depth drift over the course of a run. This was a deliberate fix after an early version accidentally let depth grow to ~1,000,000 orders over a single benchmark run, producing wildly skewed percentiles that blended costs from many different depths together. Each function here does untimed setup/replenish around a *single, fixed* timed operation per sample, so every sample in a given run reflects the same book depth.

Percentiles are computed via nearest-rank on sorted samples (`p50`, `p90`, `p99`, `p99.9`), reported in nanoseconds.

### Benchmark functions

| Function          | What it isolates                                                                 |
|-------------------|------------------------------------------------------------------------------------|
| `clockBase`       | Clock-overhead calibration (back-to-back `now()` calls) — establishes the noise floor. |
| `addResting`      | Pure insert cost — one-sided book, guaranteed no match, depth pinned via add-then-cancel. |
| `addMatching`     | Full-fill match cost — aggressor always fully consumes exactly one resting order, depth pinned via replenish. |
| `cancelOldest`    | Cancel the front (oldest) order in the queue at a given depth — worst case for `vector::erase`. |
| `cancelNewest`    | Cancel the back (newest) order in the queue — was expected to be the cheap case (see Finding 3 below). |
| `cancelRandom`    | Cancel a uniformly random resting order — the realistic average case. |
| `mixedOperations` | Integration/realism check: a weighted random mix of add/cancel/match, pooled latency across all operation types, run at a fixed depth. |

All cancel/match benchmarks are swept across depths `{1, 100, 1,000, 10,000, 100,000}`; `addResting` additionally includes depth `0`. Sample count: 100,000 per (function, depth) combination.

### Results — v0 baseline (nanoseconds)

**`addResting`**

| depth   | p50 | p90 | p99 | p99.9  |
|---------|-----|-----|-----|--------|
| 0       | 42  | 84  | 167 | 250    |
| 1       | 41  | 42  | 42  | 125    |
| 100     | 42  | 42  | 125 | 959    |
| 1,000   | 42  | 42  | 125 | 500    |
| 10,000  | 42  | 83  | 458 | 6,292  |
| 100,000 | 42  | 42  | 958 | 12,459 |

**`addMatching`**

| depth   | p50    | p90    | p99    | p99.9  |
|---------|--------|--------|--------|--------|
| 1       | 83     | 84     | 167    | 292    |
| 100     | 83     | 84     | 84     | 166    |
| 1,000   | 167    | 167    | 209    | 292    |
| 10,000  | 1,167  | 1,250  | 1,334  | 6,083  |
| 100,000 | 15,708 | 16,500 | 19,500 | 29,375 |

**`cancelOldest`**

| depth   | p50    | p90    | p99    | p99.9   |
|---------|--------|--------|--------|---------|
| 1       | 83     | 84     | 125    | 167     |
| 100     | 42     | 42     | 42     | 125     |
| 1,000   | 125    | 167    | 208    | 250     |
| 10,000  | 1,167  | 1,250  | 1,333  | 3,292   |
| 100,000 | 16,334 | 16,667 | 34,750 | 198,916 |

**`cancelNewest`**

| depth   | p50    | p90    | p99    | p99.9   |
|---------|--------|--------|--------|---------|
| 1       | 83     | 84     | 125    | 209     |
| 100     | 83     | 84     | 125    | 208     |
| 1,000   | 375    | 417    | 1,000  | 1,542   |
| 10,000  | 3,250  | 3,417  | 3,625  | 12,791  |
| 100,000 | 31,958 | 33,667 | 40,125 | 176,000 |

**`cancelRandom`**

| depth   | p50    | p90    | p99    | p99.9  |
|---------|--------|--------|--------|--------|
| 1       | 83     | 84     | 84     | 125    |
| 100     | 83     | 84     | 125    | 209    |
| 1,000   | 291    | 375    | 417    | 459    |
| 10,000  | 2,292  | 3,125  | 3,375  | 3,958  |
| 100,000 | 24,166 | 30,958 | 34,000 | 68,834 |

**`mixedOperations`** (50% add / 45% cancel / 5% match, pooled)

| depth   | p50    | p90    | p99     | p99.9   |
|---------|--------|--------|---------|---------|
| 1       | 42     | 84     | 125     | 167     |
| 100     | 42     | 84     | 125     | 333     |
| 1,000   | 125    | 292    | 375     | 459     |
| 10,000  | 1,166  | 2,833  | 3,291   | 6,125   |
| 100,000 | 15,542 | 30,959 | 153,208 | 518,916 |

### Key findings

1. **`addResting`'s median is flat (~42ns) across every depth tested, but its tail grows ~50x from depth 0 to depth 100,000.** This matches `push_back`'s amortized-O(1) behavior exactly: most inserts are cheap regardless of size, but the rare reallocation event copies the entire current vector, so it gets more expensive as the vector grows — visible only in the tail, never the median.

2. **`addMatching` and `cancelOldest` show the core O(k) bottleneck directly in the median, not just the tail** — a ~190x slowdown in typical-case latency from shallow depth to depth 100,000. Every full-fill match and every cancel of the oldest order requires `vector::erase` at the front of the price level's vector, which shifts every remaining element down by one. This is the primary target of the v1 rewrite.

3. **`cancelNewest` was expected to stay cheap (erasing the back of a vector is O(1)) — it doesn't, and grows almost identically to `cancelOldest`.** The reason: `cancelOrder`'s implementation calls `std::find()` to *locate* the order before it erases it. Finding the newest order means linearly scanning the entire vector from the front — so even though the erase itself is cheap, the search that precedes it is O(depth). `cancelOldest` and `cancelNewest` are both O(depth) overall, for opposite reasons (cheap find + expensive erase, vs. expensive find + cheap erase).

4. **`cancelRandom`'s numbers land almost exactly on the average of `cancelOldest` and `cancelNewest`,** which is strong independent confirmation of finding #3: at depth 100,000, `(16,334 + 31,958) / 2 = 24,146` vs. an actual measured value of 24,166 — within 0.1%. At depth 10,000, the predicted average is 2,208.5 vs. an actual 2,292.

5. **`mixedOperations` produces the single worst tail latency in the entire dataset — 518,916ns at depth 100,000, p99.9** — worse than any isolated benchmark's tail at the same depth. This is expected and is the actual value of having an integration benchmark: it can experience the worst of everything compounding at once (an expensive cancel landing right after a vector reallocation, for example), which no single isolated benchmark can reveal on its own.

---

## 5. Roadmap

### v1 — intrusive linked list per price level (next)

Replace `std::vector<OrderId>` per price level with an intrusive doubly linked list (`Order` gains `prev`/`next` pointers), and change `m_idToOrder` to map `OrderId → Order*` directly. This is designed to fix **both** problems findings #2 and #3 exposed, not just the erase cost: since `m_idToOrder` gives a direct pointer to the node, `cancelOrder` no longer needs to scan anything at all — it jumps straight to the node in O(1) and unlinks it via its own `prev`/`next`, regardless of whether it's the oldest, newest, or a random order in the queue. Expected result: `cancelOldest`, `cancelNewest`, and `cancelRandom` should all collapse down to roughly the same flat, cheap number across all depths, and `addMatching`'s full-fill path should show the same improvement.

Orders will need to be heap-allocated (or pool-allocated, see v2) for pointer stability, which means `OrderBook` needs a proper destructor to avoid leaking remaining nodes — this version should be validated under AddressSanitizer.

### v2 — preallocated order pool + index-based intrusive links

Replace individual `new`/`delete` per order with a preallocated contiguous pool and a free-list, and replace raw `prev`/`next` pointers with 32-bit indices into that pool. This removes the general-purpose allocator from the hot path entirely (relevant: a single heap allocation was likely the dominant cost in some of the `addMatching` measurements above, via `SubmissionResult::trades`'s first `push_back`), and improves cache locality since orders live in one contiguous block rather than scattered across the heap. Also the point at which a **price ladder** (flat array indexed by price tick, replacing `std::map` for the price-level axis) becomes worth adding — that's a separate axis from what v1 fixes (number of distinct price levels vs. per-level depth), so it needs its own benchmark family (sweeping price-level count while holding per-level depth fixed) rather than reusing the v1 depth sweeps.

### Beyond v2

- **Networking**: order-entry and market-data protocols modeled on real exchange formats (Nasdaq OUCH for order entry, ITCH for market data), rather than an ad hoc wire format.
- **Real order feeds**: a replay server that streams historical ITCH data over a socket, with the engine as a client-side feed handler — doubles as the historical-replay environment an eventual RL agent would need.
- **Concurrency**: single-threaded matching core (the "single writer principle" — how real matching engines avoid locking-related latency and correctness bugs), with concurrency handled at the I/O boundary via a lock-free queue feeding the core, not by parallelizing the book itself.
- **RL market-making agent**: once the networking layer exists, an RL agent can plug in as just another client speaking the same order-entry protocol as anything else.
