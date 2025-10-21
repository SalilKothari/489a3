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
#include <fstream>
#include "PacketHeader.hpp"
#include "Crc32.hpp"

#define PACKET_SIZE 1456

class wReceiver {
    public:
        wReceiver(int port, std::string outputDir, std::string out, int windowSize) 
        : port_(port), outputDir(outputDir), outputFile(out), windowSize(windowSize){}

        // Helpers
        PacketHeader getHeader();
        void sendAck(int ackSeqNum);
        int findIndexInWindow(size_t seqNum);
        void adjustWindow();
        std::vector<uint8_t> getData(size_t length);

        // General flow
        void initWindow();
        void initialize_listen_socket();
        void awaitStartPacket();
        void awaitDataPacket();
        void run();

    private:
        std::string outputDir;
        std::string outputFile;
        std::deque<std::pair<size_t, std::vector<uint8_t>>> window;
        sockaddr_in senderAddr{};
        socklen_t senderAddrLen = sizeof(senderAddr);
        int port_;
        int receiverfd;
        int windowSize;
        // size_t recievedSoFar = 0;
        size_t rightWindowBound;
        size_t leftWindowBound;
        size_t seqNum;
        size_t dataSeqNum = 0;
        size_t connectionIdx = 0;
        bool connected = false;
};

#endif
