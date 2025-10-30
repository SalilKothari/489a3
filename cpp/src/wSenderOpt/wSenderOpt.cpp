#include "wSenderOpt.hpp"
#include <arpa/inet.h>       // inet_ntoa
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>        // FD_SET, FD_ISSET, FD_ZERO macros
#include <unistd.h>          // close
#include <cstdlib>
#include <fstream>
#include <spdlog/spdlog.h>
#include <filesystem>
#include "Crc32.hpp"


void wSender::initialize_listen_socket() {
    recfd_ = socket(AF_INET, SOCK_DGRAM, 0);

    if (recfd_ < 0) {
        spdlog::error("Error in creating server socket");
        std::exit(EXIT_FAILURE);
    }

    // reuse of address
    int yes = 1;
    if (setsockopt(recfd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        spdlog::error("Error in setsockopt");
        std::exit(EXIT_FAILURE);
    }

    recAddr.sin_family = AF_INET;
    inet_pton(AF_INET, recHostname.c_str(), &recAddr.sin_addr);
    recAddr.sin_port = htons(port_);
    recAddrLen = sizeof(recAddr);

    // struct hostent *hp;
    // hp = gethostbyname(recHostname.c_str());
    // memcpy((char *) &recAddr.sin_addr, (char *) hp->h_addr, hp->h_length);

    sockaddr_in senderAddr{};
    senderAddr.sin_family = AF_INET;
    senderAddr.sin_addr.s_addr = INADDR_ANY;
    senderAddr.sin_port = htons(8080);

    // bind the server to listen socket
    if (bind(recfd_, reinterpret_cast<sockaddr*>(&senderAddr), sizeof(senderAddr)) < 0) {
        spdlog::error("Error in binding socket");
        std::exit(EXIT_FAILURE);
    }
}

PacketHeader wSender::getHeader() {
    PacketHeader header;

    char buff[sizeof(PacketHeader)];
    size_t totaln = 0;
    do {
      size_t n =
          recvfrom(recfd_, buff + totaln, sizeof(buff) - totaln, 0,
                   reinterpret_cast<sockaddr *>(&recAddr), &recAddrLen);
      totaln += n;
    } while (totaln < sizeof(PacketHeader));

    uint32_t type, seqNum, length, checksum;

    memcpy(&type, buff, 4);
    memcpy(&seqNum, buff + 4, 4);
    memcpy(&length, buff + 8, 4);
    memcpy(&checksum, buff + 12, 4);

    header.type = ntohl(type);
    header.seqNum = ntohl(seqNum);
    header.length = ntohl(length);
    header.checksum = ntohl(checksum);

    return header;
}

static uint32_t getSeqNum() {
    srand(time(NULL));
    return rand();
}

void wSender::logToFile(PacketHeader& header, bool reverse) {
    std::ofstream ofs(outputFile, std::ios::app);
    // ofs.clear();
    // ofs.open(outputFile, std::ios::app);

    if (!reverse) {
        ofs << header.type << ' ' << header.seqNum << ' ' << header.length << ' ' << header.checksum << '\n';
        return;
    }

    ofs << ntohl(header.type) << ' ' << ntohl(header.seqNum) << ' ' << ntohl(header.length) << ' ' << ntohl(header.checksum) << '\n';
}


void wSender::sendAwaitPacket(PacketHeader &startHeader, char * startBuf, size_t len){
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(recfd_, &readfds);

    // Await ACK for start packet
    spdlog::info("start buf");
    spdlog::info(startBuf);
    spdlog::info(startHeader.seqNum);

    ssize_t totalSent = 0;
    do {
        ssize_t n = sendto(recfd_, startBuf, len, 0, reinterpret_cast<sockaddr*>(&recAddr), recAddrLen);
        totalSent += n;
    } while (totalSent < len);
    spdlog::info("sent buf");    
    
    send:
        for (;;) {
            auto startTimeout = std::chrono::high_resolution_clock::now();

            logToFile(startHeader, true); // TODO: logToFile
            spdlog::info("after log to file");

            auto timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - startTimeout
            );

            int ready = select(recfd_ + 1, &readfds, nullptr, nullptr, nullptr); // TODO: may block 
            spdlog::info("ready chose");
            if (ready > 0) break;

            if (timeElapsed.count() >= 500) {
                spdlog::info("timed out");
                ssize_t totalSent = 0;
                do {
                    ssize_t n = sendto(recfd_, startBuf, len, 0, reinterpret_cast<sockaddr*>(&recAddr), recAddrLen);
                    totalSent += n;
                } while (totalSent < len);

                startTimeout = std::chrono::high_resolution_clock::now();
            }
        }

    PacketHeader ackHeader = getHeader();
    spdlog::info("got header");
    if (ackHeader.type != 3) {
        spdlog::error("Received non ack packet");
        goto send;
    }
}


