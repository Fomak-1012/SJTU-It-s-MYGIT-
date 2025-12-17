#ifndef MERGE_MANAGER_H
#define MERGE_MANAGER_H

#include<cstring>
#include<set>

class RepositoryCore;
class CommitManager;
class FileOperationManager;
class BranchManager;

class MergeManager{
private:
    RepositoryCore* core;
    CommitManager* commitManager;
    FileOperationManager* fileOpManager;
    BranchManager* branchManager;

    std::string findSplitPoint(const std::string& branch1,const std::string& branch2);
    std::set<std::string> getAllBranches();

public:
    MergeManager(RepositoryCore* repoCore,CommitManager* commitMgr,FileOperationManager* fileOpMgr,BranchManager* branchMgr);
   
    //分支合并
    void merge(const std::string& branchName);

    //检查合并条件
    bool checkMergeConditions(const std::string& branchName);

    //快速合并
    void performFastForwardMerge(const std::string& branchName);

    //三方合并
    void performThreeWayMerge(const std::string& branchName,const std::string& currentCommitId,
                            const std::string& givenCommitId,const std::string& splitPointId);
};

#endif // MERGE_MANAGER_H