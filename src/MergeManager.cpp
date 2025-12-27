#include"../include/MergeManager.h"
#include"../include/RepositoryCore.h"
#include"../include/CommitManager.h"
#include"../include/FileOperationManager.h"
#include"../include/BranchManager.h"
#include"../include/Utils.h"
#include"../include/Blob.h"
#include<queue>
#include<iostream>
#include<sstream>

MergeManager::MergeManager(RepositoryCore* repoCore,CommitManager* commitMgr,FileOperationManager* fileOpMgr,BranchManager* branchMgr)
    : core(repoCore),commitManager(commitMgr),fileOpManager(fileOpMgr),branchManager(branchMgr) {}

void MergeManager::merge(const std::string& branchName){
    std::string given_commit_id=core->getBranchHead(branchName); 
    if(given_commit_id.empty()){
        Utils::exitWithMessage("A branch with that name does not exist.");
    }

    std::string current_branch=core->getCurrentBranch();
    if(branchName==current_branch){
        Utils::exitWithMessage("Cannot merge a branch with itself.");
    }

    std::string current_commit_id=commitManager->getCurrentCommitId();  // 获取当前commit ID
    std::string split_point_id=findSplitPoint(current_commit_id,given_commit_id);  // 查找分割点

    core->getStagingArea().reload(); 

    // 情况1: 目标分支是当前分支的祖先，无需合并
    if(split_point_id==given_commit_id){
        std::cout<<"Given branch is an ancestor of the current branch."<<std::endl;
        return;
    }

    // 情况2: 当前分支是目标分支的祖先，执行快进合并
    if(split_point_id==current_commit_id){
        performFastForwardMerge(branchName);
        std::cout<<"Current branch fast-forwarded."<<std::endl;
        return;
    }

    // 情况3: 两个分支有分叉，执行三方合并
    performThreeWayMerge(branchName,current_commit_id,given_commit_id,split_point_id);
}

bool MergeManager::checkMergeConditions(const std::string& branchName){
    std::string given_commit_id=core->getBranchHead(branchName);
    return !given_commit_id.empty() && branchName!=core->getCurrentBranch();  // 分支存在且不是当前分支
}

// 快进合并
void MergeManager::performFastForwardMerge(const std::string& branchName){
    std::string target_commit_id=core->getBranchHead(branchName);      // 获取目标分支commit ID
    std::string current_commit_id=commitManager->getCurrentCommitId();     // 获取当前分支commit ID

    Commit target_commit=commitManager->getCommit(target_commit_id);      // 加载目标commit
    Commit current_commit=commitManager->getCommit(current_commit_id);    // 加载当前commit

    auto current_blobs=current_commit.getBlobs();     // 当前commit的文件列表
    auto target_blobs=target_commit.getBlobs();       // 目标commit的文件列表

    // 安全检查：防止覆盖未跟踪文件
    auto working_files=Utils::plainFilenamesIn(".");
    for(const auto& filename:working_files){
        if(!current_blobs.count(filename)&&target_blobs.count(filename)){
            Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
        }
    }

    // 第一步：删除当前分支有但目标分支没有的文件
    for(const auto& current_blob : current_blobs){
        if(!target_blobs.count(current_blob.first)){
            remove(current_blob.first.c_str());  // 删除工作目录中的文件
        }
    }

    // 第二步：添加或更新目标分支的文件
    for(const auto& target_blob : target_blobs){
        std::string blob_id=target_blob.second;          
        Blob blob=Blob::load(".gitlite/objects",blob_id);
        Utils::writeContents(target_blob.first,blob.getContent()); // 写入工作目录
    }

    core->setBranchHead(core->getCurrentBranch(),target_commit_id);  // 分支指向新commit
    core->clearStagingArea();                                    
    core->getStagingArea().save();                               
}


std::set<std::string> MergeManager::getAllBranches(){
    return branchManager->getAllBranchesList(); 
}

// 查找两个分支的分割点
// DFS收集第一个分支祖先 + BFS查找第二个分支最早交集
std::string MergeManager::findSplitPoint(const std::string& branch1,const std::string& branch2){
    // 第一阶段：使用DFS收集branch1的所有祖先
    std::set<std::string> ancestors1;  
    std::vector<std::string> stack;     
    if(!branch1.empty())stack.push_back(branch1);

    while(!stack.empty()){
        std::string commit_id=stack.back(); 
        stack.pop_back();
        if(commit_id.empty()||ancestors1.count(commit_id))continue; 
        ancestors1.insert(commit_id); 

        std::string commit_file=Utils::join(".gitlite/objects",commit_id);
        Commit commit=Commit::fromFile(commit_file);  
        for(const auto& parent:commit.getParents()){
            if(!parent.empty()&&!ancestors1.count(parent)){
                stack.push_back(parent);
            }
        }
    }

    // 第二阶段：使用BFS从branch2开始查找最早交集
    if(branch2.empty())return "";
    std::queue<std::string> q;        
    std::set<std::string> visited;  
    q.push(branch2);
    visited.insert(branch2);

    while(!q.empty()){
        std::string commit_id=q.front(); 
        q.pop();
        if(ancestors1.count(commit_id))return commit_id; 

        std::string commit_file=Utils::join(".gitlite/objects",commit_id);
        Commit commit=Commit::fromFile(commit_file);
        for(const auto& parent:commit.getParents()){
            if(!parent.empty()&&!visited.count(parent)){
                visited.insert(parent); 
                q.push(parent);    
            }
        }       
    }

    return ""; 
}

