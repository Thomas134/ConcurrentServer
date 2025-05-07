#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <sstream>
#include "file_manager.h"

FileManager::FileManager() {
    if (!std::filesystem::exists(base_path_)) {
        std::filesystem::create_directories(base_path_);
    }
}

bool FileManager::ValidatePath(const std::string& path) {
    try {

        return true;//Debug

        auto canonical_path = std::filesystem::canonical(base_path_ + path);
        auto base_canonical = std::filesystem::canonical(base_path_);
        return canonical_path.string().find(base_canonical.string()) == 0;
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error message: " << e.what() << std::endl;
        return false;
    }
}

//ֻ��ȡindex.html
bool FileManager::ReadFile(const std::string& filename, std::string& content) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    if (!ValidatePath(filename)) {
        return false;
    }
    std::ifstream file("./public/" + filename, std::ios::binary);
    if (!file) return false;

    //std::cout << filename << " AC\n";

    content.clear();
    const size_t bufferSize = 4096;
    char buffer[bufferSize];
    while (file.read(buffer, bufferSize) || file.gcount() > 0) {
        content.append(buffer, file.gcount());
    }
    return true;
}

//ֻ��ȡindex.html
bool FileManager::ReadDownloadFile(const std::string& filename, std::string& content) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    if (!ValidatePath(filename)) {
        return false;
    }
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;

    //std::cout << filename << " AC\n";

    content.clear();
    const size_t bufferSize = 4096;
    char buffer[bufferSize];
    while (file.read(buffer, bufferSize) || file.gcount() > 0) {
        content.append(buffer, file.gcount());
    }
    return true;
}

// �����ĺ�����д�� progress.txt��������
void FileManager::UpdateProgress(int progress) {
    std::ofstream sseFile("./public/progress.txt", std::ios::trunc);
    if (sseFile) {
        sseFile << progress;
    }
    std::cout << "���ȸ��£�" << progress << "%\n";
}

bool FileManager::SaveFile(const std::string& filename, const std::string& content) {
    if (!ValidatePath(filename)) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(file_mutex_); // ���� content �ļ�
        std::ofstream file(base_path_ + filename, std::ios::binary);
        if (!file) return false;

        const size_t bufferSize = 4096;  // 4KB
        const size_t updateInterval = 102400; // 100KB
        size_t written = 0, lastUpdated = 0;
        size_t totalSize = content.size();

        UpdateProgress(0);//��ʼ��������
        while (written < totalSize) {
            size_t chunkSize = std::min(bufferSize, totalSize - written);
            file.write(content.data() + written, chunkSize);
            written += chunkSize;

            // ����д�������� updateInterval ʱ�Ÿ��½���
            if (written - lastUpdated >= updateInterval || written == totalSize) {
                int progress = static_cast<int>((static_cast<double>(written) / totalSize) * 100);

                UpdateProgress(progress);

                lastUpdated = written;
            }
        }
    } 

    return true;
}

std::vector<std::string> FileManager::ListFiles() {
    std::vector<std::string> files;
    try {
        for (const auto& entry : std::filesystem::directory_iterator(base_path_)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path().filename().string());
            }
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "[ERROR] Listing files failed: " << e.what() << std::endl;
    }

    return files;
}

