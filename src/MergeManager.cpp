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

    std::string current_commit_id=commitManager->getCurrentCommitId();
    std::string split_point_id=findSplitPoint(current_commit_id,given_commit_id);

    core->getStagingArea().reload();

    if(split_point_id==given_commit_id){
        std::cout<<"Given branch is an ancestor of the current branch."<<std::endl;
        return;
    }

    if(split_point_id==current_commit_id){
        performFastForwardMerge(branchName);
        std::cout<<"Current branch fast-forwarded."<<std::endl;
        return;
    }

    performThreeWayMerge(branchName,current_commit_id,given_commit_id,split_point_id);
}

bool MergeManager::checkMergeConditions(const std::string& branchName){
    std::string given_commit_id=core->getBranchHead(branchName);
    return !given_commit_id.empty() && branchName!=core->getCurrentBranch();
}

void MergeManager::performFastForwardMerge(const std::string& branchName){
    std::string target_commit_id=core->getBranchHead(branchName);
    std::string current_commit_id=commitManager->getCurrentCommitId();

    Commit target_commit=commitManager->getCommit(target_commit_id);
    Commit current_commit=commitManager->getCommit(current_commit_id);

    auto current_blobs=current_commit.getBlobs();
    auto target_blobs=target_commit.getBlobs();

    auto working_files=Utils::plainFilenamesIn(".");
    for(const auto& filename:working_files){
        if(filename.empty()||filename[0]=='.'||filename=="gitlite"||filename=="gitlite.exe"){
            continue;
        }
        if(!current_blobs.count(filename)&&!target_blobs.count(filename)){
            Utils::exitWithMessage("Cannot merge untracked files.");
        }
    }

    for(const auto& current_blob : current_blobs){
        if(target_blobs.count(current_blob.first)){
            remove(current_blob.first.c_str());
        }
    }

    for(const auto& target_blob : target_blobs){
        std::string blob_id=target_blob.second;
        Blob blob=Blob::load(".gitlite/objects",blob_id);
        Utils::writeContents(target_blob.first,blob.getContent());
    }

    core->setBranchHead(core->getCurrentBranch(),target_commit_id);
    core->clearStagingArea();
    core->getStagingArea().save();
}

std::set<std::string> MergeManager::getAllBranches(){
    return branchManager->getAllBranchesList();
}

std::string MergeManager::findSplitPoint(const std::string& branch1,const std::string& branch2){
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

void MergeManager::performThreeWayMerge(const std::string& branchName,const std::string& current_commit_id,const std::string& given_commit_id,const std::string& split_point_id){
    Commit split_commit=commitManager->getCommit(split_point_id);
    Commit current_commit=commitManager->getCommit(current_commit_id);
    Commit given_commit=commitManager->getCommit(given_commit_id);

    auto split_blobs=split_commit.getBlobs();
    auto current_blobs=current_commit.getBlobs();
    auto given_blobs=given_commit.getBlobs();

    auto untracked_files=fileOpManager->getUntrackedFiles();
    for(const auto given_blob:given_blobs){
        const std::string& filename=given_blob.first;
        if(!current_blobs.count(filename)&&untracked_files.count(filename)){
            Utils::exitWithMessage("Cannot merge untracked files.");
        }
    }

    StagingArea& stagingArea=core->getStagingArea();
    const auto& staged_now=stagingArea.getStagingMap();
    const auto& removed_now=stagingArea.getRemovedFiles();
    if(!staged_now.empty()||!removed_now.empty()){
        Utils::exitWithMessage("Cannot merge with unstaged changes.");
    }

    std::map<std::string,std::string> merge_blobs=current_blobs;
    std::set<std::string> all_files;
    for(const auto& blob:split_blobs){
        all_files.insert(blob.first);
    }
    for(const auto& blob:current_blobs){
        all_files.insert(blob.first);
    }
    for(const auto& blob:given_blobs){
        all_files.insert(blob.first);
    }

    bool conflict_occurred=false;
    std::set<std::string> conflict_files;

    for(const auto& filename:all_files){
        std::string split_blob_id=split_blobs.count(filename)?split_blobs.at(filename):"";
        std::string current_blob_id=current_blobs.count(filename)?current_blobs.at(filename):"";
        std::string given_blob_id=given_blobs.count(filename)?given_blobs.at(filename):"";

        if(split_blob_id==current_blob_id&&split_blob_id==given_blob_id){
            continue;
        }

        if(split_blob_id==current_blob_id&&split_blob_id!=given_blob_id){
            if(!given_blob_id.empty()){
                merge_blobs[filename]=given_blob_id;
                commitManager->copyFileFromCommit(filename,given_commit_id);
            }
            else{
                merge_blobs.erase(filename);
                if(Utils::exists(filename)){
                    remove(filename.c_str());
                }
            }
            continue;
        }

        if(split_blob_id!=current_blob_id&&split_blob_id==given_blob_id){
            continue;
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
            if(merge_blobs.count(current_blob.first)){
                if(Utils::exists(current_blob.first)){
                    remove(current_blob.first.c_str());
                }
            }
        }
        core->clearStagingArea();
        stagingArea.save();
    }
}