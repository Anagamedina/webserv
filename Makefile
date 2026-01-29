# colors 
RESET			= \033[0m
BLACK_BOLD		= \033[1;30m
RED_BOLD		= \033[1;31m
GREEN_BOLD		= \033[1;32m
YELLOW_BOLD		= \033[1;33m
BLUE_BOLD		= \033[1;34m
MAGENTA_BOLD	= \033[1;35m
CYAN_BOLD		= \033[1;36m
WHITE_BOLD		= \033[1;37m

BLACK			= \033[0;30m
RED				= \033[0;31m
GREEN			= \033[0;32m
YELLOW			= \033[0;33m
BLUE			= \033[0;34m
MAGENTA 		= \033[0;35m
LIGHT_MAGENTA 	= \033[38;5;177m
CYAN			= \033[0;36m
WHITE			= \033[0;37m

NAME 		= webserver

CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98 -pedantic -Wshadow
LDFLAGS		=

SRC_DIR		= src
BIN_DIR		= bin
BUILD_DIR	= build
INCLUDE 	= -I$(SRC_DIR) -Iinclude

SRC_FILES = $(SRC_DIR)/main.cpp \
			$(SRC_DIR)/config/ConfigException.cpp \
			$(SRC_DIR)/config/ConfigParser.cpp \
			$(SRC_DIR)/utils/StringUtils.cpp \
			$(SRC_DIR)/network/EpollWrapper.cpp \
			$(SRC_DIR)/network/TcpListener.cpp \
			$(SRC_DIR)/network/ServerManager.cpp \
			$(SRC_DIR)/client/Client.cpp \
			$(SRC_DIR)/http/HttpRequest.cpp \
			$(SRC_DIR)/http/HttpParser.cpp \
			$(SRC_DIR)/http/HttpResponse.cpp \
			$(SRC_DIR)/client/RequestProcessor.cpp \
			$(SRC_DIR)/cgi/CgiExecutor.cpp 


#OBJ_FILES = $(SRC_FILES:%.cpp=$(BUILD_DIR)/%.o) # works with vpath
OBJ_FILES = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRC_FILES))
DEP_FILES = $(OBJ_FILES:.o=.d)

# with vpath:
#SRC_FILEs=file1.cpp file2.cpp..
#vpath %.cpp $(SRC_DIR):\
#	$(SRC_DIR)/App:\
#	$(SRC_DIR)/PhoneBook:\
#	$(SRC_DIR)/Contact:\
#	$(SRC_DIR)/utils

# target definitions 
#all: $(BIN_DIR)/$(NAME)
all: $(NAME)

#$(BIN_DIR)/$(NAME): $(OBJ_FILES)
$(NAME): $(OBJ_FILES)
	@printf "$(LIGHT_MAGENTA)==> Linking objects...$(RESET)\n"
	#@mkdir -p $(BIN_DIR)
	@$(CXX) $(OBJ_FILES) $(LDFLAGS) -o $@ \
		&& printf "$(CXX) $(OBJ_FILES) $(LDFLAGS) -o $@\n" \
		|| { printf "$(RED)==> ✖ Linking failed: $(notdir $<)$(RESET)\n"; exit 1; }
	@printf "$(GREEN)==> ✔ Build complete.$(RESET)\n"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp Makefile
	@printf "$(CYAN)==> Compiling $(WHITE_BOLD)$(notdir $<)...$(RESET)\n"
	@mkdir -p $(dir $@)
	#@mkdir -p $(BUILD_DIR)
	@$(CXX) $(CXXFLAGS) $(INCLUDE) -MMD -c $< -o $@ \
		&& printf "$(BLUE)$(CXX) $(CXXFLAGS) $(INCLUDE) -MMD -c $< -o $@$(RESET)\n" \
		|| { printf "$(RED)==> ✖ Compilation failed: $(notdir $<)$(RESET)\n"; exit 1; }
	@#printf "$(GREEN)==> Compilation complete.$(RESET)\n"

clean:
	@rm -rf $(BIN_DIR) $(BUILD_DIR)
	@printf "$(YELLOW)==> ✔ Objects and dependencies removed.$(RESET)\n"

fclean: clean
	@rm -f $(NAME)
	@printf "$(YELLOW)==> ✔ Executable: $(WHITE)$(NAME)$(YELLOW) removed.$(RESET)\n"

re: fclean all

leaks: CXXFLAGS += -g -fsanitize=leak -DTDEBUG=1
leaks: LDFLAGS += -fsanitize=leak
leaks: re

debug: CXXFLAGS += -g -fsanitize=address -DTDEBUG=1
debug: LDFLAGS += -fsanitize=address
debug: re

bear: fclean
	bear -- $(MAKE) all

# extras
-include $(DEP_FILES)

.PHONY: all clean fclean re bear debug leak
#.SILENT:
