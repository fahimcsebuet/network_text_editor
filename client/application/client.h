#ifndef CLIENT_CLIENT_H_
#define CLIENT_CLIENT_H_

#include <string>
#include <unordered_map>
#include <condition_variable>
#include <mutex>
#include <atomic>

#include <QObject>

#include "file_handler.h"

class friend_info
{
public:
    friend_info(std::string user_name, std::string ip, int port)
    {
        this->user_name = user_name;
        this->ip = ip;
        this->port = port;
        this->connected = false;
        this->sockfd = -1;
    }
    friend_info()
    {
        this->user_name = "";
        this->ip = "";
        this->port = -1;
        this->connected = false;
        this->sockfd = -1;
    }

    std::string user_name;
    std::string ip;
    int port;
    bool connected;
    int sockfd;
};

class client : public QObject
{
    Q_OBJECT
public:
    client(){}
    int init(std::string configuration_file_path);
    int start();
    void pull_file(std::string filename);
    int send_data_to_server(std::string data);
    int _exit();
    std::vector<std::string> get_response_from_server()
    {
        return response_from_server;
    }
signals:
    void change_character_received_signal(int position, QString text);
    void pull_document_received_signal(QString text);
    void pull_from_server_signal();
    void pull_finished_signal(bool finished);
private:
    std::string file_content;
    std::string configuration_file_path;
    std::unordered_map<std::string, std::string> configuration_map;
    bool response_received;
    std::mutex response_mutex;
    std::condition_variable response_condition_variable;
    std::vector<std::string> response_from_server;
    static int sockfd;
    static void * process_connection(void *arg);
    static client *_client;
    void handle_command_from_server(int in_sockfd, std::string in_command);
    std::string get_fully_qualified_domain_name();
};

#endif
