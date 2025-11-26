#include "../include/Repository.h"
#include "../include/Utils.h"
#include "../include/GitliteException.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <time.h>

// 静态路径常量初始化
const std::string Repository::gitliteDir = ".gitlite";
const std::string Repository::objectsDir = ".gitlite/objects";
const std::string Repository::branchesDir = ".gitlite/branches";
const std::string Repository::stagingAreaFile = ".gitlite/staging";
const std::string Repository::removedFile = ".gitlite/removed";
const std::string Repository::headFile = ".gitlite/HEAD";
const std::string Repository::remotesFile = ".gitlite/remotes";

// 构造函数：初始化暂存区（传入暂存区文件和删除文件列表路径）
Repository::Repository()
    : stagingArea(stagingAreaFile, removedFile) {}

// 静态方法：检查仓库是否已初始化
bool Repository::isInitialized() {
    return Utils::isDirectory(gitliteDir);
}

// 静态方法：获取仓库根目录路径
std::string Repository::getGitliteDir() {
    return gitliteDir;
}

// 核心操作：初始化仓库
void Repository::init() {
    if (isInitialized()) {
        Utils::exitWithMessage("A Gitlite version-control system already exists in the current directory.");
    }

    // 创建核心目录
    Utils::createDirectories(gitliteDir);
    Utils::createDirectories(objectsDir);
    Utils::createDirectories(branchesDir);

    // 创建初始提交（空 Blob、空父提交、 epoch 时间）
    std::map<std::string, std::string> emptyBlobs;
    std::vector<std::string> emptyParents;
    std::time_t epoch = 0; // Thu Jan 01 00:00:00 1970 +0000
    Commit initialCommit("initial commit", epoch, emptyParents, emptyBlobs);

    saveCommit(initialCommit);

    // 创建 master 分支，指向初始提交
    setBranchHead("master", initialCommit.getId());
    setCurrentBranch("master");

    // 初始化空暂存区
    clearStagingArea();
}

// 核心操作：添加文件到暂存区
void Repository::add(const std::string& filename) {
    if (!Utils::exists(filename)) {
        Utils::exitWithMessage("File does not exist.");
    }

    // 若文件已标记为删除，取消删除标记
    if (stagingArea.isRemoved(filename)) {
        stagingArea.removeRemovedFile(filename);
        stagingArea.save();
        return;
    }

    std::string currentCommitId = getCurrentCommitId();
    std::string currentBlobId = getFileBlobId(filename, currentCommitId);

    // 读取文件当前内容，生成新 Blob ID
    std::string fileContent = Utils::readContentsAsString(filename);
    std::string newBlobId = Blob::generateId(fileContent);

    // 若文件与当前提交一致，取消暂存（如果已暂存）
    if (currentBlobId == newBlobId) {
        if (stagingArea.isStaged(filename)) {
            stagingArea.removeStagedFile(filename);
            stagingArea.save();
        }
        return;
    }

    // 暂存文件：更新暂存区映射，保存 Blob 到磁盘
    stagingArea.addStagedFile(filename, newBlobId);
    stagingArea.save();
    Blob::create(objectsDir, fileContent); // 自动写入 Blob 到 objects 目录
}

// 核心操作：提交暂存区的更改
void Repository::commit(const std::string& message) {
    if (message.empty()) {
        Utils::exitWithMessage("Please enter a commit message.");
    }

    // 获取暂存区和删除文件列表
    const auto& stagingMap = stagingArea.getStagingMap();
    const auto& removedFiles = stagingArea.getRemovedFiles();

    if (stagingMap.empty() && removedFiles.empty()) {
        Utils::exitWithMessage("No changes added to the commit.");
    }

    // 获取当前提交，基于其 Blob 创建新提交的 Blob 集合
    std::string currentCommitId = getCurrentCommitId();
    Commit currentCommit = getCommit(currentCommitId);
    std::map<std::string, std::string> newBlobs = currentCommit.getBlobs();

    // 应用暂存区的文件更新
    for (const auto& staged : stagingMap) {
        newBlobs[staged.first] = staged.second;
    }

    // 应用文件删除操作
    for (const auto& removed : removedFiles) {
        newBlobs.erase(removed);
    }

    // 创建新提交（父提交为当前提交）
    std::vector<std::string> parents = {currentCommitId};
    std::time_t now = std::time(nullptr);
    Commit newCommit(message, now, parents, newBlobs);

    // 保存新提交，更新当前分支的 HEAD
    saveCommit(newCommit);
    std::string currentBranch = getCurrentBranch();
    setBranchHead(currentBranch, newCommit.getId());

    // 清空暂存区
    clearStagingArea();
}

