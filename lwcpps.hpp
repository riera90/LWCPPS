#ifndef _LWCPPS_H_
#define _LWCPPS_H_


#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <time.h>


#define LWCPPS_BUFF_SIZE 2048
#define LWCPPS_DEFAULT_PORT 7657
#define LWCPPS_DEFAULT_TIMEOUT 60

/**
 * type definition of the handler function
 * All the handlers must follow this type
 */
typedef std::string (*handlerType)(const std::string&);

/**
 * This struct stores the necessary information for the thread to read, process and respond to the client request
 */
struct serverConnection {
    int socket_fd = -1;
    struct sockaddr client_sock_addr{};
    socklen_t len{};
};

/**
 * This struct holds the necessary parameters for the clientListenThread function of the client
 * the server socket and a pointer to the future server message
 */
struct clientListenThreadParams {
    int socket;
    std::string* payload;
};

/**
 * Threaded function where the request will be process, in this function the following is done:
 * - The message is read from the file descriptor
 * - The message is passed to the handler
 * - the response of the handler is send back to the client
 * @param handler handler function
 * @param client server connection struct necessary for sending the response as this thread will not join the main
 * process
 */
void handleNextServerConnectionThreaded(handlerType handler, serverConnection client);

/**
 * Threaded function for listening to the server
 * @param params listenThreadParams struct
 * @return nothing
 */
void* clientListenThread(void* params);

/**
 * A very simple generic TCP server
 * This server does not saves the client sessions, when a request is made, the server will dispatch it via the handler
 * specified in the constructor and one done, the socket will be closed
 *
 * Each request is handled in a dedicated detached threads, the execution here is fully sequential, while the main
 * thread listens for the next request.
 */
class LWServer {
private:
    int server_fd_;
    sockaddr_in server_sock_addr_{};
    std::vector<std::thread> threads_;

public:
    handlerType handler_;

    /**
     * Class constructor
     * @param requestHandler handler function
     * @param portno port where the server will be listening on all addresses
     */
    LWServer(handlerType requestHandler, int portno = LWCPPS_DEFAULT_PORT, int sock = SOCK_STREAM, int afInet = AF_INET);

    /**
     * Serial part of the request process
     * this function will accept the connection and launch a thread to do the rest of processing
     */
    void handleNextConnection();

    /**
     * Closes the server socket
     */
    void exit();
};


/**
 * This simple TCP client to send and receive data from a TCP server
 * This server implements listen timeout, so it makes use of the pthread lib
 */
class LWClient {
private:
    int socket_fd_;
    sockaddr_in server_dir_{};
    socklen_t server_dir_len_;
public:
    /**
     * Class constructor
     * @param addr ipv4 address of the server
     * @param portno server port
     */
    LWClient(const std::string& addr, int portno = LWCPPS_DEFAULT_PORT, int sock = SOCK_STREAM, int afInet = AF_INET);

    /**
     * Message sender
     * @param payload the message that will be send to the server
     * @return error code, same as the send function
     */
    int sendMessage(const std::string& payload);

    /**
     * Timeout enable listen function
     * @param timeout time in seconds
     * @return server payload
     */
    std::string listen(int timeout = LWCPPS_DEFAULT_TIMEOUT);

    /**
     * Timeout enable listen and exit function
     * @param timeout time in seconds
     * @return server payload
     */
    std::string listenAndExit(int timeout = LWCPPS_DEFAULT_TIMEOUT);

    /**
     * closes the client socket
     */
    void exit();
};

#endif