# MatchingEngine0

A single-threaded **C++20 limit order book and matching engine**, built to explore matching rules, data-structure design, memory layout, and measurable performance tradeoffs.

The engine implements price-time priority, partial fills, cancellation, and maker-price trade reporting. Its current design combines ordered price maps, hash-based order lookup, and intrusive doubly linked FIFO queues.

In a repeated comparison against the original vector implementation, the current engine processed **22.8 million API operations/s**, a **136× throughput improvement**, on a synthetic mixed workload with approximately **100,000 orders at one price level**. The workload, hardware, and measurement boundaries are documented below.

Documentation updated 23 September 2026. Measurements are from the 21 September audit. The implementation at `0009731` is unchanged from the measured `2ee2fa6`; the intervening commit updated only documentation.

**Implemented functionality**

- Buy and sell limit orders, with validation of quantity, price, and side.
- Price priority across levels and FIFO arrival priority within each level.
- Exact fills, partial fills, and submissions that consume multiple orders or price levels.
- Execution at the resting maker's price, with unmatched incoming limit quantity added to the book.
- Cancellation by order ID and removal of empty price levels.
- Best bid, best ask, and resting-order lookup.
- Structured submission results and trade records.
- GoogleTest coverage, CMake/CTest integration, and GitHub Actions build/test configuration.

Each `OrderBook` instance represents one instrument. There is no symbol-routing layer or concurrent access to a shared book.

**Build and run**

Requirements: CMake 3.20+, a C++20 compiler, and Git/network access for the default GoogleTest dependency fetch. The warning flags and CI configuration target GCC/Clang-style toolchains.

From the repository root:

```sh
cmake -S . -B build-fresh -DCMAKE_BUILD_TYPE=Release
cmake --build build-fresh --parallel
ctest --test-dir build-fresh --output-on-failure
./build-fresh/matching_engine
./build-fresh/latency_benchmark
```

Use a fresh build directory if an existing CMake cache belongs to another filesystem location. Tests can be omitted with `-DBUILD_TESTING=OFF`.

| Target | Purpose |
| --- | --- |
| `orderbook` | Static matching-engine library |
| `matching_engine` | Scripted example of matching and cancellation |
| `orderbook_tests` | GoogleTest suite; built when `BUILD_TESTING=ON` |
| `latency_benchmark` | Tracked per-operation latency benchmark |

The demo is scripted, not interactive. It submits asks for 60 at 10,200 and 50 at 10,300, then a buy for 100 at 10,300. This generates fills of 60 at 10,200 and 40 at 10,300, leaving 10 units on the second ask before cancellation. Book-print calls are currently disabled; trade output remains available.

**Using the engine**

```cpp
#include "OrderBook.h"
#include <iostream>

int main() {
    OrderBook book;

    const auto ask = book.addOrder(60, 10'200, Side::Sell);
    if (!ask) return 1;

    const auto buy = book.addOrder(40, 10'300, Side::Buy);
    if (!buy) return 1;

    for (const Trade& trade : buy->trades) {
        std::cout << "maker=" << trade.makerId
                  << " quantity=" << trade.executionQuantity
                  << " price=" << trade.executionPrice << '\n';
    }
    // One trade: 40 units at the resting ask's price of 10,200.

    const auto remaining = book.findOrder(ask->orderId);
    if (remaining) {
        std::cout << "remaining=" << remaining->quantity << '\n'; // 20
    }

    return book.cancelOrder(ask->orderId) ? 0 : 1;
}
```

The working public API is:

```cpp
std::optional<SubmissionResult> addOrder(Quantity quantity, Price price, Side side);
bool cancelOrder(OrderId id);
std::optional<Price> bestBid() const;
std::optional<Price> bestAsk() const;
std::optional<Order> findOrder(OrderId id) const;
```

