NAME=webserv

SRC=Socket.cpp Message.cpp Request.cpp webserv.cpp Response.cpp Mimetypes.cpp

SRC:=$(addprefix src/,$(SRC))

FLAGS=-Wall -Wextra -Werror

all: $(NAME)


$(NAME):$(SRC)
	@clang++ --std=c++98 $(SRC) -o $(NAME) -Iinclude

clean:
	@rm -rf $(NAME)

re: clean all
