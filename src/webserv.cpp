#include "webserv.hpp"

// std::vector<Socket> sockets;

static std::vector<pollfd> fds;

static bool running = true;

socklen_t addrlen = sizeof(sockaddr_in);

void error(std::string message) {
	std::cerr << "webserv: " << message << ". " << std::strerror(errno) << ". " << errno << std::endl;
	for (std::vector<Config>::const_iterator it = servers.begin(); it != servers.end(); ++it) {
		it->socket->close();
		delete it->socket;
	}
	fds.clear();
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
	conf.socket->listen(MAX_CONNECTION);
}

void handle_signal(int sig) {
	debug << "SIGNAL " << sig << std::endl;
}

void exit_program(int sig) {
	(void)sig;
	running = false;
	for (std::vector<Config>::const_iterator it = servers.begin(); it != servers.end(); ++it) {
		if (it->isPrimary) {
			std::cerr << "Close Socket: " << it->socket->getFD() << std::endl;
			it->socket->close();
			delete it->socket;
		}
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
			it->isPrimary = false;
		}
	}

	signal(13, handle_signal);
	signal(SIGHUP, exit_program);
	signal(SIGQUIT, exit_program);
	signal(SIGINT, exit_program);
	signal(SIGTERM, exit_program);

	while (running) {

		// examines to all socket & connections if they are ready
		// debug << "Before\n";
		int pret = poll(&fds.front(), fds.size(), -1);
		// debug << "After\n";
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
						std::cerr << "New Connection: " << new_connection->sock.getHost() << ":" << new_connection->sock.getPort() << std::endl;
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
				if (!(fds[i].revents & POLLHUP)) {

					if (fds[i].revents & POLLIN || request.getBuffer().length() || (response.is_cgi() && !response.isCgiHeaderFinished())
						|| (response.is_cgi() && !response.isSendingBodyFinished(request))) {

						try {
							if (fds[i].revents & POLLIN || request.getBuffer().length()) {
								request.setServerConfig(getConnectionServerConfig(connection.parent.getHost(), connection.parent.getPort(), ""));

								request.receive(connection.sock);
								if (request.isHeadersFinished() && !response.isRequestHandled()) {

									response.setServerConfig(getConnectionServerConfig(connection.parent.getHost(), connection.parent.getPort(), request.getHeader("Host")));
									response.handleRequest(request, connection.sock);

								}
							}

							if (response.is_cgi() && request.isBodyFinished())
							{
								if (!response.isSendingBodyFinished(request)) {
									response.set_cgi_body(request);
								}
								if (response.isSendingBodyFinished(request)) {
									response.closeFdBody();
								}
							}

							if (response.is_cgi() && !response.isCgiHeaderFinished() && request.isBodyFinished()) {
								response.readCgiHeader();
							}
						} catch (const StatusCodeException & e) {

							if (e.getStatusCode() == 0) {
								fds[i].revents = POLLHUP;
							} else /*if (response.getStatusCode() == 200) */{
								std::cerr << "Caught exception: " << e.getStatusCode() << " " << e.what() << std::endl;
								response.setServerConfig(getConnectionServerConfig(connection.parent.getHost(), connection.parent.getPort(), request.getHeader("Host")));
								response.setEndChunkSent(false);
								response.setErrorPage(e, e.getServer());
								// if (request.isBodyFinished()) {
								// 	request.reset();
								// }
								request.setHeaderFinished(true);
								if (e.getStatusCode() == 400 || e.getStatusCode() == 413) {
									response.setHeader("Connection", "close");
									// request.setBodyFinished(true);
								}
							// } else {
								// request.setHeaderFinished(false);
							}

						} catch(const ListingException & e){
							response.setServerConfig(getConnectionServerConfig(connection.parent.getHost(), connection.parent.getPort(), request.getHeader("Host")));
							response.listingPage(e);
							response.setEndChunkSent(false);
							request.setHeaderFinished(true);
							// request.setBodyFinished(true);
						}

						if (request.isHeadersFinished() && !response.buffer_header.size && (!response.is_cgi() || response.isCgiHeaderFinished())) {
							std::string data = response.HeadertoString();
							response.buffer_header.setData(data.c_str(), data.length());
						}
					}

					if (fds[i].revents & POLLOUT && response.buffer_header.size && (response.buffer_body.length() != 0 || !response.isEndChunkSent())) {
						
						
						if (response.buffer_body.length() == 0) {
							response.readFile();
						}
						if (response.buffer_body.length() || response.buffer_header.length()) {
							try {
						    	debug << "BEFORE Body " << response.buffer_body.length() << "\n";
								connection.sock.send(response);
						    	debug << "AFTER Body " << response.buffer_body.length() << "\n";
							} catch (const StatusCodeException & e) {
								close = true;
							}
						}
					}
					if (response.isEndChunkSent() && !response.buffer_body.length() && request.isBodyFinished()) {
						if (request.getHeader("Connection") == "close") {
							response.setHeader("Connection", "close");
						}
						if (response.getHeader("Connection") == "close") {
							close = true;
						}
						response.closeFd();
						response.closeFdBody();

						connection.request.reset();
						connection.response.reset();
					}
				} else {
					debug << "POLLHUP " << connection.sock.getFD() << std::endl;
					close = true;
				}
				if (close) {
					// debug << "Close: " << connection.sock.getFD() << std::endl;
					connection.sock.close();
					fds.erase(fds.begin() + i);
					connections.erase(connection.sock.getFD());
					delete &connection;
					--i;
					--curr_size;
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