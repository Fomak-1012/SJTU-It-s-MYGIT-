#ifndef COMMIT_MANAGER_H
#define COMMIT_MANAGER_H

#include<string>
#include<vector>
#include<map>
#include"Commit.h"

class RepositoryCore;

class CommitManager{
private:
    RepositoryCore* core;
public:
    CommitManager(RepositoryCore* repoCore);

    // 提交
    void commit(const std::string& message);    
    void saveCommit(const Commit& commit);
    Commit getCommit(const std::string& commitId);

    //日志和查找
    void log();
    void globalLog();
    void find(const std::string& commitMessage);

    //辅助函数
    std::string getFileBlobId(const std::string& filename,const std::string& commitId);
    bool fileExistsInCommit(const std::string& filename,const std::string& commitId);
    void copyFileFromCommit(const std::string& filename,const std::string& commitId);
    std::map<std::string, std::string> getTrackedFiles(const std::string& commitId);
    std::string getFullCommitId(const std::string& shortId);

    //当前提交
    std::string getCurrentCommitId();
    Commit getHeadCommit();
    std::vector<std::string> getFiles(const std::string& commitId);

    //重置
    void reset(const std::string& commitId);
};

#endif // COMMIT_MANAGER_H