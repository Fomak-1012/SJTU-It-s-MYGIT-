#include"../include/RepositoryCore.h"
#include"../include/Utils.h"
#include"../include/GitliteException.h"
#include"../include/Commit.h"
#include <cctype>

const std::string RepositoryCore::gitlite_dir=".gitlite";
const std::string RepositoryCore::objects_dir=".gitlite/objects";
const std::string RepositoryCore::branches_dir=".gitlite/branches";
const std::string RepositoryCore::staging_area_file=".gitlite/staging";
const std::string RepositoryCore::removed_file=".gitlite/removed";
const std::string RepositoryCore::head_file=".gitlite/HEAD";
const std::string RepositoryCore::remotes_file=".gitlite/remotes";

RepositoryCore::RepositoryCore() : stagingArea(staging_area_file,removed_file){}

bool RepositoryCore::isInitialized(){
    return Utils::isDirectory(gitlite_dir);
}

std::string RepositoryCore::getGitliteDir(){
    return gitlite_dir;
}

void RepositoryCore::init(){
    if(isInitialized()){Utils::exitWithMessage("there's already a gitlite");}

    Utils::createDirectories(gitlite_dir);
    Utils::createDirectories(objects_dir);
    Utils::createDirectories(branches_dir);

    std::map<std::string,std::string> empty_blobs;
    std::vector<std::string> empty_parents;
    std::time_t epoch=0;
    Commit initial_commit("initial commit",epoch,empty_parents,empty_blobs);

    std::string commit_file=Utils::join(objects_dir,initial_commit.getId());
    Utils::writeContents(commit_file,initial_commit.serialize());

    setBranchHead("master",initial_commit.getId());
    setCurrentBranch("master");

    clearStagingArea();
}

std::string RepositoryCore::getCurrentBranch(){
    if(!Utils::exists(head_file)){return "";}
    return Utils::readContentsAsString(head_file);
}

// 设置当前分支
void RepositoryCore::setCurrentBranch(const std::string& branch){
    if(branch.empty()){Utils::exitWithMessage("branch name is empty");}
    Utils::writeContents(head_file,branch);  // 写入HEAD文件
}

// 获取分支指向的commit ID
std::string RepositoryCore::getBranchHead(const std::string& branch){
    if(branch.empty()){Utils::exitWithMessage("branch name is empty");}
    if(!Utils::exists(Utils::join(branches_dir,branch))){return "";} 
    
    std::string id=Utils::readContentsAsString(Utils::join(branches_dir,branch));  // 读取分支文件
    return id;
}

// 设置分支指向的commit
void RepositoryCore::setBranchHead(const std::string& branch,const std::string& commit_id){
    if(branch.empty()){Utils::exitWithMessage("branch name is empty");}
    if(commit_id.empty()){Utils::exitWithMessage("commit id is empty");}
    if(!Utils::exists(Utils::join(objects_dir,commit_id))){Utils::exitWithMessage("commit does not exist");}

    std::string branch_file=Utils::join(branches_dir,branch);  // 分支文件路径
    std::string id=commit_id;

    Utils::writeContents(branch_file,id);  // 写入分支文件
}

void RepositoryCore::clearStagingArea(){
    stagingArea.clear(); 
}

StagingArea& RepositoryCore::getStagingArea(){
    return stagingArea;
}