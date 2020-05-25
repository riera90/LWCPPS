#include <lwcpps>

void handleNextServerConnectionThreaded(handlerType handler, serverConnection client) {
    char buffer[LWCPPS_BUFF_SIZE];
    std::string payload;

    // read the message from the file descriptor until the terminating character appears
    do{
        bzero(buffer, LWCPPS_BUFF_SIZE);
        read(client.socket_fd, buffer, LWCPPS_BUFF_SIZE);
        payload.append(buffer);
    } while (buffer[strlen(buffer)-1]!='>');

    #ifdef DEBUG
        fprintf(stderr, "LWServer::handleNextServerConnectionThreaded: received {%s}%lu\n", payload.c_str(), strlen(payload.c_str()));
    #endif
    // process the message via the handler
    std::string response = handler(payload);

    // send back the response
    send(
            client.socket_fd,
            response.c_str(),
            strlen(response.c_str()),
            0
    );
    #ifdef DEBUG
        fprintf(stderr, "LWServer::handleNextServerConnectionThreaded: sending back {%s}:%lu\n", response.c_str(), strlen(response.c_str()));
    #endif
    // close the client socket
    close(client.socket_fd);
}

LWServer::LWServer(handlerType requestHandler, int portno, int sock, int afInet) {
    int retval;
    this->handler_ = requestHandler;

    // clear and sets the server address
    bzero((char*)&this->server_sock_addr_, sizeof(server_sock_addr_));
    this->server_sock_addr_.sin_port = htons(portno);
    this->server_sock_addr_.sin_family = afInet;
    this->server_sock_addr_.sin_addr.s_addr = htonl(INADDR_ANY);

    // open the socket
    this->server_fd_ = socket(afInet, sock, 0);
    if (this->server_fd_ < 0)
        throw std::runtime_error("LWServer::LWServer: ERROR, can not open socket");

    // bind the socket to the port
    retval = bind(
            this->server_fd_,
            (const sockaddr*) &this->server_sock_addr_,
            sizeof(this->server_sock_addr_)
    );
    if (retval < 0)
        throw std::runtime_error("LWServer::LWServer: ERROR, can not bind to socket");

    // set a queue of 5 to listen in at the binded port
    retval = listen(this->server_fd_, 5); // 5 is the maximum size permitted by most systems
    if (retval == -1)
        throw std::runtime_error("LWServer::LWServer: ERROR, can not set socket to listen on port");
}


void LWServer::handleNextConnection() {
    serverConnection client;
    // accepts the request from the client
    client.socket_fd = accept(
            this->server_fd_,
            (sockaddr*) &client.client_sock_addr,
            &client.len
    );
    if (client.socket_fd == -1) {
        return;
    }
    // launches the thread and detaches it from the main process
    std::thread t(handleNextServerConnectionThreaded, this->handler_, client);
    t.detach();
}

void LWServer::exit() {
    close(this->server_fd_);
    #ifdef DEBUG
        fprintf(stderr, "LWServer::exit:  server closed\n");
    #endif
}


void *clientListenThread(void* params) {
    auto* thread_params = (struct clientListenThreadParams*) params;
    char buffer[LWCPPS_BUFF_SIZE];
    // read the message from the file descriptor until the terminating character appears
    do{
        bzero(buffer, LWCPPS_BUFF_SIZE);
        read(thread_params->socket, buffer, LWCPPS_BUFF_SIZE);
        thread_params->payload->append(buffer);
    } while (buffer[strlen(buffer)-1]!='>');
    // exit the thread and join the main process
    pthread_exit(nullptr);
}

LWClient::LWClient(const std::string& addr, int portno, int sock, int afInet) {
    this->socket_fd_ = socket(afInet, sock, 0);
    // TODO: if error

    this->server_dir_.sin_family = afInet;
    this->server_dir_.sin_addr.s_addr = inet_addr(addr.c_str());
    this->server_dir_.sin_port = htons(portno);

    this->server_dir_len_ = sizeof(this->server_dir_);

    {
        int ret_val;
        ret_val = connect(
                this->socket_fd_,
                (sockaddr*) &this->server_dir_,
                this->server_dir_len_
        );
        if (ret_val == -1){
            fprintf(stderr, "LWClient::LWClient: ERROR,Server cant be found.");
        }
    }
}

int LWClient::sendMessage(const std::string& payload) {
    #ifdef DEBUG
        fprintf(stderr, "LWClient::sendMessage: sending {%s}%lu\n", payload.c_str(), strlen(payload.c_str()));
    #endif
    return send(
            this->socket_fd_,
            payload.c_str(),
            strlen(payload.c_str()),
            0
    );
}

std::string LWClient::listen(int timeout) {
    int retval;
    struct timespec timeout_{};
    std::string payload;
    pthread_t thread;
    clientListenThreadParams params{
            .socket = this->socket_fd_,
            .payload = &payload
    };
    // creates the thread and sets a timed out thread cancel
    pthread_create(&thread, nullptr, clientListenThread, (void *) &params);
    timeout_.tv_nsec = 0;
    timeout_.tv_sec = ((int)time(nullptr)) + timeout;
    retval = pthread_timedjoin_np(thread, nullptr, &timeout_);
    #ifdef DEBUG
        fprintf(stderr, "LWClient::listen: received {%s}%lu\n", payload.c_str(), strlen(payload.c_str()));
    #endif
    // if the thread has joined in time, copy the response into the payload at the struct
    if (retval == 0)
        return payload;
    // thread has failed to join, force the exit at the thread and copy a timeout error at the payload
    pthread_cancel(thread);
    #ifdef DEBUG
        fprintf(stderr, "LWClient::listen: connection timed out\n");
    #endif
    return std::string("-timeout<can not get message from orchestrator>");
}

std::string LWClient::listenAndExit(int timeout) {
    std::string retval = this->listen(timeout);
    this->exit();
    return retval;
}

void LWClient::exit() {
    close(this->socket_fd_);
}