#include"../include/CommitManager.h"
#include"../include/RepositoryCore.h"
#include"../include/Utils.h"
#include"../include/Blob.h"
#include"../include/GitliteException.h"
#include<algorithm>
#include<fstream>
#include<iostream>
#include<time.h>

CommitManager::CommitManager(RepositoryCore* repoCore):core(repoCore){}

void CommitManager::commit(const std::string& message){
    if(message.empty()){
        Utils::exitWithMessage("Please enter a commit message.");
    }

    StagingArea& stagingArea=core->getStagingArea();
    stagingArea.reload();

    auto stagingMap=stagingArea.getStagingMap();
    auto removedFiles=stagingArea.getRemovedFiles();
    if(stagingMap.empty()&&removedFiles.empty()){
        Utils::exitWithMessage("No changes added to the commit.");
    }

    std::vector<std::string> parents;
    std::string currentCommitId=getCurrentCommitId();
    if(!currentCommitId.empty()){
        parents.push_back(currentCommitId);
    }

    std::map<std::string,std::string> newBlobs;
    if(!currentCommitId.empty()){
        newBlobs=getCommit(currentCommitId).getBlobs();
    }

    for(const auto& entry:stagingMap){
        newBlobs[entry.first]=entry.second;
    }

    for(const auto& filename:removedFiles){
        newBlobs.erase(filename);
    }

    std::time_t now=std::time(nullptr);
    Commit newCommit(message,now,parents,newBlobs);
    saveCommit(newCommit);

    core->setBranchHead(core->getCurrentBranch(),newCommit.getId());

    stagingArea.clear();
    stagingArea.save();

    for(const auto& filename:removedFiles){
        if(Utils::exists(filename)){
            try{
                remove(filename.c_str());
            }catch(...){
                Utils::exitWithMessage("Failed to remove file: "+filename);
            }
        }
    }
}

void CommitManager::saveCommit(const Commit& commit){
    std::string commit_path=Utils::join(".gitlite/objects",commit.getId());
    Utils::writeContents(commit_path,commit.serialize());
}

Commit CommitManager::getCommit(const std::string& id) const {
    std::string full_id=getFullCommitId(id);
    std::string commit_path=Utils::join(".gitlite/objects",full_id);
    if(full_id.empty()){
        throw GitliteException("Commit not found: "+id);
    }

    std::string commit_file=Utils::join(".gitlite/objects",full_id);
    return Commit::fromFile(commit_file);
}   
    
void CommitManager::log(){
    std::string current_commit_id=getCurrentCommitId();
    bool first_commit=true;

    while(!current_commit_id.empty()){
        if(!first_commit){
            std::cout<<std::endl;
        }
        first_commit=false;

        Commit commit=getCommit(current_commit_id);
        
        std::cout<<"===\n";
        std::cout<<"commit "<<commit.getId()<<"\n";

        if(commit.isMergeCommit()){
            auto parents=commit.getParents();
            std::cout<<"Merge: "
            <<parents[0].substr(0,7)<<" "
            <<parents[1].substr(0,7)<<"\n";
        }

        std::cout<<"Date: "<<commit.getFormattedTimestamp()<<"\n";
        std::cout<<commit.getMessage()<<"\n";   

        auto parents=commit.getParents();
        current_commit_id=parents.empty()?"":parents[0];
    }
}

void CommitManager::globalLog(){
    auto all_commits=Utils::plainFilenamesIn(".gitlite/objects");
    bool first_commit=true;

    for(const auto& commit_id:all_commits){
        if(commit_id.length()==Utils::UID_LENGTH){
            try{
                if(!first_commit){
                    std::cout<<std::endl;
                }
                first_commit=false;

                Commit commit=getCommit(commit_id);
                std::cout<<"===\n";
                std::cout<<"commit "<<commit.getId()<<"\n";

                if(commit.isMergeCommit()){
                    auto parents=commit.getParents();
                    std::cout<<"Merge: "
                    <<parents[0].substr(0,7)<<" "
                    <<parents[1].substr(0,7)<<"\n";
                }

                std::cout<<"Date: "<<commit.getFormattedTimestamp()<<"\n";
                std::cout<<commit.getMessage()<<"\n";
            }catch(...){
                continue;
            }
        }
    }
}

void CommitManager::find(const std::string& commitMessage){
    auto all_commits=Utils::plainFilenamesIn(".gitlite/objects");
    bool found=false;

    for(const auto& commit_id:all_commits){
        if(commit_id.length()==Utils::UID_LENGTH){
            try{
                Commit commit=getCommit(commit_id);
                if(commit.getMessage()==commitMessage){
                    found=true;
                    std::cout<<commit.getId()<<"\n";
                }
            }catch(...){
                continue;
            }
        }
    }

    if(!found){
        Utils::exitWithMessage("Commit not found: "+commitMessage);
    }
}

std::string CommitManager::getFileBlobId(const std::string& filename,const std::string& commitId){
    if(commitId.empty()){
        return "";
    }

    Commit commit=getCommit(commitId);
    auto blobs=commit.getBlobs();
    if(blobs.count(filename)){
        // return blobs.at(filename)
        return blobs[filename];
    }
    return "";
}

bool CommitManager::fileExistsInCommit(const std::string& filename,const std::string& commitId){
    return !getFileBlobId(filename,commitId).empty();
}

std::string CommitManager::getCurrentCommitId() const{
    return core->getBranchHead(core->getCurrentBranch());
}

void CommitManager::copyFileFromCommit(const std::string& filename,const std::string& commitId){
    std::string blob_id=getFileBlobId(filename,commitId);
    if(blob_id.empty()){
        // Utils::exitWithMessage("File not found: "+filename);
        return ;
    }

    Blob blob=Blob::load(".gitlite/objects",blob_id);
    Utils::writeContents(filename,blob.getContent());
}
