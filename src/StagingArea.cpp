#include "../include/StagingArea.h"
#include "../include/Utils.h"

static void removeWhiteSpace(std::string& str){
    while (!str.empty()
            &&(str.back()=='\r'
            ||str.back()=='\n'
            ||isspace((unsigned char)str.back()))){str.pop_back();}
    while (!str.empty()
            &&(str.front()=='\r'
            ||str.front()=='\n'
            ||isspace((unsigned char)str.front()))){str.erase(str.begin());}
}

static bool isBlankLine(const std::string& s){
    if (s.empty())return true;
    for (unsigned char c : s) {
        if(c==0||c==0xA0)continue;
        if(!std::isspace(c))return false;
    }
    return true;
}

void StagingArea::loadStagingMap() {
    staging_map.clear();
    if(!Utils::exists(staging_file_path)){return;}

    std::string content=Utils::readContentsAsString(staging_file_path);
    size_t pos=0;
    size_t line_start=0;
    const size_t content_len=content.length();

    while(pos<=content_len){
        if(pos==content_len 
         ||content[pos]=='\n' 
         ||content[pos]=='\r'){
            std::string line=content.substr(line_start,pos-line_start);
            
            if(!line.empty()){
                size_t colon_pos=line.find(':');
                if(colon_pos!=std::string::npos) {
                    std::string filename=line.substr(0,colon_pos);
                    std::string blob_id=line.substr(colon_pos+1);

                    removeWhiteSpace(filename);
                    removeWhiteSpace(blob_id);

                    if(filename.size()>=3 
                    &&(unsigned char)filename[0]==0xEF 
                    &&(unsigned char)filename[1]==0xBB 
                    &&(unsigned char)filename[2]==0xBF){
                        filename=filename.substr(3);
                    }

                    if(filename.empty()||blob_id.empty()){
                        if(pos<content_len 
                         &&content[pos]=='\r' 
                         &&pos+1<content_len
                         &&content[pos+1]=='\n'){pos++;}
                        line_start=pos+1;
                        pos++;
                        continue;
                    }

                    staging_map[filename]=blob_id;
                }
            }

            if(pos<content_len 
             &&content[pos]=='\r' 
             &&pos+1<content_len 
             &&content[pos+1]=='\n'){pos++;}
            line_start=pos+1;
        }
        pos++;
    }
}

void StagingArea::loadRemovedFiles() {
    removed_files.clear();
    if(!Utils::exists(removed_file_path)){return;}

    std::string content=Utils::readContentsAsString(removed_file_path);
    size_t pos=0;
    size_t line_start=0;
    const size_t content_len=content.length();

    while(pos<=content_len){
        if(pos==content_len 
         ||content[pos]=='\n' 
         ||content[pos]=='\r'){
            std::string filename=content.substr(line_start,pos-line_start);

            removeWhiteSpace(filename);

            if(isBlankLine(filename)){
                if(pos<content_len 
                 &&content[pos]=='\r' 
                 &&pos+1<content_len
                 &&content[pos+1]=='\n'){pos++;}
                line_start=pos+1;
                pos++;
                continue;
            }

            removeWhiteSpace(filename);

            if(!filename.empty()){removed_files.insert(filename);}

            if(pos<content_len 
             &&content[pos]=='\r' 
             &&pos+1<content_len 
             &&content[pos+1]=='\n'){pos++;}
            line_start=pos+1;
        }
        pos++;
    }
}

void StagingArea::loadStagingMap(){
    staging_map.clear();
    if(Utils::exists(staging_file_path))return ;
    std::string content=Utils::readContentsAsString(staging_file_path);

}

const std::map<std::string,std::string>& StagingArea::getStagingMap() const {
    return staging_map;
}

const std::set<std::string>& StagingArea::getRemovedFiles() const {
    return removed_files;
}

void StagingArea::addStagedFile(const std::string& filename,const std::string& blobId){
    staging_map[filename]=blobId;
}

void StagingArea::removeStagedFile(const std::string& filename){
    staging_map.erase(filename);
}

void StagingArea::addRemovedFile(const std::string& filename){
    removed_files.insert(filename);
}

void StagingArea::removeRemovedFile(const std::string& filename){
    removed_files.erase(filename);
}

void StagingArea::save() const {
    std::string staging_content;
    for(const auto& entry : staging_map){
        std::string name=entry.first;
        std::string id=entry.second;
        
        removeWhiteSpace(name);
        removeWhiteSpace(id);
        
        if(name.empty()||id.empty()){continue;}
        
        staging_content+=name;
        staging_content+=":";
        staging_content+=id;
        staging_content+="\n";
    }
    Utils::writeContents(staging_file_path,staging_content);

    std::string removed_content;
    for (const auto& filename : removed_files) {
        std::string name=filename;

        removeWhiteSpace(name);
        
        if(name.empty()){continue;}
        
        removed_content+=name;
        removed_content+="\n";
    }
    Utils::writeContents(removed_file_path,removed_content);
}

void StagingArea::clear(){
    staging_map.clear();
    removed_files.clear();
}

void StagingArea::reload(){
    staging_map.clear();
    removed_files.clear();
    loadStagingMap();
    loadRemovedFiles();
}

bool StagingArea::isStaged(const std::string& filename) const {
    return staging_map.count(filename)>0;
}

bool StagingArea::isRemoved(const std::string& filename) const {
    return removed_files.count(filename)>0;
}