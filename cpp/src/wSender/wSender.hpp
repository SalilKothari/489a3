#ifndef WSERVER_HPP
#define WSERVER_HPP

#include <string>
#include <netdb.h>
#include <chrono>
#include "PacketHeader.hpp"
#define SERVER_PORT 3000

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

        void closeServer();


    private:
        std::string recHostname;
        int port_;
        int recfd_;
        int senderfd;
        uint32_t seqNum;
        std::string inputFile;
        std::string outputFile;
        int windowSize;
        sockaddr_in recAddr{};
        socklen_t recAddrLen{};
};




#endif
