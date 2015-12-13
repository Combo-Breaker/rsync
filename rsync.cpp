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

using namespace std;

void Protocol::RequestList() { //запрос на список файлов
    Frame f;
    f.msg_id = MsgId::GETLIST;
    conn_->WriteFrame(&f);
}

void Protocol::SendFileList(const FileList& list) { //отправить список файлов
    Frame f;
    f.msg_id = MsgId::FILELIST;
    std::stringstream ss;
    boost::archive::text_oarchive archive(ss);
    archive << list;
    f.body = ss.str();

    conn_->WriteFrame(&f);
}


void Protocol::SendOk() { //отправить ОК
    Frame f;
    f.msg_id = MsgId::OK;
    f.body = "";
    conn_->WriteFrame(&f);
}


MsgId Protocol::RecvMsg() { //получить msg
    conn_->ReadFrame(&last_received_);
    return last_received_.msg_id;
}

FileList Protocol::GetFileList() { //получить список файлов из фрейма
    std::stringstream ss;
    ss << last_received_.body;
    FileList list;
    boost::archive::text_iarchive archive(ss);
    archive >> list;
    return list;
}

void read_all(int fd, char* buf, size_t size) {
    int s = 0;
    while (int w = read(fd, buf + s, size)) {
        size -= w;
        s += w;
    }
}

void write_all(int fd, char* buf, size_t size) {
    int s = 0;
    while (int w = write(fd, buf + s, size)) {
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
    struct dirent *cur;
    while ((cur=readdir(dir))!=NULL) {
        string tmp(cur->d_name);
        if ((strcmp(cur->d_name, ".") != 0) && (strcmp(cur->d_name, "..") != 0))
            F.files.push_back(tmp);
        }                
    closedir(dir);
    return F; 
}

void rsync(string source, string dest) {
    int sockets[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
    SocketConnection sender(sockets[0]), receiver(sockets[1]);
    FileList list_dest, list_source;
    Protocol senrer_prot(&sender);
    Protocol receiver_prot(&receiver);
    thread t1([&] { 
        senrer_prot.RequestList();
        senrer_prot.RecvMsg();
        list_source = GetFiles(source);
    });
    thread t2([&] { 
        MsgId i = receiver_prot.RecvMsg();
        list_dest = GetFiles(dest);
        receiver_prot.SendFileList(list_dest);
    });
    t1.join();
    t2.join();
}
    







