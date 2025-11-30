# 关于gitlite的构造
## 一、类的定义
### 1.Blob 类
#### 实例变量
```
std::string content;
std::string id;
```
#### 作用:
封装单个文件的内容快照，作为版本控制的原子单位，通过内容的 SHA-1 哈希确保唯一性。
### 2.Commit 类
#### 实例变量
```
std::string message//提交信息，描述版本变更
std::string timestamp//提交时间戳，记录版本创建时间
std::unordered_map<std::string, std::string> file_blob_map//文件名到 Blob ID 的映射，记录版本的文件组成
std::vector<std::string> parent_ids//父提交 ID 列表，支持分支合并的多父结构
```
#### 作用:
封装一次版本提交的完整数据，通过父提交链形成版本历史。
### 3.StagingArea 类
```
std::unordered_map<std::string, std::string> add_map//待添加文件与 Blob ID 的映射
std::unordered_set<std::string> rm_set//待删除文件集合
std::string staging_path//暂存区存储路径，默认 .gitlet/staging
```
#### 作用:
作为工作目录与版本历史的中间层，临时存储待提交的文件变更，支持持久化以避免程序退出丢失状态。
### 4.Repository 类
#### 实例变量
```
const std::string repo_path = ".gitlet";
const std::string objects_path = repo_path + "/objects";
const std::string refs_path = repo_path + "/refs/heads";
const std::string head_path = repo_path + "/HEAD";
const std::string staging_path = repo_path + "/staging";
```
#### 作用:
整合前3个类，完成部分的功能实现（主要集中在离线部分）


## GitObj类
实现所有类的整合，提供对外接口
