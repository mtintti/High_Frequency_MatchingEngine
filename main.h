#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "matchingEngine/matchingEngine.h"
#include "orderbook/orderbook.h"

extern boost::asio::ssl::context contextSSL;
extern boost::asio::ssl::context coSSL;
extern std::atomic<bool> trades_isrunning;
extern std::atomic<int> pendingSnapshots;
extern std::condition_variable cv;
extern std::mutex snapshotMutex;
extern Matching engine;