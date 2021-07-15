#include "webserv.hpp"

Socket sock;
socklen_t addrlen = sizeof(sock.getAddress());

void error(std::string message) {
	std::cerr << message << ". " << std::strerror(errno) << ". " << errno << std::endl;
	sock.close();
	return exit(EXIT_FAILURE);
}

static std::string getResponse(std::string request) {
	
	std::ostringstream response("");
	std::string response_body = "<html>\n"
					"<body>\n"
					"<h1>Hello, World!</h1>\n"
					"</body>\n"
					"</html>";

	
	response	<< "HTTP/1.1 200 OK\n"
			<< "Date: " << Utils::getDate() << "\n"
			<< "Server: WebServer (MacOs)\n"
			// << "Content-Length: " << response_body.length() << "\n"
			<< "Last-Modified: Wed, 22 Jul 2009 19:15:56 GMT\n"
			<< "Transfer-Encoding: chunked\n"
			<< "Content-Type: text/html\n"
			<< "Connection: Closed\n\n"
			<< Utils::chuncked_transfer_encoding(response_body);
	return response.str();
}

void setup_server() {
	int opt = 1;

	sock.create(AF_INET, SOCK_STREAM, 0);
	sock.setState(NonBlockingSocket);
	sock.setOption(SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	sock.setAddress(AF_INET, INADDR_ANY, PORT);
	sock.bind();
	sock.listen(10);
}

void handle_signal(int sig) {
	std::cout << "SIGNAL " << sig << std::endl;
}
int main() {

	setup_server();

	std::vector<pollfd> fds;
	std::map<int, Response*> responses;
	// for (int i = 1; i < 31; ++i) {
	signal(13, handle_signal);
	// }
	fds.push_back((pollfd){sock.getFD(), POLLIN});

	while (true) {

		// examines to all socket if they are ready
		int pret = poll(&fds.front(), fds.size(), -1);
	
		if (pret == -1) {
			error("poll() failed");
		}

		// loop through all sockets
		int curr_size = fds.size();
		for (int i = 0; i < curr_size; ++i) {

			if (fds[i].revents == 0) {
				continue;
			}
			// if (fds[i].revents != 4)
			// 	std::cerr << "CONNECTION: " << i << " " << fds[i].revents << std::endl;
			// new connection
			if (fds[i].fd == sock.getFD()) {
				Socket new_connection;

				do {	
					new_connection = sock.accept();
					if (new_connection.getFD() != -1) {
						fds.push_back((pollfd){new_connection.getFD(), POLLIN});
					}

				} while (new_connection.getFD() != -1);
			// new data from a connection
			} else {

				Socket connection;
				bool close = false;
				Response * response;
				connection.setFD(fds[i].fd);

				if (fds[i].revents & POLLIN) {

					std::string message = connection.receive();

					Request request(message);
					try {
						response = new Response(request);
						
						std::string str = response->HeadertoString();

						responses.insert(std::make_pair(connection.getFD(), response));
						
						response->buffer.setData(str.c_str(), str.length());
						
						fds[i].events = POLLOUT;
					} catch(const StatusCodeException & e) {
						// std::cerr << e.getStatusCode() << " - " << e.what() << '\n';

						response = new Response();

						// std::cerr << response->getFile().is_open() << std::endl;
						std::string data = errorPage(e);
						response->buffer.setData(data.c_str(), data.length());
						// std::cout << data << std::endl;
						responses.insert(std::make_pair(connection.getFD(), response));
						fds[i].events = POLLOUT;
					}
				}

				response = responses.find(connection.getFD())->second;

				if (fds[i].revents & POLLOUT) {
					
					
					if (response->buffer.length() == 0 && response->getFile().is_open()) {
						response->readFile();
					}

					connection.send(response->buffer);

					if ((!response->getFile().is_open() || (response->getFile().is_open() && !response->getFile())) && response->buffer.length() == 0) {
						close = true;
					}
				}
				if (fds[i].revents & POLLHUP) {
					close = true;
				}
				if (close) {
					connection.close();
					fds.erase(fds.begin() + i);
					--i;
					--curr_size;
					responses.erase(connection.getFD());
					delete response;
				}
			}
		}
	}
	sock.close();

	// Bind the socket to a IP / port
	// Mark the socket for listening in
	// Accept a call
	// Close the listening socket
	// While receiving - display message, echo message
	// Close socket
	return 0;
}