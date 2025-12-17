#ifndef BRANCH_MANAGER_H
#define BRANCH_MANAGER_H

#include<cstring>
#include<set>

class RepositoryCore;

class BranchManager{
private:
    RepositoryCore* core;

    std::set<std::string> getAllBranches();
    std::string findSplitPoint(const std::string& branch1,const std::string& branch2);

public:
    BranchManager(RepositoryCore* repoCore);

    //操作
    void branch(const std::string& branchName);
    void rmBranch(const std::string& branchName);
    void checkoutBranch(const std::string& branchName);

    //查询
    std::string getCurrentBranch();
    std::set<std::string> getAllBranchesList();

    //切换
    void performBranchCheckout(const std::string& branchName);
};
#endif // BRANCH_MANAGER_H