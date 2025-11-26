#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include "Blob.h"
#include "StagingArea.h"
#include "Commit.h"
#include "GitliteException.h"

class Repository {
private:
    // 暂存区实例（管理暂存文件和被删除文件）
    StagingArea stagingArea;

    // 静态路径配置（仓库核心目录/文件）
    static const std::string gitliteDir;
    static const std::string objectsDir;
    static const std::string branchesDir;
    static const std::string stagingAreaFile;
    static const std::string removedFile;
    static const std::string headFile;
    static const std::string remotesFile;

    // 辅助方法：获取完整的 Commit ID（处理短 ID）
    std::string getFullCommitId(const std::string& shortId);
    // 辅助方法：通过 ID 获取 Commit 实例
    Commit getCommit(const std::string& commitId);
    // 辅助方法：保存 Commit 到 objects 目录
    void saveCommit(const Commit& commit);
    // 辅助方法：获取文件在指定 Commit 中的 Blob ID
    std::string getFileBlobId(const std::string& filename, const std::string& commitId);
    // 辅助方法：从指定 Commit 复制文件到工作区
    void copyFileFromCommit(const std::string& filename, const std::string& commitId);
    // 辅助方法：检查文件是否存在于指定 Commit 中
    bool fileExistsInCommit(const std::string& filename, const std::string& commitId);
    // 辅助方法：获取所有分支名称
    std::set<std::string> getAllBranches();
    // 辅助方法：查找两个 Commit 的最近公共祖先（拆分点）
    std::string findSplitPoint(const std::string& branch1, const std::string& branch2);
    // 辅助方法：获取指定 Commit 中的所有跟踪文件（文件名->BlobID）
    std::map<std::string, std::string> getTrackedFiles(const std::string& commitId);
    // 辅助方法：检查文件是否被修改（与指定 Commit 对比）
    bool isFileModified(const std::string& filename, const std::string& commitId);
    // 辅助方法：获取工作区中的未跟踪文件
    std::set<std::string> getUntrackedFiles();
    // 辅助方法：获取已修改但未暂存的文件
    std::map<std::string, std::string> getModifiedFiles();
    // 辅助方法：加载所有远程仓库配置
    std::map<std::string, std::string> getRemotes();
    // 辅助方法：保存远程仓库配置到文件
    void saveRemotes(const std::map<std::string, std::string>& remotes);
    // 辅助方法：获取指定分支的 HEAD Commit ID
    std::string getBranchHead(const std::string& branchName);
    // 辅助方法：设置指定分支的 HEAD Commit ID
    void setBranchHead(const std::string& branchName, const std::string& commitId);

public:
    // 构造函数：初始化暂存区实例
    Repository();

    // 静态方法：检查仓库是否已初始化
    static bool isInitialized();
    // 静态方法：获取仓库根目录路径
    static std::string getGitliteDir();

    // 核心操作：初始化仓库
    void init();
    // 核心操作：添加文件到暂存区
    void add(const std::string& filename);
    // 核心操作：提交暂存区的更改
    void commit(const std::string& message);
    // 核心操作：删除文件（工作区+暂存区/跟踪）
    void rm(const std::string& filename);
    // 核心操作：查看当前分支的提交日志
    void log();
    // 核心操作：查看所有提交日志
    void globalLog();
    // 核心操作：根据提交信息查找提交 ID
    void find(const std::string& commitMessage);
    // 核心操作：从当前提交检出文件
    void checkoutFile(const std::string& filename);
    // 核心操作：从指定提交检出文件
    void checkoutFileInCommit(const std::string& commitId, const std::string& filename);
    // 核心操作：切换分支
    void checkoutBranch(const std::string& branchName);
    // 核心操作：查看仓库状态
    void status();
    // 核心操作：创建分支
    void branch(const std::string& branchName);
    // 核心操作：删除分支
    void rmBranch(const std::string& branchName);
    // 核心操作：重置到指定提交
    void reset(const std::string& commitId);
    // 核心操作：合并指定分支到当前分支
    void merge(const std::string& branchName);
    // 核心操作：添加远程仓库
    void addRemote(const std::string& remoteName, const std::string& remotePath);
    // 核心操作：删除远程仓库
    void rmRemote(const std::string& remoteName);
    // 核心操作：推送本地分支到远程
    void push(const std::string& remoteName, const std::string& remoteBranchName);
    // 核心操作：拉取远程分支到本地（跟踪分支）
    void fetch(const std::string& remoteName, const std::string& remoteBranchName);
    // 核心操作：拉取并合并远程分支
    void pull(const std::string& remoteName, const std::string& remoteBranchName);

    // 辅助接口：获取当前分支名称
    std::string getCurrentBranch();
    // 辅助接口：获取当前分支的 HEAD Commit ID
    std::string getCurrentCommitId();
    // 辅助接口：设置当前分支
    void setCurrentBranch(const std::string& branchName);
    // 辅助接口：清空暂存区（含删除文件列表）
    void clearStagingArea();
};

#endif // REPOSITORY_H