`addOrder()` rejects nonpositive prices/quantities and invalid sides with `std::nullopt`; these validation failures do not consume an ID. Successful submissions return an assigned order ID and a vector of trades. Fully filled orders do not remain in the lookup index. `findOrder()` returns a copy of the resting order, not a mutable reference into the book.

`printOrderBook()` is also declared in the header, but its definition is commented out. Calling it currently fails to link.

**Data model and ownership**

| Type | Representation | Purpose |
| --- | --- | --- |
| `OrderId` | `std::uint64_t` | Book-assigned identifier |
| `Price` | `std::int32_t` | Integer price units chosen by the caller |
| `Quantity` | `std::int64_t` | Order quantity |
| `Side` | `enum class { Buy, Sell }` | Order direction |
| `Order` | ID, quantity, price, side | Order record |
| `Trade` | Maker/taker IDs, execution quantity/price, taker side | One execution |
| `SubmissionResult` | Order ID and `std::vector<Trade>` | Submission outcome |

Integer prices avoid floating-point comparisons in matching. The engine does not prescribe a currency scale or enforce an instrument-specific tick size. IDs are incremented for accepted submissions; exhaustion is not currently handled.

Current storage:

```cpp
struct RestingOrder {
    Order order;
    RestingOrder* prev;
    RestingOrder* next;
    // Constructor initializes both links to nullptr.
};

struct PriceLevel {
    RestingOrder* head;
    RestingOrder* tail;
    Quantity totalQuantity;
    // Constructor initializes an empty level.
};

std::unordered_map<OrderId, RestingOrder> m_idToOrder;
std::map<Price, PriceLevel> m_bids;
std::map<Price, PriceLevel> m_asks;
```

The hash map owns resting orders by value. FIFO links point into its nodes; rehashing does not invalidate pointers to existing elements. The price maps own price-level records. Container destruction releases their storage; the raw links do not imply separately owned heap allocations.

The best bid is the highest bid level, and the best ask is the lowest ask level. Matching visits the opposite side from best to worst, consumes its FIFO head, updates quantities, and removes fully filled orders and empty levels.

**Evolution and complexity**

The baseline used `std::map<Price, std::vector<OrderId>>`. Cancelling an order required searching the vector and erasing an element; consuming the FIFO head shifted the remaining IDs. Both become expensive as the queue at one price grows.

The current implementation uses the ID index to reach an order directly, then updates its neighboring links. Let **N** be orders at a price and **P** distinct price levels:

| Operation | Vector baseline | Current intrusive queues |
| --- | --- | --- |
| ID lookup | Expected O(1) | Expected O(1) |
| Price-level lookup | O(log P) | O(log P) |
| Remove a known order within its level | O(N) search/shift | O(1) unlink |
| Remove a filled queue head | O(N) shift | O(1) unlink |
| Append within a level | Amortized O(1), possible vector growth | O(1) linking |

The full cancellation API still performs a price-map lookup; O(1) applies to queue unlinking. Insertions still allocate hash nodes, and trade vectors can allocate during matching. Best-price access uses the extreme of an already ordered map rather than searching the tree for a price.

Key milestones were order layout in July, matching and trade reports in August, benchmark/test expansion in mid-August, and intrusive-queue integration at the end of August and start of September. There are two implementation stages here, not a contiguous-pool or direct-price-ladder implementation.

**Memory layout and allocation tradeoffs**

Reordering `Order` from `{id, price, quantity, side}` to `{id, quantity, price, side}` reduced its measured arm64 size from **32 to 24 bytes**, a **25% reduction**. This saves eight bytes per raw record, or about 7.63 MiB per million records before container overhead. Both compared benchmark versions already include this layout change.

The intrusive queue adds two pointers per resting record while removing the separate vector of IDs. It does not eliminate dynamic allocation. A separate allocation probe at 100,000-order depth found:

| 100,000 add/cancel pairs, 200,000 API operations | Baseline | Current |
| --- | --- | --- |
| C++ allocation calls | 100,000 | 100,000 |
| Allocations per operation | 0.50 | 0.50 |
| Cumulative bytes requested | 4.8 MB | 6.4 MB |

