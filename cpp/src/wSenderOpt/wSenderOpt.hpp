#ifndef WSERVEROPT_HPP
#define WSERVEROPT_HPP

#include <string>
#include <netdb.h>
#include <deque>
#include <chrono>
#include "PacketHeader.hpp"
#define SERVER_PORT 3000

struct windowItem {
    int seqNum;
    std::chrono::high_resolution_clock::time_point startTime;
    std::vector<char> data;
    bool acked = false;
};

class wSender {
    public:
        wSender(std::string recHostname, int port, std::string in, std::string out, int windowSize) 
        : recHostname(recHostname), port_(port), inputFile(in), outputFile(out), windowSize(windowSize){}
        // General
        PacketHeader getHeader();
        void logToFile(PacketHeader& header, bool reverse);

        // Flow
        void initialize_listen_socket();
        void sendStartPacket();
        void sendData();
        void sendAwaitPacket(PacketHeader &startHeader, char * startBuf, size_t len);
        void sendPacket(PacketHeader &startHeader, char * startBuf, size_t len, int dataSeq);
        void sendEndPacket();
        void closeServer();
        void run();

    private:
        std::string recHostname;
        int port_;
        int recfd_;
        uint32_t seqNum;
        std::string inputFile;
        std::string outputFile;
        int windowSize;
        sockaddr_in recAddr{};
        socklen_t recAddrLen{};
        std::deque<windowItem> sendWindow;
};

#endif
