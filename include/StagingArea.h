#ifndef STAGING_AREA_H
#define STAGING_AREA_H

#include <string>
#include <map>
#include <set>

class StagingArea {
private:
    std::map<std::string, std::string> stagingMap;  // 暂存区：文件名 -> Blob ID
    std::set<std::string> removedFiles;             // 被标记删除的文件集合
    const std::string stagingFilePath;              // 暂存区文件路径（.gitlite/staging）
    const std::string removedFilePath;              // 删除文件列表路径（.gitlite/removed）

    // 从文件加载暂存区数据
    void loadStaging();
    // 从文件加载被删除文件列表
    void loadRemovedFiles();

public:
    // 构造函数：接收暂存区和删除文件列表的路径
    StagingArea(const std::string& stagingFilePath, const std::string& removedFilePath);

    // 获取暂存区映射（只读）
    const std::map<std::string, std::string>& getStagingMap() const;
    // 获取被删除文件集合（只读）
    const std::set<std::string>& getRemovedFiles() const;

    // 暂存文件（添加/更新暂存区）
    void addStagedFile(const std::string& filename, const std::string& blobId);
    // 取消暂存文件
    void removeStagedFile(const std::string& filename);
    // 标记文件为删除
    void addRemovedFile(const std::string& filename);
    // 取消文件的删除标记
    void removeRemovedFile(const std::string& filename);

    // 保存暂存区和删除列表到文件
    void save() const;
    // 清空暂存区和删除列表
    void clear();

    // 检查文件是否已暂存
    bool isStaged(const std::string& filename) const;
    // 检查文件是否被标记删除
    bool isRemoved(const std::string& filename) const;
};

#endif // STAGING_AREA_H