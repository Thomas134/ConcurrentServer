#pragma once
#include <string>
#include <mutex>
#include <vector>

class FileManager {
public:
    FileManager();
    bool ReadFile(const std::string& filename, std::string& content);
    bool ReadDownloadFile(const std::string& filename, std::string& content);
    bool SaveFile(const std::string& filename, const std::string& content);
    bool ValidatePath(const std::string& path);
    std::vector<std::string> ListFiles();


private:
    std::mutex file_mutex_;
    std::string base_path_ = "D:/file/";
    void UpdateProgress(int progress);
};