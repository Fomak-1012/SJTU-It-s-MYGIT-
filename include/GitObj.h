#ifndef GITOBJ_H
#define GITOBJ_H

#include"Repository.h"

class GitObj {
private:
    Repository repo;

public:
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
};

#endif // GITOBJ_H