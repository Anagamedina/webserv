#include <iostream>

#include "../../src/client/RequestProcessor.hpp"
#include "../../src/http/HttpRequest.hpp"
#include "../../src/http/HttpResponse.hpp"

int main()
{
    RequestProcessor processor;
    HttpRequest request;
    HttpResponse response;

    request.setMethod("GET");
    request.setVersion("HTTP/1.1");
    request.setPath("/index.html");
    request.addHeaders("host", "localhost");

    processor.process(request, 0, 8080, false, response);

    std::vector<char> raw = response.serialize();
    std::cout.write(&raw[0], raw.size());
    std::cout << std::endl;

    return 0;
}
