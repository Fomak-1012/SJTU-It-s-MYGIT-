# GitLite - SJTUDream! It's MyGIT! ! ! !

**一言以蔽之**：类的实现就是一种封传箱子

Fomak你记一下，我做如下部署调整：以Blob、Commit、StagingArea三个类为基础，共同实现一个gitlite的基础部件，再将Repository类的实现分散到RepositoryCore、FileOperationManager、StatusManager、BranchManager、CommitManager、MergeManager、RemoteManager中，同时利用好Utils和GitliteException简化文件和报错方面的操作，最后把类全部集成的Repository类中，并通过GitObj实现对外的接口。


## 大致骨架（会有部分交叉）
```text
src/
├── utils/                        
│   ├── Utils.h
│   └── GitliteException.h
└── basis/                            
    ├── Blob.h                     # 文件快照的基础实现
    ├── Commit.h                   # 提交的基础实现
    ├── StagingArea.h              # 暂存区的基础实现
    └── Repository.h                  
        ├── RepositoryCore.h       # 仓库的核心管理
        ├── FileOperationManager.h # 文件操作的管理  
        ├── StatusManager.h        # 状态查询的实现
        ├── BranchManager.h        # 分支的基础管理
        ├── CommitManager.h        # 提交的基础管理
        ├── MergeManager.h         # 合并操作的重点实现
        └── RemoteManager.h        # 远程仓库操作的重点实现
```

## 以下是各个类的具体实现

###  Blob 类 

**功能**：文件内容的快照管理

**主要方法**：
```cpp
Blob(const std::string& content);           // 创建Blob对象，自动计算SHA1
std::string getId() const;                 // 获取SHA1哈希值
std::string getContent() const;              // 获取文件内容
static Blob create(const std::string& path, const std::string& content);
                                            // 创建并持久化到磁盘
static Blob load(const std::string& path, const std::string& id);
                                            // 从磁盘加载Blob对象
static std::string generateId(const std::string& content);
                                            // 生成内容对应的SHA1哈希
void write(const std::string& path) const;  // 将Blob写入objects目录
```

###  Commit 类

**功能**：提交记录管理，包含完整的版本快照信息和历史追踪

**主要变量**：
```cpp
std::string id;                           // SHA1唯一标识符
std::string message;                      // 提交信息
std::time_t timestamp;                    // 提交时间戳
std::vector<std::string> parents;         // 父提交列表
std::map<std::string, std::string> blobs; // 文件名到Blob映射
std::string merge_info;                   // 合并相关信息
```

**主要方法**：
```cpp
Commit(const std::string& message, const std::time_t& timestamp, 
       const std::vector<std::string>& parents,
       const std::map<std::string, std::string>& blobs); // 构造提交
std::string getId() const;                // 获取提交ID
std::string getMessage() const;           // 获取提交信息
std::time_t getTimestamp() const;         // 获取时间戳
std::vector<std::string> getParents() const; // 获取父提交列表
std::map<std::string, std::string> getBlobs() const; // 获取文件映射
void setMergeInfo(const std::string& info); // 设置合并信息
std::string serialize() const;            // 序列化提交数据
static Commit deserialize(const std::string& data); // 反序列化
static Commit fromFile(const std::string& filename); // 从文件读取
bool isMergeCommit() const;               // 判断是否为合并提交
std::string getShortId() const;           // 获取短提交ID
```

### StagingArea 类 

**功能**：暂存区管理，管理待提交的文件变更，包括添加、修改和删除操作

**主要变量**：
```cpp
std::map<std::string, std::string> staging_map;  // 文件名 -> Blob ID
std::set<std::string> removed_files;              // 待删除文件列表
```

**主要方法**：
```cpp
void addStagedFile(const std::string&, const std::string&); // 添加暂存文件
void removeStagedFile(const std::string&);        // 移除暂存文件
void addRemovedFile(const std::string&);          // 添加删除文件
void removeRemovedFile(const std::string&);       // 移除删除文件
void save() const;                               // 保存暂存状态到磁盘
void clear();                                     // 清空暂存区
void reload();                                    // 重新加载暂存区状态
bool isStaged(const std::string&) const;         // 检查文件是否已暂存
bool isRemoved(const std::string&) const;         // 检查文件是否标记删除
const std::map<std::string,std::string>& getStagingMap() const; // 获取暂存映射
const std::set<std::string>& getRemovedFiles() const; // 获取删除文件列表
```

