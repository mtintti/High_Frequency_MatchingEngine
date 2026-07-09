#include <iostream>
#include <chrono>
#include "validator_sequence.h"

// a validator layer for checking validation of incoming buy or sell orders. Checks that the funds_to_use are less then accounts amount.
// myös tehdään sequencing matching engineä varten jossa asetetaan FIFO tarvitsema aika validoituun tradeen.


std::atomic<int> insuffent_funds{0};
std::atomic<int> less_than_0_funds{0};
std::atomic<int> sell_less_than_holdings_value{0};

std::atomic<int64_t> clients_total_processed{0};
std::atomic<int64_t> clients_total_passed{0};

std::atomic<int64_t> last_printed_second{0}; 
std::atomic<int64_t> last_total_passed_snapshot{0}; 



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


    if (funds_to_use > 0)
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
                    int64_t portofolio_val = (client.bitcoin_amount * 0.01) * (6252130 * 0.01);
                    if (portofolio_val >= funds_to_use)
                    {
                        // can pass to sequencing
                        // std::cout << " portofolio val: "<< portofolio_val<< " funds to sell: " << funds_to_use;
                        clients_total_passed++;
                    }
                    else
                    {
                        sell_less_than_holdings_value++;
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
    
    int64_t last_sec = last_printed_second.load();
    if (current_second > last_sec)
    {
        if (last_printed_second.compare_exchange_strong(last_sec, current_second))
        {
            
            int64_t current_passed = clients_total_passed.load();
            int64_t total_passed_snapshot = last_total_passed_snapshot.load();
            
            int64_t clients_passed_this_second = current_passed - total_passed_snapshot;

            std::cout << "\n -- Validator Check --";
            std::cout << "\n Clients Passed/Sec: " << clients_passed_this_second;
            std::cout << "\n Total processed: " << clients_total_processed.load();
            std::cout << "\n Total passed: " << current_passed;
            std::cout << "\n Less than 0: " << less_than_0_funds.load();
            std::cout << "\n Insufficient funds: " << insuffent_funds.load();
            std::cout << "\n Sell over holdings: " << sell_less_than_holdings_value.load();
            std::cout << "\n";

            last_total_passed_snapshot.store(current_passed);
        }
    }
}
