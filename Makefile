NAME     = ircserv
CLIENT   = client

CXX      = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

SRC = main.cpp parser.cpp server.cpp client.cpp parsedMessage.cpp commandHandler.cpp errors.cpp Channel.cpp
OBJ = $(SRC:%.cpp=obj/%.o)

CLIENT_SRC = test_client.cpp
CLIENT_OBJ = $(CLIENT_SRC:%.cpp=obj/%.o)

# --- rules ---

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

client: $(CLIENT_OBJ)
	$(CXX) $(CXXFLAGS) $(CLIENT_OBJ) -o $(CLIENT)

obj/%.o: %.cpp | obj
	$(CXX) $(CXXFLAGS) -c $< -o $@

obj:
	mkdir -p obj

clean:
	rm -rf obj

fclean: clean
	rm -f $(NAME) $(CLIENT)

re: fclean all

.PHONY: all clean fclean re client
