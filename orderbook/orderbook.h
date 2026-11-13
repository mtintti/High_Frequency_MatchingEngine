#include <iostream>
#include <map>
#include <functional> 
extern std::atomic<bool>pausing_dUp;
extern std::atomic<int64_t> lastest_best_askprice;
extern std::atomic<int64_t> lastest_best_bidprice;
extern std::atomic<double> latest_best_ask_amount;
extern std::atomic<double> latest_best_bid_amount;

class orderbook{
    std::map<int64_t, int64_t, std::less<int64_t>> asks;  //sorted, lowest first
    std::map<int64_t, int64_t,std::greater<int64_t>> bids;  //'unsorted', highest first
    // seeing if revers would be optimal for performance
    //std::map<int64_t,int64_t, std::greater<int64_t>> asks;  //sorted, lowest first. 
    //std::map<int64_t, int64_t,std::less<int64_t>> bids;  //'unsorted', highest first
    int totalEnterys = 0;
    public: 
    void init();
    void addingEntery(int64_t price, int64_t amount, bool isbid, bool fromMatchingengine = false);
    void asks_current_price();
    void bids_current_price();
    void clearBooks();
    std::atomic<int64_t> lastId_Snapshot;
    std::atomic<int64_t> localId = 0;
    std::atomic<bool> isin_Sync = false;
    void updateDepthBased(struct DepthUpdate* Udp);
    void printTop(int levels);

    std::atomic<bool> pausing_dUp = false;
    
};
extern orderbook obook;