These are requested allocation bytes, not peak memory or resident-set size. The current version requests larger order nodes. Performance improvements should not be described as an allocation-free hot path or a general memory-footprint reduction.

**Tests and verification**

The repository contains **33 Google Tests**: 19 in `OrderBookValidation` and 14 in `OrderBookMatching`. They cover input validation, IDs, empty/resting state, cancellation, crossing rules, partial/exact fills, maker-price execution, multi-level price priority, and FIFO on both sides.

The 21 September audit measured:

| Existing suite metric | Result |
| --- | --- |
| Assertion macro sites | 103 |
| Assertion evaluations in one passing run | 155 |
| Executable-line coverage of `OrderBook.cpp` | 171/180 = **95.00%** |
| Branch-outcome coverage of `OrderBook.cpp` | 62/66 = **93.94%** |
| Defined functions exercised in `OrderBook.cpp` | 9/9 |

Both vector and intrusive implementations passed the unchanged suite under AddressSanitizer and UndefinedBehaviorSanitizer. These coverage values apply to `OrderBook.cpp`, not the entire repository. Sanitizers and coverage were run during the audit; the checked-in GitHub Actions workflow configures an Ubuntu CMake build and CTest run for pushes to main and PRs targeting main.

A separate audit-only reference checker also tested each version against a simple independent implementation: **200,000 randomized events across 20 seeds**, plus **8,200 targeted events** exercising rehashing, tail/middle/head cancellation, and sweeps. Each version passed 19,379,969 checker predicates and 58,989 trade comparisons under sanitizers. This external checker is separate from the repository's 33-test suite.

**Tracked latency benchmark**

`benchmarks/OrderBookBenchmark.cpp` measures individual operations using `std::chrono::steady_clock` and reports nearest-rank p50, p90, p99, and p99.9 in nanoseconds.

| Scenario | Timed operation |
| --- | --- |
| `addResting` | Submit an order that rests; cancel outside the timed region |
| `addMatching` | Submit an order consuming one maker; replenish afterward |
| `cancelOldest` | Cancel the FIFO head; replenish afterward |
| `cancelNewest` | Cancel the newest order; replenish afterward |
| `cancelRandom` | Cancel a selected live order; replenish afterward |
| `mixedOperations` | Weighted 50% add / 45% cancel / 5% match sequence |

The suite runs **100,000 samples per configuration**, with depths 1, 100, 1,000, 10,000, and 100,000; resting adds also include depth 0. This gives **31 operation/depth configurations and 3.1 million timed API operations per run**, plus 100,000 clock-calibration samples.

The original mixed sequence uses a seeded random distribution, so its proportions are approximate and its depth can drift. The other scenarios replenish or remove orders between samples to control depth. Setup, result sorting, and printing are outside timed regions; operation boundaries are those in the source.

**Repeated baseline comparison**

The following results use the pre-existing throughput harness in the separate `MatchingEngine0-benchmark-lab` worktree. That harness and its saved results were uncommitted at the audit date and are not available through the main repository's default targets. This distinction matters for reproducing the figures from a fresh clone.

Measurement setup:

- Baseline: vector implementation at `2f9ac08`.
- Optimized: intrusive implementation at `2ee2fa6`.
- Hardware: Apple M5, 16 GiB RAM, arm64 macOS 25.6.0.
- Compiler: Apple Clang 21.0.0; identical `-std=c++20 -O3 -DNDEBUG` builds, without LTO or sanitizers.
- Three sequential runs per version, reversing version order in the middle repetition.
- For each batched configuration: 512 batches × 4,096 operations = **2,097,152 timed operations**, after **65,536 warmup operations**.
- Reported rates are medians of three run-level results; speedup is the ratio of median baseline and optimized mean times.

At approximately 100,000 orders at a single price:

