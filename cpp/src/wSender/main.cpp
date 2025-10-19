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



int main(int argc, char *argv[]) {
    // cxxopts::Options options("miProxy", "Adaptive bitrate selection through an HTTP proxy server and load balancing");
    // options.add_options()
    //     ("b,balance", "Load balancing should occur", cxxopts::value<bool>())
    //     ("l,listen-port", "TCP port proxy listens on for browser connections", cxxopts::value<int>())
    //     ("h,host", "IP address of the video server", cxxopts::value<std::string>())
    //     ("p,port", "Port number", cxxopts::value<int>())
    //     ("a,alpha", "Alpha coefficient for EWMA throughput estimate", cxxopts::value<double>());
        
    // auto result = options.parse(argc, argv);

    // // check if we need to print something
    // const auto listenPort = result["listen-port"].as<int>();
    // const auto port = result["port"].as<int>();
    // const std::string hostname = result["host"].as<std::string>();
    // const auto alpha = result["alpha"].as<double>();
    // const bool balanceMode = result.count("balance") > 0;

    // verifyPort(listenPort);
    // verifyPort(port);

    // if (alpha < 0 || alpha > 1) {
    //     spdlog::error("Alpha must be in the range [0, 1]\n");
    //     std::exit(EXIT_FAILURE);
    // }


    return 0;
}
