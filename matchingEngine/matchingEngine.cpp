#include "matchingEngine.h"
#include "../clients/validator_sequencer/validator_sequence.h"
#include "../memory/matchingEngine_pool.h"
#include "../profile.h"
template <typename opposite_SideType, typename own_SideType>
void Matching::finding_match_order_using_waiting_opposite(OrdertoMatch* order,opposite_SideType& opposite_side,own_SideType &orders_own_side,BookLevel &lvl, int64_t book_price, int64_t book_qty){
    int64_t requested_at_start = order->amount;
    
    auto start = std::chrono::steady_clock::now();
    auto duration_since_epoch = start.time_since_epoch();
    int64_t current_second = std::chrono::duration_cast<std::chrono::seconds>(duration_since_epoch).count();
    int64_t expected_zero = 0;
    last_second.compare_exchange_strong(expected_zero, current_second);
    if(current_second > last_second){
        int64_t total_gen_snapshot = last_total_gen_snapshot.load();
        
        now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
        int64_t clients_matched_this_second = matched_clients - total_gen_snapshot;
        HFTProfiler::instance().generateTradeBehavior(
                static_cast<size_t>(matched_clients), static_cast<size_t>(clients_matched_this_second), HFTProfiler::MATCHING_ENGINE);
            last_total_gen_snapshot.store(matched_clients); 
        expire_orders(now_ms);
        last_second.store(current_second);

    };
    while(order->amount > 0 && !opposite_side.empty() && !opposite_side.key_comp()(order->price, opposite_side.begin()->first)){
        auto &queue_of_waiting_orders = opposite_side.begin()->second;
        OrdertoMatch *resting = queue_of_waiting_orders.front();
        
        int64_t fill = std::min(order->amount, resting->amount);
        order->amount -= fill;
        resting->amount -= fill;
        if (resting->amount == 0){
            resting->state = State::COMPLETED;
            queue_of_waiting_orders.pop_front();
            deallocateOrder(resting);
            --matched_clients;
            ScopedTimer t(HFTProfiler::MATCHING_ENGINE);
            if (queue_of_waiting_orders.empty())
                opposite_side.erase(opposite_side.begin());
        } else {
            resting->state = State::PARTICALLY_FILL;
        }
    }
     // Using the Binance's best level or then reducing what we already consumed
    if (lvl.price != book_price || lvl.seen_qty != book_qty){
        lvl = BookLevel{book_price, book_qty, 0};
    }
    if (order->amount > 0 && !opposite_side.key_comp()(order->price, book_price)){
        int64_t fill = std::min(order->amount, book_qty - lvl.consumed);
        order->amount -= fill;
        lvl.consumed += fill;
    }

    // finishing or resting the remainder amount
    if (order->amount == 0){
        order->state = State::COMPLETED;
        --matched_clients;
        ScopedTimer t(HFTProfiler::MATCHING_ENGINE);
        deallocateOrder(order);
    } else {
        order->state = (order->amount < requested_at_start) ? State::PARTICALLY_FILL : State::NEW;
        orders_own_side[order->price].push_back(order);
    }

}

 void Matching::expire_orders(int64_t now_ms){
       auto expire = [&](auto &book){
           for (auto it = book.begin(); it != book.end();){
               auto &queue = it->second;
               while (!queue.empty() && queue.front()->timestamp + 120000 <= now_ms){
                   OrdertoMatch *expired = queue.front();
                   expired->state = State::CANCELLED;
                   queue.pop_front();
                   deallocateOrder(expired);
                   --matched_clients;
                   
               }
               it = queue.empty() ? book.erase(it) : std::next(it);
           }
       };
       expire(buys);
       expire(sells);
   }

void Matching::submit_valid_order(OrdertoMatch* order){
    
    double order_amount_non_rounded = order->funds / (curr_marketPrice.load() * 0.01);
    int64_t order_amount = std::llround(order_amount_non_rounded * 100000.0);
    order->amount = order_amount;
    HFTProfiler::instance().recordPoolState(
            ++matched_clients,
            2000000, HFTProfiler::MATCHING_ENGINE);
    //stagnant client buys/sells are checked first for FIFO before getting to binance orderbook
    if(order->orderside == Side::BUY){
        int64_t matching_seller_price = lastest_best_askprice.load();
        int64_t matching_seller_amount = std::llround(latest_best_ask_amount.load() * 100000.0);
        BookLevel &level_obook = ask_level;
       finding_match_order_using_waiting_opposite(order, sells, buys,level_obook, matching_seller_price, matching_seller_amount);
    } else {
        int64_t matching_buyers_price = lastest_best_bidprice.load();
        int64_t matching_buyer_amount = std::llround(latest_best_bid_amount.load() * 100000.0);
        BookLevel &level_obook = bid_level;
        finding_match_order_using_waiting_opposite(order, buys, sells,level_obook, matching_buyers_price, matching_buyer_amount);
    }
};