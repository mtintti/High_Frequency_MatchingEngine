#include "orderbook.h"
#include "mempool.h"


orderbook obook;
void orderbook::addingEntery(long double price, long double amount, bool isbid)
{
    // std::cout << "\n in orderbook";
    // std::cout << " got price "<< price << " , amount " << amount;
    totalEnterys++;
    std::cout << "\n total enterys " << totalEnterys;

    if (amount == 0.0L)
    {
        if (isbid)
        {
            bids.erase(price);
            std::cout << "\n removed bid at " << price;
        }
        else
        {
            asks.erase(price);
            std::cout << "\n removed ask at " << price;
        }
    }
    else
    {
        if (isbid)
        {
            bids[price] = amount; 
            std::cout << "\n bids size " << bids.size();
        }
        else
        {
            asks[price] = amount;
            std::cout << "\n asks size " << asks.size();
        }
    }
}

void orderbook::updateDepthBased(struct DepthUpdate* Udp)
{
    //std::cout << " \n what got in update depth based";
    //std::cout << "p: "<<Udp->price << ", quantity: " << Udp->quantity << ", idofseq: " << Udp->id_sequence<< " isbid?: " << Udp->isitBid;
    //std::cout <<"\n";
    if (Udp->quantity == 0.0L) {
        // remove this price level from the book
        if (Udp->isitBid) {
            bids.erase(Udp->price);
        } else {
            asks.erase(Udp->price);
        }
    } else {
        // insert or update the price level
        if (Udp->isitBid) {
            bids[Udp->price] = Udp->quantity;
        } else {
            asks[Udp->price] = Udp->quantity;
        }
    }

}


void orderbook::printTop(int levels)
{
    std::cout << "\n--- top " << levels << " asks (lowest first) ---";
    int i = 0;
    for (auto it = asks.begin(); it != asks.end() && i < levels; ++it, ++i) {
        std::cout << "\n  ask: " << it->first << " qty: " << it->second;
    }

    std::cout << "\n--- top " << levels << " bids (highest first) ---";
    i = 0;
    for (auto it = bids.begin(); it != bids.end() && i < levels; ++it, ++i) {
        std::cout << "\n  bid: " << it->first << " qty: " << it->second;
    }
    std::cout << "\n--- spread: " << (asks.begin()->first - bids.begin()->first) << " ---\n";
}
