#include "../include/StatusManager.h"
#include "../include/RepositoryCore.h"
#include "../include/CommitManager.h"
#include "../include/FileOperationManager.h"
#include "../include/Utils.h"
#include "../include/Blob.h"
#include <iostream>

StatusManager::StatusManager(RepositoryCore* repoCore,CommitManager* commitMgr,FileOperationManager* fileOpMgr)
    : core(repoCore),commitManager(commitMgr),fileOpManager(fileOpMgr) {}

void StatusManager::status() {
    std::set<std::string> branches=getAllBranches();
    std::string currentBranch=core->getCurrentBranch();
    auto modified=getModifiedFiles();
    auto untracked=getUntrackedFiles();

    printBranches(branches, currentBranch);
    printStagedFiles();
    printRemovedFiles();
    printModifiedFiles(modified);
    printUntrackedFiles(untracked);
}

std::set<std::string> StatusManager::getAllBranches(){
    auto branch_files=Utils::plainFilenamesIn(".gitlite/branches");
    return std::set<std::string>(branch_files.begin(),branch_files.end());
}

std::map<std::string,std::string> StatusManager::getModifiedFiles(){
    std::map<std::string,std::string> modified_files;
    std::string current_commit_id=commitManager->getCurrentCommitId();
    auto tracked_files=commitManager->getTrackedFiles(current_commit_id);
    StagingArea& staging_area=core->getStagingArea();
    const auto& staging_map=staging_area.getStagingMap();
    const auto& removed_files=staging_area.getRemovedFiles();

    for(const auto& tracked:tracked_files){
        std::string filename=tracked.first;

        if(removed_files.count(filename)){
            continue;
        }

        if(staging_map.count(filename)){
            continue;
        }

        if(Utils::exists(filename)){
            std::string commit_blob_id=commitManager->getFileBlobId(filename,current_commit_id);
            std::string current_content=Utils::readContentsAsString(filename);
            std::string current_blob_id=Blob::generateId(current_content);

            if(commit_blob_id!=current_blob_id){
                modified_files[filename]="modified";
            }
        }
        else{
            modified_files[filename]="deleted";
        }  
    }
    
    return modified_files;
}

std::set<std::string> StatusManager::getUntrackedFiles(){
    std::set<std::string> untracked_files;
    std::string current_commit_id=commitManager->getCurrentCommitId();
    auto tracked_files=commitManager->getTrackedFiles(current_commit_id);
    StagingArea& staging_area=core->getStagingArea();
    const auto& staging_map=staging_area.getStagingMap();

    auto working_files=Utils::plainFilenamesIn(".");
    for(const auto& file:working_files){
        if(file.empty()||file[0]=='.'||file=="gitlite"||file=="gitlite.exe"){
            continue;
        }

        if(tracked_files.count(file)&&staging_map.count(file)){
            untracked_files.insert(file);
        }
    }

    return untracked_files;
}

void StatusManager::printBranches(const std::set<std::string>& branches,const std::string& currentBranch){
    std::cout<<"=== Branches ===\n";
    for(const auto& branch:branches){
        if(branch==currentBranch){
            std::cout<<"*"<<branch<<std::endl;
        }   
        else{
            std::cout<<branch<<std::endl;
        }
    }
}

void StatusManager::printStagedFiles(){
    StagingArea& staging_area=core->getStagingArea();
    const auto& staging_map=staging_area.getStagingMap();

    std::cout<<"\n=== Staged Files ===\n";
    for(const auto& staged:staging_map){
        std::cout<<staged.first<<std::endl;
    }
}

void StatusManager::printRemovedFiles(){
    StagingArea& staging_area=core->getStagingArea();
    const auto& removed_files=staging_area.getRemovedFiles();

    std::cout<<"\n=== Removed Files ===\n";
    for(const auto& file:removed_files){
        std::string name=file;

        while (!name.empty() && (name.back() == '\r' || name.back() == '\n')) name.pop_back();
        while (!name.empty() && (name.front() == '\r' || name.front() == '\n')) name.erase(name.begin());

        bool allblank=true;
        for(const auto& c:name){
            if(c!=0&&c!=0xA0&&!isspace(c)){
                allblank=false;
                break;
            }
        }
        if(allblank)continue;

        while (!name.empty() && isspace((unsigned char)name.back())) name.pop_back();
        while (!name.empty() && isspace((unsigned char)name.front())) name.erase(name.begin());
        if (name.empty()) continue;

        std::cout<<name<<std::endl;
    }
}

void StatusManager::printModifiedFiles(const std::map<std::string,std::string>& modified_files){
    std::cout<<"\n=== Modified Files ===\n";
    for(const auto& modified:modified_files){
        std::cout<<modified.first<<" ("<<modified.second<<")\n";
    }
}

void StatusManager::printUntrackedFiles(const std::set<std::string>& untracked_files){
    std::cout<<"\n=== Untracked Files ===\n";
    for(const auto& file:untracked_files){
        std::cout<<file<<std::endl;
    }
}   