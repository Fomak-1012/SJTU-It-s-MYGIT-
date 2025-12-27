#include"../include/FileOperationManager.h"
#include"../include/RepositoryCore.h"
#include"../include/CommitManager.h"
#include"../include/Utils.h"
#include"../include/Blob.h"
#include<algorithm>
#include<sstream>

FileOperationManager::FileOperationManager(RepositoryCore* repoCore, CommitManager* commitMgr) 
    : core(repoCore), commitManager(commitMgr) {}

void FileOperationManager::add(const std::string& filename){
    StagingArea& stagingArea=core->getStagingArea(); 

    // 如果文件之前被标记为删除，现在要重新添加，则先移除删除标记
    if(stagingArea.isRemoved(filename)){
        stagingArea.removeRemovedFile(filename);
        stagingArea.save();
        if(!Utils::exists(filename)){
            return;
        }
    }   

    if(!Utils::exists(filename)){
        Utils::exitWithMessage("File does not exist."); 
    }

    // 获取当前commit中文件的blob ID
    std::string current_commit_id=commitManager->getCurrentCommitId();
    std::string current_blob_id=commitManager->getFileBlobId(filename,current_commit_id);

    // 当前文件的blob ID
    std::string file_content=Utils::readContentsAsString(filename);
    std::string new_blob_id=Blob::generateId(file_content);

    auto conflict_files=getConflictFiles();
    bool is_conflict_file=conflict_files.count(filename)>0;

    // 如果文件内容没有变化
    if(current_blob_id==new_blob_id){
        if(!is_conflict_file){
            // 如果不是冲突文件且已在暂存区，则移除
            if(stagingArea.isStaged(filename)){
                stagingArea.removeStagedFile(filename);
                stagingArea.save();
            }
            return;
        }
    }

    stagingArea.addStagedFile(filename,new_blob_id);

    if(is_conflict_file){
        conflict_files.erase(filename);
        saveConflictFiles(conflict_files);
    }

    stagingArea.save();
    Blob::create(".gitlite/objects",file_content);
}

void FileOperationManager::rm(const std::string& filename){
    StagingArea& staging_area=core->getStagingArea();
    const auto& staging_map=staging_area.getStagingMap();
    std::string current_commit_id=commitManager->getCurrentCommitId();
    bool file_tracked=commitManager->fileExistsInCommit(filename,current_commit_id);  // 文件是否被跟踪
    bool file_staged=staging_area.isStaged(filename);                                 // 文件是否在暂存区

    if(!file_tracked&&!file_staged){
        Utils::exitWithMessage("No reason to remove the file.");
    }

    // 如果文件在暂存区，直接移除暂存记录
    if(file_staged){
        staging_area.removeStagedFile(filename);
        staging_area.save();
        return;
    }

    // 如果文件被跟踪，添加删除标记并删除工作目录文件
    if(file_tracked){
        staging_area.addRemovedFile(filename); 
        staging_area.save();
        if(Utils::exists(filename)){
            remove(filename.c_str());          
        }
        return;
    }
}


void FileOperationManager::checkoutFile(const std::string& filename){
    std::string current_commit_id=commitManager->getCurrentCommitId();
    checkoutFileInCommit(current_commit_id,filename);
}

void FileOperationManager::checkoutFileInCommit(const std::string& commit_id, const std::string& filename){
    std::string full_commit_id=commitManager->getFullCommitId(commit_id);
    if(full_commit_id.empty()){
        Utils::exitWithMessage("No commit with that id exists.");
    }

    if(!commitManager->fileExistsInCommit(filename,full_commit_id)){
        Utils::exitWithMessage("File does not exist in that commit.");
    }       

    commitManager->copyFileFromCommit(filename,full_commit_id);
}

bool FileOperationManager::isFileModified(const std::string& filename,const std::string& commit_id){
    if(!Utils::exists(filename)||!commitManager->fileExistsInCommit(filename,commit_id)){
        return false;  // 文件不存在或未被跟踪，不算修改
    }

    // 比较当前文件和commit中文件的blob ID
    std::string commit_blob_id=commitManager->getFileBlobId(filename,commit_id);
    std::string current_content=Utils::readContentsAsString(filename);
    std::string current_blob_id=Blob::generateId(current_content);

    return commit_blob_id!=current_blob_id;
}

std::set<std::string> FileOperationManager::getUntrackedFiles() {
    std::set<std::string> untracked_files;
    std::string current_commit_id=commitManager->getCurrentCommitId();
    auto tracked_files=commitManager->getTrackedFiles(current_commit_id);
    StagingArea& staging_area=core->getStagingArea();
    const auto& staging_map=staging_area.getStagingMap();                 

    auto working_files=Utils::plainFilenamesIn(".");
    for(const auto& file:working_files){
        if(!tracked_files.count(file)&&!staging_map.count(file)){
            untracked_files.insert(file);
        }
    }

    return untracked_files;
}

std::map<std::string,std::string> FileOperationManager::getModifiedFiles(){
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
            if(isFileModified(filename,current_commit_id)){
                modified_files[filename]="modified";
            }
        }
        else{
            modified_files[filename]="deleted";
        }  
    }
    
    return modified_files;
}

std::set<std::string> FileOperationManager::getConflictFiles(){
    std::set<std::string> conflict_files;
    std::string conflict_file=".gitlite/conflict";
    if(!Utils::exists(conflict_file)){
        return conflict_files;
    }

    std::string conflict_content=Utils::readContentsAsString(conflict_file);
    std::istringstream iss(conflict_content);
    std::string filename;
    while(std::getline(iss,filename)){
        conflict_files.insert(filename);
    }   
    return conflict_files;
}

void FileOperationManager::saveConflictFiles(const std::set<std::string>& conflict_files){
    std::string conflict_file=".gitlite/conflict";
    std::ostringstream oss;
    for(const auto& file:conflict_files){
        oss<<file<<"\n";
    }

    Utils::writeContents(conflict_file,oss.str());
}

void FileOperationManager::clearConflictFiles(){
    std::string conflict_file=".gitlite/conflict";
    if(Utils::exists(conflict_file)){
        remove(conflict_file.c_str());
    }
}

void FileOperationManager::clearStagingArea(){
    core->clearStagingArea();
}