// 核心操作：删除文件（工作区+暂存区/跟踪）
void Repository::rm(const std::string& filename) {
    const auto& stagingMap = stagingArea.getStagingMap();
    std::string currentCommitId = getCurrentCommitId();
    bool fileTracked = fileExistsInCommit(filename, currentCommitId);
    bool fileStaged = stagingArea.isStaged(filename);

    // 既未暂存也未跟踪，无需删除
    if (!fileStaged && !fileTracked) {
        Utils::exitWithMessage("No reason to remove the file.");
    }

    // 若已暂存，从暂存区移除
    if (fileStaged) {
        stagingArea.removeStagedFile(filename);
        stagingArea.save();
    }

    // 若已跟踪，标记为删除并删除工作区文件
    if (fileTracked) {
        stagingArea.addRemovedFile(filename);
        stagingArea.save();

        if (Utils::exists(filename)) {
            Utils::restrictedDelete(filename);
        }
    }
}

// 核心操作：查看当前分支的提交日志
void Repository::log() {
    std::string currentCommitId = getCurrentCommitId();

    while (!currentCommitId.empty()) {
        Commit commit = getCommit(currentCommitId);

        std::cout << "===" << std::endl;
        std::cout << "commit " << commit.getId() << std::endl;

        if (commit.isMergeCommit()) {
            auto parents = commit.getParents();
            std::cout << "Merge: "
                     << parents[0].substr(0, 7) << " "
                     << parents[1].substr(0, 7) << std::endl;
        }

        std::cout << "Date: " << commit.getFormattedTimestamp() << std::endl;
        std::cout << commit.getMessage() << std::endl;
        std::cout << std::endl;

        auto parents = commit.getParents();
        currentCommitId = parents.empty() ? "" : parents[0];
    }
}

// 核心操作：查看所有提交日志
void Repository::globalLog() {
    auto allCommits = Utils::plainFilenamesIn(objectsDir);

    for (const auto& commitId : allCommits) {
        if (commitId.length() == Utils::UID_LENGTH) {
            try {
                Commit commit = getCommit(commitId);
                std::cout << "===" << std::endl;
                std::cout << "commit " << commit.getId() << std::endl;

                if (commit.isMergeCommit()) {
                    auto parents = commit.getParents();
                    std::cout << "Merge: "
                             << parents[0].substr(0, 7) << " "
                             << parents[1].substr(0, 7) << std::endl;
                }

                std::cout << "Date: " << commit.getFormattedTimestamp() << std::endl;
                std::cout << commit.getMessage() << std::endl;
                std::cout << std::endl;
            } catch (...) {
                // 跳过无效提交
                continue;
            }
        }
    }
}

// 核心操作：根据提交信息查找提交 ID
void Repository::find(const std::string& commitMessage) {
    auto allCommits = Utils::plainFilenamesIn(objectsDir);
    bool found = false;

    for (const auto& commitId : allCommits) {
        if (commitId.length() == Utils::UID_LENGTH) {
            try {
                Commit commit = getCommit(commitId);
                if (commit.getMessage() == commitMessage) {
                    std::cout << commit.getId() << std::endl;
                    found = true;
                }
            } catch (...) {
                // 跳过无效提交
                continue;
            }
        }
    }

    if (!found) {
        Utils::exitWithMessage("Found no commit with that message.");
    }
}

// 核心操作：从当前提交检出文件
void Repository::checkoutFile(const std::string& filename) {
    std::string currentCommitId = getCurrentCommitId();
    checkoutFileInCommit(currentCommitId, filename);
}

// 核心操作：从指定提交检出文件
void Repository::checkoutFileInCommit(const std::string& commitId, const std::string& filename) {
    std::string fullCommitId = getFullCommitId(commitId);
    if (fullCommitId.empty()) {
        Utils::exitWithMessage("No commit with that id exists.");
    }

    if (!fileExistsInCommit(filename, fullCommitId)) {
        Utils::exitWithMessage("File does not exist in that commit.");
    }

    copyFileFromCommit(filename, fullCommitId);
}

