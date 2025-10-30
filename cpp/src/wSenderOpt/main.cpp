#include <arpa/inet.h>       // inet_ntoa
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>        // FD_SET, FD_ISSET, FD_ZERO macros
#include <unistd.h>          // close
#include <cerrno>
#include <csignal>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <cxxopts.hpp>
#include <spdlog/spdlog.h>
#include <cctype>
#include "wSenderOpt.hpp"


static void verifyPort(const int port) {
    if (port < 1024 || port > 0xFFFF) {
        spdlog::error("Error: port must be in the range of [1024, 65535]\n");
        std::exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {

    /*
    wSender should be invoked as follows:

        ./wSender -h 127.0.0.1 -p 8000 -w 10 -i input.in -o sender.out

        -h | --hostname The IP address of the host that wReceiver is running on.
        -p | --port The port number on which wReceiver is listening.
        -w | --window-size Maximum number of outstanding packets in the current window.
        -i | --input-file Path to the file that has to be transferred. It can be a text file or binary file (e.g., image or video).
        -o | --output-log The file path to which you should log the messages as described above.
    */

    cxxopts::Options options("miProxy", "Adaptive bitrate selection through an HTTP proxy server and load balancing");
    options.add_options()
        ("h,hostname", "The IP address of the host that wReceiver is running on", cxxopts::value<std::string>())
        ("p,port", "The port number on which wReceiver is listening", cxxopts::value<int>())
        ("w,window-size", "Maximum number of outstanding packets in the current window", cxxopts::value<int>())
        ("i,input-file", "Path to the file that has to be transferred. It can be a text file or binary file (e.g., image or video)", cxxopts::value<std::string>())
        ("o,output-log", "The file path to which you should log the messages as described above", cxxopts::value<std::string>());
        
    auto result = options.parse(argc, argv);

    // // check if we need to print something
    const std::string hostname = result["hostname"].as<std::string>();
    const auto port = result["port"].as<int>();
    const auto windowSize = result["window-size"].as<int>();
    const std::string inputFile = result["input-file"].as<std::string>();
    const std::string outputFile = result["output-log"].as<std::string>();

    verifyPort(port);
    
    wSender sender(hostname, port, inputFile, outputFile, windowSize);
    sender.run();
    
    return 0;
}
