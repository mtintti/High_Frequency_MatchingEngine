
// api.binance.com/api/v3/depth?symbol=BTCUSDT&limit=1000

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include "orderbookSnapshot.h"
#include "windcertsload.h"
#include <regex>
#include "orderbook.h"
#include "profile.h"
#include "fast_float.h"

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http;   // from <boost/beast/http.hpp>
namespace net = boost::asio;    // from <boost/asio.hpp>
using tcp = net::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
boost::json::stream_parser streamParser;

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
        // boost::asio::ssl::stream

        // Look up the domain name
        auto const results = resolver.resolve(host, port);
        for (auto &r : results)
        {
            auto endpoint = r.endpoint();
            auto name = r.host_name();
            // std::cout << "\n in orderSnapshot resolved domain endpoint " << r.endpoint() << " , name " << r.host_name();
            //  is_valid_utf8(endpoint);
            //  is_valid_utf8(name);
        }

        // next ssl context, and check of verify
        // boost::asio::ssl::context contextSSL(boost::asio::ssl::context::method::sslv23_client);
        boost::system::error_code ec;
        // This is the missing line — loads your OS trusted CA store
        // contextSSL.set_default_verify_paths(ec);
        // windowsCertificateStore(contextSSL);

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
        // stream.connect(results);
        HFTProfiler::SNAPSHOT_GET;
        boost::asio::ssl::stream<tcp::socket> socket(ioc, contextSSL);
        // tcp::socket::lowest_layer_type &sock = socket.lowest_layer();
        socket.set_verify_callback(boost::asio::ssl::host_name_verification(host));
        socket.set_verify_mode(boost::asio::ssl::verify_peer);
        // std::cout << "\n verify mode and callback done..";

        // std::cout << "\n moving to connect";
        SSL_set_tlsext_host_name(socket.native_handle(), host);

        connect(socket.lowest_layer(), results);
        socket.lowest_layer().set_option(tcp::no_delay(true));

        // std::cout << "\n connect done then hanshake..";

        // socket.set_verify_callback(boost::asio::ssl::host_name_verification(host));
        socket.handshake(boost::asio::ssl::stream_base::client, ec);
        if (ec)
        {
            std::cerr << "\n HANDSHAKE FAILED: " << ec.message()
                      << " value=" << ec.value()
                      << " category=" << ec.category().name();
            return EXIT_FAILURE;
        }
        // std::cout << "\n handshake done, get req..";

        // Set up an HTTP GET request message
        http::request<http::string_body> req{http::verb::get, target, version};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        // Send the HTTP request to the remote host
        // http::write(stream, req);
        http::write(socket, req);
        // std::cout << "\n GET req ";
        // std::cout << "\n sending GET req to host (aka binance).. ";
        //  This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::string_body> res;
        // http::message<true, res;
        //  Receive the HTTP response
        // boost::json::object res;
        // boost::beast::flat_buffer res;
        // auto buffToString = boost::beast::http::basic_parser(res);
        http::read(socket, buffer, res);
        // std::cout << "\n"
        //           << std::endl;
        //  std::cout << res.body() << std::endl;
        auto stringres = res.body();
        // std::cout << stringres << std::endl;
        //  auto httpGot = streamParser.release(res);
        // std::cout << "\n GET red and stored..";
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

        if (std::regex_search(stringres, match, idPattern))
        {
            long long parsedId = std::stoll(match[1]);
            // std::cout << "\n id: " << parsedId;
        }

        // std::cout << "\n was b or ask found? where?? "<< suba;
        // std::cout << "\n";

        for (auto c : subb)
        {
            // std::cout << c;
            num++;

            // std::cout << "\n bools,  braket? " <<insidebraket << " and getting? " << getting;
            if (outsidebraket == false && c == '[')
            {
                outsidebraket = true;
                // std::cout << "\n \n found: [[ " << " at " << num;
            }
            else if (outsidebraket == true && c == '[')
            {
                insidebraket = true;
                // std::cout << "\n found: [ " << " at " << num;
            }

            if (c == ']')
            {
                insidebraket = false;
                priceToAmount = false;
                // std::cout << "\n found: ] " << " at " << num;
            }
            else if (insidebraket == false && c == ']')
            {
                // closing the outer bids or asks array
                outsidebraket = false;
                // std::cout << "\n\n found: ]] at " << num;
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
                // std::cout << "c is not num " << c;
                // std::cout << "\n clearing or skipping again but what?? ";

                if (priceGone == false && prices.find('.') != -1)
                {
                    // std::cout << " done with price ";
                    int pricesDot = prices.find('.');
                    // std::cout << " dot found at " << pricesDot;
                    priceGone = true;
                }
                else if (priceGone == true && amounts.find('.') != -1)
                {
                    // std::cout << " sending price ";
                    // std::cout << " sending amount ";
                    int amountDot = amounts.find('.');
                    // std::cout << " dot found at " << amountDot;
                    //std::cout << "\n snapshot: stdstring " << prices << " amount: "<< amounts;
                    int64_t pricestod = stoi(prices); // was int64_t and stod. originally long double
                    int64_t amountstod = stoi(amounts); // this too ^
                    //std::cout << " , snapshot: stod " << pricestod << " amount: "<< amountstod;

                    int64_t price = pricestod * 100000; //was 100000
                    int64_t amount = amountstod * 100000;
                    //std::cout << "\n snapshot: " << price << " amount: "<< amount;
                    /*long double price, amount;
                    fast_float::from_chars(prices.data(),prices.data(),prices.size(), price);
                    fast_float::from_chars(amounts.data(),amounts.data(),amounts.size(), amount);*/

                    obook.addingEntery(price, amount, isbid);
                    prices.clear();
                    amounts.clear();
                    priceGone = false;
                };
            }
            else if (insidebraket == true && c == ',')
            {
                // std::cout << "\n moving from price to amount";
                priceToAmount = true;
            }
            else if (insidebraket == true && c != ',' && priceToAmount != true && c != '[' && c != ']')
            {
                prices += c;
                // priceTotalAmount++;
                // std::cout << " c is num " << c << " as " << prices;
            }
            else if (insidebraket == true && priceToAmount == true)
            {
                amounts += c;
                // std::cout << " c is amount " << c << " as " << amounts;
            }
        };
        // std::cout << "\n total bid or ask amounts found " << bora;

        // only inside
        /*for (auto c : suba)
        {
            std::cout << "\n " << c;
            num++;
            //std::cout << "\n bools,  braket? " <<insidebraket << " and getting? " << getting;
            if (c == '[')
            {
                insidebraket = true;
                std::cout << " found: [ " << c << " at " << num;
            }
            if (c == '[')
            {
                insidebraket = true;
                std::cout << " found: [ " << c << " at " << num;
            }
            if (c == ']')
            {
                insidebraket = false;
                priceToAmount = false;
                std::cout << " found: ] " << c << " at " << num;
                if(amount.size() > 0){
                    std::cout << "\n clearing amount.. moving on to next bracket" ;
                    amount.clear();
                }
            }
            if (c == 'b' || c == 'a'){
                bora++;
            }

            if(insidebraket == true && c == '"'){
                std::cout <<  "c is not num " << c;
                std::cout <<  "\n clearing price again ";
                price.clear();

            } else if(insidebraket == true && c == ','){
                std::cout << "\n moving from price to amount";
                std::cout << c;
                priceToAmount = true;
            } else if (insidebraket == true && c != ',' && priceToAmount != true && c != '[' && c != ']'){
                price += c;
                //priceTotalAmount++;
                std::cout <<  " c is num " << c << " as " << price;

            } else if (insidebraket == true && priceToAmount == true){
                amount += c;
                std::cout <<  " c is amount " << c << " as " << amount;
            }
        };
        std::cout << "\n total bid or ask amounts found " << bora;*/

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
