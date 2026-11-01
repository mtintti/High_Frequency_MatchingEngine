#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>
#include <type_traits>

enum class Side:std::uint8_t {
    BUY,
    SELL,
};

struct OrdertoMatch{
    long long orderid; // is sequence number (per_client_seq_array[1]) from validator::gen_timestamp()
    long long timestamp; // is our timestamp (per_client_seq_array[0]) from validator::gen_timestamp()
    Side orderside; //Side::BUY or ::SELL
    int64_t client_id; //from clients.client_id
    int64_t funds;
};