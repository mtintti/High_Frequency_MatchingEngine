#include <iostream>
#include "clients.h"
#include "../main.h"
#include "../profile.h"
#include "validator_sequencer/validator_sequence.h"
#include <algorithm>



void Clients::createClients(int max_clients){
    client_vec.clear(); // poistetaan leftover tiedot

    // volatility (1.7, 1.1, 0.5) on kuinka paljon yksi client käyttää rahaansa kerralla. Wealth (1.7, 1.1, 0.5) on kuinka iso alkusumma clientlilla on. Asetettu määrä muuttaa rahamäärän client.money:ssa
    // sekä funds_to_use mahdolliseen tradeen isommaksi, keskikokoiseen tai pienemmäksi johtaen 'realistisempaan' clientti behavioriin. 
    // client.swarm (true or false) asetetaan mahdoillistamaan tulevaisuudessa osan clienteistä jotka tekevät buy tai sell tradeja yhdessä ryömyssä jotta matching engine voidaan testata varied load:kin kanssa
    
    for(int i = 0; i < max_clients; i++){
        invi_client client_info = invi_client{};

        int endnumber = client_vec.size();
        //std::cout << endnumber;
        client_info.client_id = endnumber+1;

        int index_multiplier = rand() % (sizeof(volatility) / sizeof(volatility[0]));
        int index_wealth_multiplier = rand() % (sizeof(wealth) / sizeof(wealth[0]));
        
        client_info.typeofOrder = "not set";
        client_info.ticket_name = "bitcoin";
        double multiplier = volatility[index_multiplier];
        double wealth_multiplier = wealth[index_wealth_multiplier];
        
        int clients_funds = ((rand() % (22001 - 400)+1) * (wealth_multiplier)); // clientin funds_to_use minimi on laitettu 400. less than 0 on validator debug print statementissa 0, jo usean 14-16 minuutin program run:in ajan.
        if(clients_funds <= 0 ){clients_funds = ((rand() % (22001 - 400)+1) * (wealth_multiplier));};
        client_info.money = clients_funds;
        
        client_info.bitcoin_amount = 010 * wealth_multiplier;
        client_info.volatility = multiplier;
        client_info.wealth = wealth_multiplier;
        int swarm_or_not = rand() % 4;
        if (swarm_or_not == 1){
            client_info.swarm = true;
        } else {
            client_info.swarm = false;
        }
        
        client_vec.emplace_back(client_info);
    };

    std::cout << "\n clients created " << client_vec.size();
    auto found = std::minmax_element(begin(client_vec), end(client_vec), [] (invi_client const s1, invi_client const s2){ return s1.money < s2.money;});
    std::cout << "\n max funds for client: " <<found.second->client_id << " funds: "<< found.second->money;
    std::cout << "\n minimum funds for client: " <<found.first->client_id << " funds: " << found.first->money;
    trades_isrunning = true;
    
     
};

void Clients::generateTradeBehavior(){
    // otetaan randomisti yksi client_vec clientti. 
        
        auto start = std::chrono::steady_clock::now();
        auto duration_since_epoch = start.time_since_epoch();
        int64_t current_second = std::chrono::duration_cast<std::chrono::seconds>(duration_since_epoch).count();
        int64_t expected_zero = 0;
        last_printed_second_generate_beh.compare_exchange_strong(expected_zero, current_second);
        
   
        int clients_tochoose = rand() % client_vec.size();
        auto& client = client_vec.at(clients_tochoose);
        int buy_or_sell = rand() % 2;
        if (buy_or_sell == 1){
            client.typeofOrder = "buy";
        } else if (buy_or_sell == 0){
            client.typeofOrder = "sell";
        }

        clients_number++;
       
        
        int64_t funds_to_use = std::max<int64_t>(10,(rand() % (client.money)+1)) * (client.volatility);
        if( funds_to_use <= 0) {
            std::cout << "\n clients fund were zero, " << funds_to_use;
        };
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                          std::chrono::steady_clock::now() - start)
                          .count();
        HFTProfiler::instance().recordStage(HFTProfiler::GENERATE_TRADE_BEHAVIOR, static_cast<uint64_t>(us));
        
        
        int64_t last_sec = last_printed_second_generate_beh.load();
    if (current_second > last_sec)
    {
        if (last_printed_second_generate_beh.compare_exchange_strong(last_sec, current_second))
        {
            
            int64_t current_clients_generated = clients_number.load();
            int64_t total_gen_snapshot = last_total_gen_snapshot.load();
            
            int64_t clients_genereted_this_second = current_clients_generated - total_gen_snapshot;
            HFTProfiler::instance().generateTradeBehavior(
                static_cast<size_t>(clients_number), static_cast<size_t>(clients_genereted_this_second));
             last_total_gen_snapshot.store(current_clients_generated); 
        };
    };
        //std::cout << "\n client chosen: " << client.client_id << ", typeof: " << client.typeofOrder << ", swarm: "<< client.swarm << ", volatility: "<< client.volatility<< ", wealth: "<< client.wealth<< ", funds to use: " << funds_to_use << " in account: "<< client.money;
        //std::cout << "\n bitcoin amount: "<< client.bitcoin_amount * 0.01 << "portofolio val: "<< (client.bitcoin_amount * 0.01) * (6252130 * 0.01);
        Validator val;
        val.validate(client, funds_to_use);
        
};