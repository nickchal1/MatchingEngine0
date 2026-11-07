#include "OrderBook.h"

#include <iostream>
#include <chrono>
#include <vector>
#include <cmath>
#include <algorithm>
#include <optional>
#include <cstdlib>
#include <random>

long long nearestRankPercentile(const std::vector<long long>& samples, double percentile) {

    long long rank = std::ceil(percentile * samples.size());
    
    return samples[rank - 1];
}

void percentilePrint(std::vector<long long> samples) {

    std::sort(samples.begin(), samples.end());

    long long p50 = nearestRankPercentile(samples, 0.50);
    long long p90 = nearestRankPercentile(samples, 0.90);
    long long p99 = nearestRankPercentile(samples, 0.99);
    long long p99_9 = nearestRankPercentile(samples, 0.999);

    std::cout<<"p50 "<<p50<<'\n';
    std::cout<<"p90 "<<p90<<'\n';
    std::cout<<"p99 "<<p99<<'\n';
    std::cout<<"p99.9 "<<p99_9<<'\n';
}

std::vector<long long> clockBase(size_t sampleCount) {
    std::vector<long long> samples;
    samples.reserve(sampleCount);
    for (std::size_t i = 0; i < sampleCount; ++i) {
        const auto start{std::chrono::steady_clock::now()};
        const auto finish{std::chrono::steady_clock::now()};
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start);
        const auto ns = elapsed.count();
        samples.push_back(ns);
    }
    return samples;
}
 
