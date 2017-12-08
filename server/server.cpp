#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <algorithm>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fstream>

#include "server.h"

const unsigned MAXBUFLEN = 4096;
server *server::_server = NULL;

int server::init(std::string user_info_file_path, std::string configuration_file_path)
{
	build_directory = "/home/fahim/Documents/FSU/Courses/COP5570/network_text_editor/server/output/";
	_server = this;
	signal(SIGINT, sigint_handler);
	// handle user info file
	this->user_info_file_path = user_info_file_path;
	user_info_file_handler _user_info_file_handler(user_info_file_path);
	_user_info_file_handler.load_user_info(user_info_map);

	// handle the configuration file
	this->configuration_file_path = configuration_file_path;
	configuration_file_handler _configuration_file_handler(configuration_file_path);
	_configuration_file_handler.load_configuration(configuration_map);
	return EXIT_SUCCESS;
}

int server::run()
{
	int _port = get_port_from_configuration_map();
	int serv_sockfd; // cli_sockfd;
	struct sockaddr_in serv_addr, cli_addr;
	socklen_t sock_len;
	ssize_t _buf_size;
	char buf[MAXBUFLEN];
	fd_set readfds, masters;
	int maxfd;

	serv_sockfd = socket(PF_INET, SOCK_STREAM, 0);

	bzero((void *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(_port);

	bind(serv_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

	listen(serv_sockfd, 5);

	sock_len = sizeof(cli_addr);
	getsockname(serv_sockfd, (struct sockaddr *)&serv_addr, &sock_len);
	std::cout << "Fully Qualified Domain Name = " << get_fully_qualified_domain_name() << std::endl;
	std::cout << "Port = " << ntohs(serv_addr.sin_port) << std::endl;

	FD_ZERO(&masters);
	FD_SET(serv_sockfd, &masters);
	maxfd = serv_sockfd;
	sockfds.push_back(serv_sockfd);

	for (;;)
	{
		readfds = masters;
		select(maxfd + 1, &readfds, NULL, NULL, NULL);

		std::list<int> l_tmp(sockfds);
		std::list<int>::iterator itr;
		for (std::list<int>::iterator itr = l_tmp.begin(); itr !=
														   l_tmp.end();
			 ++itr)
		{
			int sock_tmp = *itr;

			if (FD_ISSET(sock_tmp, &readfds))
			{
				if (sock_tmp == serv_sockfd)
				{
					// new connection request
					cli_sockfd = accept(serv_sockfd, (struct sockaddr *)&cli_addr, &sock_len);
					std::cout << "New connection accepted" << std::endl;
					FD_SET(cli_sockfd, &masters);
					if (cli_sockfd > maxfd)
						maxfd = cli_sockfd;
					sockfds.push_back(cli_sockfd);
				}
				else
				{
					// data message
					_buf_size = read(sock_tmp, buf, MAXBUFLEN);
					if (_buf_size <= 0)
					{
						if (_buf_size == 0)
							std::cout << "connection closed" << std::endl;
						else
							perror("something wrong");

						std::unordered_map<int, std::string>::iterator _sockfd_to_username_itr =
							sockfd_to_username.find(sock_tmp);
						if (_sockfd_to_username_itr != sockfd_to_username.end())
						{
							std::unordered_map<std::string, user_info>::iterator _user_info_itr =
								user_info_map.find(_sockfd_to_username_itr->second);
							if (_user_info_itr != user_info_map.end())
							{
								_user_info_itr->second.is_logged_in = false;
								_user_info_itr->second.ip = "";
								_user_info_itr->second.port = -1;
								_user_info_itr->second.sockfd = -1;
							}
						}

						close(sock_tmp);
						FD_CLR(sock_tmp, &masters);
						sockfds.remove(sock_tmp);
					}
					else
					{
						buf[_buf_size] = '\0';
						std::string _buffer = std::string(buf);
						char _sentinel = -1;
						std::vector<std::string> _parsed_command = utility::split_string(_buffer, _sentinel);
						std::cout << _buffer << std::endl;
						handle_command_from_client(sock_tmp, _parsed_command);
					}
				}
			}
		}
	}
	close(serv_sockfd);
	return EXIT_SUCCESS;
}

int server::_exit()
{
	user_info_file_handler _user_info_file_handler(user_info_file_path);
	_user_info_file_handler.save_user_info(user_info_map);

	configuration_file_handler _configuration_file_handler(configuration_file_path);
	_configuration_file_handler.save_configuration(configuration_map);
	return EXIT_SUCCESS;
}

std::string server::get_fully_qualified_domain_name()
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

	for (_info_itr = _info; _info_itr != NULL; _info_itr = _info_itr->ai_next)
	{
		_fully_qualified_domain_name += _info_itr->ai_canonname;
	}

	freeaddrinfo(_info);

	return _fully_qualified_domain_name;
}

int server::get_port_from_configuration_map()
{
	int _port = 0;
	std::unordered_map<std::string, std::string>::iterator _config_map_itr =
		configuration_map.find(configuration_keys::port);

	if (_config_map_itr == configuration_map.end())
	{
		return _port;
	}
	else
	{
		return std::stoi(_config_map_itr->second);
	}
}

void server::handle_command_from_client(int sockfd, std::vector<std::string> parsed_command)
{
	char _sentinel = -1;
	if (parsed_command.empty())
	{
		std::string _error_message = "Invalid Request";
		send_data_to_client(sockfd, "no", _error_message);
		return;
	}
	std::string _command_operator = parsed_command.at(0);

	if(_command_operator == "cc")
	{
		std::string _data_to_client = "200";
		std::vector<std::string>::iterator _parsed_command_itr = parsed_command.begin();
		_parsed_command_itr++;
		while(_parsed_command_itr != parsed_command.end())
		{
			_data_to_client += _sentinel;
			_data_to_client += *_parsed_command_itr;
			_parsed_command_itr++;
		}

		broad_cast_data_to_other_clients(sockfd, "ccr", _data_to_client);
	}
	else if (_command_operator == "p")
	{
		std::string _filename = build_directory;//get_current_directory() + "/";
		if(parsed_command.size() > 1)
		{
			_filename += parsed_command.at(1);
		}

		std::cout << _filename << std::endl;
		// OPEN file, read file, stream, send stream to client
		int fd;					  /* file descriptor for file to send */
		struct stat stat_buf;	 /* argument to fstat */
		off_t offset = 0;		  /* file offset */
		fd = open(_filename.c_str(), O_RDONLY);
		if (fd == -1)
		{
			fprintf(stderr, "unable to open '%s': %s\n", _filename.c_str(), strerror(errno));
			return;
		}

		/* get the size of the file to be sent */
		fstat(fd, &stat_buf);

		/* copy file using sendfile */
		int rc;
		offset = 0;
		rc = sendfile(cli_sockfd, fd, &offset, stat_buf.st_size);
		if (rc == -1)
		{
			fprintf(stderr, "error from sendfile: %s\n", strerror(errno));
			return;
		}
		if (rc != stat_buf.st_size)
		{
			fprintf(stderr, "incomplete transfer from sendfile: %d of %d bytes\n",
					rc,
					(int)stat_buf.st_size);
			return;
		}

		/* close descriptor for file that was sent */
		close(fd);
	}
	else if(_command_operator == "pu")
	{
		if(parsed_command.size() > 1)
		{
			std::string _file_path = build_directory + "server_file";
			std::string _text = parsed_command.at(1);
			std::ofstream _file_stream(_file_path);
			if(_file_stream.fail())
			{
				std::cout << "The file does not exist" << std::endl;
				return;
			}
			_file_stream.clear();
			_file_stream.write(_text.c_str(), _text.length());
			_file_stream.close();
		}
	}
	else
	{
		send_data_to_client(sockfd, "fatal", std::string("500") + _sentinel + "Request not supported");
	}
}

void server::send_data_to_client(int sockfd, std::string command, std::string data)
{
	char _sentinel = -1;
	data = command + _sentinel + data;
	write(sockfd, data.c_str(), data.length());
}

void server::send_location_info_to_clients(std::string username)
{
	char _sentinel = -1;
	std::unordered_map<std::string, user_info>::iterator _user_info_map_itr =
		user_info_map.find(username);
	if (_user_info_map_itr != user_info_map.end())
	{
		user_info _user_info = _user_info_map_itr->second;
		std::vector<std::string> _contact_usernames_list = _user_info.contact_user_name_list;
		int _number_of_online_friends = 0;
		std::vector<std::string>::iterator _contact_usernames_list_itr =
			_contact_usernames_list.begin();

		std::string _data_to_user = "";
		while (_contact_usernames_list_itr != _contact_usernames_list.end())
		{
			std::string _friend_username = *_contact_usernames_list_itr;
			std::unordered_map<std::string, user_info>::iterator _friend_user_info_map_itr =
				user_info_map.find(_friend_username);
			if (_friend_user_info_map_itr != user_info_map.end())
			{
				user_info _friend_user_info = _friend_user_info_map_itr->second;
				if (_friend_user_info.is_logged_in)
				{
					_number_of_online_friends++;
					int _friend_sockfd = _friend_user_info.sockfd;
					std::string _friend_ip = _friend_user_info.ip;
					int _friend_port = _friend_user_info.port;
					std::string _command_to_friend = "loc_friend";
					std::string _user_ip = _user_info.ip;
					int _user_port = _user_info.port;
					std::string _data_to_friend = std::to_string(1) + _sentinel +
												  username + _sentinel + _user_ip + _sentinel + std::to_string(_user_port);
					send_data_to_client(_friend_sockfd, _command_to_friend, _data_to_friend);
					_data_to_user += (_sentinel + _friend_username + _sentinel +
									  _friend_ip + _sentinel + std::to_string(_friend_port));
				}
			}
			_contact_usernames_list_itr++;
		}
		std::string _command_to_user = "loc_friends";
		_data_to_user = std::to_string(_number_of_online_friends) + _data_to_user;
		send_data_to_client(_user_info.sockfd, _command_to_user, _data_to_user);
	}
}

void server::send_logout_info_to_clients(std::string username)
{
	std::unordered_map<std::string, user_info>::iterator _user_info_map_itr =
		user_info_map.find(username);
	if (_user_info_map_itr != user_info_map.end())
	{
		user_info _user_info = _user_info_map_itr->second;
		std::vector<std::string> _contact_usernames_list = _user_info.contact_user_name_list;
		std::vector<std::string>::iterator _contact_usernames_list_itr =
			_contact_usernames_list.begin();

		std::string _data_to_user = "";
		while (_contact_usernames_list_itr != _contact_usernames_list.end())
		{
			std::string _friend_username = *_contact_usernames_list_itr;
			std::unordered_map<std::string, user_info>::iterator _friend_user_info_map_itr =
				user_info_map.find(_friend_username);
			if (_friend_user_info_map_itr != user_info_map.end())
			{
				user_info _friend_user_info = _friend_user_info_map_itr->second;
				if (_friend_user_info.is_logged_in)
				{
					int _friend_sockfd = _friend_user_info.sockfd;
					std::string _command_to_friend = "rm_loc_friend";
					send_data_to_client(_friend_sockfd, _command_to_friend, username);
				}
			}
			_contact_usernames_list_itr++;
		}
	}
}

void server::sigint_handler(int signal)
{
	_server->_exit();
	exit(0);
}

void server::broad_cast_data_to_other_clients(int in_sockfd, std::string command, std::string data)
{
	std::list<int>::iterator _sockfds_itr = sockfds.begin();
	while(_sockfds_itr != sockfds.end())
	{
		int client_sock = *_sockfds_itr;
		if(client_sock != in_sockfd)
		{
			send_data_to_client(client_sock, command, data);
		}
		_sockfds_itr++;
	}
}
