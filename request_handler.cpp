#include "request_handler.h"
#include "performance_monitor.h"
#include <sstream>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <filesystem>
#include <boost/algorithm/string.hpp>
#include <ctime>

namespace fs = std::filesystem;

void RequestHandler::HandleRequest(const std::string& request, std::string& response) {
    std::istringstream iss(request);
    std::string method, path, protocol;
    iss >> method >> path >> protocol;


    if (method == "GET") {
        HandleGet(path, response);
    }
    else if (method == "POST") {
        std::ostringstream oss;
        oss << iss.rdbuf();
        std::string raw_data = oss.str();

        // 查找 HTTP 头部和 Body 的分隔符
        size_t body_pos = raw_data.find("\r\n\r\n");


        if (body_pos != std::string::npos) {
            // 提取 Body
            //std::cout << "Extracted body content:\n" << raw_data << std::endl;
            std::string body = raw_data.substr(body_pos + 4);

            //std::cout << "Extracted body content:\n" << body << std::endl;
            // 传递 body 进行处理
            HandlePost(path, body, response);
        }
        else {
            std::cerr << "Failed to find body in the request!" << std::endl;
            response = "HTTP/1.1 400 Bad Request\r\n\r\n";
        }
    }
    else {
        response = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
    }
}

bool ends_with(const std::string& str, const std::string& suffix) {
    if (str.size() < suffix.size()) return false;
    return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

std::string RequestHandler::GenerateFileListHTML() {
    std::ostringstream html;
    html << "<html><body>";
    html << "<h1>Uploaded Files</h1><ul>";

    std::vector<std::string> files = file_manager_.ListFiles(); 

    for (const auto& file : files) {
        html << "<li><a href=\"/" << file << "\">" << file << "</a></li>";
    }

    html << "</ul></body></html>";
    return html.str();
}

std::string GetQueryParam(const std::string& path, const std::string& key) {
    auto pos = path.find('?');
    if (pos == std::string::npos) return "";

    std::string query = path.substr(pos + 1);
    std::istringstream iss(query);
    std::string pair;
    while (std::getline(iss, pair, '&')) {
        auto eq = pair.find('=');
        if (eq != std::string::npos) {
            std::string k = pair.substr(0, eq);
            std::string v = pair.substr(eq + 1);
            if (k == key) return v;
        }
    }
    return "";
}

void RequestHandler::HandleGet(const std::string& path, std::string& response) {
    std::string file_content;
    std::string filename;

    if (path == "/files") {
        response = "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n\r\n" +
            GenerateFileListHTML();
        return;
    }
    if (path.rfind("/download", 0) == 0) {
        std::string filename = GetQueryParam(path, "file");
        if (filename.empty() || filename.find("..") != std::string::npos) {
            response = "HTTP/1.1 404 Bad Request\r\n\r\n";
            return;
        }

        std::string full_path = "D:/file/" + filename;
        std::string file_content;
        if (file_manager_.ReadDownloadFile(full_path, file_content)) {
            response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Disposition: attachment; filename=\"" + filename + "\"\r\n"
                "Content-Type: application/octet-stream\r\n"
                "Content-Length: " + std::to_string(file_content.size()) + "\r\n"
                "\r\n" +
                file_content;
        }
        else {
            response = "HTTP/1.1 400 Not Found\r\n\r\n";
        }
        return;
    }
    if (path.rfind("/delete", 0) == 0) {
        std::string filename = GetQueryParam(path, "file");
        if (filename.empty() || filename.find("..") != std::string::npos) {
            response = "HTTP/1.1 400 Bad Request\r\n\r\n";
            return;
        }

        std::string full_path = "D:/file/" + filename;
        std::error_code ec;
        bool ok = fs::remove(full_path, ec);

        if (ok && !ec) {
            response = "HTTP/1.1 200 OK\r\n\r\n";
        }
        else {
            response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
        }
        return;
    }

    if (path.rfind("/filelist.json", 0) == 0) {
        std::vector<std::string> files = file_manager_.ListFiles();

        // 获取分页参数
        int page = 1;
        std::string page_str = GetQueryParam(path, "page");
        if (!page_str.empty()) {
            try {
                page = std::stoi(page_str);
                if (page < 1) page = 1;
            }
            catch (...) {
                page = 1;
            }
        }

        const int page_size = 5;
        int start = (page - 1) * page_size;
        int end = std::min(start + page_size, static_cast<int>(files.size()));

        std::ostringstream json;
        json << "[";

        for (int i = start; i < end; ++i) {
            const std::string& file = files[i];
            std::string full_path = "D:/file/" + file;

            std::error_code ec;
            std::uintmax_t size = fs::file_size(full_path, ec);
            auto ftime = fs::last_write_time(full_path, ec);
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
            );
            std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);

            std::tm timeinfo;
            std::ostringstream time_ss;

        #if defined(_WIN32) || defined(_WIN64)
            if (localtime_s(&timeinfo, &cftime) == 0) {
                time_ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
            }
        #elif defined(__APPLE__) || defined(__linux__)
            if (localtime_r(&cftime, &timeinfo) != nullptr) {
                time_ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
            }
        #endif
            else {
                time_ss << "Invalid date";
            }

            json << "{";
            json << "\"name\":\"" << file << "\",";
            json << "\"size\":" << size << ",";
            json << "\"modified\":\"" << time_ss.str() << "\"";
            json << "}";

            if (i != end - 1) {
                json << ",";
            }
        }

        json << "]";

        response = "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n\r\n" + json.str();
        return;
    }

    if (path == "/") {
        filename = "index.html";
    }
    else {
        filename = (path.size() > 0 && path[0] == '/') ? path.substr(1) : path;
    }

    std::string content_type = "text/plain";
    if (ends_with(filename, ".html")) content_type = "text/html";
    else if (ends_with(filename, ".css")) content_type = "text/css";
    else if (ends_with(filename, ".js")) content_type = "application/javascript";

    if (file_manager_.ReadFile(filename, file_content)) {
        response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: " + content_type + "\r\n"
            "Content-Length: " + std::to_string(file_content.size()) + "\r\n"
            "\r\n" +
            file_content;
    }
    else {
        response = "HTTP/1.1 404 Not Found\r\n\r\n";
    }
}

    //std::cout << path <<"\n";



