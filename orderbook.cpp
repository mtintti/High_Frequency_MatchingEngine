#include "orderbook.h"
#include "mempool.h"

/*void orderbook::init()
{
}
*/
orderbook obook;
void orderbook::addingEntery(long double price, long double amount, bool isbid)
{
    // std::cout << "\n in orderbook";
    // std::cout << " got price "<< price << " , amount " << amount;
    totalEnterys++;
    std::cout << "\n total enterys " << totalEnterys;
    /*if(isbid == true){
        bids.emplace(price, amount);
        std::cout << "bids hashmap size " << bids.size();
    } else {
        asks.emplace(price, amount);
        std::cout << "asks hashmap size " << asks.size();
    }*/

    if (amount == 0.0L)
    {
        // quantity zero means remove this price level
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
            bids[price] = amount; // insert or update
            std::cout << "\n bids size " << bids.size();
        }
        else
        {
            asks[price] = amount; // insert or update
            std::cout << "\n asks size " << asks.size();
        }
    }
}

void orderbook::updateDepthBased(struct DepthUpdate* Udp)
{
    std::cout << " \n what got in update depth based";
    std::cout << "p: "<<Udp->price << " q: " << Udp->quantity << " idofseq: " << Udp->id_sequence<< " isbid?: " << Udp->isitBid;
    std::cout <<"\n";

}
