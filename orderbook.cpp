#include "orderbook.h"
#include "mempool.h"
#include "profile.h"
#include "orderbookSnapshot.h"
#include "main.h"

orderbook obook;
void orderbook::addingEntery(int64_t price, int64_t amount, bool isbid)
{
    // std::cout << "\n in orderbook";
    // std::cout << "\n got price "<< price << " , amount " << amount;
    totalEnterys++;
    // std::cout << "\n total enterys " << totalEnterys;
    // lastId_Snapshot = lastId;

    if (amount == 0)
    {
        if (isbid)
        {
            bids.erase(price);
        }
        else
        {
            asks.erase(price);
        }
    }
    else
    {
        if (isbid)
        {
            bids.insert_or_assign(price, amount);
        }
        else
        {
            asks.insert_or_assign(price, amount);
        }
    }
}

void orderbook::asksSize()
{
    std::cout << "\n bids size " << asks.size();
}

void orderbook::bidsSize()
{
    std::cout << "\n bids size " << bids.size();
}

void orderbook::clearBooks(){
    asks.clear();
    bids.clear();
    std::cout << "\n bids and asks cleared ";
};

void orderbook::updateDepthBased(struct DepthUpdate *Udp)
{
    // std::cout << " \n what got in update depth based";
    // std::cout << "\n p: "<<Udp->price << ", quantity: " << Udp->quantity << ", idofseq: " << Udp->id_sequence<< " isbid?: " << Udp->isitBid;
    // std::cout <<"\n";
    if (!isin_Sync)
    {
        if (Udp->id_sequence <= lastId_Snapshot)
        {
            std::cout << "\n not in sync, id_sequence is <= than snapshots id";
            std::cout << "\n ", Udp->id_sequence, " ", lastId_Snapshot;
            return;
        }
        if (Udp->U_first <= lastId_Snapshot + 1 && Udp->id_sequence >= lastId_Snapshot + 1)
        {
            isin_Sync = true;
            std::cout << "\nSYNC EVENT"
                      << "\nU=" << Udp->U_first
                      << "\nu=" << Udp->id_sequence
                      << "\nsnapshot=" << lastId_Snapshot;
            std::cout << "\n is synced just fine";
        }
        /*else
        {
            std::cout << " else cause in !isin_Sync";
            return;
        }*/
    }

    if (Udp->U_first > localId + 1)
    {
        isin_Sync = false;
        std::cout << "\n out of sync, getting orderbook snapshot again";
        std::cout
            << "\nGAP"
            << "\nU=" << Udp->U_first
            << "\nlocal=" << localId;
        getRequestOrderBook(contextSSL);
        return;
    }

    // before syncing check
    if (Udp->quantity == 0)
    {
        if (Udp->isitBid)
        {
            // std::cout << "\n removing bid, was 0";
            bids.erase(Udp->price);
        }
        else
        {
            // std::cout << "\n removing ask, was 0";
            asks.erase(Udp->price);
        }
    }
    else
    {
        // std::cout << "\n from depth " << Udp->id_sequence << " lastid for snapshot " << lastId_Snapshot;
        if (Udp->isitBid)
        {
            // std::cout << "\n adding bid, ", Udp->price ," ", Udp->quantity;
            bids.insert_or_assign(Udp->price, Udp->quantity);
        }
        else
        {
            // std::cout << "\n adding ask, ", Udp->price ," ", Udp->quantity;
            asks.insert_or_assign(Udp->price, Udp->quantity);
        };
    };
    localId = Udp->id_sequence;
    // std::cout << "\n local id is, " << localId;
    HFTProfiler::instance().recordOrderBookSize(
        obook.bids.size(), obook.asks.size());
};

void orderbook::printTop(int levels)
{
    std::cout << "\n--- top " << levels << " asks (lowest first) ---";
    int i = 0;
    for (auto it = asks.begin(); it != asks.end() && i < levels; ++it, ++i)
    {

        auto priceS = std::to_string(it->first);
        auto price = priceS.insert(5, ".");

        auto amount = (it->second / 100000.0);
        // std::cout << "\n raw " << it->first << " " << it->second;
        std::cout << "\n ask: " << price << " qty: " << amount;
    }

    std::cout << "\n--- top " << levels << " bids (highest first) ---";
    i = 0;
    for (auto it = bids.begin(); it != bids.end() && i < levels; ++it, ++i)
    {
        auto amount = it->second / 100000.0;
        auto priceS = std::to_string(it->first);
        auto price = priceS.insert(5, ".");

        // std::cout << "\n raw " << it->first << " " << it->second;
        std::cout << "\n bid: " << price << " qty: " << amount;
    }
    std::cout << "\n --- raw spread " << asks.begin()->first << " b: " << bids.begin()->first << "--\n";
    auto priceA = asks.begin()->first;
    auto priceB = bids.begin()->first;
    int64_t diff = priceA - priceB;
    // std::cout << " " << diff;
    std::cout << "\n--- spread: " << diff * 0.01 << " ---\n";
}