| Workload | Baseline M ops/s | Current M ops/s | Speedup |
| --- | --- | --- | --- |
| Mixed: 50% add / 45% cancel / 5% match | 0.167 | 22.835 | 136.5× |
| Alternate add / cancel newest | 0.172 | 78.699 | 457.8× |
| Alternate match / replenish | 0.172 | 43.222 | 251.4× |
| Alternate random cancel / replenish | 0.156 | 23.990 | 153.9× |

The mixed workload's mean measured cost fell from **5,978.20 to 43.79 ns/op**, a **99.27% reduction**. Its current throughput ranged from **22.744M to 22.873M operations/s** across the three runs. This lab scenario repeats a seeded, shuffled 100-operation schedule; depth fluctuates within a block and stays near its initial value.

Rates count in-process API operations. In a match/replenish pair, matching and replenishment each count as one operation. They do not measure network throughput or trades per second. RNG, checksums, validation, and ID-tracker bookkeeping are included where present in the timed driver loop; parsing and output I/O are excluded.

The gains are workload dependent:

| Additional workload | Baseline mean | Current mean | Observation |
| --- | --- | --- | --- |
| One submission consuming 1,000 makers at one price | 50.93 μs | 11.52 μs | **4.42× faster** |
| One submission consuming makers across 1,000 prices | 50.77 μs | 47.82 μs | **1.06× faster** |
| Add/cancel lookup across 10,000 price levels | 63.47 ns/op | 71.53 ns/op | **12.70% more time** |
| Add/cancel lookup across 100,000 price levels | 115.77 ns/op | 117.83 ns/op | **1.77% more time** |

These results distinguish deep queues from many distinct price levels. The intrusive rewrite addresses queue traversal and shifts; the price map remains a tree. The measured slowdowns do not establish a specific cache-related cause without profiling.

The original, individually timed latency suite also recorded random-cancel p99 falling from **14,541 to 125 ns** at 100,000-order depth, a **99.14% reduction** in the median reported p99. Optimized p99 ranged from 84 to 208 ns across runs.

**Interpreting the measurements**

The measured Mac timer scale was approximately **41.67 ns per tick**. A 0 ns sample does not indicate zero execution cost, and values around 42 ns are quantized. Batched timing provides useful average costs below one tick by measuring many operations together.

For batched throughput rows, reported percentiles are percentiles of **batch averages**, each covering 4,096 operations. They are not individual-operation p99/p99.9 latency. The fanout rows and original latency suite time individual submissions/operations instead.

The audit ran both suites three times for each version, totaling **198,504,768 timed engine API operations**, excluding driver controls, warmup, and untimed setup. This total describes the verification campaign, not one workload. Results were collected on a laptop without CPU pinning; they are observations for the specified builds and workloads rather than general production guarantees.

**Known limitations**

- Limit orders and cancellation only: no market orders, IOC/FOK, modification, participant ownership, or self-trade prevention.
- No network layer, persistence, market-data parser, or concurrent book access.
- Copy operations remain implicitly enabled despite internal pointers. Copying a book can make operations on the copy affect the original. Do not copy an `OrderBook`; copy/move policy needs explicit implementation and tests.
- Aggregating individually valid quantities can overflow `Quantity`; an overflow policy or checked arithmetic is needed.
- Allocation failure during submission can leave the order index and price levels inconsistent; rollback/exception guarantees need work.
- `printOrderBook()` has no active definition, and order-ID exhaustion is not handled.

The existing tests cover normal matching behavior but do not establish correctness for all failure conditions. These limitations are tracked separately from the measured performance improvements.

**Next work**

1. Resolve copy semantics, aggregate overflow, exception safety, and the unfinished printing API; add focused regressions.
2. Bring the separate benchmark harness and reproducible comparison artifacts into the repository, with batch and per-operation percentiles labeled clearly.
3. Extend order rules or add a networking layer while preserving one owner of the matching core.

A contiguous order pool, generation-checked handles, and a direct-index price ladder remain possible experiments. They are not implemented features of this version.
