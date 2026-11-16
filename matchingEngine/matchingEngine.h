#pragma once

#include <atomic>
#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <deque>
#include <map>


enum class Side:std::uint8_t {
    BUY,
    SELL,
};

enum class State:std::uint8_t {
    NEW,
    MATCHING, 
    PARTICALLY_FILL,
    COMPLETED,
    CANCELLED,
};

struct OrdertoMatch{
    long long orderid; // is sequence number (per_client_seq_array[1]) from validator::gen_timestamp()
    long long timestamp; // is our timestamp (per_client_seq_array[0]) from validator::gen_timestamp()
    Side orderside; //Side::BUY or ::SELL
    State state;
    int64_t client_id; //from clients.client_id
    int64_t funds;
    int64_t price;
    int64_t amount;
};


class Matching{

public:
    void submit_valid_order(OrdertoMatch* order);
    long long timestamp_seq_check;
    struct BookLevel { int64_t price = 0; int64_t seen_qty = 0; int64_t consumed = 0; };
    std::map<int64_t, std::deque<OrdertoMatch*>, std::greater<int64_t>> buys; 
    std::map<int64_t, std::deque<OrdertoMatch*>, std::less<int64_t>> sells;
    BookLevel ask_level, bid_level;
    int64_t now_ms = 0;
    std::atomic<size_t> matched_clients;
    std::atomic<int64_t> last_second{0}; 
    std::atomic<int64_t> last_total_gen_snapshot{0};       
    
    void expire_orders(int64_t now_ms);
    template <typename opposite_SideType,typename own_SideType>
    void finding_match_order_using_waiting_opposite(OrdertoMatch* order,opposite_SideType& opposite_side, own_SideType &orders_own_side,BookLevel &lvl, int64_t book_price, int64_t book_qty);

};