#include <iostream>
#include <chrono>
#include "validator_sequence.h"
#include "../clients.h" 
#include "../../profile.h"
#include "../../memory/matchingEngine_pool.h"
#include "../../matchingEngine/matchingEngine.h"
#include "../../main.h"
// a validator layer for checking validation of incoming buy or sell orders. Checks that the funds_to_use are less then accounts amount.
// myös tehdään sequencing matching engineä varten jossa asetetaan FIFO tarvitsema aika validoituun tradeen.
// bitcoinin current market price haetaan ensimmäisestä asks market orderbook:in hinnasta josta asiakkaan portofolion arvo lasketaan.

std::atomic<int64_t> last_printed_second{0};
std::atomic<int64_t> last_total_passed_snapshot{0};
std::atomic<int> insuffent_funds{0};
std::atomic<int> less_than_0_funds{0};
std::atomic<int> sell_less_than_holdings_value{0};
std::atomic<int> resync_inprogess_notAccepting{0};

std::atomic<int64_t> clients_total_processed{0};
std::atomic<int64_t> clients_total_passed{0};
std::atomic<long long> timestamp_client{0};
std::atomic<long long> sequence_client{0};
std::atomic<int64_t> same_ms_count{0};
std::atomic<int64_t> new_ms_count{0};
std::atomic<int64_t> sequence_calls{0};
std::atomic<int64_t> else_branche_calls{0};
std::atomic<long long> per_client_seq_array[2];
 std::atomic<int64_t> curr_marketPrice = 6400000; // asetettu numero bitcoin pricelle, ennen kuin oikea current bitcoin asks price has been gotten.
void Validator::gen_timestamp(long client_id, std::atomic<long long> per_client_seq_array[]){
    lock_guard<mutex> lock(mtx);
    long long timestamp_touse = currtime_ms();
    sequence_calls.fetch_add(1, std::memory_order_relaxed);

    if (timestamp_touse < lastused_timestamp){
        throw runtime_error("error timestamping for sequencer");
    };
    if(timestamp_touse == lastused_timestamp){
        sequence=(sequence+1) & maxnum_forSequence;
        same_ms_count.fetch_add(1, std::memory_order_relaxed);
        //std::cout << "\n sequence "<< sequence;
        if(sequence == 0){
            timestamp_touse = wait_until_nextms(lastused_timestamp);
            new_ms_count.fetch_add(1, std::memory_order_relaxed);
        }
    } else {
        sequence = 0;
        else_branche_calls.fetch_add(1, std::memory_order_relaxed);

    }
    //std::cout <<"\n lastused stamp: " << lastused_timestamp << " curr " << timestamp_touse;
    lastused_timestamp = timestamp_touse;
    timestamp_client = timestamp_touse;
    sequence_client = sequence;
    //std::cout<<"\n timestamp for client "<< client_id<<" timestamp: "<<((timestamp_touse - epoch) << 22) << " and sequence: "<<sequence << " timestamp as-is "<< timestamp_touse;
    //std::cout << " ";
    
    per_client_seq_array[0].store(timestamp_touse,std::memory_order_relaxed);
    per_client_seq_array[1].store(sequence,std::memory_order_relaxed);
    
}

OrdertoMatch *Validator::sequencer(invi_client &client, int64_t funds_to_use){
    //std::cout<<"\n timestamp for client "<< client.client_id;
    int64_t client_id = client.client_id;
    
    gen_timestamp(client_id, per_client_seq_array);
    //std::cout << "\n timestamp: " << per_client_seq_array[0] << " seq: "<<per_client_seq_array[1];
    
    auto returned_matchpool = allocateOrder(OrdertoMatch{per_client_seq_array[1].load(), per_client_seq_array[0].load(), client.typeofOrder, State::NEW, client_id, funds_to_use,client.limit_price, 0});
    return returned_matchpool;
    
  
};

