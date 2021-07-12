NAME=webserv

SRC=Socket.cpp Message.cpp Request.cpp webserv.cpp Response.cpp Mimetypes.cpp

SRC:=$(addprefix src/,$(SRC))

FLAGS=-Wall -Wextra -Werror

all: $(NAME)


$(NAME):$(SRC)
	clang++ -g -fsanitize=address --std=c++98 $(SRC) -o $(NAME) -Iinclude

clean:
	@rm -rf $(NAME)

fclean:clean

re: clean all
