#include "ConfigParser.hpp"
#include "../common/namespaces.hpp"
#include "ConfigException.hpp"
#include "ConfigUtils.hpp"
#include "LocationConfig.hpp"

#include <sstream>

ConfigParser::ConfigParser() :
      servers_count_(0U) {
}

ConfigParser::ConfigParser(const std::string& configFile) :
      config_file_path_(configFile),
      servers_count_(0U) {
}

ConfigParser::~ConfigParser() {
}

//	===================== Getters

const std::string& ConfigParser::getConfigFilePath() const {
    return config_file_path_;
}

size_t ConfigParser::getServerCount() const {
    return servers_count_;
}

const std::vector< ServerConfig >& ConfigParser::getServers() const {
    return servers_;
}

//	============= PRIVATE CONSTRUCTORS ===============

/**
 * manage if config_file_path_:
 * has valid size of length
 * has extension '.conf'
 * @return true or false
 */
/**
 * main function of parsing
 *
 */
void ConfigParser::parse() {
    if (!validateFileExtension()) {
        throw ConfigException(config::errors::invalid_extension + config_file_path_);
    } else {
        std::cout << "VALID FILE EXTENSION: ✅\n";
    }
    if (!validateFilePermissions()) {
        throw ConfigException(config::errors::cannot_open_file + config_file_path_);
    } else {
        std::cout << "VALID FILE PERMISSIONS: ✅\n";
    }

    clean_file_str_ = preprocessConfigFile();
    config::utils::exportContentToLogFile(clean_file_str_, config::paths::log_file_config);
    std::cout << "Exporting config file to config-clean.log\n";

    // TODO: need to fix error order of brackets: '} {' should be error but now is
    if (!validateBalancedBrackets()) {
        throw ConfigException("Invalid number of curly brackets " + config_file_path_);
    } else {
        std::cout << "VALID CURLY BRACKETS PAIRS: ✅\n";
    }

    loadServerBlocks();
    parseAllServerBlocks();
}

ConfigParser::ConfigParser(const ConfigParser& other) :
      config_file_path_(other.config_file_path_),
      clean_file_str_(other.clean_file_str_),
      servers_count_(other.servers_count_),
      raw_server_blocks_(other.raw_server_blocks_),
      servers_(other.servers_) {
}

ConfigParser& ConfigParser::operator=(const ConfigParser& other) {
    if (this != &other) {
        ConfigParser tmp(other);
        std::swap(config_file_path_, tmp.config_file_path_);
        std::swap(clean_file_str_, tmp.clean_file_str_);
        std::swap(servers_count_, tmp.servers_count_);
        std::swap(raw_server_blocks_, tmp.raw_server_blocks_);
        std::swap(servers_, tmp.servers_);
    }
    return *this;
}

//	VALIDATIONS
bool ConfigParser::validateFileExtension() const {
    if (config_file_path_.size() < 5 ||
        config_file_path_.substr(config_file_path_.size() - 5) != config::paths::extension_file) {
        return false;
    }
    return true;
}

bool ConfigParser::validateFilePermissions() const {
    std::ifstream ifs(config_file_path_.c_str());
    if (!ifs.is_open())
        return false;
    ifs.close();
    return true;
}

/**
 * Usar un contador dinámico (incrementa con {, decrementa con }) que nunca baje
de 0
 * Detectar { y }solo cuando son estructurales (inicio/fin de bloque, no dentro
de valores)
 * Manejar casos como:
 * server { (abre)
 * } (cierra)
 * location /path { (abre)
 * Cierres prematuros
 * @return true solo si:
 * Contador nunca negativo durante la lectura
 * Contador == 0 al final
 * Mejorar mensajes de error (línea + descripción)
 * Opcional: devolver también el nivel máximo de anidamiento o lista de errores
 */
bool ConfigParser::validateBalancedBrackets() const {
    int countBrackets = 0;

    for (size_t Index = 0; Index < clean_file_str_.size(); ++Index) {
        if (clean_file_str_.at(Index) == '{') {
            ++countBrackets;
        } else if (clean_file_str_.at(Index) == '}') {
            --countBrackets;
            if (countBrackets < 0) {
                return false;
            }
        }
    }
    return countBrackets == 0;
}

/**
 * RemoveComments(): if line start with '#' skip line
 * TrimLine(): if line find "\t\n\r" remove character
 * NormalizeSpaces(): replace 'X' spaces for one space ' '
 * iterate through each line of file.
 * @return
 */
