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
                validIds.push_back(res->orderId);
                finish = std::chrono::steady_clock::now();
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

    return 0;
}