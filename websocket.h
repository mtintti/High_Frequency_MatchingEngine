#include <vector>
//SoA way, hyvä laskennalle tai suodatuksille, ei padding koska hakee vain
// tarvittavan arvon, esim. symbol x eikä kaikkia x enterys esim. eventType x, symbol x 
/*struct websocketTradeStruct
    {
    std::vector<int> x;
    std::vector<char> eventType; // was it an buy, trade or sell??
    std::vector<int> eventTime; // what time the event happened
    std::vector<char> symbol; //name of ticker symbol
    std::vector<char> priceChangenum;
    std::vector<char> priceChangepros; // prosentage of price change
    std::vector<char> weightAverage;
    std::vector<char> lastTickerprice;
    std::vector<char> lastQuantityamount;

};*/

#include <algorithm>
#include <boost/asio.hpp>
#include <thread>
#include <type_traits>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
namespace pt = boost::property_tree;
#include <boost/json.hpp>
// #include "root_certificates.hpp" skipping ssl for now

#include <boost/beast/core.hpp>
#include <boost/beast/core/buffers_range.hpp>
#include <openssl/conf.h>
#include <openssl/configuration.h>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <cstdio>
#include <boost/asio/signal_set.hpp>
#include <thread>
#include <set>
// test
#include <stdlib.h>
#include <stdio.h>
#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/detail/type_traits.hpp>
#include <boost/asio/buffer.hpp>
#include <string>

//pod hyvä datan hallintaan
struct websocketTradeStruct
    {
    int x;
    char eventType; // was it an buy, trade or sell??
    int eventTime; // what time the event happened
    char symbol; //name of ticker symbol
    char priceChangenum;
    char priceChangepros; // prosentage of price change
    char weightAverage;
    char lastTickerprice;
    char lastQuantityamount;

};

void websocketsTrade(boost::asio::io_context &ioc, boost::asio::ssl::context &coSSL, const char *host, const char *port, const char *endpoint);
/*making 10 enterys of websocketStructs for now..*/
/* not sure if that great :< */
websocketTradeStruct *entery = (websocketTradeStruct *)malloc(10 * sizeof(websocketTradeStruct));
