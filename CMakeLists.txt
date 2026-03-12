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

add_executable(High_Frequency_MatchingEngine main.cpp websocket.cpp)

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

