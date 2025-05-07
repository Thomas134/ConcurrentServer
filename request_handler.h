#pragma once
#include <string>
#include "file_manager.h"

class RequestHandler {
public:
    void HandleRequest(const std::string& request, std::string& response);

private:
    void HandleGet(const std::string& path, std::string& response);
    void HandlePost(const std::string& path, const std::string& body, std::string& response);
    std::string GenerateHTML();
    std::string GenerateFileListHTML();

    FileManager file_manager_;
};