std::string ConfigParser::preprocessConfigFile() const {
    std::ifstream ifs(config_file_path_.c_str());
    if (!ifs.is_open()) {
        throw ConfigException(config::errors::cannot_open_file + config_file_path_ +
                              " in CleanFileConfig()");
    }

    std::ostringstream logBuffer;
    std::string line;

    while (std::getline(ifs, line)) {
        config::utils::removeComments(line);
        line = config::utils::trimLine(line);
        line = config::utils::normalizeSpaces(line);
        if (line.empty())
            continue;
        logBuffer << line << "\n";
    }
    ifs.close();
    return logBuffer.str();
}

/**
 * la idea es que dependiendo de que estado se encuentre se actualize el enum,
 * asi saber cuando esta en un bloque de server o location o fuera de bloque
 *
 * extract all blocks 'server'
 * the function extractServerBlock() search all the occurrences to fill the
 * vector raw_server_block_
 */
void ConfigParser::loadServerBlocks() {
    if (clean_file_str_.empty())
        return;

    splitContentIntoServerBlocks(clean_file_str_, config::section::server);
}

/**
 * Extracts all server { } blocks from the config content.
 * Handles nested braces within location blocks.
 * @param content The entire config file content
 * @param typeOfExtraction
 */
void ConfigParser::splitContentIntoServerBlocks(const std::string& content,
                                                const std::string& typeOfExtraction) {
    size_t currentPos = 0;
    size_t countServers = 1;

    while ((currentPos = content.find(typeOfExtraction, currentPos)) != std::string::npos) {
        size_t braceStart = content.find('{', currentPos);
        if (braceStart == std::string::npos)
            break;

        // Find matching closing brace
        int countBrackets = 1;
        size_t braceEnd = braceStart + 1;

        while (braceEnd < content.size() - 1 && countBrackets > 0) {
            if (content[braceEnd] == '\n') {
                braceEnd++;
            }
            if (content[braceEnd] == '{') {
                countBrackets++;
            } else if (content[braceEnd] == '}') {
                countBrackets--;
            }
            braceEnd++;
        }

        // Extract the complete server block
        std::string getBlock = content.substr(currentPos, braceEnd - currentPos);

        std::stringstream ss;
        ss << config::paths::log_file_block << "_" << countServers;
        config::utils::exportContentToLogFile(getBlock, ss.str());

        raw_server_blocks_.push_back(getBlock);
        currentPos = braceEnd;
        ++countServers;
    }
    servers_count_ = raw_server_blocks_.size();
}

void ConfigParser::parseAllServerBlocks() {
    for (size_t i = 0; i < raw_server_blocks_.size(); ++i) {
        ServerConfig server = parseSingleServerBlock(raw_server_blocks_[i]);
        servers_.push_back(server);
    }

    for (size_t i = 0; i < servers_.size(); ++i) {
        std::cout << config::colors::magenta << "Config of Server[" << i << "]\n"
                  << config::colors::reset;
        std::cout << servers_[i];
    }
}

void ConfigParser::parseListen(ServerConfig& server, const std::vector< std::string >& tokens) {
    if (tokens.size() < 2) {
        throw ConfigException("Missing argument for listen");
    }
    std::string value = config::utils::removeSemicolon(tokens[1]);
    size_t pos = value.find(':');

    /*
    std::cout << "current directive: [" << directive << "]" << std::endl;
    std::cout << "value [" << value << "]" << std::endl;
	function to check if pattern is correct respect to PORT:IP
    8080:192.178.1.1
    */
    if (pos != std::string::npos) {
        // Case: IP:PORT (127.0.0.1:8080)
        std::string ip = value.substr(0, pos);
        std::string port = value.substr(pos + 1);

        if (ip.find_first_not_of("0123456789") == std::string::npos) {
            server.setPort(config::utils::stringToInt(ip));
            server.setHost(port);
        } else {
            server.setHost(ip);
            server.setPort(config::utils::stringToInt(port));
        }
    } else {
        // Case: PORT (8080) or only HOST (localhost)
        // Simple heurística: Si tiene digitos es puerto, sino host
        if (value.find_first_not_of("0123456789") == std::string::npos) {
            server.setPort(config::utils::stringToInt(value));
        } else {
            // TODO: move to cosntant
            server.setHost(value);
            server.setPort(80); // DEFAULT ?
        }
    }
}

