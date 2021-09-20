#include "webserv.hpp"

// std::vector<Socket> sockets;
static bool running = true;

socklen_t addrlen = sizeof(sockaddr_in);

void error(std::string message) {
	std::cerr << "webserv: " << message << ". " << std::strerror(errno) << ". " << errno << std::endl;
	for (std::vector<Config>::const_iterator it = servers.begin(); it != servers.end(); ++it) {
		it->socket->close();
		delete it->socket;
	}
	return exit(EXIT_FAILURE);
}

void setup_server(Config & conf) {
	int opt = 1;

	conf.socket = new Socket();
	conf.socket->create(AF_INET, SOCK_STREAM, 0);
	conf.socket->setState(NonBlockingSocket);
	conf.socket->setOption(SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	conf.socket->setAddress(AF_INET, inet_addr(conf.host.c_str()), conf.port);
	conf.socket->bind();
	conf.socket->listen(10);
}

void handle_signal(int sig) {
	std::cout << "SIGNAL " << sig << std::endl;
}

void exit_program(int sig) {
	for (std::vector<Config>::const_iterator it = servers.begin(); it != servers.end(); ++it) {
		std::cerr << "Close Socket: " << it->socket->getFD() << std::endl;
		running = false;
		it->socket->close();
		delete it->socket;
	}
	return exit(EXIT_SUCCESS);
}

Socket * isSocket(int fd) {
	for (std::vector<Config>::const_iterator it = servers.begin(); it != servers.end(); ++it) {
		if (it->socket->getFD() == fd) {
			return it->socket;
		}
	}
	return NULL;
}


Socket * socketExists(const std::vector<Config>::iterator & curr_it) {
	for (std::vector<Config>::const_iterator it = servers.begin(); it != curr_it; ++it) {
		if (it->socket->getHost() == curr_it->host && it->socket->getPort() == curr_it->port) {
			return it->socket;
		}
	}
	return NULL;
}


int main(int argc, char *argv[]) {
	std::vector<pollfd> fds;
	// std::map<int, Response*> responses;
	// std::map<int, Request*> requests;
	// std::map<int, Socket> connections;
	open_config_file(argc, argv);

	// std::cerr << servers[0].host << " " << htonl(inet_addr(servers[0].host.c_str())) << ":" << servers[0].port << std::endl;

	for (std::vector<Config>::iterator it = servers.begin(); it != servers.end(); ++it) {
		Socket * sock = socketExists(it);
		if (!sock) { // checks if host:port already exists 
			setup_server(*it);
			fds.push_back((pollfd){it->socket->getFD(), POLLIN});
			std::cerr << "Listening on " << it->host << ":" << it->port << std::endl;
		} else { // else copy the already existed socket 
			it->socket = sock;
		}
	}

	signal(13, handle_signal);
	signal(SIGHUP, exit_program);
	signal(SIGQUIT, exit_program);
	signal(SIGINT, exit_program);
	signal(SIGTERM, exit_program);

	while (running) {

		// examines to all socket & connections if they are ready
		int pret = poll(&fds.front(), fds.size(), -1);
	
		if (pret == -1) {
			error("poll() failed");
		}

		// loop through all sockets & connections
		int curr_size = fds.size();
		for (int i = 0; i < curr_size; ++i) {

			if (fds[i].revents == 0) {
				continue;
			}
			Socket * sock;
			if ((sock = isSocket(fds[i].fd))) {
				Connection * new_connection;
				do {
					new_connection = new Connection(*sock);
					new_connection->sock = sock->accept();
					if (new_connection->sock.getFD() != -1) {
						std::cerr << "Socket: " << sock->getHost() << ":" << sock->getPort() << ", ";
						std::cerr << "New Connection: " << new_connection->sock.getHost() << ":" << new_connection->sock.getPort() << std::endl;
						new_connection->sock.setState(NonBlockingSocket);
						fds.push_back((pollfd){new_connection->sock.getFD(), POLLIN});
						connections.insert(std::make_pair(new_connection->sock.getFD(), new_connection));
					}

				} while (new_connection->sock.getFD() != -1);
				if (new_connection->sock.getFD() == -1) {
					delete new_connection;

				}
			// new data from a connection
			} else {

				Connection & connection = *connections.find(fds[i].fd)->second;
				
				bool close = false;
				Request & request = connection.request;
				Response & response = connection.response;
					
				if (fds[i].revents & POLLIN) {
					try {
						request.setServerConfig(getConnectionServerConfig(connection.parent.getHost(), connection.parent.getPort(), ""));
						request.receive(connection.sock);

						if (request.isFinished()) {
							std::cerr << "Request Succesful" << std::endl;
							
							response.setServerConfig(getConnectionServerConfig(connection.parent.getHost(), connection.parent.getPort(), request.getHeader("Host")));
							response.handleRequest(request);
						}
					} catch (const StatusCodeException & e) {
						std::cerr << "Caught exception: " << e.getStatusCode() << " " << e.what() << std::endl;

						response.setServerConfig(getConnectionServerConfig(connection.parent.getHost(), connection.parent.getPort(), ""));
						response.setErrorPage(e, e.getServer());
					} catch(const ListingException & e){
						response.listingPage(e);
						std::string data = response.HeadertoString();
						response.buffer_header.setData(data.c_str(), data.length());
						fds[i].events = POLLOUT;
					}

					if (response.getServerConfig()) {
						std::string data = response.HeadertoString();

						response.buffer_header.setData(data.c_str(), data.length());
						fds[i].events = POLLOUT;
					}
				}

				if (fds[i].revents & POLLOUT) {
					
					
					if (response.buffer_body.length() == 0 && (response.is_cgi() || !response.getFile()->eof())) {
						response.readFile();
					}
					if (response.buffer_body.length() || response.buffer_header.length())
						connection.sock.send(response);
						
					if (!response.is_cgi() && response.getFile()->eof() && response.buffer_header.length() == 0 && response.buffer_body.length() == 0) {
						fds[i].events = POLLIN;
						if (response.getHeader("Transfer-Encoding") == "chunked") {
				        	// write(2, "0\r\n\r\n", 5);
							send(connection.sock.getFD(), "0\r\n\r\n", 5, 0);
						}
						connection.request.reset();
						connection.response.reset();
					}
				}
				if (fds[i].revents & POLLHUP) {
					std::cerr << "POLLHUP " << connection.sock.getFD() << std::endl;
					close = true;
				}
				if (close) {
					std::cerr << "Close " << connection.sock.getFD() << std::endl;
					connection.sock.close();
					fds.erase(fds.begin() + i);
					connections.erase(connection.sock.getFD());
					delete &connection;
					--i;
					--curr_size;
					// responses.erase(connection.sock.getFD());
					// delete response;
				}
			}
		}
	}

	std::cerr << "Program Closing" << std::endl;
	// Bind the socket to a IP / port
	// Mark the socket for listening in
	// Accept a call
	// Close the listening socket
	// While receiving - display message, echo message
	// Close socket
	return 0;
}