#include"../include/GitObj.h"

void GitObj::init(){
    repo.init();  
}

void GitObj::add(const std::string& filename){
    repo.add(filename);  
}

void GitObj::commit(const std::string& message){
    repo.commit(message);  
}

void GitObj::rm(const std::string& filename){
    repo.rm(filename); 
}

void GitObj::log(){
    repo.log();
}

void GitObj::globalLog(){
    repo.globalLog(); 
}

void GitObj::find(const std::string& commitMessage){
    repo.find(commitMessage);
}

void GitObj::checkoutFile(const std::string& filename){
    repo.checkoutFile(filename);
}

void GitObj::checkoutFileInCommit(const std::string& commitId,const std::string& filename){
    repo.checkoutFileInCommit(commitId,filename);
}

void GitObj::checkoutBranch(const std::string& branchName){
    repo.checkoutBranch(branchName);
}

void GitObj::status(){
    repo.status();
}

void GitObj::branch(const std::string& branchName){
    repo.branch(branchName);
}

void GitObj::rmBranch(const std::string& branchName){
    repo.rmBranch(branchName);
}

void GitObj::reset(const std::string& commitId){
    repo.reset(commitId);
}

void GitObj::merge(const std::string& branchName){
    repo.merge(branchName);
}

void GitObj::addRemote(const std::string& remoteName,const std::string& remoteUrl){
    repo.addRemote(remoteName,remoteUrl);
}

void GitObj::rmRemote(const std::string& remoteName){
    repo.rmRemote(remoteName);
}

void GitObj::push(const std::string& remoteName,const std::string& branchName){
    repo.push(remoteName,branchName);
}

void GitObj::fetch(const std::string& remoteName,const std::string& branchName){
    repo.fetch(remoteName,branchName);
}

void GitObj::pull(const std::string& remoteName,const std::string& branchName){
    repo.pull(remoteName,branchName);
}