void ConfigParser::parseMaxSizeBody(ServerConfig& server, const std::vector< std::string >& tokens) {
    const std::string& maxSizeStr = config::utils::removeSemicolon(tokens[1]);
    if (!maxSizeStr.empty()) {
        config::utils::removeSemicolon(maxSizeStr);
        server.setMaxBodySize(config::utils::stringToInt(maxSizeStr));
    }
}

/**
 * Lógica especial para múltiples códigos de error
 * error_page 404 500 /error.html;
 * error_page 404 /404.html;
 * error_page 500 502 503 504 /50x.html;
 * el último token es siempre la ruta del archivo (ej. /404.html)
 */
void ConfigParser::parseErrorPage(ServerConfig& server, std::vector< std::string >& tokens) {
    if (tokens.size() >= 3)
    // el mínimo son 3 tokens: error_page, 404 y /ruta)
    {
        std::string path = config::utils::removeSemicolon(tokens.back());
        for (size_t i = 1; i < tokens.size() - 1; ++i) {
            server.addErrorPage(config::utils::stringToInt(tokens[i].c_str()), path);
        }
    }
}

/**
 * check number of arguments:
 * upload_store;	INVALID
 * upload_store /uploads;	VALID
 * upload_store /uploads extra;	INVALID
 * upload_store "";		no valido, str Vacío
 * upload_store /uploads;	Ruta absoluta
 * upload_store ./uploads;	Ruta relativa
 * upload_store uploads;		Ruta relativa (sin ./)
 * upload_store /up\0loads;         X Null byte
 * upload_store /up\nloads;         X Newline
 */
void ConfigParser::parseUploadBonus(LocationConfig& loc, std::vector< std::string >& locTokens) {
    if (locTokens.size() < 2) {
        throw ConfigException(config::errors::invalid_min_num_args_upload_directive);
    }
    if (locTokens.size() > 2) {
        throw ConfigException(config::errors::invalid_max_num_args_upload_directive);
    }

    std::string uploadPathClean = config::utils::removeSemicolon(locTokens[1]);
    if (uploadPathClean.empty()) {
        throw ConfigException(config::errors::empty_path_in_upload_directive);
    }
    //	validamos los caracteres no recomendables
    if (!config::utils::isValidPath(uploadPathClean)) {
        throw ConfigException(config::errors::invalid_characters_in_upload_directive +
                              uploadPathClean);
    }
    // TODO: verificar que el directorio existe o se
    /*
			(!config::utils::directoryExists(uploadPath))
			upload_store directory does not exist: "uploadPath;}
    */
    loc.setUploadStore(uploadPathClean);
}

void ConfigParser::parseReturn(LocationConfig& loc, std::vector< std::string >& locTokens) {
    //	return 301 http:://google.com;
    //	return code URL;
    //	302 - Código de estado HTTP "Found" (redirección temporal)
    // http://example.com - URL de destino
    if (locTokens.size() == 2) {
        // Caso: return URL; (default 302)
        std::string cleanUrl = config::utils::removeSemicolon(locTokens[1]);

        loc.setRedirectCode(config::section::default_return_code);
        loc.setRedirectUrl(cleanUrl);
        loc.setRedirectParamCount(1);
    } else if (locTokens.size() == 3) {
        // Caso: return CODE URL;
        int code = config::utils::stringToInt(locTokens[1]);
        std::string cleanUrl = config::utils::removeSemicolon(locTokens[2]);

        // Validamos que el codigo sea de redireccion (3XX) (relaxed to
        // 100-599)
        if ((code < 300 || code > 399) && code != 404 && code != 200 && code != 403 &&
            code != 500 && code != 405) {
            if (code < 100 || code > 599) {
                throw ConfigException(config::errors::invalid_redirect_code + locTokens[1]);
            }
        }
        loc.setRedirectCode(code);
        loc.setRedirectUrl(cleanUrl);
        loc.setRedirectParamCount(2);
    } else {
        throw ConfigException(config::errors::missing_args_in_return);
    }
}

void ConfigParser::parseRoot(ServerConfig& server, const std::vector< std::string >& tokens) {
    server.setRoot(config::utils::removeSemicolon(tokens[1]));
}

void ConfigParser::parseIndex(ServerConfig& server, const std::vector< std::string >& tokens) {
    for (size_t i = 1; i < tokens.size(); ++i) {
        server.addIndex(config::utils::removeSemicolon(tokens[i]));
    }
}

