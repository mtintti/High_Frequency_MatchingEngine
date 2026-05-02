#include <iostream>
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <numeric>
#include <vector>
#include "websocket.h"
#include "mempool.h"
// #include "mempool.cpp"
using depthPool = MemoryPool<sizeof(DepthUpdate)>;
using BuffersPool = MemoryPool<8000>;

class session : public std::enable_shared_from_this<session>
{
    boost::asio::ip::tcp::resolver resolver;
    boost::asio::strand<boost::asio::io_context::executor_type> strandWs;
    // boost::beast::websocket::stream<boost::beast::tcp_stream> ws;
    //  non ssl one^
    boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>> ws;
    std::string hostHTTPheader;
    boost::beast::flat_buffer flatbuff;
    std::string textvar;
    int totalCallsHaveHappened = 0;
    boost::json::stream_parser streamParser;
    

    // std::chrono::high_resolution_clock::time_point starttime;
    //  boost::property_tree::ptree pt;

public:
    session(boost::asio::io_context &ioc, boost::asio::ssl::context &coSSL)
        : resolver(ioc),
          strandWs(ioc.get_executor()),
          ws(boost::asio::make_strand(ioc), (coSSL))

    {
        websocketTradeStruct *entery = (websocketTradeStruct *)malloc(10 * sizeof(websocketTradeStruct));
        std::cout << "\n is our websocketTradeStruct a pod? if yes nothing is seen";
        std::cout << "\n";
        static_assert(std::is_pod<websocketTradeStruct>::value, "the struct is not a pod");
    }

    void run(const char *host, const char *port, const char *endpoint)
    {

        std::cout << "\n in run() with host: " << host << " port: " << port << " in endpoint: " << endpoint;
        resolver.async_resolve(
            host,
            port,
            boost::beast::bind_front_handler(
                &session::on_resolve,
                shared_from_this()));
        std::cout << "\n binded and on_resolve call done!";
        hostHTTPheader = host;
        std::cout << "\n endpoint found in run websocket: " << endpoint;
    }

    void on_resolve(boost::system::error_code ec,
                    boost::asio::ip::tcp::resolver::results_type results)
    {

        // ws boost::beast::websocket;
        std::cout << "\n on resolve..";
        if (ec)
        {
            std::cout << "\n error in onresolve: " << ec.message();
        }
        std::cout << "\nResolved endpoints:\n";

        for (const auto &entry : results)
        {
            auto endpoint = entry.endpoint();

            std::cout
                << endpoint.address().to_string()
                << ":"
                << endpoint.port()
                << std::endl;
        }
        std::cout << "\n next is hostname: ";

        for (const auto &entry : results)
        {
            std::cout
                << entry.host_name() << " -> "
                << entry.endpoint().address().to_string()
                << ":" << entry.endpoint().port()
                << std::endl;
            hostHTTPheader = entry.host_name();
        }
        boost::beast::get_lowest_layer(ws).expires_after(std::chrono::minutes(5));
        boost::beast::get_lowest_layer(ws).async_connect(results, boost::asio::bind_executor(strandWs, boost::beast::bind_front_handler(&session::connectOn, shared_from_this())));

        std::cout << "Resolved successfully, moving to connect in lower layer\n";
    }

    void connectOn(boost::beast::error_code errorcode, boost::asio::ip::tcp::resolver::results_type::endpoint_type endpoint)
    {
        boost::beast::get_lowest_layer(ws).expires_after(std::chrono::minutes(5));
        if (errorcode)
        {
            std::cout << "error in onconnect: " << errorcode.message();
            // std::cout << "\n" << boost::beast::websocket::error
        };
        // old web async call without ssl
        /*boost::beast::websocket::response_type res;

        hostHTTPheader ;
        std::cout << "\n hosthttpheader " << hostHTTPheader;
        std::cout << "\n endpoint " << endpoint;
       std::string stringEndpoint;
       stringEndpoint = endpoint.address().to_string();
        //ws.async_handshake(hostHTTPheader, endpoint, boost::asio::bind_executor(strandWs, boost::beast::bind_front_handler(&session::handshakeForMessage, shared_from_this())));
        //ws.async_handshake(hostHTTPheader, endpoint, [&res](boost::beast::error_code ec));
        ws.async_handshake(hostHTTPheader,stringEndpoint,boost::asio::bind_executor(strandWs, boost::beast::bind_front_handler(&session::handshakeForMessage, shared_from_this())));
        if (errorcode)
        {
            std::cout << "\n error in async_handshake after call: " << errorcode.message();
            // std::cout << "\n" << boost::beast::websocket::error
        };*/
        ws.next_layer().async_handshake(boost::asio::ssl::stream_base::client, boost::asio::bind_executor(strandWs, boost::beast::bind_front_handler(&session::when_in_ssl_handshake, shared_from_this())));
    };

