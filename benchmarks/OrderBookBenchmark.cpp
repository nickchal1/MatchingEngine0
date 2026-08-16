#include "OrderBook.h"

#include <iostream>
#include <chrono>
#include <vector>
#include <cmath>
#include <algorithm>
#include <optional>

long long nearestRankPercentile(const std::vector<long long>& clockSamples, double percentile) {

    long long rank = std::ceil(percentile * clockSamples.size());
    
    return clockSamples[rank - 1];
}

int main() {

    std::cout<<"Matching Engine Latency Benchmark\n"; 

    constexpr std::size_t sampleCount = 1'000'000;
    std::vector<long long> clockSamples;
    clockSamples.reserve(sampleCount);

    for (std::size_t i = 0; i < sampleCount; ++i) {
        const auto start{std::chrono::steady_clock::now()};
        const auto finish{std::chrono::steady_clock::now()};
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start);
        const auto ns = elapsed.count();
        clockSamples.push_back(ns);
    }

    std::sort(clockSamples.begin(), clockSamples.end());

    std::cout<<clockSamples.size()<<'\n';
    std::cout<<clockSamples[0]<<'\n';

    long long p50 = nearestRankPercentile(clockSamples, 0.50);
    long long p90 = nearestRankPercentile(clockSamples, 0.90);
    long long p99 = nearestRankPercentile(clockSamples, 0.99);
    long long p99_9 = nearestRankPercentile(clockSamples, 0.999);

    std::cout<<"p50 "<<p50<<'\n';
    std::cout<<"p90 "<<p90<<'\n';
    std::cout<<"p99 "<<p99<<'\n';
    std::cout<<"p99.9 "<<p99_9<<'\n';

    //check one order
    OrderBook book;
    const auto start{std::chrono::steady_clock::now()};
    std::optional<SubmissionResult> res = book.addOrder(100, 10'000, Side::Buy);
    if (res == std::nullopt) return 1;
    OrderId id = res->orderId;
    if (!book.cancelOrder(id)) return 1;
    const auto finish{std::chrono::steady_clock::now()};
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start);
    const auto ns = elapsed.count();
    std::cout<<"Add and Cancel time "<<ns<<'\n';
    
    //addOrder without cross
    std::cout<<"Benchmark addOrder with no order being matched"<<'\n';
    std::vector<long long> addSamples;
    addSamples.reserve(sampleCount);

    long long idCounter = 0;
    for (std::size_t i = 0; i < sampleCount; ++i) {
        const auto start{std::chrono::steady_clock::now()};
        std::optional<SubmissionResult> res = book.addOrder(100, 10'000, Side::Buy);
        const auto finish{std::chrono::steady_clock::now()};
        if (res == std::nullopt) return 1;
        idCounter += res->orderId;
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start);
        const auto ns = elapsed.count();
        addSamples.push_back(ns);
    }

    std::sort(addSamples.begin(), addSamples.end());

    std::cout<<addSamples.size()<<'\n';
    std::cout<<addSamples[0]<<'\n';
    std::cout<<"Aggregate id sum (prevents compiler dead code deletion) "<<idCounter<<'\n';

    p50 = nearestRankPercentile(addSamples, 0.50);
    p90 = nearestRankPercentile(addSamples, 0.90);
    p99 = nearestRankPercentile(addSamples, 0.99);
    p99_9 = nearestRankPercentile(addSamples, 0.999);

    std::cout<<"p50 "<<p50<<'\n';
    std::cout<<"p90 "<<p90<<'\n';
    std::cout<<"p99 "<<p99<<'\n';
    std::cout<<"p99.9 "<<p99_9<<'\n';

    //MAKE BNNEW BOOK

    //add order with match
    std::cout<<"Benchmark addOrder with nessesary matched"<<'\n';
    std::vector<long long> addMatchSamples;
    addMatchSamples.reserve(sampleCount);

    for (std::size_t i = 0; i < sampleCount; ++i) {
        std::optional<SubmissionResult> res = book.addOrder(100, 10'000, Side::Sell);
    }

    idCounter = 0;
    for (std::size_t i = 0; i < sampleCount; ++i) {
        const auto start{std::chrono::steady_clock::now()};
        std::optional<SubmissionResult> res = book.addOrder(100, 10'000, Side::Buy);
        const auto finish{std::chrono::steady_clock::now()};
        if (res == std::nullopt) return 1;
        idCounter += res->orderId;
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start);
        const auto ns = elapsed.count();
        addMatchSamples.push_back(ns);
    }

    std::sort(addMatchSamples.begin(), addMatchSamples.end());

    std::cout<<addMatchSamples.size()<<'\n';
    std::cout<<addMatchSamples[0]<<'\n';
    std::cout<<"Aggregate id sum (prevents compiler dead code deletion) "<<idCounter<<'\n';

    p50 = nearestRankPercentile(addMatchSamples, 0.50);
    p90 = nearestRankPercentile(addMatchSamples, 0.90);
    p99 = nearestRankPercentile(addMatchSamples, 0.99);
    p99_9 = nearestRankPercentile(addMatchSamples, 0.999);

    std::cout<<"p50 "<<p50<<'\n';
    std::cout<<"p90 "<<p90<<'\n';
    std::cout<<"p99 "<<p99<<'\n';
    std::cout<<"p99.9 "<<p99_9<<'\n';


    return 0;
}