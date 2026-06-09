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
#include "regex"


using depthPool = MemoryPool<sizeof(DepthUpdate)>;
using BuffersPool = MemoryPool<8000>;
int totalcount = 0;
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

public:
    session(boost::asio::io_context &ioc, boost::asio::ssl::context &coSSL)
        : resolver(ioc),
          strandWs(ioc.get_executor()),
          ws(boost::asio::make_strand(ioc), (coSSL))

    {
        depthPool::instance().prewarmAtStart(8000);
    };

    void run(const char *host, const char *port, const char *endpoint)
    {

        resolver.async_resolve(
            host,
            port,
            boost::beast::bind_front_handler(
                &session::on_resolve,
                shared_from_this()));
        hostHTTPheader = host;
    }

    void on_resolve(boost::system::error_code ec,
                    boost::asio::ip::tcp::resolver::results_type results)
    {

        std::cout << "\n on resolve..";
        if (ec)
        {
            std::cout << "\n error in onresolve: " << ec.message();
        }
       
        boost::beast::get_lowest_layer(ws).expires_after(std::chrono::minutes(20));
        boost::beast::get_lowest_layer(ws).async_connect(results, boost::asio::bind_executor(strandWs, boost::beast::bind_front_handler(&session::connectOn, shared_from_this())));
    }

    void connectOn(boost::beast::error_code errorcode, boost::asio::ip::tcp::resolver::results_type::endpoint_type endpoint)
    {
        boost::beast::get_lowest_layer(ws).expires_after(std::chrono::minutes(20));
        if (errorcode)
        {
            std::cout << "error in onconnect: " << errorcode.message();
        };

        ws.next_layer().async_handshake(boost::asio::ssl::stream_base::client, boost::asio::bind_executor(strandWs, boost::beast::bind_front_handler(&session::when_in_ssl_handshake, shared_from_this())));
    };

    void when_in_ssl_handshake(boost::beast::error_code errorcode)
    {
        if (errorcode)
        {
            std::cout << "\n in ssl handshake part, " << errorcode.message();
        };
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
        boost::ignore_unused(bytes_transferred);

        if (ec)
            std::cout << "\n error in on_write: " << ec.message();

        // Read single message
        ws.async_read(
            flatbuff,
            boost::asio::bind_executor(strandWs, boost::beast::bind_front_handler(&session::on_read, shared_from_this())));
    }
    typedef std::pair<int, int> SplitFloat;
    SplitFloat split(float val, int pres)
    {
        float left = std::floor(val);
        //std::cout << "\n left: " << left << ", val - left "<< val << " " << left;
        float right = (val - left) * float(std::pow(10, pres));
        return SplitFloat(left, right);
    };


    void on_read(
        boost::beast::error_code ec,
        std::size_t bytes_transferred)
    {

        auto ws_start = std::chrono::high_resolution_clock::now();
        boost::ignore_unused(bytes_transferred);

        if (ec)
            std::cout << "\n error in on_read " << ec.message();

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

                // std::cout << "\n converted ";
                //std::cout << "\n Pr: " << price5;
                //std::cout <<" am: " <<amount5;

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
                dUp->U_first = firstID;
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
                if (totalcount > 700)
                {
                    /*obook.asksSize();
                    obook.bidsSize();
                    obook.printTop(5);
                    std::cout << "\n sync (1) or not (0)? " << obook.isin_Sync;
                    std::cout << "\n websock priceOf: " << priceOf << " qAm "<< quantityAmount;
                    std::cout << "\n after conversion ";
                    std::cout << price5 << " " << amount5;*/
                    totalcount = 0;
                }

            };
        }

        {
            auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                          std::chrono::high_resolution_clock::now() - ws_start)
                          .count();
            HFTProfiler::instance().recordStage(
                HFTProfiler::WS_READ, static_cast<uint64_t>(us));
        }
       
        ws.async_read(
            flatbuff,
            boost::asio::bind_executor(strandWs, boost::beast::bind_front_handler(&session::on_read, shared_from_this())));
    }

    void on_close(boost::beast::error_code ec)
    {
        if (ec)
            std::cout << "\n error in on_close" << ec.message();
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