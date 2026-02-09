#include "HttpParser.hpp"

#include <algorithm>
#include <cstdlib>
#include <vector>

// PARSE BODY ---------------------------------------------------------------
void HttpParser::parseBody() {
    if (_isChunked)
        parseBodyChunked();
    else
        parseBodyFixedLength();
}

/*
 * Maneja el caso donde concemos el tamano exacto (content-length) del body.
 */
void HttpParser::parseBodyFixedLength() {
    std::size_t remaining = 0;
    if (_contentLength > _bytesRead)
        remaining = _contentLength - _bytesRead;

    std::size_t toRead = std::min(_buffer.size(), remaining);
    if (toRead > 0) {
        _request.addBody(_buffer.begin(), _buffer.begin() + toRead);
        // necesito saber cuantos bytes del body he llevo acumulados para saber si he leido todo el
        // body.
        //  y comparar con el content-length para saber si he leido todo el body.
        _bytesRead += toRead;
        // elimino los bytes leidos del buffer para no leerlos de nuevo.
        _buffer.erase(0, toRead);
    }

    if (_bytesRead == _contentLength)
        _state = COMPLETE;
}

bool HttpParser::parseChunkSizeLine(std::size_t& size) {
    std::string line;
    std::string::size_type semi;
    char* endPtr = 0;
    unsigned long value = 0;

    if (!extractLine(line))
        return false; // falta data
    // OJO! Según el protocolo, aquí debe haber un número. Si no hay, es un error.
    if (line.empty()) {
        _state = ERROR;
        return false;
    }

    // OJO! El estándar HTTP (RFC 9112) dice explícitamente: "Un receptor DEBE ignorar las
    // extensiones de chunk que no comprenda
    //  Ignorar extensiones (ej: "1a;ext=foo")
    semi = line.find(';');
    if (semi != std::string::npos)
        line = line.substr(0, semi);
    // strtoul mueve el puntero endPtr hasta donde dejó de leer.
    // Si endPtr se queda al principio del string (line.c_str()), significa que no pudo leer ni un
    // solo número válido.
    value = std::strtoul(line.c_str(), &endPtr, 16);
    if (endPtr == line.c_str()) {
        _state = ERROR;
        return false;
    }

    size = value;
    return true;
}

bool HttpParser::handleChunkSizeState() {
    std::size_t size = 0;

    if (!parseChunkSizeLine(size))
        return false; // falta data o error ya seteado

    _chunkSize = size;
    // OJO! En el protocolo HTTP Chunked, el final del mensaje se indica siempre con un bloque de
    // tamaño 0.
    if (_chunkSize == 0) {
        _stateChunk = CHUNK_END;
        return true;
    }

    _stateChunk = CHUNK_DATA;
    return true;
}

// Según el protocolo HTTP, cada fragmento de datos debe terminar con un \r\n
bool HttpParser::handleChunkDataState() {
    // Necesitamos datos + "\r\n"
    std::size_t needed = _chunkSize + 2;
    // OJO! Si el buffer no tiene suficientes bytes, faltan datos.
    if (_buffer.size() < needed)
        return false; // falta data

    if (_buffer[_chunkSize] != '\r' || _buffer[_chunkSize + 1] != '\n') {
        _state = ERROR;
        return false;
    }
    // solo copiamos los datos, no copiamos el \r\n
    if (_chunkSize > 0)
        _request.addBody(_buffer.begin(), _buffer.begin() + _chunkSize);
    _buffer.erase(0, needed); // datos + CRLF

    _stateChunk = CHUNK_SIZE;
    return true;
}

/*
* Los trailer son headers extra que vienen después del body chunked.
5\r\n        <-- Tamaño
HOLA!\r\n     <-- Datos
0\r\n        <-- EL CERO QUE PREGUNTABAS (Fin de datos)
Expires: Wed, 21 Oct 2025 07:28:00 GMT\r\n  <-- ESTO ES UN TRAILER
\r\n         <-- EL ÚLTIMO \r\n QUE MARCA EL FIN ABSOLUTO
*
*
*/
bool HttpParser::handleChunkEndState() {
    // Después de "0\r\n" puede venir "\r\n" final o trailers.
    std::string line;
    if (!extractLine(line))
        return false; // falta data

    if (line.empty()) {
        _stateChunk = CHUNK_COMPLETE;
        _state = COMPLETE;
        return false;
    }

    // Si hay trailers, los ignoramos por ahora y seguimos leyendo líneas
    return true;
}

/*
 * Maneja el caso donde el body viene en trozos (chunked)
 * chunked es un encabezado que indica que el body viene en trozos
 * y cada trozo tiene un tamano especifico.
 * el body se compone de una secuencia de trozos de datos
 * cada trozo termina con un CRLF (carriage return line feed)
 * el ultimo trozo termina con un 0 seguido de CRLF
 * el body se compone de una secuencia de trozos de datos
 * cada trozo termina con un CRLF (carriage return line feed)
 * el ultimo trozo termina con un 0 seguido de CRLF
 */

void HttpParser::parseBodyChunked() {
    while (true) {
        if (_stateChunk == CHUNK_SIZE) {
            if (!handleChunkSizeState())
                return;
            continue;
        }

        if (_stateChunk == CHUNK_DATA) {
            if (!handleChunkDataState())
                return;
            continue;
        }

        if (_stateChunk == CHUNK_END) {
            if (!handleChunkEndState())
                return;
            continue;
        }

        return;
    }
}
