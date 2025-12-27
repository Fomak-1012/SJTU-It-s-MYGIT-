#ifndef STAGINGAREA_H
#define STAGINGAREA_H

#include<string>
#include<map>   
#include<set>

class StagingArea{
private:
    std::map<std::string,std::string> staging_map;//文件名到blob id的映射
    std::set<std::string> removed_files;          //被删除的文件名集合
    const std::string staging_file_path;          //文件存储路径(.gitlite/staging)
    const std::string removed_file_path;          //被删除文件存储路径(.gitlite/removed)

    //从文件里读取暂存区文件
    void loadStagingMap();

    //从文件里读取被删除的文件
    void loadRemovedFiles();
   
public:
    StagingArea(const std::string&,const std::string&);

    const std::map<std::string,std::string>& getStagingMap() const;
    const std::set<std::string>& getRemovedFiles() const;

    void addStagedFile(const std::string&,const std::string&);
    void removeStagedFile(const std::string&);
    void addRemovedFile(const std::string&);
    void removeRemovedFile(const std::string&);

    //保存暂存区和删除列表
    void save() const;

    void clear();

    //重新加载暂存区和删除列表
    void reload();

    bool isStaged(const std::string&) const;
    bool isRemoved(const std::string&) const;
};

#endif // STAGINGAREA_H