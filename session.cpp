#include "session.h"
#include <iostream>
#include "performance_monitor.h"
#include <boost/asio/steady_timer.hpp>
#include <boost/algorithm/string/trim.hpp>

Session::Session(boost::asio::ip::tcp::socket socket)
: socket_(std::move(socket)) {}

void Session::Start() {
DoRead();
}

void Session::DoRead() {
    auto self(shared_from_this());
    auto mutable_buf = buffer_.prepare(8192);

    socket_.async_read_some(mutable_buf,
        [this, self](boost::system::error_code ec, size_t length) {
            if (!ec) {
                buffer_.commit(length);
                const auto& data = buffer_.data();
                request_.append(static_cast<const char*>(data.data()), data.size());
                buffer_.consume(length); // 只消费当前读取的部分

                size_t header_end = request_.find("\r\n\r\n");
                if (header_end != std::string::npos) {
                    // 确保 Content-Length 存在
                    size_t pos = request_.find("Content-Length:");
                    if (pos != std::string::npos) {
                        size_t cl_start = pos + std::string("Content-Length:").length();
                        size_t cl_end = request_.find("\r\n\r\n", cl_start);
                        if (cl_end != std::string::npos) {
                            std::string cl_str = request_.substr(cl_start, cl_end - cl_start);
                            boost::algorithm::trim(cl_str);
                            try {
                                size_t content_length = std::stoul(cl_str);
                                size_t body_received = request_.size() - (header_end + 4);

                                if (body_received < content_length) {
                                    // 继续读取，直到 body 完整
                                    DoRead();
                                    return;
                                }
                            }
                            catch (const std::exception& e) {
                                std::cerr << "[ERROR] Invalid Content-Length value: " << cl_str << "\n";
                                return;
                            }
                        }
                    }
                    // 请求完整后才处理
                    HandleRequest();
                }
                else {
                    // 继续读取数据
                    DoRead();
                }

                std::cout << "read " << length << " bytes\n";
            }
            else {
                std::cerr << "[ERROR] Read failed: " << ec.message() << "\n";
            }
        });
}

void Session::HandleRequest() {
try {
    PerformanceMonitor::GetInstance().RecordRequest();
    handler_.HandleRequest(request_, response_);
    //std::cout << "[DEBUG] Request handled successfully\n";
}
catch (const std::exception& e) {
    //std::cerr << "[ERROR] Request handling failed: " << e.what() << "\n";
    response_ = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
}
DoWrite(response_);
}

void Session::DoWrite(const std::string& response) {
auto self(shared_from_this());
boost::asio::async_write(socket_, boost::asio::buffer(response),
    [this, self](boost::system::error_code ec, std::size_t) {
        if (!ec) {
            auto timer = std::make_shared<boost::asio::steady_timer>(
                socket_.get_executor(), std::chrono::seconds(0)); //////////等待时间
            timer->async_wait([this, self, timer](const boost::system::error_code& /*ec*/) {
                boost::system::error_code ignored_ec;
                socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
                });
        }
        else {
            std::cerr << "[ERROR] Write failed: " << ec.message() << "\n";
        }
    });
}
