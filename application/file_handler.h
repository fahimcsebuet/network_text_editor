#ifndef CLIENT_FILE_HANDLER_H_
#define CLIENT_FILE_HANDLER_H_

#include <string>
#include <unordered_map>
#include <vector>

namespace configuration_keys
{
    const std::string server_host = "servhost";
    const std::string server_port = "servport";
}

class utility
{
public:
    static std::vector<std::string> split_string(const std::string& command_string, char delimiter);
    static void trim_string(std::string& splitted_command_string);
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
