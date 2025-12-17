#ifndef REMOTE_MANAGER_H
#define REMOTE_MANAGER_H

#include<string>
#include<map>

class RepositoryCore;
class MergeManager;

class RemoteManager{
private:
    RepositoryCore* core;

    std::map<std::string,std::string> getRemotes();
    void saveRemotes(const std::map<std::string,std::string>& remotes);
    
public:
    RemoteManager(RepositoryCore* repoCore);

    //远程操作
    void addRemote(const std::string& remoteName,const std::string& remotePath);
    void rmRemote(const std::string& remoteName);
    void push(const std::string& remoteName,const std::string& remoteBranchName);
    void pull(const std::string& remoteName,const std::string& remoteBranchName);
    void fetch(const std::string& remoteName,const std::string& remoteBranchName);

    //辅助函数
    bool validateRemoteRepository(const std::string& remotePath);
    std::string getRemoteGitliteDir(const std::string& remotePath);
};

#endif //REMOTE_MANAGER_H