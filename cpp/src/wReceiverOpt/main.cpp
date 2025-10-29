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
#include "wReceiverOpt.hpp"

static void verifyPort(const int port) {
    if (port < 1024 || port > 0xFFFF) {
        spdlog::error("Error: port must be in the range of [1024, 65535]\n");
        std::exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    cxxopts::Options options("miProxy", "Adaptive bitrate selection through an HTTP proxy server and load balancing");
    options.add_options()
        ("p,port", "The port number on which wReceiver is listening", cxxopts::value<int>())
        ("w,window-size", "Maximum number of outstanding packets in the current window", cxxopts::value<int>())
        ("d,output-dir", "The directory that the wReceiver will store the output files, i.e the FILE-i.out files.", cxxopts::value<std::string>())
        ("o,output-log", "The file path to which you should log the messages as described above", cxxopts::value<std::string>());
        
    auto result = options.parse(argc, argv);

    // // check if we need to print something
    const auto port = result["port"].as<int>();
    const auto windowSize = result["window-size"].as<int>();
    const std::string outputDir = result["output-dir"].as<std::string>();
    const std::string outputFile = result["output-log"].as<std::string>();

    verifyPort(port);
    wReceiver rcvr(port, outputDir, outputFile, windowSize);
    rcvr.run();
    
    return 0;
}
