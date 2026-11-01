#include <iostream>
#include "network/websocket.h"
#include "main.h"
#include "marketData/marketdata_Snapshot.h"
#include "windcertsload.h"
#include "profile.h"
#include "orderbook/orderbook.h"
#include "clients/clients.h"
#include "memory/matchingEngine_pool.h"

std::vector<std::string> durationvec;
std::condition_variable cv;
std::mutex snapshotMutex;
std::atomic<int> pendingSnapshots;

inline void installTerminateHandler()
{
    std::set_terminate([]()
                       {
        std::cerr << "\n========== UNHANDLED EXCEPTION ==========\n";

        // rethrow so we can inspect the actual type
        try {
            std::rethrow_exception(std::current_exception());
        }
        catch (const boost::system::system_error& e) {
            std::cerr << " type    : boost::system::system_error\n";
            std::cerr << " what    : " << e.what()                  << "\n";
            std::cerr << " code    : " << e.code().value()           << "\n";
            std::cerr << " category: " << e.code().category().name() << "\n";
        }
        catch (const std::system_error& e) {
            std::cerr << " type    : std::system_error\n";
            std::cerr << " what    : " << e.what()                  << "\n";
            std::cerr << " code    : " << e.code().value()           << "\n";
            std::cerr << " category: " << e.code().category().name() << "\n";
        }
        catch (const std::invalid_argument& e) {
            std::cerr << " type    : std::invalid_argument\n";
            std::cerr << " what    : " << e.what() << "\n";
        }
        catch (const std::out_of_range& e) {
            std::cerr << " type    : std::out_of_range\n";
            std::cerr << " what    : " << e.what() << "\n";
        }
        catch (const std::exception& e) {
            std::cerr << " type    : std::exception\n";
            std::cerr << " what    : " << e.what() << "\n";
        }
        catch (...) {
            std::cerr << " type    : unknown (not derived from std::exception)\n";
        }

        std::cerr << "=========================================\n";
        std::cerr << std::flush;

        std::abort(); });
}

// globaalit std::thread bool trade threadille. std::atomic koska booleja käytää kaksi threadiä.
std::atomic<bool> trades_isrunning = false;
boost::asio::ssl::context contextSSL(boost::asio::ssl::context::method::sslv23_client);
boost::asio::ssl::context coSSL(boost::asio::ssl::context::sslv23_client);

int main(const int argc, char *argv[])
{
    installTerminateHandler();
    // std::cout << "hello";
    // std::cout << "\n high matching engine";
    windowsCertificateStore(contextSSL);

    srand(static_cast<unsigned>(time(nullptr)));
    Clients clients;
    int threads = 2;
    boost::asio::io_context ioc;
    windowsCertificateStore(coSSL);
    //getRequestOrderBook(contextSSL);
    
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);

    signals.async_wait([&](auto, auto){
        trades_isrunning = false;
        cv.notify_all(); // huomautetaan marketData threadlle että lopetetaan odottaminen -> ohjelma sulkeutuu
        ioc.stop(); });

    
    websocketsTrade(ioc, coSSL, host, port, endpoint);
    clients.createClients(900);
    matchPool::prewarmAtStart(2000000);

    // std::cout << "getmarketdata main "<< get_market_data; std::unique_lock lock(m);

    // trades for matching engine and client creation and their tradeBehavior in a separate thread
    std::thread trades([&]{
        while (trades_isrunning){
            if(obook.isin_Sync == true){ 
            clients.generateTradeBehavior();
            };
        }; 
    });

    std::thread marketData_sync([&]{
        while (trades_isrunning){

        /*if(get_market_data == true){
            //std::cout << "\n marketData_sync getting snapshot.. ";
            getRequestOrderBook(contextSSL);

            get_market_data = false;
            //std::cout << "\n marketData_sync done snapshot.. ";*/
            std::unique_lock<std::mutex> lock(snapshotMutex);

            cv.wait(lock, []{ return pendingSnapshots > 0 || !trades_isrunning.load(); });
            if(trades_isrunning == false){
                break; // lopetetaan odottaminen, ohjelman pitäisi exittaa eikä odottaa enää
            }
            lock.unlock();
            getRequestOrderBook(contextSSL);
            obook.pausing_dUp = false;
            pendingSnapshots = 0;
            //std::cout << "\n pendingSnapshots in marketData_sync "<< pendingSnapshots;
        }
        //std::cout << "\n shutting down.. marketData_sync";
    });            
                                        

    // networking thread, websocket
    std::vector<std::thread> vec;
    vec.reserve(threads - 1);
    for (auto i = 0; i < threads - 1; i++){
        vec.emplace_back([&ioc]
                         {
            std::cout << "\n in threads loop";
            ioc.run();          
            std::cout << " after ico.run"; });
    };
    ioc.run();

    for (auto &t : vec)
    {
        t.join();
        trades.join();
        marketData_sync.join();

        obook.printTop(5);
        HFTProfiler::instance().print();
    };

    return EXIT_SUCCESS;
};
