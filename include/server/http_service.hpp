#pragma once

#define HTTPLIB_HEADER_ONLY
#include <httplib.h>

#include <memory>
#include <string>
#include <thread>

#include "server/handler.hpp"

class HttpService {
   public:
    HttpService(CommandHandler& handler, const std::string& host, int port);
    ~HttpService();

    void start();
    void stop();

   private:
    CommandHandler& handler_;
    std::string host_;
    int port_;
    httplib::Server svr_;
    std::unique_ptr<std::thread> thread_;

    void setup_routes();
};