// 核心操作：切换分支
void Repository::checkoutBranch(const std::string& branchName) {
    auto branches = getAllBranches();
    if (branches.find(branchName) == branches.end()) {
        Utils::exitWithMessage("No such branch exists.");
    }

    std::string currentBranch = getCurrentBranch();
    if (branchName == currentBranch) {
        Utils::exitWithMessage("No need to checkout the current branch.");
    }

    std::string currentCommitId = getCurrentCommitId();
    std::string targetCommitId = getBranchHead(branchName);

    // 检查未跟踪文件是否会被覆盖
    auto currentBlobs = getTrackedFiles(currentCommitId);
    auto targetBlobs = getTrackedFiles(targetCommitId);
    auto untrackedFiles = getUntrackedFiles();

    for (const auto& untracked : untrackedFiles) {
        if (targetBlobs.count(untracked)) {
            Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
        }
    }

    // 删除当前分支跟踪但目标分支不跟踪的文件
    for (const auto& currentBlob : currentBlobs) {
        if (!targetBlobs.count(currentBlob.first)) {
            Utils::restrictedDelete(currentBlob.first);
        }
    }

    // 从目标分支检出所有文件
    for (const auto& targetBlob : targetBlobs) {
        copyFileFromCommit(targetBlob.first, targetCommitId);
    }

    // 清空暂存区并切换分支
    clearStagingArea();
    setCurrentBranch(branchName);
}

// 核心操作：查看仓库状态
void Repository::status() {
    // 1. 显示所有分支（当前分支标 *）
    std::cout << "=== Branches ===" << std::endl;
    auto branches = getAllBranches();
    std::string currentBranch = getCurrentBranch();

    std::vector<std::string> sortedBranches(branches.begin(), branches.end());
    std::sort(sortedBranches.begin(), sortedBranches.end());

    for (const auto& branch : sortedBranches) {
        if (branch == currentBranch) {
            std::cout << "*";
        }
        std::cout << branch << std::endl;
    }

    std::cout << std::endl;

    // 2. 显示暂存文件
    std::cout << "=== Staged Files ===" << std::endl;
    const auto& stagingMap = stagingArea.getStagingMap();
    std::vector<std::string> stagedFiles;
    for (const auto& staged : stagingMap) {
        stagedFiles.push_back(staged.first);
    }
    std::sort(stagedFiles.begin(), stagedFiles.end());
    for (const auto& file : stagedFiles) {
        std::cout << file << std::endl;
    }

    std::cout << std::endl;

    // 3. 显示被标记删除的文件
    std::cout << "=== Removed Files ===" << std::endl;
    const auto& removedFiles = stagingArea.getRemovedFiles();
    std::vector<std::string> removedList(removedFiles.begin(), removedFiles.end());
    std::sort(removedList.begin(), removedList.end());
    for (const auto& file : removedList) {
        std::cout << file << std::endl;
    }

    std::cout << std::endl;

    // 4. 显示未暂存的修改（基于 getModifiedFiles）
    std::cout << "=== Modifications Not Staged For Commit ===" << std::endl;
    auto modifiedFiles = getModifiedFiles();
    std::vector<std::string> modifiedList;
    for (const auto& modified : modifiedFiles) {
        modifiedList.push_back(modified.first + " (" + modified.second + ")");
    }
    std::sort(modifiedList.begin(), modifiedList.end());
    for (const auto& file : modifiedList) {
        std::cout << file << std::endl;
    }

    std::cout << std::endl;

    // 5. 显示未跟踪文件
    std::cout << "=== Untracked Files ===" << std::endl;
    auto untrackedFiles = getUntrackedFiles();
    std::vector<std::string> untrackedList(untrackedFiles.begin(), untrackedFiles.end());
    std::sort(untrackedList.begin(), untrackedList.end());
    for (const auto& file : untrackedList) {
        std::cout << file << std::endl;
    }
}

// 核心操作：创建分支
void Repository::branch(const std::string& branchName) {
    auto branches = getAllBranches();
    if (branches.count(branchName)) {
        Utils::exitWithMessage("A branch with that name already exists.");
    }

    std::string currentCommitId = getCurrentCommitId();
    setBranchHead(branchName, currentCommitId);
}

// 核心操作：删除分支
void Repository::rmBranch(const std::string& branchName) {
    auto branches = getAllBranches();
    if (branches.find(branchName) == branches.end()) {
        Utils::exitWithMessage("A branch with that name does not exist.");
    }

    std::string currentBranch = getCurrentBranch();
    if (branchName == currentBranch) {
        Utils::exitWithMessage("Cannot remove the current branch.");
    }

    std::string branchFile = Utils::join(branchesDir, branchName);
    Utils::restrictedDelete(branchFile);
}

