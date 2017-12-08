#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <string>

#include "file_handler.h"

void utility::trim_string(std::string& splitted_command_string)
{
    size_t _left_trim_first = splitted_command_string.find_first_not_of(' ');
    if(_left_trim_first == std::string::npos)
    {
        splitted_command_string =  "";
        return;
    }

    size_t _right_trim_last = splitted_command_string.find_last_not_of(' ');
    splitted_command_string = splitted_command_string.substr(_left_trim_first, (_right_trim_last -
        _left_trim_first)+1);
}

std::vector<std::string> utility::split_string(const std::string& command_string, char delimiter)
{
    std::vector<std::string> _splitted_string_vector;

    std::stringstream _command_string_stream(command_string);
    std::string _splitted_string;

    while(std::getline(_command_string_stream, _splitted_string, delimiter))
    {
        //trim_string(_splitted_string);
        if(!_splitted_string.empty())
        {
            _splitted_string_vector.push_back(_splitted_string);
        }
    }

    return _splitted_string_vector;
}

int user_info_file_handler::load_user_info(std::unordered_map<std::string, user_info>& user_info_map)
{
    std::ifstream _user_info_file_stream(file_path);
    if(_user_info_file_stream.fail())
    {
        std::cout << "The user info file does not exist" << std::endl;
        return EXIT_FAILURE;
    }

    std::string _line = "";
    while(std::getline(_user_info_file_stream, _line))
    {
        std::string _user_name = "";
        std::string _password = "";
        std::vector<std::string> _contact_user_name_list;
        std::vector<std::string> _splitted_line = utility::split_string(_line, '|');
        if(_splitted_line.size() > 0)
        {
            _user_name = _splitted_line.at(0);
            if(_splitted_line.size() > 1)
            {
                _password = _splitted_line.at(1);
            }
            if(_splitted_line.size() > 2)
            {
                std::string _contacts_string = _splitted_line.at(2);
                _contact_user_name_list = utility::split_string(_contacts_string, ';');
            }
        }
        if(!_user_name.empty() && !_password.empty())
        {
            user_info _user_info(_user_name, _password, _contact_user_name_list);
            user_info_map[_user_name] = _user_info;
        }
    }

    _user_info_file_stream.close();
    return EXIT_SUCCESS;
}

int user_info_file_handler::save_user_info(std::unordered_map<std::string, user_info>& user_info_map)
{
    std::ofstream _user_info_file_stream(file_path);
    if(_user_info_file_stream.fail())
    {
        std::cout << "The user info file does not exist" << std::endl;
        return EXIT_FAILURE;
    }

    _user_info_file_stream.clear();
    if(user_info_map.empty())
    {
        _user_info_file_stream.close();
        return EXIT_SUCCESS;
    }

    std::unordered_map<std::string, user_info>::iterator _user_info_map_itr = user_info_map.begin();
    while(_user_info_map_itr != user_info_map.end())
    {
        std::string _username = _user_info_map_itr->first;
        user_info _user_info = _user_info_map_itr->second;
        std::string _password = _user_info.password;
        std::string _line_of_file = _username + "|" + _password;
        std::vector<std::string>::iterator _contact_list_itr = _user_info.contact_user_name_list.begin();
        std::string _friend_user_names = "";

        while(_contact_list_itr != _user_info.contact_user_name_list.end())
        {
            if(_contact_list_itr == _user_info.contact_user_name_list.begin())
            {
                _line_of_file += "|";
            }
            _friend_user_names += *_contact_list_itr;
            if(_contact_list_itr != _user_info.contact_user_name_list.end() - 1)
            {
                _friend_user_names += ";";
            }

            _contact_list_itr++;
        }

        _line_of_file += (_friend_user_names + "\n");
        _user_info_file_stream.write(_line_of_file.c_str(), _line_of_file.length());
        _user_info_map_itr++;
    }

    _user_info_file_stream.close();
    return EXIT_SUCCESS;
}

int configuration_file_handler::load_configuration(std::unordered_map<std::string, std::string>& configuration_map)
{
    std::ifstream _configuration_file_stream(file_path);
    if(_configuration_file_stream.fail())
    {
        std::cout << "The configuration file does not exist" << std::endl;
        return EXIT_FAILURE;
    }

    std::string _line = "";
    while(std::getline(_configuration_file_stream, _line))
    {
        std::string _key = "";
        std::string _value = "";
        std::vector<std::string> _splitted_line = utility::split_string(_line, ':');
        if(_splitted_line.size() > 0)
        {
            _key = _splitted_line.at(0);
            if(_splitted_line.size() > 1)
            {
                _value = _splitted_line.at(1);
            }
        }
        if(!_key.empty() && !_value.empty())
        {
            configuration_map[_key] = _value;
        }
    }

    _configuration_file_stream.close();
    return EXIT_SUCCESS;
}

int configuration_file_handler::save_configuration(std::unordered_map<std::string, std::string>& configuration_map)
{
    std::ofstream _configuration_file_stream(file_path);
    if(_configuration_file_stream.fail())
    {
        std::cout << "The configuration file does not exist" << std::endl;
        return EXIT_FAILURE;
    }

    _configuration_file_stream.clear();
    if(configuration_map.empty())
    {
        _configuration_file_stream.close();
        return EXIT_SUCCESS;
    }

    std::unordered_map<std::string, std::string>::iterator _configuration_map_itr = configuration_map.begin();
    while(_configuration_map_itr != configuration_map.end())
    {
        std::string _key = _configuration_map_itr->first;
        std::string _value = _configuration_map_itr->second;
        std::string _line_of_file = _key + ":" + _value + "\n";
        _configuration_file_stream.write(_line_of_file.c_str(), _line_of_file.length());
        _configuration_map_itr++;
    }

    _configuration_file_stream.close();
    return EXIT_SUCCESS;
}
