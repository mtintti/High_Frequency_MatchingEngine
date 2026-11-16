cmake_minimum_required(VERSION 3.20)

project(High_Frequency_MatchingEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(/bigobj)

set(BOOST_ROOT "C:/ohjelmointi/Libraries/boost/boost_1_90_0/boost_1_90_0")
set(OPENSSL_ROOT "C:/Program Files/OpenSSL-Win64")

find_package(Boost REQUIRED COMPONENTS json)
find_package(OpenSSL REQUIRED)

set(Boost_USE_STATIC_LIBS ON)

add_executable(High_Frequency_MatchingEngine main.cpp network/websocket.cpp network/read_parser.cpp memory/mempool.cpp memory/add_memEvent.cpp marketData/marketdata_Snapshot.cpp orderbook/orderbook.cpp windcertsload.cpp
    clients/clients.cpp
    clients/validator_sequencer/validator_sequence.cpp
    matchingEngine/matchingEngine.h
    matchingEngine/matchingEngine.cpp
    memory/matchingEngine_pool.cpp
    memory/matchingEngine_pool.h)

target_include_directories(High_Frequency_MatchingEngine
    PRIVATE ${Boost_INCLUDE_DIRS}
    PRIVATE ${OpenSSL_ROOT}
)


target_link_libraries(High_Frequency_MatchingEngine
    OpenSSL::SSL
    OpenSSL::Crypto
)

target_link_libraries(High_Frequency_MatchingEngine
    Boost::json
)