// 核心操作：重置到指定提交
void Repository::reset(const std::string& commitId) {
    std::string fullCommitId = getFullCommitId(commitId);
    if (fullCommitId.empty()) {
        Utils::exitWithMessage("No commit with that id exists.");
    }

    std::string currentCommitId = getCurrentCommitId();
    auto currentBlobs = getTrackedFiles(currentCommitId);
    auto targetBlobs = getTrackedFiles(fullCommitId);
    auto untrackedFiles = getUntrackedFiles();

    // 检查未跟踪文件是否会被覆盖
    for (const auto& untracked : untrackedFiles) {
        if (targetBlobs.count(untracked)) {
            Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
        }
    }

    // 删除当前提交跟踪但目标提交不跟踪的文件
    for (const auto& currentBlob : currentBlobs) {
        if (!targetBlobs.count(currentBlob.first)) {
            Utils::restrictedDelete(currentBlob.first);
        }
    }

    // 从目标提交检出所有文件
    for (const auto& targetBlob : targetBlobs) {
        copyFileFromCommit(targetBlob.first, fullCommitId);
    }

    // 更新当前分支的 HEAD 到目标提交，清空暂存区
    std::string currentBranch = getCurrentBranch();
    setBranchHead(currentBranch, fullCommitId);
    clearStagingArea();
}

// 核心操作：合并指定分支到当前分支
void Repository::merge(const std::string& branchName) {
    auto branches = getAllBranches();
    if (branches.find(branchName) == branches.end()) {
        Utils::exitWithMessage("A branch with that name does not exist.");
    }

    std::string currentBranch = getCurrentBranch();
    if (branchName == currentBranch) {
        Utils::exitWithMessage("Cannot merge a branch with itself.");
    }

    // 检查暂存区是否为空（不允许有未提交的修改）
    const auto& stagingMap = stagingArea.getStagingMap();
    const auto& removedFiles = stagingArea.getRemovedFiles();
    if (!stagingMap.empty() || !removedFiles.empty()) {
        Utils::exitWithMessage("You have uncommitted changes.");
    }

    // 获取当前分支、目标分支的 HEAD 提交 ID，以及拆分点
    std::string currentCommitId = getCurrentCommitId();
    std::string givenCommitId = getBranchHead(branchName);
    std::string splitPointId = findSplitPoint(currentCommitId, givenCommitId);

    // 情况 1：目标分支是当前分支的祖先（无需合并）
    if (splitPointId == givenCommitId) {
        std::cout << "Given branch is an ancestor of the current branch." << std::endl;
        return;
    }

    // 情况 2：当前分支是目标分支的祖先（快进合并）
    if (splitPointId == currentCommitId) {
        checkoutBranch(branchName);
        std::cout << "Current branch fast-forwarded." << std::endl;
        return;
    }

    // 情况 3：需要三方合并（拆分点、当前分支、目标分支）
    Commit splitCommit = getCommit(splitPointId);
    Commit currentCommit = getCommit(currentCommitId);
    Commit givenCommit = getCommit(givenCommitId);

    auto splitBlobs = splitCommit.getBlobs();
    auto currentBlobs = currentCommit.getBlobs();
    auto givenBlobs = givenCommit.getBlobs();

    std::map<std::string, std::string> mergedBlobs = currentBlobs;
    std::set<std::string> allFiles;

    // 收集所有涉及的文件（拆分点、当前分支、目标分支）
    for (const auto& blob : splitBlobs) allFiles.insert(blob.first);
    for (const auto& blob : currentBlobs) allFiles.insert(blob.first);
    for (const auto& blob : givenBlobs) allFiles.insert(blob.first);

    bool conflictOccurred = false;

    for (const auto& filename : allFiles) {
        // 获取三个版本的 Blob ID
        std::string splitBlobId = splitBlobs.count(filename) ? splitBlobs.at(filename) : "";
        std::string currentBlobId = currentBlobs.count(filename) ? currentBlobs.at(filename) : "";
        std::string givenBlobId = givenBlobs.count(filename) ? givenBlobs.at(filename) : "";

        // 情况 1：目标分支修改，当前分支未修改（采纳目标分支的修改）
        if (splitBlobId != givenBlobId && currentBlobId == splitBlobId) {
            if (!givenBlobId.empty()) {
                mergedBlobs[filename] = givenBlobId;
                copyFileFromCommit(filename, givenCommitId);
                stagingArea.addStagedFile(filename, givenBlobId);
            } else {
                mergedBlobs.erase(filename);
                Utils::restrictedDelete(filename);
                stagingArea.addRemovedFile(filename);
            }
        }
        // 情况 2：当前分支修改，目标分支未修改（保留当前分支的修改）
        else if (splitBlobId != currentBlobId && givenBlobId == splitBlobId) {
            // 无需操作，保留 currentBlobs 中的内容
        }
        // 情况 3：双方修改一致（保留当前分支的修改）
        else if (currentBlobId == givenBlobId && currentBlobId != splitBlobId) {
            // 无需操作，保留 currentBlobs 中的内容
        }
        // 情况 4：冲突（双方修改不一致）
        else if (currentBlobId != givenBlobId) {
            conflictOccurred = true;

            // 读取双方的文件内容
            std::string currentContent = currentBlobId.empty() ? "" : Blob::load(objectsDir, currentBlobId).getContent();
            std::string givenContent = givenBlobId.empty() ? "" : Blob::load(objectsDir, givenBlobId).getContent();

            // 生成冲突标记的内容
            std::string conflictContent = "<<<<<<< HEAD\n" + currentContent +
                                        "=======\n" + givenContent +
                                        ">>>>>>>\n";

            // 创建冲突 Blob 并暂存
            Blob conflictBlob = Blob::create(objectsDir, conflictContent);
            mergedBlobs[filename] = conflictBlob.getId();
            Utils::writeContents(filename, conflictContent);
            stagingArea.addStagedFile(filename, conflictBlob.getId());
        }
    }

    // 保存暂存区状态
    stagingArea.save();

    // 若无冲突，创建合并提交
    if (!conflictOccurred) {
        std::vector<std::string> parents = {currentCommitId, givenCommitId};
        std::time_t now = std::time(nullptr);
        std::string mergeMessage = "Merged " + branchName + " into " + currentBranch;
        Commit mergeCommit(mergeMessage, now, parents, mergedBlobs);

        saveCommit(mergeCommit);
        setBranchHead(currentBranch, mergeCommit.getId());
        clearStagingArea();
        std::cout << "Merge completed successfully." << std::endl;
    } else {
        std::cout << "Encountered a merge conflict. Resolve conflicts and commit." << std::endl;
    }
}

