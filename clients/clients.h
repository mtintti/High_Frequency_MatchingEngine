#pragma once
#include <iostream>
#include <vector>
#include <boost/asio/io_context.hpp>
#include <list>
#include "validator_sequencer/validator_sequence.h"


struct invi_client{
    int64_t client_id;
    int64_t money;
    std::string typeofOrder;
    int64_t bitcoin_amount = 0; // total_amount_of_coin_holdings
    std::string ticket_name;
    double volatility;
    double wealth;
    bool swarm = false;
};


class Clients{
    public:
        //int max_clients = 10;
        Validator val;
        void createClients(int max_clients);
        void generateTradeBehavior();
        double volatility[3] = {1.7, 1.1, 0.5};
        double wealth[3] = {2.5, 1.1, 0.5};

        std::atomic<int64_t> clients_number{0};
        std::atomic<int64_t> last_printed_second_generate_beh{0}; 
        std::atomic<int64_t> last_total_gen_snapshot{0}; 

    private:
        std::vector<invi_client>client_vec;
};