void Validator::validate(invi_client &client, int64_t funds_to_use){
    // std::cout <<"\n client id to validate: "<<client.client_id << ", funds to use to buy: " << funds_to_use;
    // std::cout << "\n";
    clients_total_processed++;
    auto curr_time = std::chrono::steady_clock::now();
    auto duration_since_epoch = curr_time.time_since_epoch();
    int64_t current_second = std::chrono::duration_cast<std::chrono::seconds>(duration_since_epoch).count();

    int64_t expected_zero = 0;
    last_printed_second.compare_exchange_strong(expected_zero, current_second);
    auto start = std::chrono::steady_clock::now();
    ScopedTimer validateTimer(HFTProfiler::VALIDATE);

    if (funds_to_use >= 0){
        if (client.ticket_name == "bitcoin"){
            if (client.typeofOrder == Side::BUY){
                if (funds_to_use <= client.money){
                    clients_total_passed++;
                    OrdertoMatch *order = sequencer(client, funds_to_use);
                    if (order){
                        engine.submit_valid_order(order);
                    };
                    //std::cout << "\n client id (buy) "<<client.client_id;
                } else {
                    insuffent_funds++;
                };
            }
            else if (client.typeofOrder == Side::SELL)
            {
                // checkataan että marketData orderbook ei ole hakemassa uutta snapshottia (pendingSnapshots), ja obook on sync:attu. Muuten haetaan vanhoja tietoja.
                // pendingSnapshots muutetaan main.cpp marketData thread callissa vasta kuin https binance get request on valmis, jonka jälkeen snapshot muutetaan = 0, joten tämä riitää hyvin.
                if (obook.isin_Sync == true && pendingSnapshots == 0)
                {
                    curr_marketPrice = lastest_best_askprice.load();
                    // std::cout << "\n latest best ask "<< curr_marketPrice;
                    int64_t portofolio_val = (client.bitcoin_amount * 0.01) * (curr_marketPrice * 0.01);
                    if (portofolio_val >= funds_to_use)
                    {
                        // can pass to sequencing
                        clients_total_passed++;
                        //std::cout << "\n client id (sell) "<<client.client_id;
                        OrdertoMatch *order = sequencer(client, funds_to_use);
                        if (order){
                            engine.submit_valid_order(order);
                        }
                    }else{
                        sell_less_than_holdings_value++;
                        // std::cout << " clients portofolio value: " << portofolio_val << " funds to use: " << funds_to_use;
                    };
                }
                else
                {
                    resync_inprogess_notAccepting++;
                };
            }
        };
    }
    else
    {
        less_than_0_funds++;
    };

    // profiler metrics per second koodi
    int64_t last_sec = last_printed_second.load();
    if (current_second > last_sec)
    {
        if (last_printed_second.compare_exchange_strong(last_sec, current_second))
        {

            int64_t current_passed = clients_total_passed.load();
            int64_t total_passed_snapshot = last_total_passed_snapshot.load();

            int64_t clients_passed_this_second = current_passed - total_passed_snapshot;

            HFTProfiler::instance().recordValidateSize(
                static_cast<size_t>(current_passed),
                static_cast<size_t>(clients_passed_this_second));

            /*std::cout << "\n -- Validator Check --";
            std::cout << "\n Clients Passed/Sec: " << clients_passed_this_second;
            std::cout << "\n Total processed: " << clients_total_processed.load();
            std::cout << "\n Total passed: " << current_passed;
            std::cout << "\n Less than 0: " << less_than_0_funds.load();
            std::cout << "\n Insufficient funds: " << insuffent_funds.load();
            std::cout << "\n Sell over holdings: " << sell_less_than_holdings_value.load();
            std::cout << "\n resync happened, requests not passed " << resync_inprogess_notAccepting.load();
            std::cout << "\n curr_marketPrice " << curr_marketPrice;
            std::cout << "\n latest client timestamp and sequence: " << per_client_seq_array[0].load() << " , " << per_client_seq_array[1].load();
            std::cout << "\n number of timestamp matches " << same_ms_count.load();
            std::cout << "\n number of new gens of timestamps " << new_ms_count.load();
            std::cout << "\n total of sequence calls have happened: "<< sequence_calls.load();
            std::cout << "\n else branch total calls have happened num of times: "<< else_branche_calls.load();
            std::cout << "\n";*/

            last_total_passed_snapshot.store(current_passed);
        }
    }
}
