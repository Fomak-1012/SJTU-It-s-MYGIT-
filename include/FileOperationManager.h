#ifndef FILE_OPERATION_MANAGER_H
#define FILE_OPERATION_MANAGER_H

#include<string>
#include<map>
#include<set>

class RepositoryCore;
class CommitManager;

class FileOperationManager{
private:
    RepositoryCore* core;
    CommitManager* commitManager;

    bool isFileModified(const std::string& filename,const std::string& commitId);
    std::map<std::string,std::string> getModifiedFiles();

public:
    std::set<std::string> getUntrackedFiles();
    FileOperationManager(RepositoryCore* core,CommitManager* commitManager);

    //文件操作
    void add(const std::string& filename);
    void rm(const std::string& filename);
    void checkoutFile(const std::string& filename);
    void checkoutFileInCommit(const std::string& commitId, const std::string& filename);

    //冲突文件处理
    std::set<std::string> getConflictFiles();
    void saveConflictFiles(const std::set<std::string>& conflictFiles);
    void clearConflictFiles();
    
    //暂存区操作
    void clearStagingArea();
};

#endif// FILE_OPERATION_MANAGER_H