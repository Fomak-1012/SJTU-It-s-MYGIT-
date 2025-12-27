#ifndef REPOSITORY_H
#define REPOSITORY_H

#include<string>
#include<vector>
#include<map>
#include<set>
#include"Blob.h"
#include"StagingArea.h"
#include"Commit.h"
#include"GitliteException.h"
#include"RepositoryCore.h"
#include"CommitManager.h"
#include"BranchManager.h"
#include"FileOperationManager.h"
#include"MergeManager.h"
#include"RemoteManager.h"
#include"StatusManager.h"

class Repository {
private:
    RepositoryCore* core;
    CommitManager* commitManager;
    BranchManager* branchManager;
    FileOperationManager* fileOpManager;
    MergeManager* mergeManager;
    RemoteManager* remoteManager;
    StatusManager* statusManager;

public:
    Repository();
    ~Repository();

    static bool isInitialized();
    static std::string getGitliteDir();

    void init();
    void add(const std::string& filename);
    void commit(const std::string& message);
    void rm(const std::string& filename);
    void log();
    void globalLog();
    void find(const std::string& commitMessage);
    void checkoutFile(const std::string& filename);
    void checkoutFileInCommit(const std::string& commitId,const std::string& filename);
    void checkoutBranch(const std::string& branchName);
    void status();
    void branch(const std::string& branchName);
    void rmBranch(const std::string& branchName);
    void reset(const std::string& commitId);
    void merge(const std::string& branchName);
    void addRemote(const std::string& remoteName,const std::string& remotePath);
    void rmRemote(const std::string& remoteName);
    void push(const std::string& remoteName,const std::string& remoteBranchName);
    void fetch(const std::string& remoteName,const std::string& remoteBranchName);
    void pull(const std::string& remoteName,const std::string& remoteBranchName);

    std::string getCurrentBranch();
    std::string getCurrentCommitId();
    void setCurrentBranch(const std::string& branchName);
    void clearStagingArea();
    std::set<std::string> getConflictFiles();
    void saveConflictFiles(const std::set<std::string>& files);
    void clearConflictFiles();

    Commit getHeadCommit();
    std::vector<std::string> getFiles(const std::string& commitId);
};

#endif // REPOSITORY_H
