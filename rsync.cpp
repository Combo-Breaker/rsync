#pragma once

#include <iostream>
#include <fstream>
#include <unistd.h> 
#include <sys/types.h> 
#include <fcntl.h> 
#include <cstdlib>
#include <sys/socket.h>
#include <vector>
#include <thread>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

using namespace std;


void SocketConnection::WriteFrame (Frame* frame) {
    
    stringstream archive;
    boost::archive::text_oarchive oa(archive);
    char length[4];
    sprintf(length, "%04u", (int) frame->body.length()); //not this length
    archive << length << static_cast<int> (frame->msg_id)  << frame;
    const string tmp  = archive.str();
    const char* buf = tmp.c_str();
    int w = write(fd_, buf, tmp.length()); 
}


void SocketConnection::ReadFrame(Frame* frame) {
    //  0-4 -> size
    //  5  -> id
    //  5-(5+size) -> body

    char length[4];
    int r = read(fd_, length, 4); //fd - which one
    length[4] = '\0';
    int len = atoi(length);

    char msg_id[1];
    r = read(fd_, msg_id, 1);
    msg_id[1] = '\0';
    int id = atoi(msg_id);
    frame->msg_id = (MsgId) id;

    char body[len];
    r = read(fd_, body, len);
    stringstream temp(body);
    boost::archive::text_iarchive ia(temp);
    ia >> frame->body; 

}