### RepositoryCore 类 

**功能**：仓库核心管理，提供基础的存储和访问功能，管理仓库的目录结构和核心配置

**重要常量**：
```cpp
static const std::string gitlite_dir = ".gitlite";
static const std::string objects_dir = ".gitlite/objects";
static const std::string branches_dir = ".gitlite/branches";
static const std::string staging_area_file = ".gitlite/staging";
static const std::string removed_file = ".gitlite/removed";
static const std::string head_file = ".gitlite/HEAD";
static const std::string remotes_file = ".gitlite/remotes";
```

**主要方法**：
```cpp
RepositoryCore();                              // 构造函数，初始化暂存区
static bool isInitialized();                  // 检查仓库是否已初始化
static std::string getGitliteDir();           // 获取Gitlite目录路径
void init();                                  // 初始化仓库
std::string getCurrentBranch();               // 获取当前分支名
void setCurrentBranch(const std::string& branchName); // 设置当前分支
std::string getBranchHead(const std::string& branchName); // 获取分支HEAD
void setBranchHead(const std::string& branchName, const std::string& commitId); // 设置分支HEAD
void clearStagingArea();                     // 清空暂存区
StagingArea& getStagingArea();                // 获取暂存区引用
void copyFile(const std::string& source, const std::string& destination); // 复制文件
```

### FileOperationManager 类 

**功能**：文件操作管理，处理工作区与暂存区之间的文件同步和状态跟踪

**主要方法**：
```cpp
FileOperationManager(RepositoryCore* core, CommitManager* commitManager); // 构造函数
void add(const std::string& filename);              // 添加文件到暂存区
void rm(const std::string& filename);               // 删除文件
void checkoutFile(const std::string& filename);       // 检出文件
void checkoutFileInCommit(const std::string& commitId, const std::string& filename); // 从指定提交检出文件
std::set<std::string> getUntrackedFiles();          // 获取未跟踪文件
std::set<std::string> getConflictFiles();           // 获取冲突文件
void saveConflictFiles(const std::set<std::string>& conflictFiles); // 保存冲突文件
void clearConflictFiles();                            // 清空冲突文件
void clearStagingArea();                             // 清空暂存区
private:
bool isFileModified(const std::string& filename, const std::string& commitId); // 检查文件是否修改
std::map<std::string, std::string> getModifiedFiles(); // 获取修改的文件
```

###  StatusManager 类 

**功能**：状态管理和显示，分析仓库的当前状态并展示详细的状态信息

**主要方法**：
```cpp
StatusManager(RepositoryCore* repoCore, CommitManager* commitMgr, FileOperationManager* fileOpMgr); // 构造函数
void status();                                       // 显示完整仓库状态
void printBranches(const std::set<std::string>& branches, const std::string& currentBranch); // 打印分支信息
void printStagedFiles();                            // 打印已暂存文件
void printRemovedFiles();                            // 打印已删除文件
void printModifiedFiles(const std::map<std::string, std::string>& modifiedFiles); // 打印修改文件
void printUntrackedFiles(const std::set<std::string>& untrackedFiles); // 打印未跟踪文件
private:
std::set<std::string> getAllBranches();             // 获取所有分支
std::map<std::string, std::string> getModifiedFiles(); // 获取修改文件
std::set<std::string> getUntrackedFiles();           // 获取未跟踪文件
```

###  BranchManager 类 

**功能**：分支管理，处理分支的创建、删除、切换和历史维护

**主要方法**：
```cpp
BranchManager(RepositoryCore* repoCore);            // 构造函数
void branch(const std::string& branchName);          // 创建新分支
void rmBranch(const std::string& branchName);        // 删除分支
void checkoutBranch(const std::string& branchName);  // 切换分支
std::string getCurrentBranch();                       // 获取当前分支
std::set<std::string> getAllBranchesList();          // 获取所有分支列表
void performBranchCheckout(const std::string& branchName); // 执行分支切换
private:
std::set<std::string> getAllBranches();              // 获取所有分支
std::string findSplitPoint(const std::string& branch1, const std::string& branch2); // 查找分割点
```

