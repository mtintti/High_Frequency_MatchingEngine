#include <iostream>
#include <string>
#include "../profile.h"
#include "../memory/mempool.h"
#include "../memory/add_memEvent.h"
#include "read_parser.h"
#include "../main.h"


typedef std::pair<int, int> SplitFloat;
    SplitFloat split(float val, int pres)
    {
        float left = std::floor(val);
        //std::cout << "\n left: " << left << ", val - left "<< val << " " << left;
        float right = (val - left) * float(std::pow(10, pres));
        return SplitFloat(left, right);
    };

void parser_json(const boost::json::object dataObj){
    
    std::string_view symbol = dataObj.at("s").as_string();
        auto timeOfEvent = dataObj.at("E").as_int64();
        auto firstID = dataObj.at("U").as_int64();
        auto lastID = dataObj.at("u").as_int64();
        auto num = 0;
        for (const auto *nestedAskBid : {"b", "a"})
        {
            bool bidExist = (nestedAskBid[0] == 'b');
            auto &nestedArray = dataObj.at(nestedAskBid).as_array();

            for (auto &entry : nestedArray)
            {
                num++;
                auto &pair = entry.as_array();
                std::string priceOfs = (std::string(pair[0].as_string()));
                std::string quantityAmounts = (std::string(pair[1].as_string()));

                float priceOf = stof(priceOfs); // was stof, for float and not long
                std::string frac1;
                std::string frac2;
                int dot = priceOfs.find('.');
                frac1 = priceOfs[dot +1];
                frac2 = priceOfs[dot +2];

                double quantityAmount = stod(quantityAmounts);
                SplitFloat p = split(priceOf,3);
                std::string price5strings = std::to_string(p.first) + frac1 + frac2;
                int64_t price5 = std::stoll(price5strings);
                int64_t amount5 = quantityAmount * 100000.0;
                memoryEvent(price5, amount5, bidExist, firstID, lastID);

                // std::cout << "\n converted ";
                //std::cout << "\n Pr: " << price5;
                //std::cout <<" am: " <<amount5;

                

            };
        }
}