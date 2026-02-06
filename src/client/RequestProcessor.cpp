#include "RequestProcessor.hpp"
#include "ErrorUtils.hpp"
#include "ResponseUtils.hpp"
#include "StaticPathHandler.hpp"
#include "RequestProcessorUtils.hpp"


// Helpers movidos a RequestProcessorUtils.cpp
//MAIN FUNCTION: LOGIC OF THE PROCESSOR.
// Flujo general:
// 1) Inicializar status/body/shouldClose
// 2) Matching virtual host (ServerConfig por PORT)
// 3) Matching location (LocationConfig por URI)
// 4) Validaciones (metodo, body size, redirect)
// 5) Resolver path real (root/alias + uri)
// 6) Decidir respuesta (estatico o CGI) + errores
// 7) Rellenar HttpResponse
void RequestProcessor::process(const HttpRequest& request,
								const std::vector<ServerConfig>* configs,
								int listenPort,
								bool parseError,
								HttpResponse& response)
{
	int statusCode = HTTP_STATUS_OK;
	std::string path_real = "";
	bool isCgi = false;
	std::vector<char> body;
	bool shouldClose = request.shouldCloseConnection();
	const ServerConfig* server = 0;
	const LocationConfig* location = 0;

	// 1) Errores primero: si el parser falló, respondemos 400 y salimos.
	if (parseError || request.getMethod() == HTTP_METHOD_UNKNOWN)
	{
		statusCode = HTTP_STATUS_BAD_REQUEST;
		body = toBody("Bad Request\n");
		shouldClose = true;
		fillBaseResponse(response, request, statusCode, shouldClose, body);
		return;
	}
	//2) Select server, match location
	server = selectServerByPort(listenPort, configs);
	// Host como extra (opcional):
	// if (server == 0) {
	//     server = selectServerByHost(request.getHeader("Host"), configs);
	// }
	if (server)
		location = matchLocation(*server, request.getPath());

	if (location)
	{
		//si hay location, resolver la ruta real.
		int validationCode = validateLocation(request, server, location);
		if (validationCode != 0)
		{
			if (validationCode == 301 || validationCode == 302)
			{
				//TODO revisar redirect
				body.clear();
				fillBaseResponse(response, request, validationCode, shouldClose, body);
				response.setHeader("Location", location->getRedirectUrl());
				//Header obligatorio: 
				//Location: http://www.google.com
				return;
			}
			buildErrorResponse(response, request, validationCode, true, server);
			return;
		}

		path_real = resolvePath(*server, location, request.getPath());
		isCgi = isCgiRequest(path_real);

		if (isCgi)
		{
			// TODO: integrar CGI: delegar en CgiHandler (Carles).
			// TODO tengo que llamar a una funcion que recoja el fd del
			// cliente? o ya deberia hacerlo en la funcion de cgiHnadler? 
			buildErrorResponse(response, request, 501, true, server);
			return;
		}

		if (handleStaticPath(request, server, location, path_real, body, response))
			return;
	}
	else
	{
		buildErrorResponse(response, request, 404, false, server);
		return;
	}

	fillBaseResponse(response, request, statusCode, shouldClose, body);
}