    void when_in_ssl_handshake(boost::beast::error_code errorcode)
    {
        if (errorcode)
        {
            std::cout << "\n in ssl handshake part, " << errorcode.message();
        };
        std::cout << "\n webstruct: " << hostHTTPheader << " " << endpoint;
        ws.async_handshake(hostHTTPheader, "/ws/btcusdt@depth@100ms", boost::asio::bind_executor(strandWs, boost::beast::bind_front_handler(&session::handshakeForMessage, shared_from_this())));
    }

    void handshakeForMessage(boost::beast::error_code errorcode)
    {
        if (errorcode)
        {
            std::cout << "\n error in handshake: " << errorcode.message();
        };
        ws.async_read(
            flatbuff,
            boost::asio::bind_executor(
                strandWs,
                boost::beast::bind_front_handler(
                    &session::on_read,
                    shared_from_this())));
    }

    void
    on_write(
        boost::beast::error_code ec,
        std::size_t bytes_transferred)
    {
        // Supress unused variable compiler warnings
        // bytes_transferred contains total bytes read/written to the websocket stream
        boost::ignore_unused(bytes_transferred);

        if (ec)
            std::cout << "\n error in on_write: " << ec.message();

        // Read single message
        ws.async_read(
            flatbuff,
            boost::asio::bind_executor(strandWs, boost::beast::bind_front_handler(&session::on_read, shared_from_this())));
    }

void on_read(
    boost::beast::error_code ec,
    std::size_t bytes_transferred)
{
    // std::cout << "\n does this happen more than ones :?";

    const auto starttime = std::chrono::high_resolution_clock::now();
    boost::ignore_unused(bytes_transferred);

    if (ec)
        std::cout << "\n error in on_read " << ec.message();
    // flatbuff.data()

    if (ec == boost::asio::error::eof)
    {
        std::cout << "connection closed by peer :<\n";
        return;
    }
    std::error_code jsonError;
    boost::system::error_code systemError;
    auto gotdata = flatbuff.data();
    streamParser.reset();
    //std::string_view stringview(static_cast<const char*>(gotdata.data()), gotdata.size());
    streamParser.write(static_cast<const char*>(gotdata.data()), gotdata.size(), jsonError);
    //std::error_code jsonError;
    //auto parser = boost::json::parse(stringview, jsonError);
    flatbuff.consume(flatbuff.size());
    if(jsonError){
        std::cout << "\n JSON parse error: " << jsonError.message();
    };
    if(!streamParser.done()){
        std::cout << "\n parser is not quite done. skipping it..";
    };
    auto parserDone = streamParser.release();
    auto& dataObj = parserDone.as_object();
    std::string_view symbol = dataObj.at("s").as_string();
    auto timeOfEvent = dataObj.at("E").as_int64();
    auto lastID = dataObj.at("u").as_int64();
    auto num = 0;
    for(const auto* nestedAskBid : {"b", "a"}){
        bool bidExist = (nestedAskBid[0] == 'b');
        std::cout << "\n is bid (true) or ask (false)? ";
        std::cout <<bidExist ;
        auto& nestedArray = dataObj.at(nestedAskBid).as_array();
        
        for(auto& entry : nestedArray){
            std::cout << "\n num of pair: " <<num;
            num++;
            auto& pair = entry.as_array();
            double priceOf = std::stod(std::string(pair[0].as_string()));
            double quantityAmount = std::stod(std::string(pair[1].as_string()));
            void* mpool = depthPool::instance().allocate();
            DepthUpdate* dUp = new(mpool) DepthUpdate{};
            if(bidExist){
                dUp->price=priceOf;
                dUp->isitBid=true;
            } else {
                dUp->price=priceOf;
                dUp->isitBid=false;
            }
            dUp->quantity=quantityAmount;
            std::cout << "\n price "<< dUp->price << ", quantity "<< dUp->quantity;  
        };
    }
    
    // std::cout << "\n duration of call, " << duration;

    // Read single message
    ws.async_read(
        flatbuff,
        boost::asio::bind_executor(strandWs, boost::beast::bind_front_handler(&session::on_read, shared_from_this())));
    // beast::bind_front_handler(
    //     &session::on_read,
    //     shared_from_this()));
}

// Check how to close when a signal handler is triggered? does the websocket auto close?
void on_close(boost::beast::error_code ec)
{
    if (ec)
        std::cout << "\n error in on_close" << ec.message();

    // If we get here then the connection is closed gracefully
    // The make_printable() function helps print a ConstBufferSequence
    // std::cout << boost::beast::make_printable(flatbuff.data()) << std::endl; print data out
}
};

void websocketsTrade(
    boost::asio::io_context &ioc, boost::asio::ssl::context &coSSL,
    const char *host,
    const char *port, const char *endpoint)
{
    // next is our async operation for our shared session to make sure
    // there are no threads open when connection is shut
    std::make_shared<session>(ioc, coSSL)->run(host, port, endpoint);
}