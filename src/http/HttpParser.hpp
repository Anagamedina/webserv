#ifndef HTTP_PARSER_HPP
#define HTTP_PARSER_HPP

#include <cstddef>
#include <string>

#include "HttpRequest.hpp"

enum State {
    PARSING_START_LINE,
    PARSING_HEADERS,
    PARSING_BODY,
    COMPLETE,
    ERROR
};

enum StateChunk {
    CHUNK_SIZE,
    CHUNK_DATA,
    CHUNK_END,
    CHUNK_ERROR,
    CHUNK_COMPLETE
};

class HttpParser {
  public:
    HttpParser();
    ~HttpParser();
    // Estas funciones es lo unico que otra clase puede hacer con el parser
    void reset();
    void consume(const std::string& data);
    State getState() const;
    const HttpRequest& getRequest() const;

  private:
    // Estado y datos internos
    State _state;
    StateChunk _stateChunk;
    HttpRequest _request;
    std::string _buffer;      // acumulador interno para líneas/body pendientes
    std::string _chunkBuffer; // acumulador interno para cuerpos de chunked
    std::size_t _contentLength;
    bool _isChunked;
    std::size_t _bytesRead;
    std::size_t _chunkSize; // tamaño del chunk actual
    // std::size_t _maxBodySize;

    // Helpers generales
    bool extractLine(std::string& line);

    // Start line
    bool splitStartLine(const std::string& line, std::string& method, std::string& uri,
                        std::string& version);
    void parseUri(const std::string& uri);
    void parseStartLine();

    // Headers
    bool splitHeaderLine(const std::string& line, std::string& key, std::string& value);
    void handleHeader(const std::string& key, const std::string& value);
    bool processHeaderLine(const std::string& line);
    void parseHeaders();
    bool validateHeaders() const;

    // Body
    bool parseChunkSizeLine(std::size_t& size);
    void parseBodyFixedLength();
    void parseBodyChunked();
    void parseBody();
    bool handleChunkSizeState();
    bool handleChunkDataState();
    bool handleChunkEndState();
};

#endif // HTTP_PARSER_HPP
