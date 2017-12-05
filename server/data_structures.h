#ifndef SERVER_DATA_STRUCTURES_H_
#define SERVER_DATA_STRUCTURES_H_

#include <string>
#include <vector>
#include <unordered_map>

class user_info
{
public:
    user_info(std::string user_name, std::string password, 
        std::vector<std::string>& contact_user_name_list)
    {
        this->user_name = user_name;
        this->password = password;
        this->contact_user_name_list = contact_user_name_list;
        this->potential_friends_list = {};
        this->ip = "";
        this->port = -1;
        this->is_logged_in = false;
        this->sockfd = -1;
    }
    user_info()
    {
        this->user_name = "";
        this->password = "";
        this->contact_user_name_list = {};
        this->potential_friends_list = {};
        this->ip = "";
        this->port = -1;
        this->is_logged_in = false;
        this->sockfd = -1;
    }

    std::string user_name;
    std::string password;
    std::vector<std::string> contact_user_name_list;
    std::vector<std::string> potential_friends_list;
    std::string ip;
    int port;
    bool is_logged_in;
    int sockfd;
};

class configuration_info
{
public:
    configuration_info(int port)
    {
        this->port = port;
    }
private:
    int port;
};

#endif
