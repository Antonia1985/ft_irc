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

# create obj folder automatically
obj:
	mkdir -p obj

# server binary
$(NAME): obj $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

# client binary
client: obj $(CLIENT_OBJ)
	$(CXX) $(CXXFLAGS) $(CLIENT_OBJ) -o $(CLIENT)

# compile server objects
obj/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# compile client object
obj/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf obj

fclean: clean
	rm -f $(NAME) $(CLIENT)

re: fclean all

.PHONY: all clean fclean re client obj
