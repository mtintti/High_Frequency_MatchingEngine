#include <iostream>
#include "websocket.h"
#include "main.h"
#include "orderbookSnapshot.h"
#include "windcertsload.h"
#include "profile.h"
#include "orderbook.h"

std::vector<std::string> durationvec;
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

        std::abort();
    });
}


boost::asio::ssl::context contextSSL(boost::asio::ssl::context::method::sslv23_client);
boost::asio::ssl::context coSSL(boost::asio::ssl::context::sslv23_client);

int main(const int argc, char *argv[])
{
    installTerminateHandler();
    //std::cout << "hello";
    //std::cout << "\n high matching engine";
    windowsCertificateStore(contextSSL);
    getRequestOrderBook(contextSSL);
    
    
    int threads = 2;
    boost::asio::io_context ioc;
    windowsCertificateStore(coSSL);
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto){
        ioc.stop();
    });
    websocketsTrade(ioc, coSSL, host, port, endpoint);
    std::vector<std::thread> vec;
    vec.reserve(threads - 1);
    for (auto i = 0; i < threads - 1; i++)
    {
        vec.emplace_back([&ioc]{
            std::cout << "\n in threads loop";
            ioc.run();
                      
            std::cout << " after ico.run"; 
        });
    };
    ioc.run();
    for (auto &t : vec)
    {            
        t.join();
        
        obook.printTop(5);
        HFTProfiler::instance().print(); 
    };
    for(size_t j = 0; j < durationvec.size(); j++){
    };
    return EXIT_SUCCESS;
    
};
