#include "wReceiver.hpp"

void wReceiver::initialize_listen_socket() {
    receiverfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (receiverfd < 0) {
        spdlog::error("Error in creating server socket");
        std::exit(EXIT_FAILURE);
    }

    // reuse of address
    int yes = 1;
    if (setsockopt(receiverfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        spdlog::error("Error in setsockopt");
        std::exit(EXIT_FAILURE);
    }

    // create & init the server address
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    // bind the server to listen socket
    if (bind(receiverfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        spdlog::error("Error in binding socket");
        std::exit(EXIT_FAILURE);
    }
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ipStr, sizeof(ipStr));
    spdlog::info("Receiver bound to {}:{}", ipStr, ntohs(addr.sin_port));

    spdlog::info("finished socket stuff");
}

PacketHeader wReceiver::getHeader() {
    PacketHeader header;
    spdlog::info("in func");

    char buff[sizeof(PacketHeader)];
    size_t totaln = 0;
    do {
        size_t n = recvfrom(receiverfd, buff + totaln, sizeof(buff) - totaln, 0, reinterpret_cast<sockaddr*>(&senderAddr), &senderAddrLen);
        totaln += n;
    } while(totaln < sizeof(PacketHeader));
    spdlog::info("RECIEVED");

    uint32_t type, seqNum, length, checksum;

    memcpy(&type, buff, 4);
    memcpy(&seqNum, buff + 4, 4);
    memcpy(&length, buff + 8, 4);
    memcpy(&checksum, buff + 12, 4);

    header.type = ntohl(type);
    header.seqNum = ntohl(seqNum);
    header.length = ntohl(length);
    header.checksum = ntohl(checksum);
    spdlog::info("Header parsed:");
    spdlog::info("  type     = {}", header.type);
    spdlog::info("  seqNum   = {}", header.seqNum);
    spdlog::info("  length   = {}", header.length);
    spdlog::info("  checksum = {:#010x}", header.checksum); // hex formatting

    return header;
}

PacketData wReceiver::getPacket() {
    PacketData packet;
    std::vector<uint8_t> buffer(1472); // header & packet

    ssize_t totalReceived = recvfrom(receiverfd, buffer.data(), buffer.size(), 0, reinterpret_cast<sockaddr*>(&senderAddr), &senderAddrLen);

    if (totalReceived < 16) {
        spdlog::error("Received packet too small: {} bytes", totalReceived);
    }

    uint32_t type, seqNum, length, checksum;
    memcpy(&type, buffer.data(), 4);
    memcpy(&seqNum, buffer.data() + 4, 4);
    memcpy(&length, buffer.data() + 8, 4);
    memcpy(&checksum, buffer.data() + 12, 4);

    packet.header.type = ntohl(type);
    packet.header.seqNum = ntohl(seqNum);
    packet.header.length = ntohl(length);
    packet.header.checksum = ntohl(checksum);

    spdlog::info("Header parsed:");
    spdlog::info("  type     = {}", packet.header.type);
    spdlog::info("  seqNum   = {}", packet.header.seqNum);
    spdlog::info("  length   = {}", packet.header.length);
    spdlog::info("  checksum = {:#010x}", packet.header.checksum); // hex formatting

    if (packet.header.length > 0) {
        if (totalReceived < 16 + static_cast<ssize_t>(packet.header.length)) {
            spdlog::error("Packet truncated: expected {} bytes, got {}", 
                         16 + packet.header.length, totalReceived);
            packet.header.type = 999; // Invalid
            return packet;
        }
        
        packet.payload.resize(packet.header.length);
        memcpy(packet.payload.data(), buffer.data() + 16, packet.header.length);
        
        // Log payload
        spdlog::info("Payload (hex):");
        std::string hexStr;
        for (auto b : packet.payload) {
            char hex[4];
            snprintf(hex, sizeof(hex), "%02x ", b);
            hexStr += hex;
        }
        spdlog::info("{}", hexStr);
        
        std::string strPayload;
        for (auto b : packet.payload) {
            if (isprint(b)) {
                strPayload += static_cast<char>(b);
            } else {
                strPayload += '.';
            }
        }
        spdlog::info("Payload (as printable string): {}", strPayload);
    }
    
    return packet;

}


void wReceiver::sendAck(int ackSeqNum) {
    spdlog::info(" ACK func");
    PacketHeader ack;

    ack.type = htonl(3);
    ack.seqNum = htonl(ackSeqNum);
    ack.length = htonl(0);
    ack.checksum = htonl(0);

    uint8_t ackBuf[sizeof(PacketHeader)];
    memcpy(ackBuf, &ack.type, 4);
    memcpy(ackBuf + 4, &ack.seqNum, 4);
    memcpy(ackBuf + 8, &ack.length, 4);
    memcpy(ackBuf + 12, &ack.checksum, 4);

    ssize_t totalSent = 0;
    do {
        ssize_t n = sendto(receiverfd, ackBuf, sizeof(ackBuf), 0, reinterpret_cast<sockaddr*>(&senderAddr), senderAddrLen);
        totalSent += n;
    } while (totalSent < sizeof(ackBuf));
    spdlog::info(" sent ACK");

    std::ofstream log(outputFile, std::ios::app);
    // log.clear();
    // log.open(outputFile, std::ios::app);
    spdlog::info("log about to ");
    log << ntohl(ack.type) << ' ' << ntohl(ack.seqNum) << ' ' << ntohl(ack.length) << ' ' << ntohl(ack.checksum) << '\n';
    log.close();
}

void wReceiver::awaitStartPacket() {
    // if (connected) {
    //     spdlog::error("Connection called when already connected");
    //     return;
    // }
    spdlog::info("start packet");
    PacketHeader header = getHeader();
    spdlog::info("got header");
    if (header.type != 0) {
        spdlog::error("Failed to receive initial start packet!");
        std::exit(EXIT_FAILURE);
    }

    seqNum = header.seqNum;

    if (header.length != 0) {
        spdlog::error("Start packet must have length of 0");
        std::exit(EXIT_FAILURE);
    }
    spdlog::info("HELLO");
    spdlog::info("Output log file: {}", outputFile);
    std::ofstream log(outputFile, std::ios::app);
    // log.clear();
    // log.open(outputFile, std::ios::app);
    if (!log.is_open()) {
        spdlog::error("Failed to open log file: {}", outputFile);
    }

    log << header.type << ' ' << header.seqNum << ' ' << header.length << ' ' << header.checksum << '\n';
    log.close();
    spdlog::info("Packet log: type={} seqNum={} length={} checksum={}",
             header.type, header.seqNum, header.length, header.checksum);

    spdlog::info("log in awaitStart");
    sendAck(seqNum);
    spdlog::info("done with ACK and start");

    connected = true;
    outputFileStream.open(outputDir + "/FILE-" + std::to_string(connectionIdx) + ".out", 
                          std::ios::trunc | std::ios::binary);
    if (!outputFileStream.is_open()) {
        spdlog::error("Failed to open output file");
    }

}

void wReceiver::initWindow() {
    window.resize(windowSize);
    leftWindowBound = 0;
    rightWindowBound = windowSize - 1;
}

int wReceiver::findIndexInWindow(size_t seqNum) {
    if (leftWindowBound > seqNum || seqNum > rightWindowBound) {
        spdlog::debug("Drop packet");
        return -1;
    }

    return seqNum - leftWindowBound;
} 

// std::vector<uint8_t> wReceiver::getData(size_t length) {
//     spdlog::info("get data length: {}", length);
//     auto data = std::vector<uint8_t>(length);

//     size_t totaln = 0;
//     do {
//         size_t n = recvfrom(receiverfd, data.data(), length, 0, reinterpret_cast<sockaddr*>(&senderAddr), &senderAddrLen);
//         totaln += n;
//     } while(totaln < length);
//     spdlog::info("data: {}", std::string(data.begin(), data.end()));
//     return data;
// }

void wReceiver::adjustWindow() {
    // std::ofstream ofs(outputDir + "/FILE-" + std::to_string(connectionIdx) + ".out", std::ios::app);
    // ofs.clear();
    // ofs.open(outputDir + "/FILE-" + std::to_string(connectionIdx) + ".out", std::ios::app);

    while (!window.empty() && window.begin()->second.size() != 0) {
        outputFileStream.write(reinterpret_cast<char *>(window.begin()->second.data()), window.begin()->second.size());
        window.pop_front();
        std::vector<uint8_t> m;
        window.push_back(std::make_pair(0, m));

        rightWindowBound++;
        leftWindowBound++;
        dataSeqNum++;
        
    }
    outputFileStream.flush();
}

void wReceiver::awaitDataPacket() {
    spdlog::info("Now await Data");
    // PacketHeader header = getHeader();
    // auto data = getData(header.length);
    PacketData packet = getPacket();

    spdlog::info("after get data");
    if (packet.header.type == 1 && packet.header.seqNum == seqNum) {
        spdlog::info("in end packet");
        outputFileStream.close();
        connectionIdx++;
        std::ofstream log(outputFile, std::ios::app);
        // log.clear();
        // log.open(outputFile, std::ios::app);
        log << packet.header.type << ' ' << packet.header.seqNum << ' ' << packet.header.length << ' ' << packet.header.checksum << '\n';
        log.close();

        sendAck(seqNum);

        leftWindowBound = 0;
        rightWindowBound = windowSize - 1;
        connected = false;
        dataSeqNum = 0;
        return;
    }

    if (packet.header.type != 2) {
        spdlog::info("oh no");
        spdlog::debug("Expecting data packet");
        return;
    }

    if (packet.payload.size() > 0) {
        if (crc32(packet.payload.data(), packet.payload.size()) != packet.header.checksum) {
            spdlog::info("2");
            spdlog::debug("DATA PACKET IS CORRUPTED");
            return;
        }
    }

    int index = findIndexInWindow(packet.header.seqNum);
    spdlog::info("index: {}", index);
    
    if (index == -1) {
        spdlog::debug("No space in window - dropping packet");
        if (packet.header.seqNum < leftWindowBound) {
            spdlog::debug("Received old packet seqNum={}, sending ACK", packet.header.seqNum);
            sendAck(dataSeqNum);
        } else {
            // case that its greater
            spdlog::debug("Packet seqNum={} is too far ahead, dropping without ACK", packet.header.seqNum);
        }
        // sendAck(dataSeqNum); // check this l8tr
        return;
    }    


    window[index] = std::make_pair(packet.header.seqNum, packet.payload);
    spdlog::info("before adjsust window");
    adjustWindow();
    spdlog::info("after adjsust window");
    std::ofstream log(outputFile, std::ios::app);
    // log.clear();
    // log.open(outputFile, std::ios::app);

    log << packet.header.type << ' ' << packet.header.seqNum << ' ' << packet.header.length << ' ' << packet.header.checksum << '\n';
    log.close();

    spdlog::info("after data ack log & send ack");
    sendAck(dataSeqNum);
    spdlog::info("after data send ack");
}

void wReceiver::run() {
    initWindow();
    initialize_listen_socket();

    while (true) {
        if (!connected) {
            awaitStartPacket();
        }
        else {
            awaitDataPacket();
        }
    }
}