std::vector<long long> addResting(size_t sampleCount, size_t depth) {
    OrderBook book;
    std::vector<long long> samples;
    samples.reserve(sampleCount);

    for (std::size_t i = 0; i < depth; ++i) {
        book.addOrder(100, 10'000, Side::Buy);
    }
    
    for (std::size_t i = 0; i < sampleCount; ++i) {
        const auto start{std::chrono::steady_clock::now()};
        std::optional<SubmissionResult> res =  book.addOrder(100, 10'000, Side::Buy);
        const auto finish{std::chrono::steady_clock::now()};
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start);
        const auto ns = elapsed.count();
        samples.push_back(ns);

        //untimed
        book.cancelOrder(res->orderId);
    }
    return samples;
}

std::vector<long long> addMatching(size_t sampleCount, size_t depth) {
    OrderBook book;
    std::vector<long long> samples;
    samples.reserve(sampleCount);

    for (std::size_t i = 0; i < depth; ++i) {
        book.addOrder(100, 10'000, Side::Buy);
    }
    
    for (std::size_t i = 0; i < sampleCount; ++i) {
        const auto start{std::chrono::steady_clock::now()};
        book.addOrder(100, 10'000, Side::Sell);
        const auto finish{std::chrono::steady_clock::now()};
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start);
        const auto ns = elapsed.count();
        samples.push_back(ns);

        //untimed
        book.addOrder(100, 10'000, Side::Buy);
    }
    return samples;
}

std::vector<long long> cancelOldest(size_t sampleCount, size_t depth) {
    OrderBook book;
    std::vector<long long> samples;
    samples.reserve(sampleCount);

    for (std::size_t i = 0; i < depth; ++i) {
        book.addOrder(100, 10'000, Side::Buy);
    }

    for (std::size_t i = 0; i < sampleCount; ++i) {
        const auto start{std::chrono::steady_clock::now()};
        book.cancelOrder(i + 1);
        const auto finish{std::chrono::steady_clock::now()};
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start);
        const auto ns = elapsed.count();
        samples.push_back(ns);
        
        //untimed
        book.addOrder(100, 10'000, Side::Buy);
    }
    return samples;
}

std::vector<long long> cancelNewest(size_t sampleCount, size_t depth) {
    OrderBook book;
    std::vector<long long> samples;
    samples.reserve(sampleCount);

    for (std::size_t i = 0; i < depth; ++i) {
        book.addOrder(100, 10'000, Side::Buy);
    }

    for (std::size_t i = 0; i < sampleCount; ++i) {
        const auto start{std::chrono::steady_clock::now()};
        book.cancelOrder(depth + i);
        const auto finish{std::chrono::steady_clock::now()};
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start);
        const auto ns = elapsed.count();
        samples.push_back(ns);
        
        //untimed
        book.addOrder(100, 10'000, Side::Buy);
    }
    return samples;
}

std::vector<long long> cancelRandom(size_t sampleCount, size_t depth) {
    OrderBook book;
    std::vector<long long> samples;
    samples.reserve(sampleCount);
    std::vector<OrderId> validIds;

    for (std::size_t i = 0; i < depth; ++i) {
        std::optional<SubmissionResult> res = book.addOrder(100, 10'000, Side::Buy);
        validIds.push_back(res->orderId);
    }

    srand(2029);
    for (std::size_t i = 0; i < sampleCount; ++i) {
        size_t randomIndex = rand() % depth;
        const auto start{std::chrono::steady_clock::now()};
        book.cancelOrder(validIds[randomIndex]);
        const auto finish{std::chrono::steady_clock::now()};
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start);
        const auto ns = elapsed.count();
        
        //untimed
        samples.push_back(ns);
        std::optional<SubmissionResult> res = book.addOrder(100, 10'000, Side::Buy);
        std::swap(validIds[randomIndex], validIds[validIds.size() - 1]);
        validIds.pop_back();
        validIds.push_back(res->orderId);
    }
    return samples;
}

std::vector<long long> mixedOperations(size_t sampleCount, size_t depth) {
    OrderBook book;
    std::vector<long long> samples;
    samples.reserve(sampleCount);
    std::vector<OrderId> validIds;

    for (std::size_t i = 0; i < depth; ++i) {
        std::optional<SubmissionResult> res = book.addOrder(100, 10'000, Side::Buy);
        validIds.push_back(res->orderId);
    }
    //50% add, 45% cancel, 5% match
    std::mt19937 generator{2029};
    std::discrete_distribution<int> opDist{50, 45, 5};
    srand(2029);

    for (size_t i = 0; i < sampleCount; ++i) {
        int op = opDist(generator);

        //force add
        if (op != 0 && validIds.empty()) {
            op = 0;
        }
        std::chrono::steady_clock::time_point start;
        std::chrono::steady_clock::time_point finish;
        switch (op) {
            case 0: {
                start = std::chrono::steady_clock::now();
                std::optional<SubmissionResult> res = book.addOrder(100, 10'000, Side::Buy);
                finish = std::chrono::steady_clock::now();
                validIds.push_back(res->orderId);
                break;
            }
            case 1: {
                size_t randomIndex = rand() % validIds.size();
                start = std::chrono::steady_clock::now();
                book.cancelOrder(validIds[randomIndex]);
                finish = std::chrono::steady_clock::now();
                std::swap(validIds[randomIndex], validIds[validIds.size() - 1]);
                validIds.pop_back();
                break;
            }
            case 2: {
                start = std::chrono::steady_clock::now();
                std::optional<SubmissionResult> res = book.addOrder(100, 10'000, Side::Sell);
                finish = std::chrono::steady_clock::now();
                OrderId invalidId = res->trades[0].makerId;
                auto pos = std::find(validIds.begin(), validIds.end(), invalidId);
                std::swap(*pos, validIds[validIds.size() - 1]);
                validIds.pop_back();
                break;
            }
        }
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start);
        const auto ns = elapsed.count();
        samples.push_back(ns);
    }
    return samples;
}

void meanPrint(double nsPerOp) {

    std::cout<<"mean ns "<<nsPerOp<<'\n';
    std::cout<<"M ops/s "<<1'000.0 / nsPerOp<<'\n';
}

double addCancelThroughput(size_t opCount, size_t depth) {
    OrderBook book;
    const size_t warmup = 100'000;

    for (std::size_t i = 0; i < depth; ++i) {
        book.addOrder(100, 10'000, Side::Buy);
    }

    std::chrono::steady_clock::time_point start;
    OrderId lastId = 0;

    for (std::size_t i = 0; i < warmup + opCount; ++i) {
        if (i == warmup) {
            start = std::chrono::steady_clock::now();
        }
        //add then cancel it
        if (i % 2 == 0) {
            std::optional<SubmissionResult> res = book.addOrder(100, 10'000, Side::Buy);
            lastId = res->orderId;
        }
        else {
            book.cancelOrder(lastId);
        }
    }
    const auto finish{std::chrono::steady_clock::now()};
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start);
    return static_cast<double>(elapsed.count()) / opCount;
}

double matchThroughput(size_t opCount, size_t depth) {
    OrderBook book;
    const size_t warmup = 100'000;

    for (std::size_t i = 0; i < depth; ++i) {
        book.addOrder(100, 10'000, Side::Buy);
    }

    std::chrono::steady_clock::time_point start;

    for (std::size_t i = 0; i < warmup + opCount; ++i) {
        if (i == warmup) {
            start = std::chrono::steady_clock::now();
        }
        //match oldest buy then replenish
        if (i % 2 == 0) {
            book.addOrder(100, 10'000, Side::Sell);
        }
        else {
            book.addOrder(100, 10'000, Side::Buy);
        }
    }
    const auto finish{std::chrono::steady_clock::now()};
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start);
    return static_cast<double>(elapsed.count()) / opCount;
}

double cancelRandomThroughput(size_t opCount, size_t depth) {
    OrderBook book;
    const size_t warmup = 100'000;
    std::vector<OrderId> validIds;

    for (std::size_t i = 0; i < depth; ++i) {
        std::optional<SubmissionResult> res = book.addOrder(100, 10'000, Side::Buy);
        validIds.push_back(res->orderId);
    }

    srand(2029);
    std::chrono::steady_clock::time_point start;
    size_t randomIndex = 0;

    for (std::size_t i = 0; i < warmup + opCount; ++i) {
        if (i == warmup) {
            start = std::chrono::steady_clock::now();
        }
        //cancel random then replenish same slot
        if (i % 2 == 0) {
            randomIndex = rand() % depth;
            book.cancelOrder(validIds[randomIndex]);
        }
        else {
            std::optional<SubmissionResult> res = book.addOrder(100, 10'000, Side::Buy);
            validIds[randomIndex] = res->orderId;
        }
    }
    const auto finish{std::chrono::steady_clock::now()};
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start);
    return static_cast<double>(elapsed.count()) / opCount;
}

double mixedThroughput(size_t opCount, size_t depth) {
    OrderBook book;
    const size_t warmup = 100'000;
    std::vector<OrderId> validIds;
    //where each id sits in validIds
    std::vector<size_t> idToIndex(depth + warmup + opCount + 1);

    for (std::size_t i = 0; i < depth; ++i) {
        std::optional<SubmissionResult> res = book.addOrder(100, 10'000, Side::Buy);
        idToIndex[res->orderId] = validIds.size();
        validIds.push_back(res->orderId);
    }

    //50 add, 45 cancel, 5 match in a fixed shuffled order
    std::vector<int> schedule;
    for (int i = 0; i < 50; ++i) {
        schedule.push_back(0);
    }
    for (int i = 0; i < 45; ++i) {
        schedule.push_back(1);
    }
    for (int i = 0; i < 5; ++i) {
        schedule.push_back(2);
    }
    std::mt19937 generator{2029};
    std::shuffle(schedule.begin(), schedule.end(), generator);
    srand(2029);

    std::chrono::steady_clock::time_point start;

    for (std::size_t i = 0; i < warmup + opCount; ++i) {
        if (i == warmup) {
            start = std::chrono::steady_clock::now();
        }
        int op = schedule[i % schedule.size()];

        //force add
        if (op != 0 && validIds.empty()) {
            op = 0;
        }
        OrderId removedId = 0;
        switch (op) {
            case 0: {
                std::optional<SubmissionResult> res = book.addOrder(100, 10'000, Side::Buy);
                idToIndex[res->orderId] = validIds.size();
                validIds.push_back(res->orderId);
                break;
            }
            case 1: {
                removedId = validIds[rand() % validIds.size()];
                book.cancelOrder(removedId);
                break;
            }
            case 2: {
                std::optional<SubmissionResult> res = book.addOrder(100, 10'000, Side::Sell);
                removedId = res->trades[0].makerId;
                break;
            }
        }
        //swap with last and pop
        if (op != 0) {
            size_t index = idToIndex[removedId];
            OrderId lastId = validIds[validIds.size() - 1];
            validIds[index] = lastId;
            idToIndex[lastId] = index;
            validIds.pop_back();
        }
    }
    const auto finish{std::chrono::steady_clock::now()};
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start);
    return static_cast<double>(elapsed.count()) / opCount;
}

double addCancelLevelsThroughput(size_t opCount, size_t levels) {
    OrderBook book;
    const size_t warmup = 100'000;

    //one order per price level
    for (std::size_t i = 0; i < levels; ++i) {
        book.addOrder(100, static_cast<Price>(1'000 + i), Side::Buy);
    }

    srand(2029);
    std::chrono::steady_clock::time_point start;
    OrderId lastId = 0;

    for (std::size_t i = 0; i < warmup + opCount; ++i) {
        if (i == warmup) {
            start = std::chrono::steady_clock::now();
        }
        //add at random level then cancel it
        if (i % 2 == 0) {
            Price price = static_cast<Price>(1'000 + rand() % levels);
            std::optional<SubmissionResult> res = book.addOrder(100, price, Side::Buy);
            lastId = res->orderId;
        }
        else {
            book.cancelOrder(lastId);
        }
    }
    const auto finish{std::chrono::steady_clock::now()};
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start);
    return static_cast<double>(elapsed.count()) / opCount;
}

double sweepMean(size_t sampleCount, size_t makers, bool acrossLevels) {
    OrderBook book;
    long long totalNs = 0;

    for (std::size_t i = 0; i < sampleCount; ++i) {
        //untimed
        for (std::size_t m = 0; m < makers; ++m) {
            Price price = acrossLevels ? static_cast<Price>(10'000 + m) : 10'000;
            book.addOrder(100, price, Side::Buy);
        }

        const auto start{std::chrono::steady_clock::now()};
        book.addOrder(static_cast<Quantity>(100 * makers), 10'000, Side::Sell);
        const auto finish{std::chrono::steady_clock::now()};
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start);
        totalNs += elapsed.count();
    }
    return static_cast<double>(totalNs) / sampleCount;
}

int main() {

    std::cout<<"Matching Engine Latency Benchmark\n"; 

    const std::size_t sampleCount = 100'000;
    std::vector<size_t> depths{0, 1, 100, 1'000, 10'000, 100'000};

    //CLOCK
    std::cout<<"Clock samples\n";
    std::vector<long long> clockSamples;
    clockSamples = clockBase(sampleCount);
    percentilePrint(clockSamples);
    
    //RESTING
    for (size_t depth : depths) {
        std::cout<<"addResting depth: "<<depth<<'\n';
        percentilePrint(addResting(sampleCount, depth));
    }
    depths = {1, 100, 1'000, 10'000, 100'000};
    //MATCHING
    for (size_t depth : depths) {
        std::cout<<"addMatching depth: "<<depth<<'\n';
        percentilePrint(addMatching(sampleCount, depth));
    }
    //CANCEL OLDEST
    for (size_t depth : depths) {
        std::cout<<"cancelOldest depth: "<<depth<<'\n';
        percentilePrint(cancelOldest(sampleCount, depth));
    }
    //CANCEL NEWEST
    for (size_t depth : depths) {
        std::cout<<"cancelNewest depth: "<<depth<<'\n';
        percentilePrint(cancelNewest(sampleCount, depth));
    }
    //CANCEL RANDOM
    for (size_t depth : depths) {
        std::cout<<"cancelRandom depth: "<<depth<<'\n';
        percentilePrint(cancelRandom(sampleCount, depth));
    }
    //MIXED OPERATIONS
    for (size_t depth : depths) {
        std::cout<<"mixedOperations depth: "<<depth<<'\n';
        percentilePrint(mixedOperations(sampleCount, depth));
    }

    //THROUGHPUT
    const size_t opCount = 2'000'000;
    for (size_t depth : depths) {
        std::cout<<"addCancel throughput depth: "<<depth<<'\n';
        meanPrint(addCancelThroughput(opCount, depth));
    }
    for (size_t depth : depths) {
        std::cout<<"match throughput depth: "<<depth<<'\n';
        meanPrint(matchThroughput(opCount, depth));
    }
    for (size_t depth : depths) {
        std::cout<<"cancelRandom throughput depth: "<<depth<<'\n';
        meanPrint(cancelRandomThroughput(opCount, depth));
    }
    for (size_t depth : depths) {
        std::cout<<"mixed throughput depth: "<<depth<<'\n';
        meanPrint(mixedThroughput(opCount, depth));
    }
    //PRICE LEVELS
    std::vector<size_t> levelCounts{1, 100, 10'000, 100'000};
    for (size_t levels : levelCounts) {
        std::cout<<"addCancel throughput levels: "<<levels<<'\n';
        meanPrint(addCancelLevelsThroughput(opCount, levels));
    }
    //SWEEPS
    std::vector<size_t> makerCounts{10, 100, 1'000};
    for (size_t makers : makerCounts) {
        std::cout<<"sweep one level makers: "<<makers<<'\n';
        std::cout<<"mean ns "<<sweepMean(2'000, makers, false)<<'\n';
    }
    for (size_t makers : makerCounts) {
        std::cout<<"sweep across levels makers: "<<makers<<'\n';
        std::cout<<"mean ns "<<sweepMean(2'000, makers, true)<<'\n';
    }

    return 0;
}