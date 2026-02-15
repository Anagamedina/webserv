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
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98 -pedantic -Wshadow -DDEBUG -g # -g is esential for valgrind
LDFLAGS		=

SRC_DIR		= src
BIN_DIR		= bin
BUILD_DIR	= build
INCLUDE 	= -I$(SRC_DIR) -Iinclude

SRC_FILES = $(SRC_DIR)/main.cpp \
			$(SRC_DIR)/network/EpollWrapper.cpp \
			$(SRC_DIR)/network/TcpListener.cpp \
			$(SRC_DIR)/network/ServerManager.cpp \
			$(SRC_DIR)/cgi/CgiExecutor.cpp \
			$(SRC_DIR)/cgi/CgiProcess.cpp \
			$(SRC_DIR)/client/Client.cpp \
			$(SRC_DIR)/client/ClientCgi.cpp \
			$(SRC_DIR)/client/ErrorUtils.cpp \
			$(SRC_DIR)/client/ResponseUtils.cpp \
			$(SRC_DIR)/client/SessionUtils.cpp \
			$(SRC_DIR)/client/AutoindexRenderer.cpp \
			$(SRC_DIR)/client/StaticPathHandler.cpp \
			$(SRC_DIR)/client/RequestProcessorUtils.cpp \
			$(SRC_DIR)/client/RequestProcessor.cpp \
			$(SRC_DIR)/http/HttpHeaderUtils.cpp \
			$(SRC_DIR)/http/HttpParser.cpp \
			$(SRC_DIR)/http/HttpParserStartLine.cpp \
			$(SRC_DIR)/http/HttpParserHeaders.cpp \
			$(SRC_DIR)/config/ServerConfig.cpp \
			$(SRC_DIR)/config/LocationConfig.cpp \
			$(SRC_DIR)/config/ConfigParser.cpp \
			$(SRC_DIR)/config/ConfigException.cpp \
			$(SRC_DIR)/config/ConfigUtils.cpp \
			$(SRC_DIR)/http/HttpParserBody.cpp \
			$(SRC_DIR)/http/HttpRequest.cpp \
			$(SRC_DIR)/http/HttpResponse.cpp \
			$(SRC_DIR)/common/StringUtils.cpp
			


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
####################################HTTP TESTS#######################################
# tests (manual) - HTTP (parser/request)
TEST_HTTP_REQUEST_BIN = tests/manual_http_request
TEST_HTTP_PARSER_BIN  = tests/manual_http_parser

TEST_HTTP_REQUEST_SRC = tests/manual_http_request.cpp \
				   $(SRC_DIR)/http/HttpRequest.cpp

TEST_HTTP_PARSER_SRC = tests/manual_http_parser.cpp \
				  $(SRC_DIR)/http/HttpParser.cpp \
				  $(SRC_DIR)/http/HttpParserStartLine.cpp \
				  $(SRC_DIR)/http/HttpParserHeaders.cpp \
				  $(SRC_DIR)/http/HttpParserBody.cpp \
				  $(SRC_DIR)/http/HttpRequest.cpp

TEST_REQUEST_PROCESSOR_BIN = tests/manual_request_processor
TEST_REQUEST_PROCESSOR_SRC = tests/manual_processor/manual_request_processor.cpp \
				  $(SRC_DIR)/client/ErrorUtils.cpp \
				  $(SRC_DIR)/client/ResponseUtils.cpp \
				  $(SRC_DIR)/client/SessionUtils.cpp \
				  $(SRC_DIR)/client/StaticPathHandler.cpp \
				  $(SRC_DIR)/client/RequestProcessor.cpp \
				  $(SRC_DIR)/http/HttpRequest.cpp \
				  $(SRC_DIR)/http/HttpResponse.cpp

TEST_CLIENT_BIN = tests/manual_client
TEST_CLIENT_SRC = tests/manual_client/manual_client.cpp \
				  $(SRC_DIR)/client/Client.cpp \
				  $(SRC_DIR)/client/ErrorUtils.cpp \
				  $(SRC_DIR)/client/ResponseUtils.cpp \
				  $(SRC_DIR)/client/SessionUtils.cpp \
				  $(SRC_DIR)/client/StaticPathHandler.cpp \
				  $(SRC_DIR)/client/RequestProcessor.cpp \
				  $(SRC_DIR)/http/HttpParser.cpp \
				  $(SRC_DIR)/http/HttpParserStartLine.cpp \
				  $(SRC_DIR)/http/HttpParserHeaders.cpp \
				  $(SRC_DIR)/http/HttpParserBody.cpp \
				  $(SRC_DIR)/http/HttpRequest.cpp \
				  $(SRC_DIR)/http/HttpResponse.cpp

test_http_request:
	@$(CXX) $(CXXFLAGS) $(INCLUDE) $(TEST_HTTP_REQUEST_SRC) -o $(TEST_HTTP_REQUEST_BIN) \
		&& ./$(TEST_HTTP_REQUEST_BIN)

test_http_parser:
	@$(CXX) $(CXXFLAGS) $(INCLUDE) $(TEST_HTTP_PARSER_SRC) -o $(TEST_HTTP_PARSER_BIN) \
		&& ./$(TEST_HTTP_PARSER_BIN)

test_request_processor:
	@$(CXX) $(CXXFLAGS) $(INCLUDE) $(TEST_REQUEST_PROCESSOR_SRC) -o $(TEST_REQUEST_PROCESSOR_BIN) \
		&& ./$(TEST_REQUEST_PROCESSOR_BIN)

test_client:
	@$(CXX) $(CXXFLAGS) $(INCLUDE) $(TEST_CLIENT_SRC) -o $(TEST_CLIENT_BIN) \
		&& ./$(TEST_CLIENT_BIN)
####################################HTTP TESTS#######################################
bear: fclean
	bear -- $(MAKE) all

# extras
-include $(DEP_FILES)

.PHONY: all clean fclean re bear debug leak test_http_request test_http_parser test_request_processor test_client
#.SILENT:
