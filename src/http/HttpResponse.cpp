#include "HttpResponse.hpp"

#include <sstream>
//el reto es pegar la cabecera 
//setters para binarios (imagenes)
void HttpResponse::setBody(const std::vector<char>& body)
{
    _body = body;
}


void HttpResponse::setBody(const std::string& body)
{
    _body.assign(body.begin(), body.end());
}



//SERIALIZE
//
std::vector<char> HttpResponse::serialize() const
{
    std::stringstream buffer;
    
    //construir la primera linia y header
    buffer << _version << " " << _status << " " << _reasonPhrase << "\r\n";


    //iterar pior el mapa de headers, headers: key : value\r\n
   //no se declara _headers porque? 
    for (HeaderMap::const_iterator it = _headers.begin();
            it != _headers.end(); ++it) {
        buffer << it->first << ": " << it->second << "\r\n";
    }


    buffer << "Content-Length: " << _body.size() << "\r\n";

    buffer << "\r\n";
    
    //convertir la parte de texto a vector 
    std::string headStr =  buffer.str();
    std::vector<char> response(headStr.begin(), headStr.end());

//insertar el cuerpo binario al final

    response.insert(response.end(), _body.begin(), _body.end());

    return (response);
}