// 核心操作：添加远程仓库
void Repository::addRemote(const std::string& remoteName, const std::string& remotePath) {
    auto remotes = getRemotes();
    if (remotes.count(remoteName)) {
        Utils::exitWithMessage("A remote with that name already exists.");
    }

    remotes[remoteName] = remotePath;
    saveRemotes(remotes);
}

// 核心操作：删除远程仓库
void Repository::rmRemote(const std::string& remoteName) {
    auto remotes = getRemotes();
    if (remotes.find(remoteName) == remotes.end()) {
        Utils::exitWithMessage("A remote with that name does not exist.");
    }

    remotes.erase(remoteName);
    saveRemotes(remotes);
}

// 核心操作：推送本地分支到远程
void Repository::push(const std::string& remoteName, const std::string& remoteBranchName) {
    auto remotes = getRemotes();
    if (remotes.find(remoteName) == remotes.end()) {
        Utils::exitWithMessage("A remote with that name does not exist.");
    }

    std::string remotePath = remotes[remoteName];
    if (!Utils::isDirectory(remotePath)) {
        Utils::exitWithMessage("Remote directory not found.");
    }

    // 检查远程仓库是否初始化
    std::string remoteGitliteDir = Utils::join(remotePath, gitliteDir.substr(2)); // 移除 ./ 前缀
    if (!Utils::isDirectory(remoteGitliteDir)) {
        Utils::exitWithMessage("Remote is not a Gitlite repository.");
    }

    std::string localBranchHead = getCurrentCommitId();
    std::string remoteBranchFile = Utils::join(remoteGitliteDir, "branches", remoteBranchName);

    // 获取远程分支的当前 HEAD
    std::string remoteBranchHead;
    if (Utils::exists(remoteBranchFile)) {
        remoteBranchHead = Utils::readContentsAsString(remoteBranchFile);
    }

    // 检查远程分支是否在本地分支的历史中（避免非快进推送）
    bool foundInHistory = false;
    std::string currentCommit = localBranchHead;
    while (!currentCommit.empty()) {
        if (currentCommit == remoteBranchHead) {
            foundInHistory = true;
            break;
        }
        Commit commit = getCommit(currentCommit);
        auto parents = commit.getParents();
        currentCommit = parents.empty() ? "" : parents[0];
    }

    if (!remoteBranchHead.empty() && !foundInHistory) {
        Utils::exitWithMessage("Please pull down remote changes before pushing.");
    }

    // 收集需要推送的提交（从本地 HEAD 到远程 HEAD 的路径）
    std::vector<std::string> commitsToCopy;
    currentCommit = localBranchHead;
    while (!currentCommit.empty() && currentCommit != remoteBranchHead) {
        commitsToCopy.push_back(currentCommit);
        Commit commit = getCommit(currentCommit);
        auto parents = commit.getParents();
        currentCommit = parents.empty() ? "" : parents[0];
    }

    // 反向推送（从祖先到后代，确保依赖正确）
    for (auto it = commitsToCopy.rbegin(); it != commitsToCopy.rend(); ++it) {
        std::string commitId = *it;
        std::string localCommitPath = Utils::join(objectsDir, commitId);
        std::string remoteCommitPath = Utils::join(remoteGitliteDir, "objects", commitId);

        if (!Utils::exists(remoteCommitPath)) {
            // 推送提交文件
            Commit commit = getCommit(commitId);
            Utils::writeContents(remoteCommitPath, commit.serialize());

            // 推送该提交依赖的所有 Blob
            for (const auto& blob : commit.getBlobs()) {
                std::string blobId = blob.second;
                std::string localBlobPath = Utils::join(objectsDir, blobId);
                std::string remoteBlobPath = Utils::join(remoteGitliteDir, "objects", blobId);

                if (!Utils::exists(remoteBlobPath)) {
                    std::string blobContent = Utils::readContentsAsString(localBlobPath);
                    Utils::writeContents(remoteBlobPath, blobContent);
                }
            }
        }
    }

    // 更新远程分支的 HEAD
    Utils::writeContents(remoteBranchFile, localBranchHead);
    std::cout << "Pushed to " << remoteName << "/" << remoteBranchName << std::endl;
}

