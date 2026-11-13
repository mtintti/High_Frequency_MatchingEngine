#pragma once
#include <chrono>
#include <atomic>
#include <string>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <numeric>
#include <mutex>

class HFTProfiler {
public:
    struct StageStats {
        std::atomic<uint64_t> count{0};
        std::atomic<uint64_t> total_us{0};
        std::atomic<uint64_t> min_us{UINT64_MAX};
        std::atomic<uint64_t> max_us{0};
    };

    enum Stage {
        WS_READ = 0,
        JSON_PARSE,
        ORDERBOOK_UPDATE,
        POOL_ALLOC,
        POOL_DEALLOC,
        MATCHPOOL_ALLOC,
        MATCHPOOL_DEALLOC,
        SNAPSHOT_GET,
        SNAPSHOT_PARSE,
        GENERATE_TRADE_BEHAVIOR,
        VALIDATE,
        SEQUENCE,
        MATCHING_ENGINE,
        STAGE_COUNT
    };

    static HFTProfiler& instance() {
        static HFTProfiler p;
        return p;
    }

    void recordStage(Stage s, uint64_t duration_us) {
        auto& st = stages[s];
        st.count.fetch_add(1, std::memory_order_relaxed);
        st.total_us.fetch_add(duration_us, std::memory_order_relaxed);

        uint64_t prev_min = st.min_us.load(std::memory_order_relaxed);
        while (duration_us < prev_min &&
               !st.min_us.compare_exchange_weak(prev_min, duration_us,
                   std::memory_order_relaxed)) {}

        uint64_t prev_max = st.max_us.load(std::memory_order_relaxed);
        while (duration_us > prev_max &&
               !st.max_us.compare_exchange_weak(prev_max, duration_us,
                   std::memory_order_relaxed)) {}
    }

    void recordPoolState(size_t allocated, size_t pool_size, enum Stage s) {
        switch (s) {
            case Stage::POOL_ALLOC:{
                std::lock_guard<std::mutex> lock(pool_mutex);
                pool_allocated = allocated;
                pool_capacity  = pool_size;
                pool_peak      = std::max(pool_peak, allocated);
                break;
            }
            case Stage::MATCHPOOL_ALLOC:{
                std::lock_guard<std::mutex> lockmatch(matchpool_mutex);
                matchpool_alloc = allocated;
                matchpool_capacity  = pool_size;
                matchpool_peak      = std::max(matchpool_peak, allocated);
                break;
            }
            case Stage::MATCHING_ENGINE:{
                std::lock_guard<std::mutex> lockmatch(matching_engine_mutex);
                matching_engine_alloc = allocated;
                matching_engine_capacity  = pool_size;
                matching_engine_peak      = std::max(matching_engine_peak, allocated);
                break;
            }
        }
       
    }

    void recordOrderBookSize(size_t bids, size_t asks) {
        book_bids.store(bids, std::memory_order_relaxed);
        book_asks.store(asks, std::memory_order_relaxed);
    }

    void recordValidateSize(size_t clients_total_passed, size_t clients_passed_this_second) {
        total_passed.store(clients_total_passed, std::memory_order_relaxed);
        total_passed_second.store(clients_passed_this_second, std::memory_order_relaxed);

        // Track peak *throughput* (best passed/sec seen), not peak of the
        // ever-growing total - that number is only ever equal to "latest".
        size_t prev_peak = validate_peak.load(std::memory_order_relaxed);
        while (clients_passed_this_second > prev_peak &&
               !validate_peak.compare_exchange_weak(prev_peak, clients_passed_this_second,
                   std::memory_order_relaxed)) {}
    } 

    void generateTradeBehavior(size_t clients_generated, size_t generated_this_second, enum Stage s) {
        if (s == Stage::MATCHING_ENGINE){
            //client_trade_behavior.store(clients_generated, std::memory_order_relaxed);
            total_matched_this_second.store(generated_this_second, std::memory_order_relaxed);

            // Track peak *throughput* (best passed/sec seen), not peak of the
            // ever-growing total - that number is only ever equal to "latest".
            size_t prev_peak = matching_engine_peak;
            while (generated_this_second > prev_peak &&
                !trade_behavior_peak.compare_exchange_weak(prev_peak, generated_this_second,
                    std::memory_order_relaxed)) {}
        } else if (s == Stage::GENERATE_TRADE_BEHAVIOR){
            client_trade_behavior.store(clients_generated, std::memory_order_relaxed);
            total_generated_second.store(generated_this_second, std::memory_order_relaxed);

            // Track peak *throughput* (best passed/sec seen), not peak of the
            // ever-growing total - that number is only ever equal to "latest".
            size_t prev_peak = trade_behavior_peak.load(std::memory_order_relaxed);
            while (generated_this_second > prev_peak &&
                !trade_behavior_peak.compare_exchange_weak(prev_peak, generated_this_second,
                    std::memory_order_relaxed)) {}
        }
        
    } 

