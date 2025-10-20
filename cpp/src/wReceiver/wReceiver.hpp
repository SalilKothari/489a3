#ifndef WRECEIVER_H
#define WRECEIVER_H

#include <arpa/inet.h>       // inet_ntoa
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>        // FD_SET, FD_ISSET, FD_ZERO macros
#include <unistd.h>          // close
#include <string>
#include <cxxopts.hpp>
#include <spdlog/spdlog.h>
#include <cctype>
#include <deque>
#include "PacketHeader.hpp"

#define PACKET_SIZE 1456

class wReceiver {
    public:
        wReceiver(std::string recHostname, int port, std::string outputDir, std::string out, int windowSize) 
        : port_(port), outputDir(outputDir), outputFile(out), windowSize(windowSize){}

        // Helpers
        PacketHeader getHeader();
        void sendAck(int ackSeqNum);

        // General flow
        void initWindow();
        void initialize_listen_socket();
        void awaitStartPacket();
        void closeServer();
    private:
        std::string outputDir;
        std::string outputFile;
        std::deque<std::pair<size_t, uint8_t*>> window;
        sockaddr_in senderAddr{};
        socklen_t senderAddrLen = sizeof(senderAddr);
        int port_;
        int receiverfd;
        int windowSize;
        size_t seqNum;
        size_t dataSeqNum = 0;
        size_t connectionIdx = 0;
        bool connected = false;
};

#endif