// 核心操作：拉取远程分支到本地跟踪分支
void Repository::fetch(const std::string& remoteName, const std::string& remoteBranchName) {
    auto remotes = getRemotes();
    if (remotes.find(remoteName) == remotes.end()) {
        Utils::exitWithMessage("A remote with that name does not exist.");
    }

    std::string remotePath = remotes[remoteName];
    if (!Utils::isDirectory(remotePath)) {
        Utils::exitWithMessage("Remote directory not found.");
    }

    // 检查远程仓库是否初始化
    std::string remoteGitliteDir = Utils::join(remotePath, gitliteDir.substr(2));
    if (!Utils::isDirectory(remoteGitliteDir)) {
        Utils::exitWithMessage("Remote is not a Gitlite repository.");
    }

    // 检查远程分支是否存在
    std::string remoteBranchFile = Utils::join(remoteGitliteDir, "branches", remoteBranchName);
    if (!Utils::exists(remoteBranchFile)) {
        Utils::exitWithMessage("That remote does not have that branch.");
    }

    // 获取远程分支的 HEAD 提交 ID
    std::string remoteBranchHead = Utils::readContentsAsString(remoteBranchFile);
    std::string localTrackingBranch = remoteName + "/" + remoteBranchName;

    // 收集需要拉取的提交（从远程 HEAD 到本地已有的祖先）
    std::set<std::string> copiedCommits;
    std::vector<std::string> commitsToCopy;
    std::string currentCommit = remoteBranchHead;

    while (!currentCommit.empty() && copiedCommits.find(currentCommit) == copiedCommits.end()) {
        // 检查本地是否已存在该提交
        std::string localCommitPath = Utils::join(objectsDir, currentCommit);
        if (Utils::exists(localCommitPath)) {
            copiedCommits.insert(currentCommit);
            // 递归处理父提交
            Commit commit = getCommit(currentCommit);
            bool hasNewParent = false;
            for (const auto& parent : commit.getParents()) {
                if (copiedCommits.find(parent) == copiedCommits.end()) {
                    currentCommit = parent;
                    hasNewParent = true;
                    break;
                }
            }
            if (!hasNewParent) break;
        } else {
            commitsToCopy.push_back(currentCommit);
            copiedCommits.insert(currentCommit);
            // 读取远程提交，获取父提交
            std::string remoteCommitPath = Utils::join(remoteGitliteDir, "objects", currentCommit);
            if (Utils::exists(remoteCommitPath)) {
                Commit remoteCommit = Commit::fromFile(remoteCommitPath);
                bool hasNewParent = false;
                for (const auto& parent : remoteCommit.getParents()) {
                    if (copiedCommits.find(parent) == copiedCommits.end()) {
                        currentCommit = parent;
                        hasNewParent = true;
                        break;
                    }
                }
                if (!hasNewParent) break;
            } else {
                throw GitliteException("Remote commit not found: " + currentCommit);
            }
        }
    }

    // 反向拉取（从祖先到后代，确保依赖正确）
    for (auto it = commitsToCopy.rbegin(); it != commitsToCopy.rend(); ++it) {
        std::string commitId = *it;
        std::string localCommitPath = Utils::join(objectsDir, commitId);
        std::string remoteCommitPath = Utils::join(remoteGitliteDir, "objects", commitId);

        if (!Utils::exists(localCommitPath)) {
            // 拉取提交文件
            Commit remoteCommit = Commit::fromFile(remoteCommitPath);
            Utils::writeContents(localCommitPath, remoteCommit.serialize());

            // 拉取该提交依赖的所有 Blob
            for (const auto& blob : remoteCommit.getBlobs()) {
                std::string blobId = blob.second;
                std::string localBlobPath = Utils::join(objectsDir, blobId);
                std::string remoteBlobPath = Utils::join(remoteGitliteDir, "objects", blobId);

                if (!Utils::exists(localBlobPath)) {
                    std::string blobContent = Utils::readContentsAsString(remoteBlobPath);
                    Utils::writeContents(localBlobPath, blobContent);
                }
            }
        }
    }

    // 创建/更新本地跟踪分支
    setBranchHead(localTrackingBranch, remoteBranchHead);
    std::cout << "Fetched " << remoteName << "/" << remoteBranchName << " to local tracking branch." << std::endl;
}

