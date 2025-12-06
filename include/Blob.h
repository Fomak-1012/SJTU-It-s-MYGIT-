#ifndef BLOB_H
#define BLOB_H

#include<cstring>

class Blob{
private:
    std::string id;         // SHA1码
    std::string content;    // 文件内容
public:
    //通过文件内容构造blob对象
    explicit Blob(const std::string& content);

    //通过文件内容和id构造blob对象
    Blob(const std::string& id,const std::string& content);

    //获取blob的id
    std::string getId() const;

    //获取blob的内容
    std::string getContent() const;

    //将blob写入objects目录
    void write(const std::string& path) const;

    //从objects目录读取blob
    static Blob load(const std::string& path,const std::string& id);

    //创建blob对象并写入objects目录
    static Blob create(const std::string& path,const std::string& content);

    //获取blob的sha1码
    static std::string generateId(const std::string& content);
};

#endif // BLOB_H