#include <iostream>
#include <map>
#include <functional> 

class orderbook{
    std::map<int64_t, int64_t, std::less<int64_t>> asks;  //sorted, lowest first
    std::map<int64_t, int64_t,std::greater<int64_t>> bids;  //'unsorted', highest first
    // seeing if revers would be optimal for performance
    //std::map<int64_t,int64_t, std::greater<int64_t>> asks;  //sorted, lowest first. 
    //std::map<int64_t, int64_t,std::less<int64_t>> bids;  //'unsorted', highest first
    int totalEnterys = 0;
    public: 
    void init();
    void addingEntery(int64_t price, int64_t amount, bool isbid);
    void updateDepthBased(struct DepthUpdate* Udp);
    void printTop(int levels);
    
};
extern orderbook obook;