#include "webserv.hpp"

std::string to_str(long i)
{
    std::ostringstream s;
    s << i;
    return s.str();
}

std::string wc(char const *fname)
{
    static const long BUFFER_SIZE = 16*1024;
    std::string res;
    int fd = open(fname, O_RDONLY);
    if(fd == -1)
    {
        std::cerr << "error\n";
        exit(1);
    }

    /* Advise the kernel of our access pattern.  */
    // posix_fadvise(fd, 0, 0, 1);  // FDADVICE_SEQUENTIAL

    char buf[BUFFER_SIZE + 1];

    while(size_t bytes_read = read(fd, buf, BUFFER_SIZE))
    {
        if(bytes_read == (size_t)-1)
        {
            std::cerr << "read failed\n";
            exit(1);    
        }
        if (!bytes_read)
            break;
        
        // std::cout << buf << std::endl;
        res += buf;

        // for(char *p = buf; (p = (char*) memchr(p, '\n', (buf + bytes_read) - p)); ++p)
        //     ++lines;
    }

    return res;
}

int main()
{
    int     server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in sock_addr;
    if (server_fd < 0)
    {
        std::cerr << "error creating socket\n";
    }
    std::cout << server_fd << std::endl;
    memset((char *)&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    sock_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) < 0)
    {
        std::cerr << "error binding\n" << strerror(errno);
        exit(1);
    }
    if (listen(server_fd, 3) < 0)
    {
        std::cerr << "In listen\n";
        exit(1);
    }
    while (1)
    {
        int len = sizeof(sock_addr);
        int new_socket = accept(server_fd, (struct sockaddr *)&sock_addr, (socklen_t*)&len);
        if (new_socket < 0)
        {
            std::cerr << "In accept\n";
            exit(1);
        }
        char buffer[3000] = {0};
        int valread = read(new_socket, buffer, 3000);
        std::cout << buffer << std::endl;
        if (valread < 0)
        {
            std::cerr << "nothing to read\n";
            exit(1);
        }
        std::string content = wc("index.html");
        std::string hello("HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: ");
        hello += to_str(content.length());
        hello += "\n\n";
        hello += content;

        write(new_socket, hello.c_str(), hello.length());
        close(new_socket);
    }
}