### CommitManager 类

**功能**：提交管理，处理提交的创建、历史查询和提交验证

**主要方法**：
```cpp
CommitManager(RepositoryCore* repoCore);             // 构造函数
void commit(const std::string& message);             // 创建提交
void saveCommit(const Commit& commit);               // 保存提交到磁盘
Commit getCommit(const std::string& commitId);       // 获取提交对象
void log();                                           // 显示当前分支提交历史
void globalLog();                                     // 显示所有分支提交历史
void find(const std::string& commitMessage);         // 根据提交信息查找提交
std::string getFileBlobId(const std::string& filename, const std::string& commitId); // 获取文件在提交中的Blob ID
bool fileExistsInCommit(const std::string& filename, const std::string& commitId); // 检查文件在提交中是否存在
void copyFileFromCommit(const std::string& filename, const std::string& commitId); // 从提交复制文件
std::map<std::string, std::string> getTrackedFiles(const std::string& commitId); // 获取提交跟踪的文件
std::string getFullCommitId(const std::string& shortId); // 获取完整提交ID
std::string getCurrentCommitId();                     // 获取当前提交ID
Commit getHeadCommit();                               // 获取HEAD提交
std::vector<std::string> getFiles(const std::string& commitId); // 获取提交中的文件列表
void reset(const std::string& commitId);              // 重置到指定提交
```

###  MergeManager 类 

**功能**：合并管理，处理分支间的合并操作，包括快进合并和三方合并，支持冲突检测和处理

**主要方法**：
```cpp
MergeManager(RepositoryCore* repoCore, CommitManager* commitMgr, FileOperationManager* fileOpMgr, BranchManager* branchMgr); // 构造函数
void merge(const std::string& branchName);            // 执行分支合并
bool checkMergeConditions(const std::string& branchName); // 检查合并条件
void performFastForwardMerge(const std::string& branchName); // 执行快进合并
void performThreeWayMerge(const std::string& branchName, const std::string& currentCommitId, const std::string& givenCommitId, const std::string& splitPointId); // 执行三方合并
private:
std::string findSplitPoint(const std::string& branch1, const std::string& branch2); // 查找分割点
std::set<std::string> getAllBranches();               // 获取所有分支
```

### RemoteManager 类 

**功能**：远程仓库管理，处理与远程仓库的交互操作，包括推送、拉取、获取和远程仓库配置

**主要方法**：
```cpp
RemoteManager(RepositoryCore* repoCore);           // 构造函数
void addRemote(const std::string& remoteName, const std::string& remotePath); // 添加远程仓库
void rmRemote(const std::string& remoteName);       // 删除远程仓库
void push(const std::string& remoteName, const std::string& remoteBranchName); // 推送到远程
void pull(const std::string& remoteName, const std::string& remoteBranchName); // 拉取并合并
void fetch(const std::string& remoteName, const std::string& remoteBranchName); // 从远程获取
bool validateRemoteRepository(const std::string& remotePath); // 验证远程仓库
std::string getRemoteGitliteDir(const std::string& remotePath); // 获取远程仓库路径
private:
std::map<std::string, std::string> getRemotes();    // 获取远程仓库映射
void saveRemotes(const std::map<std::string, std::string>& remotes); // 保存远程仓库配置
```

###  Repository 类 

**功能**：整合所有功能模块，提供统一的操作接口和命令行功能

**主要变量**：
```cpp
RepositoryCore* core;                                 // 仓库核心管理器
CommitManager* commitManager;                         // 提交管理器
FileOperationManager* fileOpManager;                  // 文件操作管理器
StatusManager* statusManager;                         // 状态管理器
BranchManager* branchManager;                         // 分支管理器
MergeManager* mergeManager;                           // 合并管理器
RemoteManager* remoteManager;                         // 远程管理器
```

