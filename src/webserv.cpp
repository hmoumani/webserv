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
			std::cerr << "CONNECTION: " << i << std::endl;
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

				connection.setFD(fds[i].fd);

				if (fds[i].revents & POLLIN) {

					std::string message = connection.receive();

					Request request(message);
					try {
						Response * response = new Response(request);
	
						std::string str = response->HeadertoString();

						response->buffer.setData(str.c_str(), str.length());
						
						responses.insert(std::make_pair(connection.getFD(), response));

						fds[i].events = POLLOUT;
					} catch(const StatusCodeException & e) {
						std::cerr << e.getStatusCode() << " - " << e.what() << '\n';
					}
				}
				if (fds[i].revents & POLLOUT) {
					
					Response * response = responses.find(connection.getFD())->second;
					
					if (response->buffer.length() == 0) {
						response->readFile();
					}

					connection.send(response->buffer);

					if (!response->getFile() && response->buffer.length() == 0) {
						connection.close();
						fds.erase(fds.begin() + i);
						responses.erase(connection.getFD());
						delete response;
					}
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