#include"../include/Commit.h"
#include"../include/Utils.h"
#include<sstream>
#include<iomanip>
#include<iostream>

Commit::Commit():id(""),message(""),timestamp(0){}

Commit::Commit(const std::string& message,
               const std::time_t& timestamp,
               const std::vector<std::string>& parents,
               const std::map<std::string, std::string>& blobs)
    :message(message),timestamp(timestamp),parents(parents),blobs(blobs),merge_info("") {
    id=generateId(message,timestamp,parents,blobs);
}

std::string Commit::getId() const {return id;}
std::string Commit::getMessage() const {return message;}
std::time_t Commit::getTimestamp() const {return timestamp;}
std::vector<std::string> Commit::getParents() const {return parents;}
std::map<std::string, std::string> Commit::getBlobs() const {return blobs;}
std::string Commit::getMergeInfo() const {return merge_info;}

void Commit::setMergeInfo(const std::string& info) {merge_info=info;}

std::string Commit::serialize() const {
    std::ostringstream oss;

    oss<<"Message:"<<message<<"\n";

    oss<<"Time:"<<timestamp<<"\n";

    oss<<"Parents:";
    for(size_t i=0;i<parents.size();i++){
        if(i>0)oss<<",";
        oss<<parents[i];
    }
    oss<<"\n";

    oss<<"Merge:"<<merge_info<<"\n";

    oss<<"Blobs:";
    bool flag=0;
    for(const auto& pair:blobs){
        if(flag)oss<<",";
        oss<<pair.first<<":"<<pair.second;
        flag=1;
    }
    oss<<"\n";

    return oss.str();
}

Commit Commit::deserialize(const std::string& data) {
    std::istringstream iss(data);
    std::string line;
    std::string message;
    std::time_t timestamp=0;
    std::vector<std::string> parents;
    std::map<std::string, std::string> blobs;
    std::string merge_info;

    while(std::getline(iss, line)){
        if(line.rfind("Message:",0)==0) 
            message=line.substr(8);
        
        else if(line.rfind("Time:",0)==0)
            timestamp=std::stoll(line.substr(5));
        
        else if(line.rfind("Parents:",0)==0){
            std::string parents_str=line.substr(8);
            parents.clear();
            if(!parents_str.empty()){
                std::stringstream ss(parents_str);
                std::string parent;
                while(std::getline(ss,parent,','))
                    parents.push_back(parent);
            }
        } 

        else if (line.rfind("Merge:",0) == 0) 
            merge_info=line.substr(6);
        
        else if (line.rfind("Blobs:",0) == 0) {
            std::string blobs_str=line.substr(6);
            blobs.clear();
            if (!blobs_str.empty()) {
                std::stringstream ss(blobs_str);
                std::string pair;
                while (std::getline(ss,pair,',')){
                    size_t pos=pair.find(':');
                    if (pos!=std::string::npos){
                        std::string key=pair.substr(0,pos);
                        std::string value=pair.substr(pos+1);
                        blobs[key]=value;
                    }
                }
            }
        }
    }
    Commit commit(message, timestamp, parents, blobs);
    commit.setMergeInfo(merge_info);
    return commit;
}

Commit Commit::fromFile(const std::string& filename) {
    std::string data = Utils::readContentsAsString(filename);
    return deserialize(data);
}

std::string Commit::generateId(const std::string& message, 
                               const std::time_t& timestamp,
                               const std::vector<std::string>& parents,
                               const std::map<std::string, std::string>& blobs) {
    std::ostringstream oss;
    oss<<message<<timestamp;
    for(const auto& parent : parents) 
        oss<<parent;
    
    for(const auto& blob : blobs) 
        oss<<blob.first<<blob.second;
    return Utils::sha1(oss.str());
}

std::string Commit::timeToString(const std::time_t& timestamp) {
    return std::to_string(timestamp);
}

std::time_t Commit::stringToTime(const std::string& timeStr) {
    return std::stoll(timeStr);
}