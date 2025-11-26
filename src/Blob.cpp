#include "../include/Blob.h"
#include "../include/Utils.h"
#include "../include/GitliteException.h"
#include <fstream>

// 通过内容创建 Blob（自动生成 ID）
Blob::Blob(const std::string& content) 
    : content(content), id(generateId(content)) {}

// 通过 ID 和内容创建 Blob
Blob::Blob(const std::string& id, const std::string& content) 
    : id(id), content(content) {}

std::string Blob::getId() const {
    return id;
}

std::string Blob::getContent() const {
    return content;
}

// 写入 Blob 到 objects 目录
void Blob::write(const std::string& objectsDir) const {
    std::string blobPath = Utils::join(objectsDir, id);
    Utils::writeContents(blobPath, content);
}

// 从 objects 目录加载 Blob
Blob Blob::load(const std::string& objectsDir, const std::string& blobId) {
    std::string blobPath = Utils::join(objectsDir, blobId);
    if (!Utils::exists(blobPath)) {
        throw GitliteException("Blob not found: " + blobId);
    }
    std::string content = Utils::readContentsAsString(blobPath);
    return Blob(blobId, content);
}

// 创建 Blob 并写入磁盘
Blob Blob::create(const std::string& objectsDir, const std::string& content) {
    Blob blob(content);
    blob.write(objectsDir);
    return blob;
}

// 生成 SHA1 标识
std::string Blob::generateId(const std::string& content) {
    return Utils::sha1(content);
}