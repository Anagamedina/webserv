Excelente pregunta. Te explico cómo funciona la directiva return en Nginx:

Directiva return en Nginx
Sintaxis básica:
nginx
return code [text];
return code URL;
return URL;
Tu ejemplo: return 302 http://example.com;
Esta línea hace lo siguiente:

302 - Código de estado HTTP "Found" (redirección temporal)
http://example.com - URL de destino
Comportamiento:

Cuando un cliente hace una petición a esta location, Nginx inmediatamente responde con:
http
HTTP/1.1 302 Found
Location: http://example.com
El navegador del cliente automáticamente redirige a http://example.com
No se sirve ningún archivo, la redirección ocurre antes
Códigos de redirección comunes:
nginx
return 301 http://new-site.com;  # Permanent redirect (SEO friendly)
return 302 http://temp-site.com; # Temporary redirect
return 303 http://other.com;     # See Other
return 307 http://temp.com;      # Temporary redirect (mantiene método HTTP)
return 308 http://perm.com;      # Permanent redirect (mantiene método HTTP)
¿Qué pasa si hay más de 3 parámetros?
Respuesta corta: Nginx ignora los parámetros extra o lanza un error de configuración.

Casos:
1. Con código + URL (2 parámetros) ✅
   nginx
   return 302 http://example.com;  # ✅ VÁLIDO
2. Con código + texto (2 parámetros) ✅
   nginx
   return 404 "Page not found";    # ✅ VÁLIDO
3. Solo URL (1 parámetro) ✅
   nginx
   return http://example.com;      # ✅ VÁLIDO (asume 302)
4. Más de 2 parámetros ❌
   nginx
   return 302 http://example.com extra;  # ❌ ERROR de sintaxis
   return 302 http://example.com "text"; # ❌ ERROR de sintaxis
   Error que lanzaría Nginx:

nginx: [emerg] invalid number of arguments in "return" directive
Implementación en tu parser
Mirando tu código actual en
ConfigParser.cpp:466-474
:

cpp
else if (locTokens[0] == config::section::return_str)
{
// simple support: return 301 /url;
if (locTokens.size() >= 3)
{
loc.setRedirect(
config::utils::removeSemicolon(locTokens[2]));
}
}
Problemas con tu implementación actual:
Solo guardas la URL, no el código de estado
No validas el número de parámetros
No soportas return URL; (sin código)
Mejora sugerida:
cpp
else if (locTokens[0] == config::section::return_str)
{
// return puede tener 2 o 3 tokens:
// return 301 /url;        -> 3 tokens (return, 301, /url;)
// return http://url;      -> 2 tokens (return, http://url;)

    if (locTokens.size() == 2)
    {
        // Caso: return URL; (asume 302)
        std::string url = config::utils::removeSemicolon(locTokens[1]);
        loc.setRedirectCode(302);  // Default
        loc.setRedirectUrl(url);
    }
    else if (locTokens.size() == 3)
    {
        // Caso: return CODE URL;
        int code = config::utils::stringToInt(locTokens[1]);
        std::string url = config::utils::removeSemicolon(locTokens[2]);
        
        // Validar que el código sea de redirección (3xx)
        if (code < 300 || code > 399)
        {
            throw ConfigException("Invalid redirect code: " + locTokens[1]);
        }
        
        loc.setRedirectCode(code);
        loc.setRedirectUrl(url);
    }
    else if (locTokens.size() > 3)
    {
        throw ConfigException("Invalid number of arguments in 'return' directive");
    }
    else
    {
        throw ConfigException("Missing arguments in 'return' directive");
    }
}
Cambios necesarios en
LocationConfig
:
cpp
// LocationConfig.hpp
private:
int redirect_code_;        // 301, 302, etc.
std::string redirect_url_; // URL de destino
public:
void setRedirectCode(int code);
void setRedirectUrl(const std::string& url);
int getRedirectCode() const;
const std::string& getRedirectUrl() const;
bool hasRedirect() const;  // Helper para saber si hay redirect
cpp
// LocationConfig.cpp
LocationConfig::LocationConfig() :
autoindex_(false),
redirect_code_(0)  // 0 = sin redirect
{
}
void LocationConfig::setRedirectCode(int code) { redirect_code_ = code; }
void LocationConfig::setRedirectUrl(const std::string& url) { redirect_url_ = url; }
int LocationConfig::getRedirectCode() const { return redirect_code_; }
const std::string& LocationConfig::getRedirectUrl() const { return redirect_url_; }
bool LocationConfig::hasRedirect() const { return redirect_code_ != 0; }
Resumen
Sintaxis	Tokens	Comportamiento
return 302 http://example.com;	3	✅ Redirect temporal a URL
return http://example.com;	2	✅ Redirect (asume 302)
return 404 "Not found";	3	✅ Respuesta con texto
return 302 http://example.com extra;	4+	❌ ERROR
Tu implementación actual solo maneja el caso de 3 tokens y solo guarda la URL, no el código. Te recomiendo implementar la mejora que te sugerí para tener soporte completo de return. 🚀