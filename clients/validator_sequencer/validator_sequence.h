#pragma once
#include <iostream>
#include <chrono>
#include <atomic>
#include <mutex>
#include "../../matchingEngine/matchingEngine.h"

using namespace std;
struct invi_client;

class Validator{

public:
    void validate(invi_client &client, int64_t funds_to_use);
    void sequencer(invi_client &client, int64_t funds_to_use);
    long long gen_timestamp(long client_id, std::atomic<long long> per_client_seq_array[]);
    OrdertoMatch order_for_matching;

    private:
    const long long epoch = 1704067200000LL;
    const int machineIdBits = 10;
    const int sequenceBits = 12;

    const long long maxMachineId = (1LL << machineIdBits) - 1;
    const long long maxnum_forSequence = (1LL << sequenceBits) - 1;

    long long machineId;
    long long lastused_timestamp = 0;
    long long sequence = 0;
    mutex mtx;

    long long currtime_ms() {
        return chrono::duration_cast<chrono::milliseconds>(
            chrono::system_clock::now().time_since_epoch()
        ).count();
    }

    long long wait_until_nextms(long long timestamp) {
        long long now = currtime_ms();
        while (now <= timestamp) {
            now = currtime_ms();
        }
        return now;
    }
    
};