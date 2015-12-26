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
};

class Connection {
public:
    virtual ~Connection() {}
    virtual void WriteFrame(Frame* frame) = 0;
    virtual void ReadFrame(Frame* frame) = 0;
};

class SocketConnection : public Connection {
public:
    SocketConnection(int fd) : fd_(fd) {}
//    ~SocketConnection();

    virtual void WriteFrame(Frame* frame);
    virtual void ReadFrame(Frame* frame);

private:
    int fd_;
};

struct FileList {
    std::vector<std::string> files;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & files;
    }
};

class Protocol {
public:
    Protocol(Connection* conn) : conn_(conn) {}

    void RequestList();
    void SendOk();
    void SendFileList(const FileList& list);

    MsgId RecvMsg(); 
    FileList GetFileList();
protected:
    Connection* conn_;
    Frame last_received_;
};

class Sender : public Protocol  {
public:
    Sender(SocketConnection* conn, std::string source, std::string dest) : Protocol(conn) {
        source_ = source;
        dest_ = dest;
    }
    void Launch(std::string s);
private:
    std::string source_;
    std::string dest_;
    
};

class Receiver : public Protocol {
public:
    Receiver(SocketConnection* conn) : Protocol(conn) {
        dest_ = "";
    }
    void Launch(std::string s);
private:
    std::string dest_;    
};

