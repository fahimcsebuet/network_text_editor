#ifndef SERVER_SERVER_H_
#define SERVER_SERVER_H_

#include <string>
#include <unordered_map>

#include "file_handler.h"

class server
{
public:
    int init(std::string user_info_file_path, std::string configuration_file_path);
    int run();
    int _exit();

private:
    
	int cli_sockfd;
    std::string user_info_file_path;
    std::string configuration_file_path;
    std::unordered_map<std::string, user_info> user_info_map;
    std::unordered_map<std::string, std::string> configuration_map;
    std::unordered_map<int, std::string> sockfd_to_username;

    std::string get_fully_qualified_domain_name();
    int get_port_from_configuration_map();
    void handle_command_from_client(int sockfd, std::vector<std::string> parsed_command);
    void send_data_to_client(int sockfd, std::string command, std::string data);
    void send_location_info_to_clients(std::string username);
    void send_logout_info_to_clients(std::string username);
    static void sigint_handler(int signal);
    static server *_server;
    void download_file_net(std::string path);
    void download_file_net1();
};

#endif
