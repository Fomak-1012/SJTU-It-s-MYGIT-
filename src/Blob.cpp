#include"../include/Blob.h"
#include"../include/Utils.h"
#include"../include/GitliteException.h"

Blob::Blob(const std::string& content):content(content){
    id=generateId(content); 
}

Blob::Blob(const std::string& id,const std::string& content):id(id),
                                                             content(content){}

std::string Blob::getId()const{return id;}

std::string Blob::getContent()const{return content;}

void Blob::write(const std::string& path)const{
    std::string blob_path=Utils::join(path,id); 
    Utils::writeContents(blob_path,content);     
}

// 从磁盘加载Blob对象
Blob Blob::load(const std::string& path,const std::string& id){
    std::string blob_path=Utils::join(path,id);
    if(!Utils::exists(blob_path)||!Utils::isFile(blob_path)){
        throw GitliteException("Blob not found: "+id); 
    }
    std::string content=Utils::readContentsAsString(blob_path); 
    return Blob(id,content);  // 创建并返回Blob对象
}

// 创建Blob对象并立即写入磁盘
Blob Blob::create(const std::string& path,const std::string& content){
    Blob blob(content);    
    blob.write(path);     
    return blob;          
}

std::string Blob::generateId(const std::string& content){
    return Utils::sha1(content); 
}