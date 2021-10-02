NAME	:= webserv
SRC		:= parser.cpp webserv.cpp Socket.cpp Message.cpp Request.cpp Response.cpp MimeTypes.cpp Config.cpp
SRC		:= $(addprefix src/,$(SRC))
FLAGS	:= -Wall -Wextra -Werror

all: $(NAME)

$(NAME):$(SRC)
	clang++ --std=c++98 $(SRC) -Iinclude -o $@

debug:$(SRC)
	clang++ --std=c++98 -g -fsanitize=address $(SRC) -Iinclude -o $(NAME)

clean:
	@rm -rf $(NAME)

fclean:clean

re: clean all
