#include <iostream>
#include "matchingEngine.cpp"
#include "websocket.h"
#include "orderbookSnapshot.h"
#include "windcertsload.h"

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
            // boost::json as_object()/as_array() throw this on wrong type
            std::cerr << " type    : std::invalid_argument\n";
            std::cerr << " what    : " << e.what() << "\n";
        }
        catch (const std::out_of_range& e) {
            // boost::json .at() throws this on missing key
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

        // abort() produces a core dump you can inspect in a debugger
        // call default terminate behavior
        std::abort();
    });
}



int main(const int argc, char *argv[])
{
    installTerminateHandler();
    std::cout << "hello";
    std::cout << "\n high matching engine";
    boost::asio::ssl::context contextSSL(boost::asio::ssl::context::method::sslv23_client);
    windowsCertificateStore(contextSSL);
    getRequestOrderBook(contextSSL);
    
    
    int threads = 2;
    boost::asio::io_context ioc;
    boost::asio::ssl::context coSSL(boost::asio::ssl::context::sslv23_client);
    windowsCertificateStore(coSSL);
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
        //return EXIT_SUCCESS;
    };
    for(size_t j = 0; j < durationvec.size(); j++){
        std::cout<< "\n "<< durationvec[j];
    };
    return EXIT_SUCCESS;
    
};