**主要方法**：
```cpp
// 构造和初始化
Repository();                                          // 构造函数，初始化所有管理器
~Repository();                                         // 析构函数，清理资源

// 基础操作
void init();                                           // 初始化仓库
void add(const std::string& filename);                // 添加文件到暂存区
void commit(const std::string& message);               // 提交变更
void status();                                         // 显示仓库状态

// 分支操作
void branch(const std::string& branch_name);          // 创建分支
void checkout(const std::string& target);              // 检出操作
void rm_branch(const std::string& branch_name);        // 删除分支

// 合并与历史
void merge(const std::string& branch_name);           // 合并分支
void reset(const std::string& commit_id);             // 重置到指定提交
void log();                                            // 显示当前分支历史
void global_log();                                     // 显示所有分支历史
void find(const std::string& message);                // 根据信息查找提交

// 远程操作
void add_remote(const std::string& name, const std::string& path); // 添加远程仓库
void rm_remote(const std::string& name);              // 删除远程仓库
void push(const std::string& remote, const std::string& branch); // 推送到远程
void fetch(const std::string& remote, const std::string& branch); // 从远程获取
void pull(const std::string& remote, const std::string& branch); // 拉取并合并

// 文件操作
void rm(const std::string& filename);                 // 删除文件

// 命令执行
void executeCommand(const std::vector<std::string>& args); // 执行Git命令
private:
void initializeManagers();                            // 初始化所有管理器
void cleanupManagers();                               // 清理管理器资源
```

### GitObj 类 

**功能**：GitLite的对外接口类，提供统一的命令行接口，作为整个系统的入口点

**主要方法**：
```cpp
GitObj();                                            // 构造函数，初始化Repository
~GitObj();                                           // 析构函数
void executeCommand(const std::vector<std::string>& args); // 执行命令行参数
void run(int argc, char* argv[]);                   // 运行GitLite程序
static void printUsage();                            // 打印使用说明
static void printVersion();                          // 打印版本信息
private:
Repository* repository;                              // 仓库实例指针
void parseAndExecute(const std::string& command, const std::vector<std::string>& args); // 解析并执行命令
```

## 命令

### 基础命令

```bash
# 初始化仓库
gitlite init

# 文件操作
gitlite add <filename>              # 添加文件到暂存区
gitlite commit -m "message"        # 提交暂存区变更
gitlite rm <filename>              # 删除文件
gitlite status                    # 显示仓库状态

# 历史查询
gitlite log                       # 显示当前分支历史
gitlite global-log                # 显示所有分支历史
gitlite find "message"            # 根据提交信息查找
```

### 分支管理

```bash
gitlite branch <name>              # 创建新分支
gitlite checkout <branch>         # 切换分支
gitlite checkout -- <file>        # 恢复文件
gitlite checkout <commit> -- <file> # 从指定提交恢复文件
gitlite rm-branch <name>         # 删除分支
```

### 合并与重置

```bash
gitlite merge <branch>            # 合并分支
gitlite reset <commit>            # 重置到指定提交
```

### 远程操作

```bash
gitlite add-remote <name> <path>  # 添加远程仓库
gitlite rm-remote <name>           # 删除远程仓库
gitlite push <remote> <branch>    # 推送到远程
gitlite fetch <remote> <branch>   # 从远程获取
gitlite pull <remote> <branch>     # 从远程拉取并合并
```

##  存储格式

### 磁盘目录结构
```
.gitlite/
├── objects/          # 对象存储目录
│   ├── {blob-id}    # 文件内容对象
│   └── {commit-id} # 提交对象
├── branches/         # 分支指针目录
│   ├── master       # 主分支
│   └── {branch}    # 其他分支
├── HEAD            # 当前分支指针
├── staging         # 暂存区状态文件
├── removed         # 删除文件列表
├── conflict       # 冲突文件列表
└── remotes        # 远程仓库配置
```

### 文件格式

**暂存区格式** (.gitlite/staging)：
```
文件名:BlobID
文件名:BlobID
...
```

**删除文件格式** (.gitlite/removed)：
```
文件名1
文件名2
...
```

**提交对象格式** (.gitlite/objects/{commit-id})：
```
Message:{提交信息}
Time:{时间戳}
Parents:{父提交ID1},{父提交ID2},...
Merge:{合并信息}
Blobs:{文件名}:{BlobID},{文件名}:{BlobID},...
```

**分支文件格式** (.gitlite/branches/{branch-name})：
```
{CommitID}
```

**远程配置格式** (.gitlite/remotes)：
```
{远程名称} {远程路径}
{远程名称} {远程路径}
...
```
