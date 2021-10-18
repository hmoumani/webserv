NAME	:= webserv
SRC		:= parser.cpp webserv.cpp Socket.cpp Message.cpp Request.cpp Response.cpp MimeTypes.cpp Config.cpp debug.cpp
SRC		:= $(addprefix src/,$(SRC))
FLAGS	:= -Wall -Wextra -Werror --std=c++98

all: $(NAME)

$(NAME):$(SRC)
	clang++ $(FLAGS) $(SRC) -Iinclude -o $@

debug:$(SRC)
	clang++ $(FLAGS) -g -glldb -fsanitize=address  $(SRC) -Iinclude -o $(NAME)

clean:
	@rm -rf $(NAME)

fclean:clean

re: clean all