// 三方合并
void MergeManager::performThreeWayMerge(const std::string& branchName,const std::string& current_commit_id,const std::string& given_commit_id,const std::string& split_point_id){
    Commit split_commit=commitManager->getCommit(split_point_id);  // 分割点
    Commit current_commit=commitManager->getCommit(current_commit_id);  // 当前分支
    Commit given_commit=commitManager->getCommit(given_commit_id);    // 目标分支

    auto split_blobs=split_commit.getBlobs();    // 分割点的文件
    auto current_blobs=current_commit.getBlobs();  // 当前分支的文件
    auto given_blobs=given_commit.getBlobs();      // 目标分支的文件

    auto untracked_files=fileOpManager->getUntrackedFiles();
    for(const auto given_blob:given_blobs){
        const std::string& filename=given_blob.first;
        if(!current_blobs.count(filename)&&untracked_files.count(filename)){
            Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
        }
    }

    // 检查暂存区状态
    StagingArea& stagingArea=core->getStagingArea();
    const auto& staged_now=stagingArea.getStagingMap();    // 当前暂存的文件
    const auto& removed_now=stagingArea.getRemovedFiles(); // 标记删除的文件
    if(!staged_now.empty()||!removed_now.empty()){
        Utils::exitWithMessage("You have uncommitted changes.");
    }

    std::map<std::string,std::string> merge_blobs=current_blobs;  // 基于当前分支的文件
    std::set<std::string> all_files;  // 收集所有涉及的文件
    
    for(const auto& blob:split_blobs){all_files.insert(blob.first);}  // 分割点的文件
    for(const auto& blob:current_blobs){all_files.insert(blob.first);} // 当前分支的文件
    for(const auto& blob:given_blobs){all_files.insert(blob.first);}   // 目标分支的文件

    bool conflict_occurred=false;           // 是否发生冲突
    std::set<std::string> conflict_files;  // 冲突文件列表

    for(const auto& filename:all_files){
        std::string split_blob_id=split_blobs.count(filename)?split_blobs.at(filename):"";
        std::string current_blob_id=current_blobs.count(filename)?current_blobs.at(filename):"";
        std::string given_blob_id=given_blobs.count(filename)?given_blobs.at(filename):"";

        // 情况1: 三个版本都相同，无需处理
        if(split_blob_id==current_blob_id&&split_blob_id==given_blob_id){
            continue;
        }

        // 情况2: 当前分支和分割点相同，但目标分支不同 - 直接采用目标分支版本
        if(split_blob_id==current_blob_id&&split_blob_id!=given_blob_id){
            if(!given_blob_id.empty()){
                merge_blobs[filename]=given_blob_id;  // 使用目标分支的文件
                commitManager->copyFileFromCommit(filename,given_commit_id);  // 复制文件到工作目录
            }
            else{
                merge_blobs.erase(filename);  // 目标分支删除了文件
                if(Utils::exists(filename)){
                    remove(filename.c_str());  // 删除工作目录文件
                }
            }
            continue;
        }

        // 情况3: 目标分支和分割点相同，但当前分支不同 - 保持当前分支版本
        if(split_blob_id!=current_blob_id&&split_blob_id==given_blob_id){
            continue;  // 保持在merge_blobs中的当前版本
        }

        if(split_blob_id.empty()){
            if(current_blob_id.empty()){
                if(given_blob_id.empty()){
                    continue;
                }
                else{
                    merge_blobs[filename]=given_blob_id;
                    commitManager->copyFileFromCommit(filename,given_commit_id);
                }
            }
            else{
                if(given_blob_id.empty()){
                    continue;
                }
                else{
                    conflict_occurred=true;
                }
            }
        }

        if(!split_blob_id.empty()&&current_blob_id.empty()&&given_blob_id.empty()){
            merge_blobs.erase(filename);
            continue;
        }

        if(current_blob_id==given_blob_id){
            continue;
        }

        // 生成冲突文件
        conflict_occurred=true;
        conflict_files.insert(filename);
        std::string current_content=current_blob_id.empty()?"":Blob::load(".gitlite/objects",current_blob_id).getContent();
        std::string given_content=given_blob_id.empty()?"":Blob::load(".gitlite/objects",given_blob_id).getContent();

        std::string conflict_content="<<<<<<< HEAD\n"+current_content
                                    +"=======\n"+given_content
                                    +">>>>>>>\n";

        Blob conflict_blob=Blob::create(".gitlite/objects",conflict_content);
        merge_blobs[filename]=conflict_blob.getId();
        Utils::writeContents(filename,conflict_content);
    }

    std::vector<std::string> parents;
    parents.push_back(current_commit_id);
    parents.push_back(given_commit_id);

    std::time_t now=std::time(nullptr);
    std::string merge_message="Merged "+branchName+" into "+core->getCurrentBranch()+".";
    Commit merge_commit(merge_message,now,parents,merge_blobs);

    commitManager->saveCommit(merge_commit);
    core->setBranchHead(core->getCurrentBranch(),merge_commit.getId());

    // 保存冲突文件
    if(conflict_occurred){
        fileOpManager->saveConflictFiles(conflict_files);
        stagingArea.save();
        std::cout<<"Encountered a merge conflict."<<std::endl;
    }
    else{
        for(const auto& merged_blob:merge_blobs){
            commitManager->copyFileFromCommit(merged_blob.first,merge_commit.getId());
        }
        for(const auto& current_blob:current_blobs){
            if(!merge_blobs.count(current_blob.first)){
                if(Utils::exists(current_blob.first)){
                    remove(current_blob.first.c_str());
                }
            }
        }
        core->clearStagingArea();
        stagingArea.save();
    }
}