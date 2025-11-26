#ifndef BLOB_H
#define BLOB_H

#include <string>

class Blob {
private:
    std::string id;       // Blob 的 SHA1 唯一标识
    std::string content;  // Blob 存储的文件内容

public:
    // 1. 通过文件内容创建 Blob（自动生成 SHA1 ID）
    explicit Blob(const std::string& content);
    // 2. 通过已有 ID 和内容创建 Blob（用于加载已存在的 Blob）
    Blob(const std::string& id, const std::string& content);

    // 获取 Blob ID
    std::string getId() const;
    // 获取 Blob 内容
    std::string getContent() const;

    // 将 Blob 写入到指定 objects 目录
    void write(const std::string& objectsDir) const;

    // 静态方法：从 objects 目录加载 Blob
    static Blob load(const std::string& objectsDir, const std::string& blobId);
    // 静态方法：创建 Blob 并写入 objects 目录（返回创建的 Blob）
    static Blob create(const std::string& objectsDir, const std::string& content);
    // 静态方法：生成内容的 SHA1 哈希（封装 Utils::sha1）
    static std::string generateId(const std::string& content);
};

#endif // BLOB_H