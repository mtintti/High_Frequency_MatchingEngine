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
        SNAPSHOT_GET,
        SNAPSHOT_PARSE,
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

    void recordPoolState(size_t allocated, size_t pool_size) {
        std::lock_guard<std::mutex> lock(pool_mutex);
        pool_allocated = allocated;
        pool_capacity  = pool_size;
        pool_peak      = std::max(pool_peak, allocated);
    }

    void recordOrderBookSize(size_t bids, size_t asks) {
        book_bids.store(bids, std::memory_order_relaxed);
        book_asks.store(asks, std::memory_order_relaxed);
    }

    void print() const {
        static const char* names[] = {
            "ws_read       ",
            "json_parse    ",
            "orderbook_upd ",
            "pool_alloc    ",
            "pool_dealloc  ",
            "snapshot_get  ",
            "snapshot_parse",
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
        std::cout << "memory pool:\n";
        std::cout << "  allocated now : " << pool_allocated << " blocks\n";
        std::cout << "  peak allocated: " << pool_peak      << " blocks\n";
        std::cout << "  pool capacity : " << pool_capacity  << " blocks\n";
        std::cout << "order book:\n";
        std::cout << "  bid levels    : " << book_bids.load() << "\n";
        std::cout << "  ask levels    : " << book_asks.load() << "\n";
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
    size_t pool_allocated{0};
    size_t pool_capacity{0};
    size_t pool_peak{0};
    std::atomic<size_t> book_bids{0};
    std::atomic<size_t> book_asks{0};
};

struct ScopedTimer {
    using clock = std::chrono::high_resolution_clock;
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