// 核心操作：拉取并合并远程分支
void Repository::pull(const std::string& remoteName, const std::string& remoteBranchName) {
    fetch(remoteName, remoteBranchName);
    merge(remoteName + "/" + remoteBranchName);
}

// 辅助接口：获取当前分支名称
std::string Repository::getCurrentBranch() {
    if (!Utils::exists(headFile)) {
        return "";
    }
    return Utils::readContentsAsString(headFile);
}

// 辅助接口：获取当前分支的 HEAD Commit ID
std::string Repository::getCurrentCommitId() {
    std::string currentBranch = getCurrentBranch();
    return getBranchHead(currentBranch);
}

// 辅助接口：设置当前分支
void Repository::setCurrentBranch(const std::string& branchName) {
    Utils::writeContents(headFile, branchName);
}

// 辅助接口：清空暂存区（含删除文件列表）
void Repository::clearStagingArea() {
    stagingArea.clear();
}

// 辅助方法：获取指定分支的 HEAD Commit ID
std::string Repository::getBranchHead(const std::string& branchName) {
    std::string branchFile = Utils::join(branchesDir, branchName);
    if (!Utils::exists(branchFile)) {
        return "";
    }
    return Utils::readContentsAsString(branchFile);
}

// 辅助方法：设置指定分支的 HEAD Commit ID
void Repository::setBranchHead(const std::string& branchName, const std::string& commitId) {
    std::string branchFile = Utils::join(branchesDir, branchName);
    Utils::writeContents(branchFile, commitId);
}

// 辅助方法：通过 ID 获取 Commit 实例（支持短 ID）
Commit Repository::getCommit(const std::string& commitId) {
    std::string fullId = getFullCommitId(commitId);
    if (fullId.empty()) {
        throw GitliteException("Commit not found: " + commitId);
    }

    std::string commitFile = Utils::join(objectsDir, fullId);
    return Commit::fromFile(commitFile);
}

// 辅助方法：保存 Commit 到 objects 目录
void Repository::saveCommit(const Commit& commit) {
    std::string commitFile = Utils::join(objectsDir, commit.getId());
    Utils::writeContents(commitFile, commit.serialize());
}

// 辅助方法：将短 ID 转换为完整 ID（支持前缀匹配）
std::string Repository::getFullCommitId(const std::string& shortId) {
    if (shortId.empty()) return "";

    auto allCommits = Utils::plainFilenamesIn(objectsDir);
    std::string match;

    for (const auto& commitId : allCommits) {
        if (commitId.length() == Utils::UID_LENGTH && commitId.find(shortId) == 0) {
            if (!match.empty()) {
                Utils::exitWithMessage("Ambiguous commit ID: " + shortId);
            }
            match = commitId;
        }
    }

    return match;
}

// 辅助方法：获取文件在指定提交中的 Blob ID
std::string Repository::getFileBlobId(const std::string& filename, const std::string& commitId) {
    if (commitId.empty()) return "";

    Commit commit = getCommit(commitId);
    auto blobs = commit.getBlobs();
    if (blobs.count(filename)) {
        return blobs.at(filename);
    }
    return "";
}

// 辅助方法：从指定提交复制文件到工作区
void Repository::copyFileFromCommit(const std::string& filename, const std::string& commitId) {
    std::string blobId = getFileBlobId(filename, commitId);
    if (blobId.empty()) return;

    Blob blob = Blob::load(objectsDir, blobId);
    Utils::writeContents(filename, blob.getContent());
}

// 辅助方法：检查文件是否存在于指定提交中
bool Repository::fileExistsInCommit(const std::string& filename, const std::string& commitId) {
    return !getFileBlobId(filename, commitId).empty();
}

// 辅助方法：获取所有分支名称
std::set<std::string> Repository::getAllBranches() {
    auto branchFiles = Utils::plainFilenamesIn(branchesDir);
    return std::set<std::string>(branchFiles.begin(), branchFiles.end());
}

