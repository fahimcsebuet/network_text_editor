#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <list>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <algorithm>
#include <curl/curl.h>
#include <boost/filesystem.hpp>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>

#include "server.h"

const unsigned MAXBUFLEN = 1024;
server *server::_server = NULL;

int server::init(std::string user_info_file_path, std::string configuration_file_path)
{
	download_file_net("hello");
	download_file_net1();
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

void server::download_file_net(std::string path)
{
	CURL *curl;
	FILE *fp;
	CURLcode res;
	char *url = "http://stackoverflow.com";
	char outfilename[FILENAME_MAX] = "curlcop";
	curl = curl_easy_init();
	if (curl)
	{
		fp = fopen(outfilename, "wb");
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		fclose(fp);
	}
}

void server::download_file_net1()
{
	boost::filesystem::path path("/home/salman"); // random pathname
	bool result = boost::filesystem::is_directory(path);
	//printf(“Path is a directory : %d\n”, result);
	std::cout << "Path is a directory : " << result << std::endl;
	if (result)
	{
		boost::filesystem::path const &path1 = "/home/salman/test";
		boost::filesystem::path const &path2 = "/home/salman/testcopy";
		boost::filesystem::copy_file(path1, path2, boost::filesystem::copy_option::overwrite_if_exists);
	}
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
	std::list<int> sockfds;

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
	if (_command_operator == "r")
	{
		if (parsed_command.size() != 3)
		{
			std::string _error_message = "Invalid Registration Request";
			send_data_to_client(sockfd, _command_operator, _error_message);
			return;
		}
		std::string _username = parsed_command.at(1);
		std::string _password = parsed_command.at(2);
		std::unordered_map<std::string, user_info>::iterator _user_info_map_itr = user_info_map.find(_username);
		if (_user_info_map_itr != user_info_map.end())
		{
			std::string _error_message = std::string("500");
			send_data_to_client(sockfd, _command_operator, _error_message);
			return;
		}
		else
		{
			user_info _user_info;
			_user_info.user_name = _username;
			_user_info.password = _password;
			_user_info.contact_user_name_list = {};
			user_info_map[_username] = _user_info;
			user_info_file_handler _user_info_file_handler(user_info_file_path);
			_user_info_file_handler.save_user_info(user_info_map);
			std::string _message = std::string("200") + _sentinel + _username + _sentinel + _password;
			send_data_to_client(sockfd, _command_operator, _message);
			return;
		}
	}
	else if (_command_operator == "l")
	{
		if (parsed_command.size() != 3)
		{
			std::string _error_message = "Invalid Login Request";
			send_data_to_client(sockfd, _command_operator, _error_message);
			return;
		}
		std::string _username = parsed_command.at(1);
		std::string _password = parsed_command.at(2);
		std::unordered_map<std::string, user_info>::iterator _user_info_map_itr = user_info_map.find(_username);
		std::string _message = "";
		if (_user_info_map_itr != user_info_map.end())
		{
			if (_password == _user_info_map_itr->second.password)
			{
				_message = std::string("200");
				_message = _message + _sentinel + _username + _sentinel + _password;
				sockfd_to_username[sockfd] = _username;
			}
			else
			{
				_message = std::string("500");
			}
		}
		else
		{
			_message = std::string("500");
		}
		send_data_to_client(sockfd, _command_operator, _message);
		return;
	}
	else if (_command_operator == "loc")
	{
		if (parsed_command.size() == 4)
		{
			std::string _username = parsed_command.at(1);
			std::string _p2p_ip = parsed_command.at(2);
			int _p2p_port = std::stoi(parsed_command.at(3));
			std::unordered_map<std::string, user_info>::iterator _user_info_map_itr =
				user_info_map.find(_username);
			if (_user_info_map_itr != user_info_map.end())
			{
				user_info _user_info = _user_info_map_itr->second;
				_user_info.ip = _p2p_ip;
				_user_info.port = _p2p_port;
				_user_info.is_logged_in = true;
				_user_info.sockfd = sockfd;
				_user_info_map_itr->second = _user_info;
				send_location_info_to_clients(_username);
			}
		}
	}
	else if (_command_operator == "i")
	{
		if (parsed_command.size() < 3)
		{
			send_data_to_client(sockfd, _command_operator, std::string("500") + _sentinel + "Invalid Command");
		}
		else
		{
			std::string _username = parsed_command.at(1);
			std::string _potential_friend_username = parsed_command.at(2);
			std::unordered_map<std::string, user_info>::iterator _user_info_map_itr =
				user_info_map.find(_username);
			std::unordered_map<std::string, user_info>::iterator _friend_user_info_map_itr =
				user_info_map.find(_potential_friend_username);
			if (_friend_user_info_map_itr != user_info_map.end() &&
				_user_info_map_itr != user_info_map.end())
			{
				user_info _user_info = _user_info_map_itr->second;
				user_info _friend_user_info = _friend_user_info_map_itr->second;
				bool _is_friend = false;
				for (unsigned int i = 0; i < _user_info.contact_user_name_list.size(); i++)
				{
					if (_user_info.contact_user_name_list.at(i) == _potential_friend_username)
					{
						_is_friend = true;
						break;
					}
				}

				if (_is_friend)
				{
					send_data_to_client(sockfd, _command_operator, std::string("500") + _sentinel + "Already friend");
				}
				else
				{
					std::string _data_to_friend = _username;
					std::string _message_from_friend = "";
					if (parsed_command.size() > 3)
					{
						_message_from_friend = parsed_command.at(3);
						_data_to_friend += (_sentinel + _message_from_friend);
					}
					_friend_user_info_map_itr->second.potential_friends_list.push_back(_username);
					if (_friend_user_info.is_logged_in)
						send_data_to_client(_friend_user_info.sockfd, "ir", _data_to_friend);

					send_data_to_client(sockfd, _command_operator, std::string("200"));
				}
			}
			else
			{
				send_data_to_client(sockfd, _command_operator, std::string("500") + _sentinel + "User not found");
			}
		}
	}
	else if (_command_operator == "m")
	{
		if (parsed_command.size() < 4)
		{
			send_data_to_client(sockfd, _command_operator, std::string("500") + _sentinel + "Invalid Message");
		}
		else
		{
			std::string _username = parsed_command.at(1);
			std::string _potential_friend_username = parsed_command.at(2);
			std::unordered_map<std::string, user_info>::iterator _user_info_map_itr =
				user_info_map.find(_username);
			std::unordered_map<std::string, user_info>::iterator _friend_user_info_map_itr =
				user_info_map.find(_potential_friend_username);
			if (_friend_user_info_map_itr != user_info_map.end() &&
				_user_info_map_itr != user_info_map.end())
			{
				user_info _user_info = _user_info_map_itr->second;
				user_info _friend_user_info = _friend_user_info_map_itr->second;
				bool _is_friend = false;
				for (unsigned int i = 0; i < _user_info.contact_user_name_list.size(); i++)
				{
					if (_user_info.contact_user_name_list.at(i) == _potential_friend_username)
					{
						_is_friend = true;
						break;
					}
				}

				if (_is_friend)
				{
					std::string _data_to_friend = _username;
					std::string _message_from_friend = "";
					if (parsed_command.size() > 3)
					{
						_message_from_friend = parsed_command.at(3);
						_data_to_friend += (_sentinel + _message_from_friend);
					}

					if (_friend_user_info.is_logged_in)
						send_data_to_client(_friend_user_info.sockfd, "mr", _data_to_friend);
					else
					{
						send_data_to_client(sockfd, _command_operator, std::string("500") + _sentinel + "Not Online");
					}

					send_data_to_client(sockfd, _command_operator, std::string("200"));
				}
				else
				{
					send_data_to_client(sockfd, _command_operator, std::string("500") + _sentinel + "Not Friend");
				}
			}
			else
			{
				send_data_to_client(sockfd, _command_operator, std::string("500") + _sentinel + "User not found");
			}
		}
	}
	else if (_command_operator == "ia" || _command_operator == "id")
	{
		if (parsed_command.size() < 3)
		{
			send_data_to_client(sockfd, _command_operator, std::string("500") + _sentinel + "Invalid Command");
		}
		else
		{
			std::string _username = parsed_command.at(1);
			std::string _potential_friend_username = parsed_command.at(2);
			std::unordered_map<std::string, user_info>::iterator _user_info_map_itr =
				user_info_map.find(_username);
			std::unordered_map<std::string, user_info>::iterator _friend_user_info_map_itr =
				user_info_map.find(_potential_friend_username);
			if (_friend_user_info_map_itr != user_info_map.end() &&
				_user_info_map_itr != user_info_map.end())
			{
				user_info _user_info = _user_info_map_itr->second;
				user_info _friend_user_info = _friend_user_info_map_itr->second;

				bool _is_friend = false;
				for (unsigned int i = 0; i < _user_info.contact_user_name_list.size(); i++)
				{
					if (_user_info.contact_user_name_list.at(i) == _potential_friend_username)
					{
						_is_friend = true;
						break;
					}
				}

				bool _is_potential_friend = false;
				for (unsigned int i = 0; i < _user_info.potential_friends_list.size(); i++)
				{
					if (_user_info.potential_friends_list.at(i) == _potential_friend_username)
					{
						_is_potential_friend = true;
						break;
					}
				}

				if (_is_friend)
				{
					send_data_to_client(sockfd, _command_operator, std::string("500") + _sentinel + "Already friend");
				}
				else if (!_is_potential_friend)
				{
					send_data_to_client(sockfd, _command_operator, std::string("500") + _sentinel + "Did not get Request");
				}
				else
				{
					std::string _data_to_friend = _username;
					std::string _message_from_friend = "";
					if (parsed_command.size() > 3)
					{
						_message_from_friend = parsed_command.at(3);
						_data_to_friend += (_sentinel + _message_from_friend);
					}
					if (_command_operator == "ia")
					{
						_user_info_map_itr->second.contact_user_name_list.push_back(_potential_friend_username);
						_friend_user_info_map_itr->second.contact_user_name_list.push_back(_username);
						std::vector<std::string>::iterator _potential_itr =
							std::find(_user_info_map_itr->second.potential_friends_list.begin(),
									  _user_info_map_itr->second.potential_friends_list.end(),
									  _potential_friend_username);
						if (_potential_itr != _user_info_map_itr->second.potential_friends_list.end())
							_user_info_map_itr->second.potential_friends_list.erase(_potential_itr);

						if (_friend_user_info.is_logged_in)
							send_data_to_client(_friend_user_info.sockfd, "iar", _data_to_friend);
					}
					else
					{
						std::vector<std::string>::iterator _potential_itr =
							std::find(_user_info_map_itr->second.potential_friends_list.begin(),
									  _user_info_map_itr->second.potential_friends_list.end(),
									  _potential_friend_username);
						if (_potential_itr != _user_info_map_itr->second.potential_friends_list.end())
							_user_info_map_itr->second.potential_friends_list.erase(_potential_itr);
						if (_friend_user_info.is_logged_in)
							send_data_to_client(_friend_user_info.sockfd, "idr", _data_to_friend);
					}

					send_data_to_client(sockfd, _command_operator, std::string("200"));
				}
			}
			else
			{
				send_data_to_client(sockfd, _command_operator, std::string("500") + _sentinel + "User not found");
			}
		}
	}
	else if (_command_operator == "logout")
	{
		send_data_to_client(sockfd, _command_operator, std::string("200"));
		std::string _username = parsed_command.at(1);
		std::unordered_map<std::string, user_info>::iterator _user_info_itr =
			user_info_map.find(_username);
		if (_user_info_itr != user_info_map.end())
		{
			_user_info_itr->second.is_logged_in = false;
			_user_info_itr->second.ip = "";
			_user_info_itr->second.port = -1;
			_user_info_itr->second.sockfd = -1;
		}
		send_logout_info_to_clients(_username);
	}
	else if (_command_operator == "p")
	{
		send_data_to_client(sockfd, "pa", "");
		// OPEN file, read file, stream, send stream to client
		int fd;					  /* file descriptor for file to send */
		struct stat stat_buf;	 /* argument to fstat */
		off_t offset = 0;		  /* file offset */
		fd = open("salman", O_RDONLY);
		if (fd == -1)
		{
			fprintf(stderr, "unable to open '%s': %s\n", "salman", strerror(errno));
			exit(1);
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
			exit(1);
		}
		if (rc != stat_buf.st_size)
		{
			fprintf(stderr, "incomplete transfer from sendfile: %d of %d bytes\n",
					rc,
					(int)stat_buf.st_size);
			exit(1);
		}

		/* close descriptor for file that was sent */
		close(fd);
		//send_data_to_client(sockfd, "pa", "");
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
