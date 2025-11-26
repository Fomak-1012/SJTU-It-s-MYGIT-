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
    this->id=generateId(message,timestamp,parents,blobs);
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
    // Serialize message
    oss<<"MESSAGE:"<<message<<"\n";

    // Serialize timestamp
    oss<<"TIME:"<<timestamp<<"\n";

    // Serialize parents
    oss<<"PARENTS:";
    for(size_t i=0;i<parents.size();i++) {
        if(i>0)oss<<",";
        oss<<parents[i];
    }
    oss<<"\n";

    // Serialize merge info
    oss<<"MERGE:"<<merge_info<<"\n";

    // Serialize blobs
    oss<<"BLOBS:";
    size_t count=0;
    for (const auto& pair:blobs) {
        if(count>0)oss<<",";
        oss<<pair.first<<":"<<pair.second;
        count++;
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

    while (std::getline(iss, line)) {
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) continue;

        std::string key = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);

        if (key == "MESSAGE") {
            message = value;
        } else if (key == "TIME") {
            timestamp = std::stoll(value);
        } else if (key == "PARENTS") {
            if (!value.empty()) {
                std::istringstream parentStream(value);
                std::string parent;
                while (std::getline(parentStream, parent, ',')) {
                    parents.push_back(parent);
                }
            }
        } else if (key == "MERGE") {
            merge_info = value;
        } else if (key == "BLOBS") {
            if (!value.empty()) {
                std::istringstream blobStream(value);
                std::string blobPair;
                while (std::getline(blobStream, blobPair, ',')) {
                    size_t colonPos = blobPair.find(':');
                    if (colonPos != std::string::npos) {
                        std::string filename = blobPair.substr(0, colonPos);
                        std::string blobId = blobPair.substr(colonPos + 1);
                        blobs[filename] = blobId;
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
    std::string content = Utils::readContentsAsString(filename);
    return deserialize(content);
}

std::string Commit::getFormattedTimestamp() const {
    std::tm* tm_info = std::localtime(&timestamp);
    std::ostringstream oss;
    oss << std::put_time(tm_info, "%a %b %d %H:%M:%S %Y %z");
    return oss.str();
}

bool Commit::isMergeCommit() const {
    return parents.size() > 1;
}

std::string Commit::getShortId() const {
    return id.substr(0, 7);
}

std::string Commit::generateId(const std::string& message,
                               const std::time_t& timestamp,
                               const std::vector<std::string>& parents,
                               const std::map<std::string, std::string>& blobs) {
    std::ostringstream oss;
    oss << message << timestamp;
    for (const auto& parent : parents) {
        oss << parent;
    }
    for (const auto& pair : blobs) {
        oss << pair.first << pair.second;
    }
    return Utils::sha1(oss.str());
}

std::string Commit::timeToString(const std::time_t& timestamp) {
    return std::to_string(timestamp);
}

std::time_t Commit::stringToTime(const std::string& timeStr) {
    return std::stoll(timeStr);
}