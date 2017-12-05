#include <iostream>
#include <stdlib.h>

#include "server.h"

// Take the path to user_info_file and configuration_file
int main(int argc, char **argv)
{
    if(argc != 3)
    {
        std::cout << "Invalid number of parameters!" << std::endl;
        return EXIT_FAILURE;
    }
    std::string _user_info_file(argv[1]);
    std::string _configuration_file(argv[2]);
    server _server;
    _server.init(_user_info_file, _configuration_file);
    _server.run();
    _server._exit();
    return EXIT_SUCCESS;
}
