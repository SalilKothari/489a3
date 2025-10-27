#include "wSender.hpp"
#include <arpa/inet.h>       // inet_ntoa
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>        // FD_SET, FD_ISSET, FD_ZERO macros
#include <unistd.h>          // close
#include <cstdlib>
#include <fstream>
#include <spdlog/spdlog.h>
#include <cassert>
#include <filesystem>
#include "Crc32.hpp"


void wSender::initialize_listen_socket(){
    senderfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (senderfd < 0) {
        spdlog::error("Error in creating server socket");
        std::exit(EXIT_FAILURE);
    }

    // reuse of address
    int yes = 1;
    if (setsockopt(senderfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        spdlog::error("Error in setsockopt");
        std::exit(EXIT_FAILURE);
    }

    // create & init the server address
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(SERVER_PORT);

    recAddr.sin_family = AF_INET;
    recAddr.sin_addr.s_addr = INADDR_ANY;
    recAddr.sin_port = htons(SERVER_PORT);

    struct hostent *hp;
    hp = gethostbyname(recHostname.c_str());
    memcpy((char *) &recAddr.sin_addr, (char *) hp->h_addr, hp->h_length);

    recAddrLen = sizeof(recAddr);

    // bind the server to listen socket
    if (bind(senderfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        spdlog::error("Error in binding socket");
        std::exit(EXIT_FAILURE);
    }
}

PacketHeader wSender::getHeader() {
    PacketHeader header;

    char buff[sizeof(PacketHeader)];
    size_t totaln = 0;
    do {
        size_t n = recvfrom(recfd_, buff + totaln, sizeof(buff) - totaln, 0, reinterpret_cast<sockaddr*>(&recAddr), &recAddrLen);
        totaln += n;
    } while(totaln < sizeof(PacketHeader));

    uint32_t type, seqNum, length, checksum;

    memcpy(&type, buff, 4);
    memcpy(&seqNum, buff + 4, 4);
    memcpy(&length, buff + 8, 4);
    memcpy(&checksum, buff + 12, 4);

    header.type = ntohs(type);
    header.seqNum = ntohs(seqNum);
    header.length = ntohs(length);
    header.checksum = ntohs(checksum);

    return header;
}

static uint32_t getSeqNum() {
    srand(time(NULL));

    return rand();
}

void wSender::logToFile(PacketHeader& header, bool reverse) {
    std::ofstream ofs;
    ofs.clear();
    ofs.open(outputFile, std::ios::app);

    if (!reverse) {
        ofs << header.type << ' ' << header.seqNum << ' ' << header.length << ' ' << header.checksum << '\n';
        return;
    }

    ofs << ntohs(header.type) << ' ' << ntohs(header.seqNum) << ' ' << ntohs(header.length) << ' ' << ntohs(header.checksum) << '\n';
}


void wSender::sendAwaitPacket(PacketHeader &startHeader, char * startBuf, size_t len){
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(recfd_, &readfds);

    // Await ACK for start packet

    ssize_t totalSent = 0;
                do {
                    ssize_t n = sendto(recfd_, startBuf, len, 0, reinterpret_cast<sockaddr*>(&recAddr), recAddrLen);
                    totalSent += n;
                } while (totalSent < len);
    
    send:
        for (;;) {

            auto startTimeout = std::chrono::high_resolution_clock::now();

            logToFile(startHeader, true);

            auto timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - startTimeout
            );

            int ready = select(recfd_ + 1, &readfds, nullptr, nullptr, nullptr);

            if (ready > 0) break;

            if (timeElapsed.count() >= 500) {
                ssize_t totalSent = 0;
                do {
                    ssize_t n = sendto(recfd_, startBuf, len, 0, reinterpret_cast<sockaddr*>(&recAddr), recAddrLen);
                    totalSent += n;
                } while (totalSent < len);

                startTimeout = std::chrono::high_resolution_clock::now();
            }
        }

    PacketHeader ackHeader = getHeader();
    if(ackHeader.type != 3){
        spdlog::error("Received non ack packet");
        goto send;
    }
}


void wSender::sendPacket(PacketHeader &startHeader, char * startBuf, size_t len, int dataSeq){
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(recfd_, &readfds);

    // Await ACK for start packet

    ssize_t totalSent = 0;
        do {
            ssize_t n = sendto(recfd_, startBuf, len, 0, reinterpret_cast<sockaddr*>(&recAddr), recAddrLen);
            totalSent += n;
        } while (totalSent < len);
    
     logToFile(startHeader, true);
     sendWindow.emplace_back(std::make_pair(dataSeq, std::chrono::high_resolution_clock::now()));
}

void wSender::sendStartPacket() {
    PacketHeader startHeader;
    seqNum = getSeqNum();

    startHeader.type = htons(3);
    startHeader.length = htons(3);
    startHeader.seqNum = htons(seqNum);
    startHeader.checksum = htons(0);

    char startBuf[sizeof(PacketHeader)];
    memcpy(startBuf, &startHeader.type, 4);
    memcpy(startBuf + 4, &startHeader.seqNum, 4);
    memcpy(startBuf + 8, &startHeader.length, 4);
    memcpy(startBuf + 12, &startHeader.checksum, 4);

    ssize_t totalSent = 0;
    do {
        ssize_t n = sendto(recfd_, startBuf, sizeof(startBuf), 0, reinterpret_cast<sockaddr*>(&recAddr), recAddrLen);
        totalSent += n;
    } while (totalSent < sizeof(startBuf));

    sendAwaitPacket(startHeader, startBuf, 16);

}

void wSender::sendData(){


    std::ifstream ifs;
    ifs.clear();
    ifs.open(inputFile, std::ios::binary);

    //1472 for a packet = Header + data

    std::vector<char> buf;
    buf.resize(1472);

    size_t totalRead = 0;
    size_t totalFileSize = std::filesystem::file_size(inputFile);
    int dataSeq = 0;


    while(true){


        if(totalRead >= totalFileSize) break;

        if(sendWindow.size() < windowSize){
            //send a packet
            size_t len;
            //extract data first
            if(totalFileSize - totalRead < 1456 ){
                len = totalFileSize - totalRead;
                ifs.read(buf.data() + 16, len);

            }
            else{
                ifs.read(buf.data() + 16, 1456);
                len = 1456;
            }

            //Create packetHeader
            PacketHeader data;
            data.type = htons(2);
            data.length = htons(len);
            data.seqNum = htons(dataSeq);
            data.checksum = htons(crc32(buf.data(), len) );

            memcpy(buf.data(), &data.type, 4);
            memcpy(buf.data() + 4, &data.seqNum, 4);
            memcpy(buf.data() + 8, &data.length, 4);
            memcpy(buf.data() + 12, &data.checksum, 4);

            //send 
            sendPacket(data, buf.data(), len, dataSeq);
        }
        

        //TODO: Implement await
        // if()

        // if we receive an ack, pop from deque
        // wait 500ms, if no ack - retransmit


    }



    

}



void wSender::closeServer(){
    close(senderfd);    
    close(recfd_);
}
