#include <iostream>
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <numeric>
#include <vector>
#include "websocket.h"
#include "mempool.h"
#include "orderbook.h"
#include "profile.h"
#include "fast_float.h"
// #include "mempool.cpp"
using depthPool = MemoryPool<sizeof(DepthUpdate)>;
using BuffersPool = MemoryPool<8000>;
int totalcount = 0;
// template MemoryPool<8000>;
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
    std::atomic<size_t> pool_blocks_in_use;

    // std::chrono::high_resolution_clock::time_point starttime;
    //  boost::property_tree::ptree pt;

public:
    session(boost::asio::io_context &ioc, boost::asio::ssl::context &coSSL)
        : resolver(ioc),
          strandWs(ioc.get_executor()),
          ws(boost::asio::make_strand(ioc), (coSSL))

    {
        depthPool::instance().prewarmAtStart(8000);
        // std::cout << "\n";
        // std::cout << "\n depth mempool size " << sizeof(depthPool);
    };

    void run(const char *host, const char *port, const char *endpoint)
    {

        // std::cout << "\n in run() with host: " << host << " port: " << port << " in endpoint: " << endpoint;
        resolver.async_resolve(
            host,
            port,
            boost::beast::bind_front_handler(
                &session::on_resolve,
                shared_from_this()));
        // std::cout << "\n binded and on_resolve call done!";
        hostHTTPheader = host;
        // std::cout << "\n endpoint found in run websocket: " << endpoint;
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
        // std::cout << "\nResolved endpoints:\n";

        /*for (const auto &entry : results)
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
        }*/
        boost::beast::get_lowest_layer(ws).expires_after(std::chrono::minutes(20));
        boost::beast::get_lowest_layer(ws).async_connect(results, boost::asio::bind_executor(strandWs, boost::beast::bind_front_handler(&session::connectOn, shared_from_this())));
        // std::cout << "Resolved successfully, moving to connect in lower layer\n";
    }

    void connectOn(boost::beast::error_code errorcode, boost::asio::ip::tcp::resolver::results_type::endpoint_type endpoint)
    {
        boost::beast::get_lowest_layer(ws).expires_after(std::chrono::minutes(20));
        if (errorcode)
        {
            std::cout << "error in onconnect: " << errorcode.message();
            // std::cout << "\n" << boost::beast::websocket::error
        };

        ws.next_layer().async_handshake(boost::asio::ssl::stream_base::client, boost::asio::bind_executor(strandWs, boost::beast::bind_front_handler(&session::when_in_ssl_handshake, shared_from_this())));
    };

    void when_in_ssl_handshake(boost::beast::error_code errorcode)
    {
        if (errorcode)
        {
            std::cout << "\n in ssl handshake part, " << errorcode.message();
        };
        // std::cout << "\n webstruct: " << hostHTTPheader << " " << endpoint;
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
        // std::cout << "\n size of memorypool: "<< sizeof(MemoryPool);
        // commented out for profiling std::cout << "\n size of bufferspool: " << sizeof(BuffersPool);

        auto ws_start = std::chrono::high_resolution_clock::now();
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
        {
            ScopedTimer t(HFTProfiler::JSON_PARSE);
            streamParser.write(
                static_cast<const char *>(gotdata.data()),
                gotdata.size(), jsonError);
        };
        // std::string_view stringview(static_cast<const char*>(gotdata.data()), gotdata.size());
        // commented out for profiling
        /*streamParser.write(static_cast<const char*>(gotdata.data()), gotdata.size(), jsonError);*/
        // std::error_code jsonError;
        // auto parser = boost::json::parse(stringview, jsonError);
        flatbuff.consume(flatbuff.size());
        if (jsonError)
        {
            std::cout << "\n JSON parse error: " << jsonError.message();
        };
        if (!streamParser.done())
        {
            std::cout << "\n parser is not quite done. skipping it..";
            std::cout << streamParser.release();
        };
        auto parserDone = streamParser.release();
        auto &dataObj = parserDone.as_object();
        std::string_view symbol = dataObj.at("s").as_string();
        auto timeOfEvent = dataObj.at("E").as_int64();
        auto lastID = dataObj.at("u").as_int64();
        auto num = 0;
        for (const auto *nestedAskBid : {"b", "a"})
        {
            bool bidExist = (nestedAskBid[0] == 'b');
            // std::cout << "\n is bid (true) or ask (false)? ";
            // std::cout << bidExist;
            auto &nestedArray = dataObj.at(nestedAskBid).as_array();

            for (auto &entry : nestedArray)
            {
                // std::cout << "\n num of pair: " <<num;
                num++;
                auto &pair = entry.as_array();
                // long double priceOf = std::stod(std::string(pair[0].as_string()));
                // long double quantityAmount = std::stod(std::string(pair[1].as_string()));

                double priceOf = 0.0, quantityAmount = 0.0;
                auto pStr = pair[0].as_string();
                auto qStr = pair[1].as_string();
                fast_float::from_chars(pStr.data(), pStr.data() + pStr.size(), priceOf);
                fast_float::from_chars(qStr.data(), qStr.data() + qStr.size(), quantityAmount);

                // std::cout << "\n websock priceOf: " << priceOf << " qAm "<< quantityAmount;
                // std::cout << "\n";
                int64_t price5 = priceOf * 100000;
                // std::cout << "after 10^5";
                // std::cout << "\n Pr: " << price5;
                int64_t amount5 = quantityAmount * 100000; // was 100000
                // std::cout << "after 10^5";
                // std::cout <<" am: " <<amount5;

                

                void *mpool;
                {
                    ScopedTimer t(HFTProfiler::POOL_ALLOC);
                    mpool = depthPool::instance().allocate();
                    HFTProfiler::instance().recordPoolState(
                        ++pool_blocks_in_use,
                        8000);
                }
                DepthUpdate *dUp = new (mpool) DepthUpdate{};
                if (bidExist)
                {
                    dUp->price = price5;
                    dUp->isitBid = true;
                }
                else
                {
                    dUp->price = price5;
                    dUp->isitBid = false;
                }
                dUp->quantity = amount5;
                dUp->id_sequence = lastID;
                totalcount++;

                {
                    ScopedTimer t(HFTProfiler::ORDERBOOK_UPDATE);
                    obook.updateDepthBased(dUp);
                }

                {
                    ScopedTimer t(HFTProfiler::POOL_DEALLOC);
                    dUp->~DepthUpdate();
                    depthPool::instance().deallocate(mpool);
                    --pool_blocks_in_use;
                }
                if (totalcount > 500)
                {
                    // obook.printTop(5);
                    //  std::cout << "\n websock priceOf: " << priceOf << " qAm "<< quantityAmount;
                    //  std::cout << "\n after conversion ";
                    //  std::cout << price5 << " " << amount5;
                    totalcount = 0;
                }

                // std::cout << "\n size of DepthUpdate struct: " << sizeof(DepthUpdate);
                // std::cout << "\n size of depthPool block:    " << sizeof(depthPool);
                // std::cout << "\n actual struct via pointer:  " << sizeof(*dUp);
            };
        }

        {
            auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                          std::chrono::high_resolution_clock::now() - ws_start)
                          .count();
            HFTProfiler::instance().recordStage(
                HFTProfiler::WS_READ, static_cast<uint64_t>(us));
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