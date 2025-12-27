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

    // 从新commit中移除被标记删除的文件
    for(const auto& filename: removedFiles){
        newBlobs.erase(filename);
    }

    // 创建并保存新的commit对象
    std::time_t now = std::time(nullptr);
    Commit newCommit(message, now, parents, newBlobs);
    saveCommit(newCommit);

    // 更新当前分支指向新的commit
    std::string current_branch = core->getCurrentBranch();
    core->setBranchHead(current_branch, newCommit.getId());

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

Commit CommitManager::getCommit(const std::string& id){
    std::string full_id=getFullCommitId(id); 
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

        // 如果是merge commit，显示父commit信息
        if(commit.isMergeCommit()){
            auto parents=commit.getParents();
            std::cout<<"Merge: "
            <<parents[0].substr(0,7)<<" " 
            <<parents[1].substr(0,7)<<"\n";
        }

        std::cout<<"Date: "<<commit.getFormattedTimestamp()<<"\n"; 
        std::cout<<commit.getMessage()<<"\n";                        

        // 移动到父commit
        auto parents=commit.getParents();
        current_commit_id=parents.empty()?"":parents[0];
    }
}

void CommitManager::globalLog(){
    auto all_commits=Utils::plainFilenamesIn(".gitlite/objects");
    bool first_commit=true;

    // 遍历所有对象，筛选出commit文件
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
        Utils::exitWithMessage("Found no commit with that message.");
    }
}

std::string CommitManager::getFileBlobId(const std::string& filename,const std::string& commitId){
    if(commitId.empty()){
        return "";  
    }

    Commit commit=getCommit(commitId);
    auto blobs=commit.getBlobs();     
    if(blobs.count(filename)){        
        return blobs[filename];       
    }
    return ""; 
}
bool CommitManager::fileExistsInCommit(const std::string& filename,const std::string& commitId){
    return !getFileBlobId(filename,commitId).empty();  // blob ID非空即为存在
}

std::string CommitManager::getCurrentCommitId(){
    return core->getBranchHead(core->getCurrentBranch());
}

void CommitManager::copyFileFromCommit(const std::string& filename,const std::string& commitId){
    std::string blob_id=getFileBlobId(filename,commitId);
    if(blob_id.empty()){
        return ;
    }

    Blob blob=Blob::load(".gitlite/objects",blob_id);     
    Utils::writeContents(filename,blob.getContent());     
}

std::map<std::string,std::string> CommitManager::getTrackedFiles(const std::string& commitId){
    if(commitId.empty()){
        return {};
    }

    Commit commit=getCommit(commitId);
    return commit.getBlobs();           
}

Commit CommitManager::getHeadCommit(){
    std::string current_commit_id=getCurrentCommitId();
    return getCommit(current_commit_id);                
}

std::vector<std::string> CommitManager::getFiles(const std::string& commitId){
    std::vector<std::string> files;
    Commit commit=getCommit(commitId);     
    auto blobs=commit.getBlobs();           

    for(const auto& blob:blobs){
        files.push_back(blob.first);        
    }

    return files;
}

// 找缩写
std::string CommitManager::getFullCommitId(const std::string& id){
    if(id.empty())return "";
    auto all_commits=Utils::plainFilenamesIn(".gitlite/objects"); 
    std::string match;
    for(const auto& commit_id:all_commits){
        if(commit_id.length()==Utils::UID_LENGTH&&commit_id.find(id)==0){
            if(!match.empty()){
                Utils::exitWithMessage("Ambiguous commit id: "+id); 
            }
            match=commit_id;
        }
    }

    return match;  
}

void CommitManager::reset(const std::string& commitId){
    std::string full_commit_id=getFullCommitId(commitId);
    if(full_commit_id.empty()){
        Utils::exitWithMessage("No commit with that id exists.");
    }

    std::string current_commit_id=getCurrentCommitId();  
    auto current_blobs=getTrackedFiles(current_commit_id); 
    auto target_blobs=getTrackedFiles(full_commit_id);     

    auto working_files=Utils::plainFilenamesIn(".");
    for(const auto& filename:working_files){
        if(!current_blobs.count(filename)&&target_blobs.count(filename))
            Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
    }

    // 删除当前commit有但目标commit没有的文件
    for(const auto& current_blob:current_blobs){
        if(!target_blobs.count(current_blob.first)){
            remove(current_blob.first.c_str());
        }
    }

    // 添加或更新目标commit中的文件
    for(const auto& target_blob:target_blobs){
        copyFileFromCommit(target_blob.first,full_commit_id);
    }

    std::string current_branch=core->getCurrentBranch();
    core->setBranchHead(current_branch,full_commit_id);  // 将分支指针指向目标commit
    core->clearStagingArea();                           
    core->getStagingArea().save();                       
}