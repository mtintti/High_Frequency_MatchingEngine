#include <iostream>
#include "matchingEngine.cpp"
#include "websocket.h"


int main(const int argc, char *argv[])
{
    std::cout << "hello";
    std::cout << "\n high matching engine";
    /*start up on threads for 10 sec*/

    /* c++ fetching data and inputing it in a pod structured thinga linga*/
    //auto const host = "echo.websocket.events";
    auto const host = "stream.binance.com";
    // auto const port = "443"; if we use ssl then this port. otherways ->
    auto const port = "9443";
    auto const endpoint = "/ws/btcusdt@trade";
    int threads = 2;
    

    boost::asio::io_context ioc;
    boost::asio::ssl::context coSSL(boost::asio::ssl::context::sslv23);
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto){
        ioc.stop();
        
    });

    websocketsTrade(ioc, coSSL, host, port, endpoint);
    std::vector<std::thread> vec;
    std::cout << "\n after websocketsTrade(), reserving threads -1";
    vec.reserve(threads - 1);
    std::cout << "\n vec size: " << vec.size();
    for (auto i = 0; i < threads - 1; i++)
    {
        vec.emplace_back([&ioc]
                         {
            std::cout << "\n in threads loop";
            ioc.run();
            std::cout << " after ico.run"; });
        ioc.run();
        for (auto &t : vec)
        {
            t.join();
            return EXIT_SUCCESS;
        }
    };
}
