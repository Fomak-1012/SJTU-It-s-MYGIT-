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
    std::string id;
    std::string message;
    std::time_t timestamp;
    std::vector<std::string> parents;
    std::map<std::string, std::string> blobs; // 
    std::string merge_info;// 

    //
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
    
    //
    std::string getId() const;
    std::string getMessage() const;
    std::time_t getTimestamp() const;
    std::vector<std::string> getParents() const;
    std::map<std::string, std::string> getBlobs() const;
    std::string getMergeInfo() const;

    //
    void setMergeInfo(const std::string& info);

    //
    std::string serialize() const;
    static Commit deserialize(const std::string& data);
    static Commit fromFile(const std::string& filename);

    //
    std::string getFormattedTimestamp() const;
    bool isMergeCommit() const;
    std::string getShortId() const;
};

#endif // COMMIT_H
