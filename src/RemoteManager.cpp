#include "../include/RemoteManager.h"
#include "../include/RepositoryCore.h"
#include "../include/Utils.h"
#include "../include/Commit.h"
#include "../include/Blob.h"
#include <sstream>
#include <iostream>

RemoteManager::RemoteManager(RepositoryCore* repoCore) : core(repoCore) {}

void RemoteManager::addRemote(const std::string& remoteName,const std::string& remotePath){
    auto remotes=getRemotes();
    if(remotes.count(remoteName)){
        Utils::exitWithMessage("A remote with that name already exists.");
    }

    remotes[remoteName]=remotePath;
    saveRemotes(remotes);
}

void RemoteManager::rmRemote(const std::string& remoteName){
    auto remotes=getRemotes();
    if(remotes.find(remoteName)==remotes.end()){
        Utils::exitWithMessage("A remote with that name does not exist.");
    }

    remotes.erase(remoteName);
    saveRemotes(remotes);
}

void RemoteManager::push(const std::string& remoteName,const std::string& remoteBranchName){
    auto remotes=getRemotes();
    if(remotes.find(remoteName)==remotes.end()){
        Utils::exitWithMessage("A remote with that name does not exist.");
    }

    std::string remote_path=remotes[remoteName];
    if(!Utils::isDirectory(remote_path)){
        Utils::exitWithMessage("Remote directory not found.");
    }

    std::string remote_gitlite_dir=getRemoteGitliteDir(remote_path);
    if(!Utils::isDirectory(remote_gitlite_dir)){
        Utils::exitWithMessage("Remote is not a Gitlite repository.");
    }

    std::string local_branch_head=core->getBranchHead(core->getCurrentBranch());
    std::string remote_branch_file=Utils::join(remote_gitlite_dir,"branches",remoteBranchName);

    std::string remote_branch_head;
    if(Utils::exists(remote_branch_file)){
        remote_branch_head=Utils::readContentsAsString(remote_branch_file);
        while(!remote_branch_head.empty()&&(remote_branch_head.back()=='\r'||remote_branch_head.back()=='\n'||isspace((unsigned char)remote_branch_head.back()))) remote_branch_head.pop_back();
        while(!remote_branch_head.empty()&&(remote_branch_head.front()=='\r'||remote_branch_head.front()=='\n'||isspace((unsigned char)remote_branch_head.front()))) remote_branch_head.erase(remote_branch_head.begin());
    }

    bool found_in_history=false;
    std::string current_commit=local_branch_head;
    while(!current_commit.empty()){
        if(current_commit==remote_branch_head){
            found_in_history=true;
            break;
        }
        std::string commit_file=Utils::join(".gitlite/objects",current_commit);
        Commit commit=Commit::fromFile(commit_file);
        auto parents=commit.getParents();
        current_commit=parents.empty()?"":parents[0];
    }

    if(!remote_branch_head.empty()&&!found_in_history){
        Utils::exitWithMessage("Remote branch is ahead of local branch.");
    }

    std::vector<std::string> commits_to_copy;
    current_commit=local_branch_head;
    while(!current_commit.empty()&&current_commit!=remote_branch_head){
        commits_to_copy.push_back(current_commit);
        std::string commit_file=Utils::join(".gitlite/objects",current_commit);
        Commit commit=Commit::fromFile(commit_file);
        auto parents=commit.getParents();
        current_commit=parents.empty()?"":parents[0];
    }

    for(auto it=commits_to_copy.rbegin();it!=commits_to_copy.rend();it++){
        std::string commit_id=*it;
        std::string local_commit_path=Utils::join(".gitlite/objects",commit_id);
        std::string remote_commit_path=Utils::join(remote_gitlite_dir,"objects",commit_id);
        
        if(!Utils::exists(remote_commit_path)){
            std::string commit_file=Utils::join(".gitlite/objects",commit_id);
            Commit commit=Commit::fromFile(commit_file);
            Utils::writeContents(remote_commit_path,commit.serialize());

            for(const auto& blob:commit.getBlobs()){
                std::string blob_id=blob.second;
                std::string local_blob_path=Utils::join(".gitlite/objects",blob_id);
                std::string remote_blob_path=Utils::join(remote_gitlite_dir,"objects",blob_id);

                if(!Utils::exists(remote_blob_path)){
                    std::string blob_content=Utils::readContentsAsString(local_blob_path);
                    Utils::writeContents(remote_blob_path,blob_content);
                }
            }
        }
    }

    Utils::writeContents(remote_branch_file,local_branch_head);
}

