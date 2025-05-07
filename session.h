#pragma once
#include <boost/asio.hpp>
#include "request_handler.h"

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(boost::asio::ip::tcp::socket socket);
    void Start();

private:
    void DoRead();
    void DoWrite(const std::string& response);
    void HandleRequest();

    boost::asio::ip::tcp::socket socket_;
    boost::asio::streambuf buffer_;
    RequestHandler handler_;
    std::string request_;
    std::string response_;
};