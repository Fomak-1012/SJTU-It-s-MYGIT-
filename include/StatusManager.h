#ifndef STATUS_MANAGER_H
#define STATUS_MANAGER_H

#include<cstring>
#include<set>
#include<map>

class RepositoryCore;
class CommitManager;
class FileOperationManager;

class StatusManager{
private:
    RepositoryCore* core;
    CommitManager* commitManager;
    FileOperationManager* fileOpManager;

    std::set<std::string> getAllBranches();
    std::map<std::string,std::string> getModifiedFiles();
    std::set<std::string> getUntrackedFiles();
public:
    StatusManager(RepositoryCore* repoCore, CommitManager* commitMgr, FileOperationManager* fileOpMgr);

    //状态
    void status();
    
    //辅助函数
    void printBranches(const std::set<std::string>& branches, const std::string& currentBranch);
    void printStagedFiles();
    void printRemovedFiles();
    void printModifiedFiles(const std::map<std::string, std::string>& modifiedFiles);
    void printUntrackedFiles(const std::set<std::string>& untrackedFiles);

};
#endif //STATUS_MANAGER_H