    void print() const {
        static const char* names[] = {
            "ws_read       ", // WS_READ           = 0
            "json_parse    ", // JSON_PARSE        = 1
            "orderbook_upd ", // ORDERBOOK_UPDATE  = 2
            "pool_alloc    ", // POOL_ALLOC        = 3
            "pool_dealloc  ", // POOL_DEALLOC      = 4
            "matchpool_alloc ",
            "matchpool_dealloc ",
            "snapshot_get  ", // SNAPSHOT_GET      = 5
            "snapshot_parse", // SNAPSHOT_PARSE    = 6
            "gene_trade_beh",
            "validate      ", // VALIDATE          = 7
            "sequence      ", // SEQUENCE          = 8
            "matching engine ",
        };

        std::cout << "\n";
        std::cout << "========== HFT PROFILER REPORT ==========\n";
        std::cout << std::left
                  << std::setw(16) << "stage"
                  << std::setw(10) << "calls"
                  << std::setw(12) << "avg (us)"
                  << std::setw(12) << "min (us)"
                  << std::setw(12) << "max (us)"
                  << std::setw(14) << "total (ms)"
                  << "\n";
        std::cout << std::string(76, '-') << "\n";

        for (int i = 0; i < STAGE_COUNT; i++) {
            const auto& st = stages[i];
            uint64_t cnt   = st.count.load(std::memory_order_relaxed);
            if (cnt == 0) continue;
            uint64_t total = st.total_us.load(std::memory_order_relaxed);
            uint64_t mn    = st.min_us.load(std::memory_order_relaxed);
            uint64_t mx    = st.max_us.load(std::memory_order_relaxed);
            double   avg   = static_cast<double>(total) / cnt;

            std::cout << std::left
                      << std::setw(16) << names[i]
                      << std::setw(10) << cnt
                      << std::fixed << std::setprecision(1)
                      << std::setw(12) << avg
                      << std::setw(12) << mn
                      << std::setw(12) << mx
                      << std::setw(14) << (total / 1000.0)
                      << "\n";
        }

        std::cout << std::string(76, '-') << "\n";
        std::cout << "memory pools:\n";
        std::cout << "  allocated now: " << pool_allocated << " blocks\n";
        std::cout << "  peak allocated: " << pool_peak      << " blocks\n";
        std::cout << "  pool capacity: " << pool_capacity  << " blocks\n";
         std::cout << " matchpool alloc now: " << matchpool_alloc << " blocks\n";
        std::cout << "  matchpool peak alloc: " << matchpool_peak      << " blocks\n";
        std::cout << "  matchpool capacity: " << matchpool_capacity  << " blocks\n";
        std::cout << "marketData order book:\n";
        std::cout << "  bid levels: " << book_bids.load() << "\n";
        std::cout << "  ask levels: " << book_asks.load() << "\n";
        std::cout << "generate trade behavior:\n";
        std::cout << "  generated total: " << client_trade_behavior.load() << "\n";
        std::cout << "  generated per second: " << total_generated_second.load() << "\n";
        std::cout << "  peak: " << trade_behavior_peak.load() << "\n";
        std::cout << "validate:\n";
        std::cout << "  passed total: " << total_passed.load() << "\n";
        std::cout << "  passed per second: " << total_passed_second.load() << "\n";
        std::cout << "  peak: " << validate_peak.load() << "\n";
        std::cout << "matching engine:\n";
        std::cout << "  matching engine alloc: " << matching_engine_alloc<< " blocks\n";
        std::cout << "  matching engine peak alloc: " << matching_engine_peak << " blocks\n";
        std::cout << "  matched per second: "<<total_matched_this_second;
        std::cout << "  matching engine capacity: " << matching_engine_capacity << " blocks\n";
        std::cout << "=========================================\n";
    }

    /*void printEvery(uint64_t n_messages) {
        uint64_t cnt = stages[WS_READ].count.load(std::memory_order_relaxed);
        if (cnt > 0 && cnt % n_messages == 0) {
            print();
        }
    }*/

private:
    HFTProfiler() = default;
    StageStats stages[STAGE_COUNT];
    std::mutex pool_mutex;
    std::mutex matchpool_mutex;
    std::mutex matching_engine_mutex;
    size_t pool_allocated{0};
    size_t pool_capacity{0};
    size_t pool_peak{0};
    size_t matchpool_alloc{0};
    size_t matchpool_capacity{0};
    size_t matchpool_peak{0};
    size_t matching_engine_alloc{0};
    size_t matching_engine_capacity{0};
    size_t matching_engine_peak{0};
    std::atomic<size_t> book_bids{0};
    std::atomic<size_t> book_asks{0};
    std::atomic<size_t> client_trade_behavior{0};
    std::atomic<size_t> total_generated_second{0};
    std::atomic<size_t> total_matched_this_second{0};
    std::atomic<size_t> trade_behavior_peak{0};
    std::atomic<size_t> total_passed{0};
    std::atomic<size_t> total_passed_second{0};
    std::atomic<size_t> validate_peak{0};
};

struct ScopedTimer {
    using clock = std::chrono::steady_clock;
    HFTProfiler::Stage stage;
    clock::time_point  start;

    explicit ScopedTimer(HFTProfiler::Stage s)
        : stage(s), start(clock::now()) {}

    ~ScopedTimer() {
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(
            clock::now() - start).count();
        HFTProfiler::instance().recordStage(stage, static_cast<uint64_t>(us));
    }
};