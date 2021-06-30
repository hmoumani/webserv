
SRCS=training.cpp

FLAGS=-Wextra -Wall -Werror -std=c++98

NAME=webserv

$(NAME):$(SRCS)
	clang++ $(FLAGS) $(SRCS) -o $(NAME)

all: $(NAME)

re: fclean $(NAME)

clean:
	rm -rf $(NAME)

fclean: clean