void wSender::sendStartPacket() {
    PacketHeader startHeader;
    seqNum = getSeqNum();

    startHeader.type = htonl(0);
    startHeader.length = htonl(0);
    startHeader.seqNum = htonl(seqNum);
    startHeader.checksum = htonl(0);

    char startBuf[sizeof(PacketHeader)];
    memcpy(startBuf, &startHeader.type, 4);
    memcpy(startBuf + 4, &startHeader.seqNum, 4);
    memcpy(startBuf + 8, &startHeader.length, 4);
    memcpy(startBuf + 12, &startHeader.checksum, 4);


    spdlog::info("Before Await Packet");
    sendAwaitPacket(startHeader, startBuf, 16);
}

void wSender::sendPacket(PacketHeader &startHeader, char * startBuf, size_t len, int dataSeq){
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(recfd_, &readfds);

    // Await ACK for start packet
    // spdlog::info("Sending packet seqNum={} length={}", dataSeq, len);
    // spdlog::info("Payload (as string): {}", std::string(startBuf + 16, startBuf + 16 + len));
    
    ssize_t totalSent = 0;
    do {
        ssize_t n = sendto(recfd_, startBuf, len, 0, reinterpret_cast<sockaddr*>(&recAddr), recAddrLen);
        totalSent += n;
    } while (totalSent < len);
    spdlog::info("sent to reciever");
    
    logToFile(startHeader, true);

    windowItem item;
    item.seqNum = dataSeq;
    item.startTime = std::chrono::high_resolution_clock::now();
    item.data.resize(len);
    memcpy(item.data.data(), startBuf, len);
    spdlog::info("doene");

    sendWindow.emplace_back(item);
}

