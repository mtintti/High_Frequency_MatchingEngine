#include <iostream>
#include <chrono>
#include "validator_sequence.h"
#include "../../profile.h"
#include "../../orderbook/orderbook.h"
#include "../../main.h"


// a validator layer for checking validation of incoming buy or sell orders. Checks that the funds_to_use are less then accounts amount.
// myös tehdään sequencing matching engineä varten jossa asetetaan FIFO tarvitsema aika validoituun tradeen.
// bitcoinin current market price haetaan ensimmäisestä asks market orderbook:in hinnasta josta asiakkaan portofolion arvo lasketaan.

/* mitä tapahtuu tässä, previous metrics
    Sustained 1M+/sec order-validation throughput on a single core with sub-microsecond average per-order latency.
*/

std::atomic<int64_t> last_printed_second{0}; 
std::atomic<int64_t> last_total_passed_snapshot{0}; 
std::atomic<int> insuffent_funds{0};
std::atomic<int> less_than_0_funds{0};
std::atomic<int> sell_less_than_holdings_value{0};
std::atomic<int> resync_inprogess_notAccepting{0};

std::atomic<int64_t> clients_total_processed{0};
std::atomic<int64_t> clients_total_passed{0};
std::atomic<int64_t> curr_marketPrice = 64000; // asetettu numero bitcoin pricelle, ennen kuin oikea current bitcoin asks price has been gotten.


void Validator::validate(invi_client &client, int64_t funds_to_use)
{
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


    if (funds_to_use >= 0)
    {
        if (funds_to_use <= client.money)
        {
            if (client.ticket_name == "bitcoin")
            {
                if (client.typeofOrder == "buy")
                {
                    // can pass to sequencing
                    clients_total_passed++;
                }
                else if (client.typeofOrder == "sell")
                {  
                    // checkataan että marketData orderbook ei ole hakemassa uutta snapshottia (pendingSnapshots), ja obook on sync:attu. Muuten haetaan vanhoja tietoja.
                    // pendingSnapshots muutetaan main.cpp marketData thread callissa vasta kuin https binance get request on valmis, jonka jälkeen snapshot muutetaan = 0, joten tämä riitää hyvin.
                    if(obook.isin_Sync == true && pendingSnapshots == 0){
                        curr_marketPrice = obook.lastest_best_askprice.load();
                        //std::cout << "\n latest best ask "<< curr_marketPrice;
                        int64_t portofolio_val = (client.bitcoin_amount * 0.01) * (curr_marketPrice * 0.01);
                        if (portofolio_val >= funds_to_use)
                        {
                            // can pass to sequencing
                            clients_total_passed++;
                        }else{
                            sell_less_than_holdings_value++;
                        };
                    } else {
                        resync_inprogess_notAccepting++;
                    };
                }
            };
        }
        else
        {
            insuffent_funds++;
            // std::cout << "client.money " << client.money << " funds to use: " << funds_to_use;
        };
    }
    else
    {
        less_than_0_funds++;
    };

    //profiler metrics per second koodi
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

            
            std::cout << "\n -- Validator Check --";
            std::cout << "\n Clients Passed/Sec: " << clients_passed_this_second;
            std::cout << "\n Total processed: " << clients_total_processed.load();
            std::cout << "\n Total passed: " << current_passed;
            std::cout << "\n Less than 0: " << less_than_0_funds.load();
            std::cout << "\n Insufficient funds: " << insuffent_funds.load();
            std::cout << "\n Sell over holdings: " << sell_less_than_holdings_value.load();
            std::cout << "\n resync happened, requests not passed " << resync_inprogess_notAccepting.load();
            std::cout <<"\n curr_marketPrice "<< curr_marketPrice;
            std::cout << "\n";
            

            last_total_passed_snapshot.store(current_passed);
        }
    }
}
