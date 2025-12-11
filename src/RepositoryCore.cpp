#include"../include/RepositoryCore.h"
#include"../include/Utils.h"
#include"../include/GitliteException.h"
#include"../include/Commit.h"

const std::string RepositoryCore::gitlite_dir=".gitlite";
const std::string RepositoryCore::objects_dir=".gitlite/objects";
const std::string RepositoryCore::branches_dir=".gitlite/branches";
const std::string RepositoryCore::staging_area_file=".gitlite/staging";
const std::string RepositoryCore::removed_file=".gitlite/removed";
const std::string RepositoryCore::head_file=".gitlite/HEAD";
const std::string RepositoryCore::remotes_file=".gitlite/remotes";

static void removeWhiteSpace(std::string& str){
    while (!str.empty()
            &&(str.back()=='\r'
            ||str.back()=='\n'
            ||isspace((unsigned char)str.back()))){str.pop_back();}
    while (!str.empty()
            &&(str.front()=='\r'
            ||str.front()=='\n'
            ||isspace((unsigned char)str.front()))){str.erase(str.begin());}
}

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

void RepositoryCore::setCurrentBranch(const std::string& branch){
    if(branch.empty()){Utils::exitWithMessage("branch name is empty");}
    if(!Utils::exists(Utils::join(branches_dir,branch))){Utils::exitWithMessage("branch does not exist");}
    Utils::writeContents(head_file,branch);
}

std::string RepositoryCore::getBranchHead(const std::string& branch){
    if(branch.empty()){Utils::exitWithMessage("branch name is empty");}
    if(!Utils::exists(Utils::join(branches_dir,branch))){Utils::exitWithMessage("branch does not exist");}
    std::string id=Utils::readContentsAsString(Utils::join(branches_dir,branch));
    removeWhiteSpace(id);
    return id;
}

void RepositoryCore::setBranchHead(const std::string& branch,const std::string& commit_id){
    if(branch.empty()){Utils::exitWithMessage("branch name is empty");}
    if(commit_id.empty()){Utils::exitWithMessage("commit id is empty");}
    if(!Utils::exists(Utils::join(objects_dir,commit_id))){Utils::exitWithMessage("commit does not exist");}

    std::string branch_file=Utils::join(branches_dir,branch);
    std::string id=commit_id;

    removeWhiteSpace(id);

    Utils::writeContents(Utils::join(branches_dir,branch),commit_id);
}

void RepositoryCore::clearStagingArea(){
    stagingArea.clear();
}


StagingArea& RepositoryCore::getStagingArea(){
    return stagingArea;
}

