
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

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http;   // from <boost/beast/http.hpp>
namespace net = boost::asio;    // from <boost/asio.hpp>
using tcp = net::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// Performs an HTTP GET and prints the response
int getRequestOrderBook(boost::asio::ssl::context& contextSSL)
{
    try
    {
        // Usage: http-client-sync <host> <port> <target> [<HTTP version: 1.0 or 1.1(default)>]\n"
        auto const host = "api.binance.com";
        auto const port = "443";
        auto const target = "/api/v3/depth?symbol=BTCUSDT&limit=1000";
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
            std::cout << "\n in orderSnapshot resolved domain endpoint " << r.endpoint() << " , name " << r.host_name();
            // is_valid_utf8(endpoint);
            // is_valid_utf8(name);
        }

        // next ssl context, and check of verify
        //boost::asio::ssl::context contextSSL(boost::asio::ssl::context::method::sslv23_client);
        boost::system::error_code ec;
        // This is the missing line — loads your OS trusted CA store
        //contextSSL.set_default_verify_paths(ec);
        //windowsCertificateStore(contextSSL);
        
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
        boost::asio::ssl::stream<tcp::socket> socket(ioc, contextSSL);
        // tcp::socket::lowest_layer_type &sock = socket.lowest_layer();
        socket.set_verify_callback(boost::asio::ssl::host_name_verification(host));
        socket.set_verify_mode(boost::asio::ssl::verify_peer);
        std::cout << "\n verify mode and callback done..";

        std::cout << "\n moving to connect";
        SSL_set_tlsext_host_name(socket.native_handle(), host);

        connect(socket.lowest_layer(), results);
        socket.lowest_layer().set_option(tcp::no_delay(true));

        std::cout << "\n connect done then hanshake..";

        // socket.set_verify_callback(boost::asio::ssl::host_name_verification(host));
        socket.handshake(boost::asio::ssl::stream_base::client, ec);
        if (ec)
        {
            std::cerr << "\n HANDSHAKE FAILED: " << ec.message()
                      << " value=" << ec.value()
                      << " category=" << ec.category().name();
            return EXIT_FAILURE;
        }
        std::cout << "\n handshake done, get req..";

        // Set up an HTTP GET request message
        http::request<http::string_body> req{http::verb::get, target, version};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        // Send the HTTP request to the remote host
        // http::write(stream, req);
        http::write(socket, req);
        std::cout << "\n GET req ";
        std::cout << "\n sending GET req to host (aka binance).. ";
        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::dynamic_body> res;
        // Receive the HTTP response
        http::read(socket, buffer, res);
        for (auto &r : res)
        {
            auto name = r.name_string();
            auto parent = r.parent_;
            auto next = r.next_;
            auto prev = r.prev_;
            std::cout << "\n reading what ever we have, maybe?? name: " << name << " parent: " << parent << " next: " << next << " prev: " << prev;
        }
        std::cout << "\n response received";
        std::cout << "\n response status: " << res.result_int();
        std::cout << "\n body size: " << boost::beast::buffers_to_string(res.body().data()).size();
        std::cout << res << std::endl;

        // Write the message to standard out
        std::cout << res << std::endl;
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
