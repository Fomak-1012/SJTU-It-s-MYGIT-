#include "../include/StagingArea.h"
#include "../include/Utils.h"
#include <fstream>
#include <sstream>

// 构造函数：初始化路径并加载数据
StagingArea::StagingArea(const std::string& stagingFilePath, const std::string& removedFilePath)
    : stagingFilePath(stagingFilePath), removedFilePath(removedFilePath) {
    loadStaging();
    loadRemovedFiles();
}

// 加载暂存区文件
void StagingArea::loadStaging() {
    stagingMap.clear();
    if (!Utils::exists(stagingFilePath)) return;

    std::string content = Utils::readContentsAsString(stagingFilePath);
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string filename = line.substr(0, colonPos);
            std::string blobId = line.substr(colonPos + 1);
            stagingMap[filename] = blobId;
        }
    }
}

// 加载被删除文件列表
void StagingArea::loadRemovedFiles() {
    removedFiles.clear();
    if (!Utils::exists(removedFilePath)) return;

    std::string content = Utils::readContentsAsString(removedFilePath);
    std::istringstream iss(content);
    std::string filename;
    while (std::getline(iss, filename)) {
        if (!filename.empty()) {
            removedFiles.insert(filename);
        }
    }
}

const std::map<std::string, std::string>& StagingArea::getStagingMap() const {
    return stagingMap;
}

const std::set<std::string>& StagingArea::getRemovedFiles() const {
    return removedFiles;
}

void StagingArea::addStagedFile(const std::string& filename, const std::string& blobId) {
    stagingMap[filename] = blobId;
}

void StagingArea::removeStagedFile(const std::string& filename) {
    stagingMap.erase(filename);
}

void StagingArea::addRemovedFile(const std::string& filename) {
    removedFiles.insert(filename);
}

void StagingArea::removeRemovedFile(const std::string& filename) {
    removedFiles.erase(filename);
}

// 保存暂存区和删除列表到磁盘
void StagingArea::save() const {
    // 保存暂存区
    std::ostringstream ossStaging;
    for (const auto& entry : stagingMap) {
        ossStaging << entry.first << ":" << entry.second << "\n";
    }
    Utils::writeContents(stagingFilePath, ossStaging.str());

    // 保存删除列表
    std::ostringstream ossRemoved;
    for (const auto& filename : removedFiles) {
        ossRemoved << filename << "\n";
    }
    Utils::writeContents(removedFilePath, ossRemoved.str());
}

// 清空暂存区和删除列表
void StagingArea::clear() {
    stagingMap.clear();
    removedFiles.clear();
    save();
}

bool StagingArea::isStaged(const std::string& filename) const {
    return stagingMap.count(filename) > 0;
}

bool StagingArea::isRemoved(const std::string& filename) const {
    return removedFiles.count(filename) > 0;
}