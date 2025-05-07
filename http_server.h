#pragma once
#include <boost/asio.hpp>
#include "session.h"

class HttpServer {
public:
    HttpServer(boost::asio::io_context& io_context, unsigned short port);
    void Start();

private:
    void DoAccept();

    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::io_context& io_context_;
};