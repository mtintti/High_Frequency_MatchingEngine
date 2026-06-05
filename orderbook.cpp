#include "orderbook.h"
#include "mempool.h"
#include "profile.h"
//#include "orderbookSnapshot.h"

orderbook obook;
void orderbook::addingEntery(int64_t price, int64_t amount, bool isbid, int64_t lastId)
{
    // std::cout << "\n in orderbook";
    // std::cout << "\n got price "<< price << " , amount " << amount;
    totalEnterys++;
    // std::cout << "\n total enterys " << totalEnterys;
    lastId_Snapshot = lastId;

    if (amount == 0) // was 0.0L, the same as update
    {
        if (isbid)
        {
            bids.erase(price);
            // std::cout << "\n removed bid at " << price;
        }
        else
        {
            asks.erase(price);
            // std::cout << "\n removed ask at " << price;
        }
    }
    else
    {
        if (isbid)
        {
            // bids[price] = amount;
            bids.insert_or_assign(price, amount);
            // std::cout << "\n bids size " << bids.size();
        }
        else
        {
            // asks[price] = amount;

            asks.insert_or_assign(price, amount);
            // std::cout << "\n asks size " << asks.size();
        }
    }
}

void orderbook::updateDepthBased(struct DepthUpdate *Udp)
{
    // std::cout << " \n what got in update depth based";
    // std::cout << "\n p: "<<Udp->price << ", quantity: " << Udp->quantity << ", idofseq: " << Udp->id_sequence<< " isbid?: " << Udp->isitBid;
    // std::cout <<"\n";
    if (Udp->quantity == 0)
    {
        // remove this price level from the book
        if (Udp->isitBid)
        {
            // std::cout <<"\n eraising.. b: "<< Udp->price << " " << Udp->quantity;
            bids.erase(Udp->price);
        }
        else
        {
            // std::cout <<"\n eraising.. a: "<< Udp->price << " " << Udp->quantity;
            asks.erase(Udp->price);
        }
    }
    else
    {
        if (Udp->id_sequence <= lastId_Snapshot)
        {
            std::cout << "\n from depth " << Udp->id_sequence << " lastid for snapshot " << lastId_Snapshot;
            
        }else{
                // insert or update the price level
                if (Udp->isitBid)
                {
                    // std::cout <<"\n b: "<< Udp->price << " amount: " << Udp->quantity;
                    bids.insert_or_assign(Udp->price, Udp->quantity);

                    // bids.insert_or_assign(Udp->price, Udp->quantity);
                }
                else
                {
                    // asks[Udp->price] = Udp->quantity;
                    // std::cout <<"\n a: "<< Udp->price << " amount: " << Udp->quantity;
                    asks.insert_or_assign(Udp->price, Udp->quantity);
                };
            };
        
    };
    // record book size and pool state after each message
    HFTProfiler::instance().recordOrderBookSize(
        obook.bids.size(), obook.asks.size());
}

void orderbook::printTop(int levels)
{
    std::cout << "\n--- top " << levels << " asks (lowest first) ---";
    int i = 0;
    for (auto it = asks.begin(); it != asks.end() && i < levels; ++it, ++i)
    {
        // auto price = it->first / 100000.0;
        // auto amount = it->second / 100000.0;
        // auto price = (it->first * 0.01);
        auto priceS = std::to_string(it->first);
        auto price = priceS.insert(5, ".");

        auto amount = (it->second / 100000.0);
        std::cout << "\n raw " << it->first << " " << it->second;
        std::cout << "\n ask: " << price << " qty: " << amount;
        // std::cout << "\n";

        // std::cout << "\n ask: " << price << " qty: " << amount;
    }

    std::cout << "\n--- top " << levels << " bids (highest first) ---";
    i = 0;
    for (auto it = bids.begin(); it != bids.end() && i < levels; ++it, ++i)
    {
        // auto price = it->first / 100000.0;
        auto amount = it->second / 100000.0;
        // auto price = (it->first * 0.01); // was 0.01, tick amount
        auto priceS = std::to_string(it->first);
        auto price = priceS.insert(5, ".");

        std::cout << "\n raw " << it->first << " " << it->second;
        std::cout << "\n bid: " << price << " qty: " << amount;
        // std::cout << "\n";
    }
    std::cout << "\n --- raw spread " << asks.begin()->first << " b: " << bids.begin()->first << "--\n";
    auto priceA = asks.begin()->first;
    auto priceB = bids.begin()->first;
    // auto priceasks = priceA.insert(4, ".");
    // auto priceA = asks.begin()->first;
    // auto priceB = bids.begin()->first;
    int64_t diff = priceA - priceB;
    // std::cout <<"\n--- ask "<< priceA << " changed " << priceask << " and bid "<< priceB << " changed " << pricebid<< " ---";
    std::cout << " " << diff;
    std::cout << "\n--- spread: " << diff * 0.01 << " ---\n";
    // std::cout << "\n--- raw spread: " << (asks.begin()->first - bids.begin()->first) << " ---\n";
}
