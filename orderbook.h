#include <iostream>
#include <map>
#include <functional> 

class orderbook{
    std::map<long double, long double> asks;  //sorted, lowest first
    std::map<long double,long double, std::greater<long double>> bids;  //'unsorted', highest first
    int totalEnterys = 0;
    public: 
    void init();
    void addingEntery(long double price, long double amount, bool isbid);
    void updateDepthBased(struct DepthUpdate* Udp);
    void printTop(int levels);
};
extern orderbook obook;