void RemoteManager::fetch(const std::string& remoteName,const std::string& remoteBranchName){
    auto remotes=getRemotes();
    if(remotes.find(remoteName)==remotes.end()){
        Utils::exitWithMessage("A remote with that name does not exist.");
    }

    std::string remote_path=remotes[remoteName];

    if(!Utils::isDirectory(remote_path)){
        Utils::exitWithMessage("Remote directory not found.");
    }

    std::string remote_gitlite_dir=getRemoteGitliteDir(remote_path);
    if(!Utils::isDirectory(remote_gitlite_dir)){
        Utils::exitWithMessage("Remote is not a Gitlite repository.");
    }

    std::string remote_branch_file=Utils::join(remote_gitlite_dir,"branches",remoteBranchName);
    if(!Utils::exists(remote_branch_file)){
        Utils::exitWithMessage("That remote does not have that branch.");
    }

    std::string remote_branch_head=Utils::readContentsAsString(remote_branch_file);
    while(!remote_branch_head.empty()&&(remote_branch_head.back()=='\r'||remote_branch_head.back()=='\n'||isspace((unsigned char)remote_branch_head.back()))) remote_branch_head.pop_back();
    while(!remote_branch_head.empty()&&(remote_branch_head.front()=='\r'||remote_branch_head.front()=='\n'||isspace((unsigned char)remote_branch_head.front()))) remote_branch_head.erase(remote_branch_head.begin());
    std::string local_tracking_branch=remoteName+"/"+remoteBranchName;

    std::set<std::string> copied_commits;
    std::vector<std::string> commits_to_copy;
    std::string current_commit=remote_branch_head;

    while(!current_commit.empty()&&copied_commits.find(current_commit)==copied_commits.end()){
        std::string local_commit_path=Utils::join(".gitlite/objects",current_commit);
        
        if(Utils::exists(local_commit_path)){
            copied_commits.insert(current_commit);
            std::string commit_file=Utils::join(".gitlite/objects",current_commit);
            Commit commit=Commit::fromFile(commit_file);
            bool has_new_parent=false;
            auto parents=commit.getParents();
            for(const auto& parent : parents){
                if(copied_commits.find(parent)==copied_commits.end()){
                    current_commit=parent;
                    has_new_parent=true;
                    break;
                }
            }

            if(!has_new_parent){
                break;
            }
        }
        else{
            commits_to_copy.push_back(current_commit);
            copied_commits.insert(current_commit);
            std::string remote_commit_path=Utils::join(remote_gitlite_dir,"objects",current_commit);
            if(Utils::exists(remote_commit_path)){
                Commit remote_commit=Commit::fromFile(remote_commit_path);
                bool has_new_parent=false;
                auto parents=remote_commit.getParents();
                for(const auto& parent : parents){
                    if(copied_commits.find(parent)==copied_commits.end()){
                        current_commit=parent;
                        has_new_parent=true;
                        break;
                    }
                }
                if(!has_new_parent){
                    break;
                }
            }
            else{
                Utils::exitWithMessage("Remote commit not found.");
            } 
        }
        
        for(auto it=commits_to_copy.rbegin();it!=commits_to_copy.rend();it++){
            std::string commit_id=*it;
            std::string local_commit_path=Utils::join(".gitlite/objects",commit_id);
            std::string remote_commit_path=Utils::join(remote_gitlite_dir,"objects",commit_id);

            if(!Utils::exists(local_commit_path)){
                Commit remote_commit=Commit::fromFile(remote_commit_path);
                Utils::writeContents(local_commit_path,remote_commit.serialize());

                auto blobs=remote_commit.getBlobs();
                for(const auto& blob : blobs){
                    std::string blob_id=blob.second;
                    std::string local_blob_path=Utils::join(".gitlite/objects",blob_id);
                    std::string remote_blob_path=Utils::join(remote_gitlite_dir,"objects",blob_id);

                    if(!Utils::exists(local_blob_path)){
                        std::string blob_content=Utils::readContentsAsString(remote_blob_path);
                        Utils::writeContents(local_blob_path,blob_content);
                    }
                }
            }
        }
    }

    core->setBranchHead(local_tracking_branch,remote_branch_head);
}

void RemoteManager::pull(const std::string& remoteName,const std::string& remoteBranchName){
    fetch(remoteName,remoteBranchName);
}

std::map<std::string,std::string> RemoteManager::getRemotes(){
    std::map<std::string,std::string> remotes;
    if(!Utils::exists(".gitlite/remotes")){
        return remotes;
    }
    
    std::string content=Utils::readContentsAsString(".gitlite/remotes");
    std::istringstream iss(content);
    std::string line;

    while(std::getline(iss,line)){
        size_t pos=line.find(' ');
        if(pos==std::string::npos){
            continue;
        }
        std::string name=line.substr(0,pos);
        std::string path=line.substr(pos+1);
        remotes[name]=path;
    }

    return remotes;
}

void RemoteManager::saveRemotes(const std::map<std::string,std::string>& remotes){
    std::ostringstream oss;
    for(const auto& remote : remotes){
        oss<<remote.first<<" "<<remote.second<<std::endl;
    }

    Utils::writeContents(".gitlite/remotes",oss.str());
}

bool RemoteManager::validateRemoteRepository(const std::string& remotePath){
    std::string remote_gitlite_dir=getRemoteGitliteDir(remotePath);
    if(!Utils::isDirectory(remote_gitlite_dir)){
        return false;
    }
    return true;
}

std::string RemoteManager::getRemoteGitliteDir(const std::string& remotePath){
    std::string remote_gitlite_dir=remotePath;
    if(remotePath.length()<8||remotePath.substr(remotePath.length()-8)!=".gitlite"){
        remote_gitlite_dir=Utils::join(remotePath,".gitlite");
    }
    return remote_gitlite_dir;
}