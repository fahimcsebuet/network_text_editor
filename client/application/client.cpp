#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <list>

#include <QDebug>

#include "client.h"

const unsigned MAXBUFLEN = 4096;
int client::sockfd = -1;
client* client::_client = NULL;

int client::init(std::string configuration_file_path)
{
    _client = this;
    response_received = false;
    this->configuration_file_path = configuration_file_path;
    configuration_file_handler _configuration_file_handler(configuration_file_path);
    _configuration_file_handler.load_configuration(configuration_map);
    response_from_server = {};
    return EXIT_SUCCESS;
}

int client::start()
{
    int rv, flag;
    struct addrinfo hints, *res, *ressave;
    pthread_t tid;

    const char * _servhost = 
        configuration_map.find(configuration_keys::server_host) != configuration_map.end() ? 
        configuration_map.find(configuration_keys::server_host)->second.c_str()
        : NULL;

    const char * _servport = configuration_map.find(configuration_keys::server_port) != configuration_map.end() ? 
        configuration_map.find(configuration_keys::server_port)->second.c_str()
        : NULL;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    if ((rv = getaddrinfo(_servhost, _servport, &hints, &res)) != 0)
    {
        std::cout << "getaddrinfo wrong: " << gai_strerror(rv) << std::endl;
        return EXIT_FAILURE;
    }

    ressave = res;
    flag = 0;
    do 
    {
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0) 
            continue;
        if (::connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
        {
            flag = 1;
            break;
        }
        close(sockfd);
    } while ((res = res->ai_next) != NULL);

    freeaddrinfo(ressave);

    if (flag == 0)
    {
        fprintf(stderr, "cannot connect\n");
        return EXIT_FAILURE;
    }

    pull_file("server_file");
    pthread_create(&tid, NULL, &process_connection, NULL);
    return EXIT_SUCCESS;
}

void client::pull_file(std::string filename)
{
    std::string _command_operator = "p";
    char _sentinel = -1;
    send_data_to_server(_command_operator + _sentinel + filename);
}

int client::send_data_to_server(std::string data)
{
    if(data.empty())
    {
        return EXIT_FAILURE;
    }
    if(sockfd != -1)
    {
        response_from_server.clear();
        response_received = false;
        write(sockfd, data.c_str(), data.length());
    }
    return EXIT_SUCCESS;
}

int client::_exit()
{
    emit pull_from_server_signal();
    if(sockfd != -1) close(sockfd);
    configuration_file_handler _configuration_file_handler(configuration_file_path);
    _configuration_file_handler.save_configuration(configuration_map);
    return EXIT_SUCCESS;
}

void * client::process_connection(void *arg)
{
    int n;
    char buf[MAXBUFLEN];
    pthread_detach(pthread_self());
    while (true)
    {
        n = read(sockfd, buf, MAXBUFLEN);
        if (n <= 0)
        {
            if (n == 0)
            {
                std::cout << "server closed" << std::endl;
            }
            else
            {
                std::cout << "something wrong" << std::endl;
            }
            close(sockfd);
            _client->_exit();
            return(NULL);
            //exit(1);
        }
        buf[n] = '\0';
        _client->handle_command_from_server(sockfd, std::string(buf));
    }
}

void client::handle_command_from_server(int in_sockfd, std::string in_command)
{
//    std::unique_lock<std::mutex> _response_lock(_client->response_mutex);
    char _sentinel = -1;
    _client->response_from_server = utility::split_string(in_command, _sentinel);

    std::cout << in_command << std::endl;
    if(_client->response_from_server.size() == 1)
    {
        emit pull_document_received_signal(QString::fromStdString(in_command));
        _client->response_received = true;
        emit pull_finished_signal(true);
        return;
    }
    if(_client->response_from_server.size() < 4)
    {
        //std::cout << "Bad Response from Server" << std::endl;
        _client->response_received = true;
        return;
    }

    std::string _command_operator = _client->response_from_server.at(0);
    std::string _command_message = _client->response_from_server.at(1);

    if(_command_operator == "ccr")
    {
        if(_command_message == "200")
        {
            qDebug() << QString::fromStdString(in_command);
            int _position = std::stoi(_client->response_from_server.at(2));
            qDebug() << _position;
            QString _changed_text = QString::fromStdString(_client->response_from_server.at(3));
            qDebug() << _changed_text;

            if(_changed_text.length() > 0 && _position != -1)
            {
                emit change_character_received_signal(_position, _changed_text);
            }
            _client->response_from_server.clear();
        }
    }
    else if(_command_operator == "pl")
    {
        if(_command_message == "200")
        {
            emit pull_from_server_signal();
        }
    }

//    _client->response_received = true;
//    _client->response_condition_variable.notify_all();
}

std::string client::get_fully_qualified_domain_name()
{
	std::string _fully_qualified_domain_name = "";
	struct addrinfo _hints, *_info, *_info_itr;
	int gai_result;

	char hostname[1024];
	hostname[1023] = '\0';
	gethostname(hostname, 1023);

	bzero(&_hints, sizeof(_hints));
	_hints.ai_family = AF_INET; /*either IPV4 or IPV6*/
	_hints.ai_socktype = SOCK_STREAM;
	_hints.ai_flags = AI_CANONNAME;

	if ((gai_result = getaddrinfo(hostname, "http", &_hints, &_info)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gai_result));
		return _fully_qualified_domain_name;
	}

	for(_info_itr = _info; _info_itr != NULL; _info_itr = _info_itr->ai_next)
	{
		_fully_qualified_domain_name += _info_itr->ai_canonname;
	}

	freeaddrinfo(_info);

	return _fully_qualified_domain_name;
}
