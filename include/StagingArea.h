#ifndef STAGINGAREA_H
#define STAGINGAREA_H

#include<cstring>
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
    StagingArea(const std::string& );
};

#endif // STAGINGAREA_H