void RequestHandler::HandlePost(const std::string& path, const std::string& body, std::string& response) {
    if (path == "/upload") {
        try {
            // 生成唯一文件名
            std::string filename = "upload_" + std::to_string(time(nullptr)) + ".dat";

            if (body.find("Content-Disposition: form-data") != std::string::npos) {
                size_t pos = body.find("\r\n\r\n");
                if (pos != std::string::npos) {

                    size_t filename_pos = body.find("filename=");
                    filename = body.substr(filename_pos + 10);
                    size_t filename_end_pos = filename.find("\"");
                    filename = filename.substr(0 , filename_end_pos);

                    std::string file_content = body.substr(pos + 4);

                    size_t end_pos = file_content.find("------");
                    file_content = file_content.substr(0, end_pos);
                    if (file_manager_.SaveFile(filename, file_content)) {
                        response =
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/plain\r\n"
                            "\r\n"
                            "File uploaded successfully: " + filename;
                    }
                    else {
                        response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
                    }
                }
                else {
                    response = "HTTP/1.1 400 Bad Request\r\n\r\n";
                }
            }
            else {
                if (file_manager_.SaveFile(filename, body)) {
                    response =
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/plain\r\n"
                        "\r\n"
                        "File uploaded successfully: " + filename;
                    PerformanceMonitor::GetInstance().RecordRequest();
                }
                else {
                    response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Upload failed: " << e.what() << std::endl;
            response = "HTTP/1.1 413 Payload Too Large\r\n\r\n";
        }
    }
    else {
        response = "HTTP/1.1 404 Not Found\r\n\r\n";
    }
}

std::string RequestHandler::GenerateHTML() {
    return R"(
        <html>
            <body>
                <h1>File Server</h1>
                <form action="/upload" method="post" enctype="multipart/form-data">
                    <input type="file" name="file">
                    <input type="submit" value="Upload">
                </form>
            </body>
        </html>
    )";
}
