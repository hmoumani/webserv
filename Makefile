NAME	:= webserv
SRC		:= parser.cpp webserv.cpp Socket.cpp Message.cpp Request.cpp Response.cpp MimeTypes.cpp Config.cpp
SRC		:= $(addprefix src/,$(SRC))
FLAGS	:= -Wall -Wextra -Werror

all: $(NAME)

$(NAME):$(SRC)
	clang++ --std=c++98 -g -glldb $(SRC) -Iinclude -o $@

clean:
	@rm -rf $(NAME)

fclean:clean

re: clean all
