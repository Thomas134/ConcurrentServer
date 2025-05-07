#include "http_server.h"
#include <iostream>

HttpServer::HttpServer(boost::asio::io_context& io_context, unsigned short port)
    : io_context_(io_context),
    acceptor_(io_context,
        boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)) {
    DoAccept();
}

void HttpServer::Start() {
    std::cout << "Server started on port: " << acceptor_.local_endpoint().port() << std::endl;
}

void HttpServer::DoAccept() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            if (!ec) {
                std::make_shared<Session>(std::move(socket))->Start();
            }
            DoAccept();
        });
}