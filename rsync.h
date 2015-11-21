#pragma once

#include <vector>
#include <string>

#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

enum class MsgId {
    GETLIST = 1,
    FILELIST = 2,
    REMOVE = 3,
    FILE = 4,
    FILE_PART = 5,
    OK = 6
};

struct Frame {
    MsgId msg_id;
    std::string body;
    void serialize(Archive & ar, const unsigned int version) {
         ar & msg_id;
         ar & body;
    }
};

class Connection {
public:
    virtual ~Connection() {}
    virtual void WriteFrame(Frame* frame) {};
    virtual void ReadFrame(Frame* frame) {};
};


class SocketConnection {
public:
    SocketConnection(int fd) : fd_(fd) {}
//    ~SocketConnection();

    virtual void WriteFrame(Frame* frame) {}
    virtual void ReadFrame(Frame* frame) {}

private:
    int fd_;
};


struct FileList {
    std::vector<std::string> files;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
         ar & files;
    }
};


class Protocol {
public:
    Protocol(Connection* conn) : conn_(conn) {}

    void SendGetList();
    void SendFileList(const FileList& list);

    MsgId RecvMsg(); // reads next frame
    FileList GetFileList();

private:
    Connection* conn_;
    Frame last_received_;
};



