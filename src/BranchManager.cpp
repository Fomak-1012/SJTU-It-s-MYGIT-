#include"../include/BranchManager.h"
#include"../include/RepositoryCore.h"
#include"../include/Utils.h"
#include"../include/CommitManager.h"
#include"../include/Blob.h"
#include<iostream>
#include<queue>

BranchManager::BranchManager(RepositoryCore* repoCore) : core(repoCore) {}

void BranchManager::branch(const std::string& branchName){
    std::string branch_head=core->getBranchHead(branchName);
    if(!branch_head.empty()){
        Utils::exitWithMessage("A branch with that name already exists.");
    }

    std::string current_commit_id=core->getBranchHead(core->getCurrentBranch());
    core->setBranchHead(branchName,current_commit_id);
}

void BranchManager::rmBranch(const std::string& branchName){
    std::string branch_head=core->getBranchHead(branchName);
    if(branch_head.empty()){
        Utils::exitWithMessage("A branch with that name does not exist.");
    }

    std::string current_branch=core->getCurrentBranch();
    if(branchName==current_branch){
        Utils::exitWithMessage("Cannot remove the current branch.");
    }

    std::string branch_file=Utils::join(".gitlite/branches",branchName);
    std::remove(branch_file.c_str());
}

void BranchManager::checkoutBranch(const std::string& branchName){
    std::string branch_head=core->getBranchHead(branchName);
    if(branch_head.empty()){
        Utils::exitWithMessage("No such branch exists.");
    }

    std::string current_branch=core->getCurrentBranch();
    if(branchName==current_branch){
        Utils::exitWithMessage("No need to checkout the current branch.");
    }

    performBranchCheckout(branchName);
}

std::string BranchManager::getCurrentBranch(){
    return core->getCurrentBranch();
}

std::set<std::string> BranchManager::getAllBranchesList(){
    return getAllBranches();
}

std::set<std::string> BranchManager::getAllBranches(){
    auto branch_files=Utils::plainFilenamesIn(".gitlite/branches");
    return std::set<std::string>(branch_files.begin(),branch_files.end());
}

std::string BranchManager::findSplitPoint(const std::string& branch1,const std::string& branch2){
    std::set<std::string> ancestor1;
    std::vector<std::string> stack;
    
    if(!branch1.empty())stack.push_back(branch1);

    while(!stack.empty()){
        std::string commit_id=stack.back();
        stack.pop_back();
        if(commit_id.empty()||ancestor1.count(commit_id))continue;
        ancestor1.insert(commit_id);

        std::string commit_file=Utils::join(".gitlite/commits",commit_id);
        Commit commit=Commit::fromFile(commit_file);
        for(const auto& parent_id:commit.getParents()){
            if(!parent_id.empty()&&!ancestor1.count(parent_id))stack.push_back(parent_id);
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
        if(ancestor1.count(commit_id))return commit_id;

        std::string commit_file=Utils::join(".gitlite/commits",commit_id);
        Commit commit=Commit::fromFile(commit_file);
        for(const auto& parent_id:commit.getParents()){
            if(!parent_id.empty()&&!visited.count(parent_id)){
                q.push(parent_id);
                visited.insert(parent_id);
            }
        }
    }   

    return "";
}

void BranchManager::performBranchCheckout(const std::string& branchName){
    std::string current_branch=core->getCurrentBranch();
    std::string current_commit_id=core->getBranchHead(current_branch);
    std::string target_commit_id=core->getBranchHead(branchName);

    std::string current_commit_file=Utils::join(".gitlite/objects",current_commit_id);
    std::string target_commit_file=Utils::join(".gitlite/objects",target_commit_id);

    Commit current_commit=Commit::fromFile(current_commit_file);
    Commit target_commit=Commit::fromFile(target_commit_file);
    
    auto current_blobs=current_commit.getBlobs();
    auto target_blobs=target_commit.getBlobs();

    auto working_files=Utils::plainFilenamesIn(".");
    for(const auto& filename : working_files){
        if(filename.empty()||filename[0]=='.'||filename=="gitlite"||filename=="gitlite.exe"){
            continue;
        }
        if(!current_blobs.count(filename)&&target_blobs.count(filename)){
            Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
        }
    }

    for(const auto& current_blob : current_blobs){
        if(!target_blobs.count(current_blob.first)){
            remove(current_blob.first.c_str());
        }
    }

    for(const auto& target_blob : target_blobs){
        std::string blob_id=target_blob.second;
        Blob blob=Blob::load(".gilite/objects",blob_id);
        Utils::writeContents(target_blob.first,blob.getContent());
    }

    core->clearStagingArea();
    core->getStagingArea().save();
    core->setCurrentBranch(branchName);
}