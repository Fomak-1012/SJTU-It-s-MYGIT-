#ifndef REPOSITORY_CORE_H
#define REPOSITORY_CORE_H

#include<string>
#include"StagingArea.h"

class RepositoryCore{
private:
    StagingArea stagingArea;

protected:
    static const std::string gitlite_dir;
    static const std::string objects_dir;
    static const std::string branches_dir;
    static const std::string staging_area_file;
    static const std::string removed_file;
    static const std::string head_file;
    static const std::string remotes_file;

public:
    RepositoryCore();
    
    static bool isInitialized();
    static std::string getGitliteDir();

    void init();

    //分支操作
    std::string getCurrentBranch();
    void setCurrentBranch(const std::string& branchName);
    std::string getBranchHead(const std::string& branchName);
    void setBranchHead(const std::string& branchName,const std::string& commitId);

    //暂存区操作
    void clearStagingArea();
    StagingArea& getStagingArea();
    
    //复制文件
    void copyFile(const std::string& source,const std::string& destination);
};

#endif //REPOSITORY_CORE_H