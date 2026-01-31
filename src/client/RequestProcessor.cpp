#include "RequestProcessor.hpp"
#include "ErrorUtils.hpp"
#include "ResponseUtils.hpp"
#include "StaticPathHandler.hpp"
#include "../config/LocationConfig.hpp"


namespace
{
	//Helper function to strip the port from the host header.
	//ej: "localhost:8080" -> "localhost"
	//ej: "localhost" -> "localhost" (no tiene puerto)
/*	static std::string stripPortFromHost(const std::string& hostHeader)
	{
		std::string result_header = hostHeader;
		std::string::size_type colon = result_header.find(':');
		if (colon != std::string::npos)
			result_header.erase(colon);

		return (result_header);
	}*/

	/*static const ServerConfig* selectServerByHost(const std::string& hostHeader,
												const std::vector<ServerConfig>*
												configs)
	{
		if (configs == 0 || configs->empty())
			return 0;

		std::string host = stripPortFromHost(hostHeader);
		for (size_t i = 0; i < configs->size(); ++i)
		{
			if ((*configs)[i].getServerName() == host)
				return &(*configs)[i];
		}
		//comportamiento por defecto: usar el primer server.
		return &(*configs)[0];
	}*/

	static const ServerConfig* selectServerByPort(int port,
												const std::vector<ServerConfig>*
												configs)
	{
		if (configs == 0 || configs->empty())
			return 0;

		for (size_t i = 0; i < configs->size(); ++i)
		{
			if ((*configs)[i].getPort() == port)
				return &(*configs)[i];
		}
		// comportamiento por defecto: usar el primer server.
		return &(*configs)[0];
	}

	static const LocationConfig* matchLocation(const ServerConfig& server,
												const std::string& uri)
	{
		const std::vector<LocationConfig>& locations = server.getLocations();
		if (locations.empty())
			return 0;

		// TODO: elegir la location con el mejor match de path.
		// Falta acceso a getters de LocationConfig para comparar path.
		// Cuando existan getters Daru:
		// const std::string& path = locations[i].getPath();
		// const std::string& root = locations[i].getRoot();
		// const std::string& index = locations[i].getIndex();
		// bool autoindex = locations[i].getAutoindex();
		// const std::vector<std::string>& methods = locations[i].getMethods();
		// const std::string& upload = locations[i].getUploadStore();
		// const std::string& redirect = locations[i].getRedirect();

		// PSEUDOCODIGO MATCH LOCATION : mejor match de path.

		// Pseudologica de match:
		// size_t best = 0;
		//const LocationConfig* bestLoc = 0;
		// for cada location:
		//   if uri empieza por path y (path == "/" o uri[path.size()] == '/'):
		//       si path.size() > best -> best = path.size(); bestLoc = &location;
		// return bestLoc;
		(void)uri;
		//por defecto, usar la primera location.
		return &locations[0];
	}


	//funcion auxiliar para resolver la ruta real del archivo.
	//ej: "/index.html" -> "./www/index.html"
	//ej: "/index.html" -> "./www/index.html"
	//path real : root + uri (si no existe root, usar "./www")
	//si uri no empieza por "/", agregar "/" al principio.

	static std::string resolvePath(const ServerConfig& server,
									const LocationConfig* location,
									const std::string& uri)
	{
		std::string root = "./www";
		// TODO: usar getters cuando existan:
		// if (location) root
		//       root = location->getRoot();
		// if (root.empty())
		//      root = server.getRoot();

		std::string path = root;
		if (!path.empty() && path[path.size() - 1] != '/' && !uri.empty() && uri
			[0] != '/')
			path += "/";
		path += uri;
		(void)server;
		(void)location;
		return path;
	}

	//funcion auxiliar para verificar si la peticion es un request CGI.
	//ej: "/index.php" -> true
	//ej: "/index.html" -> false
	// dotPos es la posicion del ultimo punto en la ruta.
	// ext es la extension de la ruta.
	// ej: "/index.php" -> "php"
	// ej: "/index.html" -> "html"
	static bool isCgiRequest(const std::string& path)
	{
		// Borrador: por ahora, CGI si la extension es .py o .php
		std::string::size_type dotPos = path.find_last_of('.');
		if (dotPos == std::string::npos)
			return false;
		std::string ext = path;
		ext.erase(0, dotPos);
		return (ext == ".py" || ext == ".php");
	}

	static std::string methodToString(HttpMethod method)
	{
		if (method == HTTP_METHOD_GET)
			return "GET";
		if (method == HTTP_METHOD_POST)
			return "POST";
		if (method == HTTP_METHOD_DELETE)
			return "DELETE";
		return "";
	}

	static int validateLocation(const HttpRequest& request,
								const ServerConfig* server,
								const LocationConfig* location)
	{
		// 1) Redirect -> responder y salir (pendiente de getters de LocationConfig)
		// TODO: if (!location->getRedirect().empty()) return 301;

		// 2) Metodo permitido
		if (!location->isMethodAllowed(methodToString(request.getMethod())))
			return 405;

		// 3) Body size (usar limite del server por ahora)
		if (server && request.getBody().size() > server->getMaxBodySize())
			return 413;

		return 0;
	}
} // namespace


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
			buildErrorResponse(response, request, validationCode, true, server);
			return;
		}

		path_real = resolvePath(*server, location, request.getPath());
		isCgi = isCgiRequest(path_real);

		if (isCgi)
		{
			// TODO: integrar CGI: delegar en CgiHandler (Carles).
			buildErrorResponse(response, request, 501, true, server);
			return;
		}

		if (handleStaticPath(request, server, path_real, body, response))
			return;
	}
	else
	{
		buildErrorResponse(response, request, 404, false, server);
		return;
	}

	// TODO: pseudologica de decision estatica vs CGI (cuando LocationConfig tenga getters):
	// if (location) {
	//   if (!location->getRedirect().empty()) -> 301/302
	//   if (!location->isMethodAllowed(request.getMethod())) -> 405
	//   if (existe CGI para la extension) -> CgiHandler
	//   else -> servir estatico (root + uri)
	// }
	fillBaseResponse(response, request, statusCode, shouldClose, body);
}
