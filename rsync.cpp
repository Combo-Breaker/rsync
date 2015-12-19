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
        if (w == -1) throw ("Can`t read the frame");
        size -= w;
        s += w;
    }
}

void write_all(int fd, char* buf, size_t size) {
    int s = 0;
    while (int w = write(fd, buf + s, size)) {
        if (w == -1) throw ("Can`t write the frame");
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
    if (dir == NULL) throw ("Can`t open the directory");
    struct dirent *cur;
    while ((cur = readdir(dir)) != NULL) {
        string tmp(cur->d_name);
        if ((strcmp(cur->d_name, ".") != 0) && (strcmp(cur->d_name, "..") != 0))
            F.files.push_back(tmp);
        }                
    int d = closedir(dir);
    if (d == -1) throw("Can`t close the directory");
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


void rsync(string source, string dest) {
    int sock[2];    
    sock[0] = socket(AF_INET, SOCK_STREAM, 0);
    sock[1] = socket(AF_INET, SOCK_STREAM, 0);
    SocketConnection sender(sock[0]), receiver(sock[1]);
    FileList list_dest, list_source;
    Protocol senrer_prot(&sender);
    Protocol receiver_prot(&receiver);

    thread client([&] { // sender - source
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(3425);
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); //тут пока замыкание на себя
         if (connect(sock[0], (struct sockaddr *)&addr, sizeof(addr)) < 0)
            throw ("Can`t connect to the server");
        
        senrer_prot.RequestList();
        senrer_prot.RecvMsg();
        list_source = GetFiles(source);
        pair < vector<string>, vector<string> > diff;
        diff = difference(list_source.files, senrer_prot.GetFileList().files);

        /* ВЫВОД РАЗНОСТИ */
        for (auto i : diff.first)
            cout << "cp" << " " << i << endl; 
        for (auto i : diff.second)
            cout << "rm" << " " << i << endl; 


        senrer_prot.SendOk(); 
        close(sock[0]);        
    });


    thread server([&] { // reciever - dest
        int listener;
        struct sockaddr_in addr;

        listener = socket(AF_INET, SOCK_STREAM, 0);
        addr.sin_family = AF_INET;
        addr.sin_port = htons(3425);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        int t = sizeof(addr);
        if (bind(listener, (struct sockaddr *)&addr, t) < 0) 
            throw("Bind error"); // переписать
        while(1) {
            sock[1] = accept(listener, NULL, NULL);
            MsgId i = receiver_prot.RecvMsg();
            switch (i) {
                case MsgId::OK:
                   close(sock[1]);
                   break; // нужно ли?
                case MsgId::GETLIST:
                    list_dest = GetFiles(dest);
                    receiver_prot.SendFileList(list_dest); 
            }
        }       
    });
    client.join();
    server.join();    
}





