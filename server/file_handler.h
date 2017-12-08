#ifndef SERVER_FILE_HANDLER_H_
#define SERVER_FILE_HANDLER_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "data_structures.h"

namespace configuration_keys
{
    const std::string port = "port";
    const std::string db_path = "db_path";
}

class utility
{
public:
    static std::vector<std::string> split_string(const std::string& command_string, char delimiter);
    static void trim_string(std::string& splitted_command_string);
};

class user_info_file_handler
{
public:
    user_info_file_handler(std::string file_path)
    {
        this->file_path = file_path;
    }

    int load_user_info(std::unordered_map<std::string, user_info>& user_info_map);
    int save_user_info(std::unordered_map<std::string, user_info>& user_info_map);

private:
    std::string file_path;
};

class configuration_file_handler
{
public:
    configuration_file_handler(std::string file_path)
    {
        this->file_path = file_path;
    }

    int load_configuration(std::unordered_map<std::string, std::string>& configuration_map);
    int save_configuration(std::unordered_map<std::string, std::string>& configuration_map);

private:
    std::string file_path;
};

#endif
