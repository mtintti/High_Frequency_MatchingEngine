#include <iostream>
#include <unordered_map>
#include <map>

class orderbook{
    std::map<long double, long double> asks;  //sorted, lowest first
    std::unordered_map<long double, long double> bids;  //'unsorted', highest first
    int totalEnterys = 0;
    public: 
    void init();
    void addingEntery(long double price, long double amount, bool isbid);
    void updateDepthBased(struct DepthUpdate* Udp);
};
extern orderbook obook;