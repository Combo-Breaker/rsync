#include "rsync.h"
#include <thread>
#include <stdio.h>  
#include <unistd.h>  
#include <stdlib.h>  
#include <dirent.h>  
#include <string.h>  
#include <sys/types.h>  
#include <sys/stat.h>  
#include <sys/socket.h>  
#include <sstream>
#include <algorithm> 
#include <utility>
#include <netinet/in.h>

using namespace std;

void Protocol::RequestList() { //запрос на список файлов
    Frame f;
    f.msg_id = MsgId::GETLIST;
    conn_->WriteFrame(&f);
}

void Protocol::SendFileList(const FileList& list) { //отправить список файлов
    Frame f;
    f.msg_id = MsgId::FILELIST;
    stringstream ss;
    boost::archive::text_oarchive archive(ss);
    archive << list;
    f.body = ss.str();
    conn_->WriteFrame(&f);
}


void Protocol::SendOk() { 
    Frame f;
    f.msg_id = MsgId::OK;
    f.body = "";
    conn_->WriteFrame(&f);
}


MsgId Protocol::RecvMsg() { 
    conn_->ReadFrame(&last_received_);
    return last_received_.msg_id;
}

FileList Protocol::GetFileList() { //получить список файлов из фрейма
    stringstream ss;
    ss << last_received_.body;
    FileList list;
    boost::archive::text_iarchive archive(ss);
    archive >> list;
    return list;
}

void read_all(int fd, char* buf, size_t size) {
    int s = 0;
    while (int w = read(fd, buf + s, size)) {
        if (w == -1) throw std::runtime_error("Can`t read the frame");
        size -= w;
        s += w;
    }
}

void write_all(int fd, char* buf, size_t size) {
    int s = 0;
    while (int w = write(fd, buf + s, size)) {
        if (w == -1) throw runtime_error("Can`t write the frame");
        size -= w;
        s += w;
    }
}

void SocketConnection::WriteFrame(Frame* frame) {  
    uint32_t len = frame->body.length();
    write_all(fd_, (char*) &len, 4);
    uint8_t id = static_cast<uint8_t> (frame->msg_id);
    write_all(fd_, (char*) &id, 1);
    write_all(fd_, (char*) frame->body.c_str(), len);
}

void SocketConnection::ReadFrame(Frame* frame) {
    uint32_t len;
    read_all(fd_, (char*) &len, 4);
    uint8_t msg_id;
    read_all(fd_, (char*) &msg_id, 1);
    frame->msg_id = MsgId(msg_id);
    frame->body.resize(len);
    read_all(fd_, (char*) frame->body.c_str(), len);
}

FileList GetFiles(string path) {   //получить список файлов директории
    FileList F;      
    DIR *dir;
    dir = opendir(path.c_str());
    if (dir == NULL) throw runtime_error("Can`t open the directory");
    struct dirent *cur;
    while ((cur = readdir(dir)) != NULL) {
        string tmp(cur->d_name);
        if ((strcmp(cur->d_name, ".") != 0) && (strcmp(cur->d_name, "..") != 0))
            F.files.push_back(tmp);
        }                
    int d = closedir(dir);
    if (d == -1) throw runtime_error("Can`t close the directory");
    return F; 
}

pair < vector<string>, vector<string> > difference (vector<string> source, vector<string> dest) { //вектор cp / вектор rm
    sort (source.begin(), source.end());
    sort (dest.begin(), dest.end());
    pair < vector<string>, vector<string> > diff;
    vector<string>::iterator it;
    vector<string> cp(source.size());
    vector<string> rm(dest.size());
    it = set_difference (source.begin(), source.end(), dest.begin(), dest.end(), cp.begin()); 
    cp.resize(it - cp.begin()); 
    it = set_difference (dest.begin(), dest.end(), source.begin(), source.end(), rm.begin()); 
    rm.resize(it - rm.begin());      
    diff = make_pair(cp, rm);   
    return diff; 
}


void Sender::Launch(string source) {
    this->RequestList();
    FileList source_files = GetFiles(source);
    bool state = true;
    while (state) { 
        this->RecvMsg();
        switch (this->last_received_.msg_id) {
            case MsgId::FILELIST: {
                FileList dest_files = this->GetFileList();                
                
                pair <vector<string>, vector<string> > diff = difference(source_files.files, dest_files.files);
                for (auto it = diff.first.begin(); it != diff.first.end(); ++it) {
                    cout << "cp" << ' ' << (*it) << endl;
                }
                for (auto it = diff.second.begin(); it != diff.second.end(); ++it) {
                    cout << "rm" << ' ' << (*it) << endl;
                }
                
                this->SendOk();

                state = false;
                break;
            }
        }
    }
}

void Receiver::Launch(string dest) {
    bool state = true;
    while (state) {
        this->RecvMsg();
        switch (this->last_received_.msg_id) {
            case MsgId::GETLIST: {
                this->dest_ = this->last_received_.body;
                FileList files = GetFileList();
                this->SendFileList(files);
                break;
            }
            case MsgId::OK: {
                state = false;
                break;
            }
            default: {}
        }
    }
}


void rsync(string source, string dest, int mode) {
     if (mode == 1) {
            int sock;    
            sock = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in addr;

            int listener = socket(AF_INET, SOCK_STREAM, 0);
            if(listener < 0)
                    throw runtime_error("listener error");
    
            addr.sin_family = AF_INET;
            addr.sin_port = htons(3425);
            addr.sin_addr.s_addr = htonl(INADDR_ANY);
            if(bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0)
                throw runtime_error("bind error");

            listen(listener, 1);
            sock = accept(listener, NULL, NULL);
            if(sock < 0)
                throw runtime_error("accept error");
            
            SocketConnection s(sock);
            Sender sender(&s, source, dest);
            sender.Launch(source);
            

    } else if (mode == 2) {
        
            int sock;    
            sock = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(3425);
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); //тут пока замыкание на себя
            if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
                throw runtime_error("Can`t connect to the server");
            
            SocketConnection r(sock);
            Receiver receiver(&r);
            receiver.Launch(dest);
    }    
}