// 辅助方法：查找两个提交的最近公共祖先
std::string Repository::findSplitPoint(const std::string& branch1, const std::string& branch2) {
    std::set<std::string> ancestors1;
    std::string current = branch1;

    // 收集第一个提交的所有祖先
    while (!current.empty()) {
        ancestors1.insert(current);
        Commit commit = getCommit(current);
        auto parents = commit.getParents();
        current = parents.empty() ? "" : parents[0];
    }

    // 查找第二个提交的第一个祖先（存在于 ancestors1 中）
    current = branch2;
    while (!current.empty()) {
        if (ancestors1.count(current)) {
            return current;
        }
        Commit commit = getCommit(current);
        auto parents = commit.getParents();
        current = parents.empty() ? "" : parents[0];
    }

    return "";
}

// 辅助方法：获取指定提交中的所有跟踪文件
std::map<std::string, std::string> Repository::getTrackedFiles(const std::string& commitId) {
    if (commitId.empty()) return {};

    Commit commit = getCommit(commitId);
    return commit.getBlobs();
}

// 辅助方法：检查文件是否被修改（与指定提交对比）
bool Repository::isFileModified(const std::string& filename, const std::string& commitId) {
    if (!Utils::exists(filename) || !fileExistsInCommit(filename, commitId)) {
        return false;
    }

    std::string commitBlobId = getFileBlobId(filename, commitId);
    std::string currentContent = Utils::readContentsAsString(filename);
    std::string currentBlobId = Blob::generateId(currentContent);

    return commitBlobId != currentBlobId;
}

// 辅助方法：获取工作区中的未跟踪文件
std::set<std::string> Repository::getUntrackedFiles() {
    std::set<std::string> untrackedFiles;
    std::string currentCommitId = getCurrentCommitId();
    auto trackedFiles = getTrackedFiles(currentCommitId);
    const auto& stagingMap = stagingArea.getStagingMap();

    // 获取工作区所有非隐藏文件（排除 gitlite 可执行文件）
    auto workingFiles = Utils::plainFilenamesIn(".");
    for (const auto& filename : workingFiles) {
        if (filename.empty()) continue;
        if (filename[0] == '.') continue; // 跳过隐藏文件
        if (filename == "gitlite" || filename == "gitlite.exe") continue; // 跳过可执行文件

        // 未被跟踪且未被暂存，视为未跟踪文件
        if (!trackedFiles.count(filename) && !stagingMap.count(filename)) {
            untrackedFiles.insert(filename);
        }
    }

    return untrackedFiles;
}

// 辅助方法：获取已修改但未暂存的文件
std::map<std::string, std::string> Repository::getModifiedFiles() {
    std::map<std::string, std::string> modifiedFiles;
    std::string currentCommitId = getCurrentCommitId();
    auto trackedFiles = getTrackedFiles(currentCommitId);
    const auto& stagingMap = stagingArea.getStagingMap();

    // 检查已跟踪文件的修改状态
    for (const auto& tracked : trackedFiles) {
        std::string filename = tracked.first;
        if (Utils::exists(filename)) {
            if (isFileModified(filename, currentCommitId)) {
                modifiedFiles[filename] = "modified";
            }
        } else {
            // 文件被删除但未标记
            modifiedFiles[filename] = "deleted";
        }
    }

    // 检查已暂存文件的修改状态（暂存后又修改）
    for (const auto& staged : stagingMap) {
        std::string filename = staged.first;
        if (Utils::exists(filename)) {
            std::string currentContent = Utils::readContentsAsString(filename);
            std::string currentBlobId = Blob::generateId(currentContent);
            if (currentBlobId != staged.second) {
                modifiedFiles[filename] = "modified";
            }
        } else {
            modifiedFiles[filename] = "deleted";
        }
    }

    return modifiedFiles;
}

// 辅助方法：加载所有远程仓库配置
std::map<std::string, std::string> Repository::getRemotes() {
    std::map<std::string, std::string> remotes;
    if (!Utils::exists(remotesFile)) {
        return remotes;
    }

    std::string content = Utils::readContentsAsString(remotesFile);
    std::istringstream iss(content);
    std::string line;

    while (std::getline(iss, line)) {
        size_t spacePos = line.find(' ');
        if (spacePos != std::string::npos) {
            std::string name = line.substr(0, spacePos);
            std::string path = line.substr(spacePos + 1);
            remotes[name] = path;
        }
    }

    return remotes;
}

// 辅助方法：保存远程仓库配置到文件
void Repository::saveRemotes(const std::map<std::string, std::string>& remotes) {
    std::ostringstream oss;
    for (const auto& remote : remotes) {
        oss << remote.first << " " << remote.second << "\n";
    }
    Utils::writeContents(remotesFile, oss.str());
}