void ConfigParser::parseCgi(LocationConfig& loc, const std::vector< std::string >& tokens) {
    if (tokens.size() < 3) {
        throw ConfigException("Missing arguments for cgi directive");
    }
    std::string extension = tokens[1];
    std::string binaryPath = config::utils::removeSemicolon(tokens[2]);

    if (extension[0] != '.') {
        throw ConfigException("CGI extension must start with '.' " + extension);
    }

    loc.addCgiHandler(extension, binaryPath);
}

void ConfigParser::parseServerName(ServerConfig& server, const std::vector< std::string >& tokens) {
    server.setServerName(config::utils::removeSemicolon(tokens[1]));
}

void ConfigParser::parseLocationBlock(ServerConfig& server, std::stringstream& ss,
                                      std::string& line, std::vector< std::string >& tokens) {
    size_t pathIndex = 1;
    std::string modifier = ""; // +, ~, ~*, ^~

    if (tokens.size() > 2 && (tokens[1] == config::section::exact_match_modifier ||
                              tokens[1] == config::section::preferential_prefix_modifier)) {
        modifier = tokens[1];
        pathIndex = 2;
    }

    std::string locationPath = tokens[pathIndex];
    LocationConfig loc;
    loc.setPath(locationPath);

    while (std::getline(ss, line)) {
        line = config::utils::trimLine(line);

        if (line.empty())
            continue;
        std::vector< std::string > locTokens = config::utils::tokenize(line);
        if (locTokens.empty())
            continue;
        const std::string& directive = locTokens[0];
        if (directive == "}") {
            break; // End of location block
        } else if (directive == config::section::location) {
            throw ConfigException(config::errors::invalid_new_location_block + line);
        } else if (directive == config::section::root) {
            loc.setRoot(config::utils::removeSemicolon(locTokens[1]));
        } else if (directive == config::section::index) {
            for (size_t i = 1; i < locTokens.size(); ++i) {
                loc.addIndex(config::utils::removeSemicolon(locTokens[i]));
            }
        } else if (directive == config::section::autoindex) {
            std::string val = config::utils::removeSemicolon(locTokens[1]);
            if (val != config::section::autoindex_on && val != config::section::autoindex_off) {
                throw ConfigException(config::errors::invalid_autoindex);
            }
            loc.setAutoIndex(val == config::section::autoindex_on);
        } else if (directive == config::section::methods ||
                   directive == config::section::allow_methods ||
                   directive == config::section::limit_except) {
            for (size_t i = 1; i < locTokens.size(); ++i) {
                loc.addMethod(config::utils::removeSemicolon(locTokens[i]));
            }
        } else if (directive == config::section::return_str) {
            parseReturn(loc, locTokens);
        } else if (directive == config::section::uploads_bonus ||
                   directive == config::section::upload_bonus) {
            parseUploadBonus(loc, locTokens);
        } else if (directive == config::section::cgi || directive == config::section::cgi_fast) {
            parseCgi(loc, locTokens);
        }
    }
    server.addLocation(loc);
}

ServerConfig ConfigParser::parseSingleServerBlock(const std::string& blockContent) {
    ServerConfig server;
    std::stringstream ss(blockContent);
    std::string line;

    while (getline(ss, line)) {
        int indexTokens = 0;
        line = config::utils::trimLine(line);

        std::vector< std::string > tokens = config::utils::tokenize(line);
        if (tokens.empty())
            continue;

        const std::string& directive = tokens[indexTokens];

        //	LISTEN
        // std::cout << "current directive: [" << directive << "]" << std::endl;
        if (directive == config::section::listen) {
            parseListen(server, tokens);
        } else if (directive == config::section::server_name) {
            parseServerName(server, tokens);
        } else if (directive == config::section::root) {
            parseRoot(server, tokens);
        } else if (directive == config::section::index) {
            parseIndex(server, tokens);
        } else if (directive == config::section::client_max_body_size) {
            parseMaxSizeBody(server, tokens);
        } else if (directive == config::section::error_page) {
            parseErrorPage(server, tokens);
        }
        //	TODO: this case fail(the char '='): location = /50x.html {
        else if (directive == config::section::location) {
            parseLocationBlock(server, ss, line, tokens);
        }
        ++indexTokens;
    }
    return server;
}
