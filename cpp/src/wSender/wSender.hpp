#ifndef WSERVER_HPP
#define WSERVER_HPP

#include <string>
#define SERVER_PORT 3000

class wSender {
    public:
        wSender(std::string recHostname, int port, std::string in, std::string out, int windowSize) 
        : recHostname(recHostname), port_(port), inputFile(in), outputFile(out), windowSize(windowSize){}

        void initialize_listen_socket();

        void closeServer();


    private:
        std::string recHostname;
        int port_;
        int recfd_;
        int serverfd;
        std::string inputFile;
        std::string outputFile;
        int windowSize;

};




#endif
