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

    header.type = ntohs(type);
    header.seqNum = ntohs(seqNum);
    header.length = ntohs(length);
    header.checksum = ntohs(checksum);

    return header;
}

void wReceiver::sendAck(int ackSeqNum) {
    spdlog::info(" ACK func");
    PacketHeader ack;

    ack.type = htons(3);
    ack.seqNum = htons(ackSeqNum);
    ack.length = htons(0);
    ack.checksum = htons(0);

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

    std::ofstream log;
    log.clear();
    log.open(outputFile, std::ios::app);
    spdlog::info("log about to ");
    log << ntohs(ack.type) << ' ' << ntohs(ack.seqNum) << ' ' << ntohs(ack.length) << ' ' << ntohs(ack.checksum) << '\n';
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
    std::ofstream log;
    log.clear();
    log.open(outputFile, std::ios::app);

    log << header.type << ' ' << header.seqNum << ' ' << header.length << ' ' << header.checksum << '\n';
    spdlog::info("Packet log: type={} seqNum={} length={} checksum={}",
             ntohs(header.type), ntohs(header.seqNum), ntohs(header.length), ntohs(header.checksum));

    spdlog::info("log in awaitStart");
    sendAck(seqNum);
    spdlog::info("done with ACK and start");

    connected = true;
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

std::vector<uint8_t> wReceiver::getData(size_t length) {
    auto data = std::vector<uint8_t>(length);

    size_t totaln = 0;
    do {
        size_t n = recvfrom(receiverfd, data.data() + totaln, length - totaln, 0, reinterpret_cast<sockaddr*>(&senderAddr), &senderAddrLen);
        totaln += n;
    } while(totaln < length);

    return data;
}

void wReceiver::adjustWindow() {
    std::ofstream ofs;
    ofs.clear();
    ofs.open(outputDir + "/FILE-" + std::to_string(connectionIdx) + ".out", std::ios::app);

    while (window.begin()->second.size() != 0) {
        ofs.write(reinterpret_cast<char *>(window.begin()->second.data()), window.begin()->second.size());
        window.pop_front();
        std::vector<uint8_t> m;
        window.push_back(std::make_pair(0, m));

        rightWindowBound++;
        leftWindowBound++;
        dataSeqNum++;
        
    }
}

void wReceiver::awaitDataPacket() {
    spdlog::info("Now await Data");
    PacketHeader header = getHeader();
    auto data = getData(header.length);

    if (header.type == 1 && header.seqNum == seqNum) {
        connectionIdx++;
        std::ofstream log;
        log.clear();
        log.open(outputFile, std::ios::app);
        log << header.type << ' ' << header.seqNum << ' ' << header.length << ' ' << header.checksum << '\n';
        
        sendAck(seqNum);
        leftWindowBound = 0;
        rightWindowBound = windowSize - 1;
        connected = false;
        dataSeqNum = 0;
    }

    if (header.type != 2) {
        spdlog::debug("Expecting data packet");
        return;
    }

    if (crc32(data.data(), data.size()) != header.checksum) {
        spdlog::debug("DATA PACKET IS CORRUPTED");
        return;
    }

    int index = findIndexInWindow(header.seqNum);
    if (index == -1) {
        spdlog::debug("No space in window - dropping packet");
        sendAck(dataSeqNum); // check this l8tr
        return;
    }    

    window[index] = std::make_pair(seqNum, data);
    
    adjustWindow();

    std::ofstream log;
    log.clear();
    log.open(outputFile, std::ios::app);

    log << header.type << ' ' << header.seqNum << ' ' << header.length << ' ' << header.checksum << '\n';

    sendAck(dataSeqNum);
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
