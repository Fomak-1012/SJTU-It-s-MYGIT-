#ifndef COMMIT_H
#define COMMIT_H

#include<cstring>
#include<ctime>
#include<map>
#include<vector>
#include<sstream>
#include<unistd.h>

class Commit {
private:
    std::string id;                           // 提交的信息
    std::string message;                      // 提交信息  
    std::time_t timestamp;                    // 提交的时间戳
    std::vector<std::string> parents;         // 父提交的ID列表
    std::map<std::string, std::string> blobs; // 存储文件名到blob id的映射
    std::string merge_info;                   // merge commit的额外信息

    //辅助函数
    static std::string generateId(const std::string& message, 
                                 const std::time_t& timestamp,
                                 const std::vector<std::string>& parents,
                                 const std::map<std::string, std::string>& blobs);
    static std::string timeToString(const std::time_t& timestamp);
    static std::time_t stringToTime(const std::string& timeStr);
public:
    Commit();
    Commit(const std::string& message, const std::time_t& timestamp, 
           const std::vector<std::string>& parents,
           const std::map<std::string, std::string>& blobs);
    
    //获取器
    std::string getId() const;
    std::string getMessage() const;
    std::time_t getTimestamp() const;
    std::vector<std::string> getParents() const;
    std::map<std::string, std::string> getBlobs() const;
    std::string getMergeInfo() const;

    //把merge信息给到这个commit
    void setMergeInfo(const std::string& info);

    //序列化与反序列化
    std::string serialize() const;
    static Commit deserialize(const std::string& data);
    static Commit fromFile(const std::string& filename);

    //部分辅助函数
    std::string getFormattedTimestamp() const;
    bool isMergeCommit() const;
    std::string getShortId() const;
};

#endif // COMMIT_H
