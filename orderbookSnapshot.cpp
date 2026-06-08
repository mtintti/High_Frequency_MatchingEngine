
// api.binance.com/api/v3/depth?symbol=BTCUSDT&limit=1000

#include <cstdlib>
#include <iostream>
#include <string>
#include "orderbookSnapshot.h"
#include "windcertsload.h"
#include <regex>
#include "orderbook.h"
#include "profile.h"
#include "fast_float.h"

namespace beast = boost::beast;
namespace http = beast::http;   
namespace net = boost::asio;   
using tcp = net::ip::tcp;       
boost::json::stream_parser streamParser;

typedef std::pair<int, int> SplitFloat;
SplitFloat split(double val, int pres)
{
    double left = std::floor(val);
    double right = (val - left) * double(std::pow(10, pres));
    return SplitFloat(left, right);
};


// Performs an HTTP GET and prints the response
int getRequestOrderBook(boost::asio::ssl::context &contextSSL)
{
    try
    {
        // Usage: http-client-sync <host> <port> <target> [<HTTP version: 1.0 or 1.1(default)>]\n"
        auto const host = "api.binance.com";
        auto const port = "443";
        auto const target = "/api/v3/depth?symbol=BTCUSDT"; // &limit=1000;
        int version = 11;

        // The io_context is required for all I/O
        net::io_context ioc;

        // These objects perform our I/O
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc); // was ioc

        // Look up the domain name
        auto const results = resolver.resolve(host, port);
        for (auto &r : results)
        {
            auto endpoint = r.endpoint();
            auto name = r.host_name();
        }
        boost::system::error_code ec;

        if (ec)
        {
            std::cout << "\n failed to load verify paths: " << ec.message();
            return EXIT_FAILURE;
        }
        contextSSL.set_verify_mode(boost::asio::ssl::verify_peer | boost::asio::ssl::verify_fail_if_no_peer_cert);
        if (ec)
        {
            std::cout << "\n error " << ec.message();
            std::cout << "\n loc: " << ec.location();
            std::cout << "\n what?: " << ec.what();
        } // Make the connection on the IP address we get from a lookup
        HFTProfiler::SNAPSHOT_GET;
        boost::asio::ssl::stream<tcp::socket> socket(ioc, contextSSL);
        socket.set_verify_callback(boost::asio::ssl::host_name_verification(host));
        socket.set_verify_mode(boost::asio::ssl::verify_peer);
        SSL_set_tlsext_host_name(socket.native_handle(), host);

        connect(socket.lowest_layer(), results);
        socket.lowest_layer().set_option(tcp::no_delay(true));
        socket.handshake(boost::asio::ssl::stream_base::client, ec);
        if (ec)
        {
            std::cerr << "\n HANDSHAKE FAILED: " << ec.message()
                      << " value=" << ec.value()
                      << " category=" << ec.category().name();
            return EXIT_FAILURE;
        }
        http::request<http::string_body> req{http::verb::get, target, version};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        http::write(socket, req);
        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(socket, buffer, res);
        auto stringres = res.body();
        HFTProfiler::SNAPSHOT_PARSE;
        int num = 0;
        bool insidebraket = false;
        bool outsidebraket = false;
        bool priceToAmount = false;
        bool isbid = false;
        bool priceGone = false;
        int bora = 0;
        std::string prices;
        std::string amounts;
        int posb = stringres.find("bid");
        int posa = stringres.find("ask");
        long long parsedId;
        // std::cout << "\n was b or ask found? where " << posb << " , a: " << posa;
        std::string suba = stringres.substr(posa);
        std::string subb = stringres.substr(posb);
        std::regex idPattern(R"("lastUpdateId":(\d+))");
        std::smatch match;
        int64_t lastId;

        std::string frac1;
        std::string frac2;
        obook.clearBooks();

        if (std::regex_search(stringres, match, idPattern))
        {
            long long parsedId = std::stoll(match[1]);
            std::cout << "\n id: " << parsedId;
            lastId = parsedId;
        }


        for (auto c : subb)
        {
            num++;
            if (outsidebraket == false && c == '[')
            {
                outsidebraket = true;
            }
            else if (outsidebraket == true && c == '[')
            {
                insidebraket = true;
            }

            if (c == ']')
            {
                insidebraket = false;
                priceToAmount = false;
            }
            else if (insidebraket == false && c == ']')
            {
                // closing the outer bids or asks array
                outsidebraket = false;
                
            } // new end ^^
            // prev code below
            if (c == 'b' || c == 'a')
            {
                // std::cout << "\n b or a found " << c;
                if (isbid == true)
                {
                    isbid = false;
                }
                else
                {
                    isbid = true;
                }
                // std::cout << "\n isbid (1) or (0) " << isbid;
                bora++;
            }

            if (insidebraket == true && c == '"')
            {

                if (priceGone == false && prices.find('.') != -1)
                {
                    int pricesDot = prices.find('.');
                
                    frac1 = prices[pricesDot +1];
                    frac2 = prices[pricesDot +2];
                    
                    priceGone = true;
                }
                else if (priceGone == true && amounts.find('.') != -1)
                {
                    int amountDot = amounts.find('.');                    
                    double priceOf = stod(prices);
                    double quantityAmount = stod(amounts);
                    SplitFloat p = split(priceOf, 2);
                    std::string price5strings = std::to_string(p.first) + frac1+ frac2;
                    int64_t price = std::stoll(price5strings);
                    int64_t amount = quantityAmount * 100000.0;

                    obook.addingEntery(price, amount, isbid);
                    prices.clear();
                    amounts.clear();
                    priceGone = false;
                };
            }
            else if (insidebraket == true && c == ',')
            {
                priceToAmount = true;
            }
            else if (insidebraket == true && c != ',' && priceToAmount != true && c != '[' && c != ']')
            {
                prices += c;
            }
            else if (insidebraket == true && priceToAmount == true)
            {
                amounts += c;
            }
        };
        obook.lastId_Snapshot = lastId;
        obook.localId = lastId;
        obook.isin_Sync = false;
        

        std::cout << "\n message ended. shutting down..";

        // Gracefully close the socket
        // stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        socket.shutdown(ec);
        if (ec &&
            ec != beast::errc::not_connected)
        {
            std::cerr << "\n unexpected shutdown error: " << ec.message();
            throw beast::system_error{ec};
        }
        if (ec == net::ssl::error::stream_truncated)
        {
            std::cout << "\n connection trunc error";
            ec = {}; // clear it, not a real error
        }
        std::cout << "\n connection closed cleanly";

        // If we get here then the connection is closed gracefully
    }
    catch (std::exception const &e)
    {

        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
