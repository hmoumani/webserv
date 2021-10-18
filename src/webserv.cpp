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
	debug << "SIGNAL " << sig << std::endl;
}

void exit_program(int sig) {
	(void)sig;
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
			fds.push_back((pollfd){it->socket->getFD(), POLLIN | POLLOUT, 0});
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
						// std::cerr << "Socket: " << sock->getHost() << ":" << sock->getPort() << ", ";
						// std::cerr << "New Connection: " << new_connection->sock.getHost() << ":" << new_connection->sock.getPort() << std::endl;
						new_connection->sock.setState(NonBlockingSocket);
						fds.push_back((pollfd){new_connection->sock.getFD(), POLLIN | POLLOUT, 0});
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
				if (fds[i].revents & POLLIN || request.getBuffer().length() || (response.is_cgi() && !response.isCgiHeaderFinished())) {

					try {
						if (fds[i].revents & POLLIN || request.getBuffer().length()) {
							request.setServerConfig(getConnectionServerConfig(connection.parent.getHost(), connection.parent.getPort(), ""));

							request.receive(connection.sock);
							if (request.isHeadersFinished()) {
								// debug << "Request Succesful" << std::endl;
								
								response.setServerConfig(getConnectionServerConfig(connection.parent.getHost(), connection.parent.getPort(), request.getHeader("Host")));
								response.handleRequest(request);
							}
							// if (request.isBodyFinished()) {
								// response.cgi_read(request);
							// }
								// std::cerr << std::boolalpha << response.is_cgi() << " " << !response.isSendingBodyFinished(request) <<
							// " " << request.getBodySize() <<  std::endl;
							if (response.is_cgi() && !response.isSendingBodyFinished(request))
							{
								response.set_cgi_body(request);
							}
						}

						// debug << "CGI Header: " << std::boolalpha << response.isCgiHeaderFinished() << std::endl;
						// if (request.isHeadersFinished() && (!response.is_cgi() || request.isBodyFinished())) {

						if (response.is_cgi() && !response.isCgiHeaderFinished()) {
							response.readCgiHeader();
						}
					} catch (const StatusCodeException & e) {

						if (e.getStatusCode() == 0) {
							fds[i].revents = POLLHUP;
						} else {
							// exit(EXIT_FAILURE);
							std::cerr << "Caught exception: " << e.getStatusCode() << " " << e.what() << std::endl;
							response.setServerConfig(getConnectionServerConfig(connection.parent.getHost(), connection.parent.getPort(), ""));
							response.setEndChunkSent(false);
							response.setErrorPage(e, e.getServer());
							request.setHeaderFinished(true);
							request.getBuffer().resize(0);
							if (e.getStatusCode() >= 400) {
								response.setHeader("Connection", "close");
								request.setBodyFinished(true);
							}
						}

					} catch(const ListingException & e){
						response.setServerConfig(getConnectionServerConfig(connection.parent.getHost(), connection.parent.getPort(), ""));
						response.listingPage(e);
						response.setEndChunkSent(false);
						request.setHeaderFinished(true);
						request.setBodyFinished(true);
					}

					if (request.isHeadersFinished() && (!response.is_cgi() || request.isBodyFinished())) {
						
						if (!response.is_cgi() || response.isCgiHeaderFinished())
						{
							std::string data = response.HeadertoString();
								// std::cerr << "Response: " << response.getStatusCode() <<  << std::endl;
							response.buffer_header.setData(data.c_str(), data.length());
							request.setHeaderFinished(false);
						}
					}
				}

				if (fds[i].revents & POLLOUT && response.buffer_header.size) {
					
					
					if (response.buffer_body.length() == 0) {
						response.readFile();
					}
					if (response.buffer_body.length() || response.buffer_header.length()) {
						try {
							connection.sock.send(response);

						} catch (const StatusCodeException & e) {
							close = true;
						}
						// if (response.getBody()->eof() && response.buffer_header.length() == 0 && response.buffer_body.length() == 0) {
						// 	if (response.getHeader("Transfer-Encoding") == "chunked") {
						// 		write(2, "0\r\n\r\n", 5);
						// 		send(connection.sock.getFD(), "0\r\n\r\n", 5, 0);
						// 	}
						// }
					}
					
				}
				if (response.isEndChunkSent() && response.buffer_header.length() == 0 && response.buffer_body.length() == 0) {
					if (request.isBodyFinished()) {
						if (request.getHeader("Connection") == "close") {
							response.setHeader("Connection", "close");
						}
						if (response.getHeader("Connection") == "close") {
							close = true;
						}
						connection.request.reset();
						connection.response.reset();
					}
				}
				if (fds[i].revents & POLLHUP) {
					debug << "POLLHUP " << connection.sock.getFD() << std::endl;
					close = true;
				}
				if (close) {
					// std::cerr << "Close " << connection.sock.getFD() << std::endl;
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