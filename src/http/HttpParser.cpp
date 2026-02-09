#include "HttpParser.hpp"

HttpParser::HttpParser() {
    reset();
}

HttpParser::~HttpParser() {
}

State HttpParser::getState() const {
    return _state;
}

const HttpRequest& HttpParser::getRequest() const {
    return _request;
}

void HttpParser::reset() {
    _state = PARSING_START_LINE;
    _stateChunk = CHUNK_SIZE;
    _request.clear();
    _chunkBuffer.clear();
    _contentLength = 0;
    _isChunked = false;
    _bytesRead = 0;
    _chunkSize = 0;
    //_maxBodySize = 0; // TODO: setear desde config (Daru)
}

/*
 * Consume los datos recibidos del cliente y los procesa.
 * @param data: Los datos recibidos del cliente.
 */
void HttpParser::consume(const std::string& data) {
    _buffer.append(data);

    while (true) {
        State prevState = _state;

        switch (_state) {
        case PARSING_START_LINE:
            parseStartLine();
            break;
        case PARSING_HEADERS:
            parseHeaders();
            break;
        case PARSING_BODY:
            parseBody();
            break;
        default:
            break;
        }

        if (_state == ERROR)
            break;

        if (_state == COMPLETE) {
            // Dejar la petición disponible para el caller.
            // El caller decide cuándo llamar a reset().
            // IMPORTANTE!!: no se debe perder el request antes de leerlo.
            break;
        }

        // Si no se cambió de estado, asumimos que falta más data.
        if (_state == prevState)
            break;
    }
}