void wSender::sendData() {
    spdlog::info("will start sending data");
    std::ifstream ifs(inputFile, std::ios::binary);
    // ifs.clear();
    // ifs.open(inputFile, std::ios::binary);
    spdlog::info("opened input file");

    // 1472 for a packet = Header + data
    std::vector<char> buf;
    buf.resize(1472);

    size_t totalRead = 0;
    size_t totalFileSize = std::filesystem::file_size(inputFile);
    int dataSeq = 0;
    spdlog::info("random check here");

    while (totalRead < totalFileSize || !sendWindow.empty()) {
        // spdlog::info("Loop start: totalRead = {}, window size = {}", totalRead, sendWindow.size());

        spdlog::info("enter first while");
        while (sendWindow.size() < static_cast<size_t>(windowSize) && totalRead < totalFileSize) {      
            spdlog::info("enter 2nd while");     
            size_t len;
            //extract data first
            if (totalFileSize - totalRead < 1456) {
                spdlog::info("less than 1456");
                len = totalFileSize - totalRead;
                ifs.read(buf.data() + 16, len);
                spdlog::info("Read last chunk size: {}", len);
            }
            else {
                ifs.read(buf.data() + 16, 1456);
                len = 1456;
                spdlog::info("larger than 1456");
            }

            PacketHeader data;
            data.type = htonl(2);
            data.length = htonl(len);
            data.seqNum = htonl(dataSeq);
            data.checksum = htonl(crc32(buf.data() + 16, len)); // i think? bc its over data

            memcpy(buf.data(), &data.type, 4);
            memcpy(buf.data() + 4, &data.seqNum, 4);
            memcpy(buf.data() + 8, &data.length, 4);
            memcpy(buf.data() + 12, &data.checksum, 4);
            spdlog::info("done making header");
            sendPacket(data, buf.data(), len + 16, dataSeq);
            spdlog::info("sent packet already");

            dataSeq++;
            totalRead += len;
        }
        spdlog::info("exit while");

        // if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - sendWindow.front().startTime) >= std::chrono::milliseconds(500)) {
        //     spdlog::info("timeout case");
        //     for (auto &item : sendWindow) {
        //         ssize_t totalSent = 0;
        //         do {
        //             ssize_t n = sendto(recfd_, item.data.data(), item.data.size(), 0, reinterpret_cast<sockaddr*>(&recAddr), recAddrLen);
        //             totalSent += n;
        //         } while (totalSent < item.data.size());
        //         item.startTime = std::chrono::high_resolution_clock::now();
        //     }
        // }

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(recfd_, &readfds);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 5000;

        int await = select(recfd_ + 1, &readfds, nullptr, nullptr, &timeout);
        if (await < 0) {
            spdlog::error("Select returned 0!");
            break;
        }

        if (await > 0 && FD_ISSET(recfd_, &readfds)) {
            spdlog::info("await >0");
            PacketHeader ackPacket = getHeader();

            if (ackPacket.type != 3) {
                spdlog::error("Did not recieve ACK");
                continue;
            }

            if (sendWindow.empty()) {
                spdlog::warn("Received ACK but sendWindow is empty");
                continue;
            }

            spdlog::info(
                "ACK received for seqNum: {} (expecting next packet {})",
                ackPacket.seqNum - 1, ackPacket.seqNum);
            spdlog::info("SendWindow before popping: size={}, front seqNum={}, "
                         "back seqNum={}",
                         sendWindow.size(), sendWindow.front().seqNum,
                         sendWindow.back().seqNum);

            for (auto& packet : sendWindow) {
                if (ackPacket.seqNum == packet.seqNum && !packet.acked) {
                    packet.acked = true;
                }
            }
        }

        for (auto& packet : sendWindow) {
            if (packet.acked) {
                continue;
            }

            auto timeElapsed =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::high_resolution_clock::now() - packet.startTime);

            if (timeElapsed.count() >= 500) {
                // Retransmit packet
                PacketHeader header;
                packet.data.data();
                memcpy(&header.type, packet.data.data(), 4);
                memcpy(&header.seqNum, packet.data.data() + 4, 4);
                memcpy(&header.length, packet.data.data() + 8, 4);
                memcpy(&header.checksum, packet.data.data() + 12, 4);

                sendPacket(header, packet.data.data(), packet.data.size(),
                           packet.seqNum);

                packet.startTime = std::chrono::high_resolution_clock::now();
            }
        }

        while (!sendWindow.empty() && sendWindow.front().acked) {
            sendWindow.pop_front();
        }

    }
        /*
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;

        int await = select(recfd_ + 1, &readfds, nullptr, nullptr, &timeout);
        if (await < 0) {
            spdlog::error("Select returned 0!");
            break;
        }

        if (await > 0 && FD_ISSET(recfd_, &readfds)) {
            spdlog::info("await >0");
            PacketHeader ackPacket = getHeader();

            if (ackPacket.type != 3) {
                spdlog::error("Did not recieve ACK");
                continue;
            }

            // if (ackPacket.seqNum > sendWindow.back().seqNum) {
            //     spdlog::error("Did not recieve correct number");
            //     continue;
            // }

            // spdlog::info("SendWindow size: {}", sendWindow.size());
            if (sendWindow.empty()) {
                spdlog::warn("Received ACK but sendWindow is empty");
                continue;
            }

            // if (ackPacket.seqNum > sendWindow.back().seqNum) {
            //     spdlog::error("ACK seqNum {} beyond last sent seqNum {}", ackPacket.seqNum, sendWindow.back().seqNum);
            //     continue;
            // }

            spdlog::info("ACK received for seqNum: {} (expecting next packet {})", 
                         ackPacket.seqNum - 1, ackPacket.seqNum);
            spdlog::info("SendWindow before popping: size={}, front seqNum={}, back seqNum={}", 
                         sendWindow.size(), sendWindow.front().seqNum, sendWindow.back().seqNum);

            while (!sendWindow.empty() &&
                sendWindow.front().seqNum < ackPacket.seqNum) {
                spdlog::info("Removing acknowledged packet seqNum={}",
                             sendWindow.front().seqNum);
                sendWindow.pop_front();
            }
            spdlog::info("SendWindow after popping: size={}", sendWindow.size());

            if (!sendWindow.empty()) {
                sendWindow.front().startTime =
                    std::chrono::high_resolution_clock::now();
            }

        } else if (await == 0) {
            spdlog::info(
                "Timeout waiting for ACK; retransmitting unacknowledged packets");
            for (auto &item : sendWindow) {
                ssize_t totalSent = 0;
                do {
                  ssize_t n = sendto(recfd_, item.data.data() + totalSent,
                                     item.data.size() - totalSent, 0,
                                     reinterpret_cast<sockaddr *>(&recAddr),
                                     recAddrLen);
                  if (n < 0) {
                    spdlog::info("ERROR");
                    }
                    totalSent += n;
                } while (totalSent < static_cast<ssize_t>(item.data.size()));
                item.startTime = std::chrono::high_resolution_clock::now();
                spdlog::info("Retransmitted packet seqNum: {}", item.seqNum);
            }

        }
        spdlog::info("total Read and file size");
        spdlog::info(totalRead);
        spdlog::info(totalFileSize);
    }
    */

    ifs.close();
    spdlog::info("Finished sending all data and receiving ACKs");
}

void wSender::sendEndPacket() {
    if (!sendWindow.empty()) {
        spdlog::error("sliding window should be empty");
        return;
    }

    PacketHeader endPacket;
    endPacket.type = htonl(1); 
    endPacket.length = htonl(0);
    endPacket.seqNum = htonl(seqNum);
    endPacket.checksum = htonl(0);

    char endBuf[sizeof(PacketHeader)];
    memcpy(endBuf, &endPacket.type, 4);
    memcpy(endBuf + 4, &endPacket.seqNum, 4);
    memcpy(endBuf + 8, &endPacket.length, 4);
    memcpy(endBuf + 12, &endPacket.checksum, 4);

    sendAwaitPacket(endPacket, endBuf, 16);
}

void wSender::run() {
    initialize_listen_socket();
    spdlog::info("Successfully init socket");
    sendStartPacket();
    spdlog::info("Sent start packet");
    sendData();
    spdlog::info("Sent data packet");
    sendEndPacket();
    spdlog::info("Sent end packet");
    closeServer();
    spdlog::info("Closed server");
}


void wSender::closeServer() {
    close(recfd_);
}
