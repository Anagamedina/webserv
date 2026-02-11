#include "HttpParser.hpp"

HttpParser::HttpParser() : _maxBodySize(0), _errorStatusCode(400) { reset(); }

HttpParser::~HttpParser() {}

State HttpParser::getState() const { return _state; }

const HttpRequest& HttpParser::getRequest() const { return _request; }

int HttpParser::getErrorStatusCode() const {
  return (_state == ERROR) ? _errorStatusCode : 0;
}

void HttpParser::reset() {
  // Limpia estado de parsing y contenedores de la petición actual. No toca _buffer
  // (puede contener datos de la siguiente petición pipelined).
  _state = PARSING_START_LINE;
  _stateChunk = CHUNK_SIZE;
  _request.clear();
  _chunkBuffer.clear();
  _contentLength = 0;
  _isChunked = false;
  _bytesRead = 0;
  _chunkSize = 0;
  _errorStatusCode = 400;
  // _maxBodySize NO se resetea: se establece una vez en el constructor de Client
  // y debe persistir para que todas las peticiones Keep-Alive usen el mismo límite.
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

    if (_state == ERROR) break;

    if (_state == COMPLETE) {
      // Dejar la petición disponible para el caller.
      // El caller decide cuándo llamar a reset().
      // IMPORTANTE!!: no se debe perder el request antes de leerlo.
      break;
    }

    // Si no se cambió de estado, asumimos que falta más data.
    if (_state == prevState) break;
  }
}
