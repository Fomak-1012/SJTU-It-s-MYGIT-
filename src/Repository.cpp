#include"../include/Repository.h"
#include"../include/GitliteException.h"
#include"../include/Utils.h"

Repository::Repository(){
    core=new RepositoryCore();
    commitManager=new CommitManager(core);
    branchManager=new BranchManager(core);
    fileOpManager=new FileOperationManager(core,commitManager);
    mergeManager=new MergeManager(core,commitManager,fileOpManager,branchManager);
    remoteManager=new RemoteManager(core);
    statusManager=new StatusManager(core,commitManager,fileOpManager);
}

Repository::~Repository(){
    delete core;
    delete commitManager;
    delete branchManager;
    delete fileOpManager;
    delete mergeManager;
    delete remoteManager;
    delete statusManager;
}

bool Repository::isInitialized(){
    return RepositoryCore::isInitialized();
}

std::string Repository::getGitliteDir(){
    return RepositoryCore::getGitliteDir();
}

void Repository::init(){
    core->init();
}

void Repository::add(const std::string& filename){
    fileOpManager->add(filename);
}

void Repository::commit(const std::string& message){
    commitManager->commit(message);
}

void Repository::rm(const std::string& filename){
    fileOpManager->rm(filename);
}

void Repository::log(){
    commitManager->log();
}

void Repository::globalLog(){
    commitManager->globalLog();
}

void Repository::status(){
    statusManager->status();
}

void Repository::find(const std::string& commitMessage){
    commitManager->find(commitMessage);
}

void Repository::checkoutFile(const std::string& filename){
    fileOpManager->checkoutFile(filename);
}

void Repository::checkoutFileInCommit(const std::string& commitId,const std::string& filename){
    fileOpManager->checkoutFileInCommit(commitId,filename);
}

void Repository::checkoutBranch(const std::string& branchName){
    branchManager->checkoutBranch(branchName);
}

void Repository::branch(const std::string& branchName){
    branchManager->branch(branchName);
}

void Repository::rmBranch(const std::string& branchName){
    branchManager->rmBranch(branchName);
}

void Repository::reset(const std::string& commitId){
    commitManager->reset(commitId);
}

void Repository::merge(const std::string& branchName){
    mergeManager->merge(branchName);
}

void Repository::push(const std::string& remoteName,const std::string& branchName){
    remoteManager->push(remoteName,branchName);
}

void Repository::pull(const std::string& remoteName,const std::string& branchName){
    remoteManager->pull(remoteName,branchName);
}

void Repository::addRemote(const std::string& remoteName,const std::string& remotePath){
    remoteManager->addRemote(remoteName,remotePath);
}

void Repository::rmRemote(const std::string& remoteName){
    remoteManager->rmRemote(remoteName);
}

void Repository::fetch(const std::string& remoteName,const std::string& branchName){
    remoteManager->fetch(remoteName,branchName);
}


