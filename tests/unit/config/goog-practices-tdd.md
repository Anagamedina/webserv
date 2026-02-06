Para testear esta función específica tienes un reto interesante: es un método privado y depende de una variable interna (clean_file_str_).

Esto es lo que dice la teoría de buenas prácticas: "No testees métodos privados, testea el comportamiento público". Sin embargo, para aprender y para lógica compleja como esta, a veces queremos probarla aislada.

Aquí te presento las 2 mejores opciones. Yo te recomiendo la Opción 1 por ser la más limpia.

Opción 1: Refactorizar a "Función Pura" (Recomendada)
El método
ValidateCurlyBrackets
no necesita acceder a this ni a variables de la clase si le pasas el string como argumento. Si lo conviertes en una función estática (o la mueves a una clase Utils), se vuelve facilísima de probar.

1. Modifica
   ConfigParser.hpp
   :

dataconf
public:
// Hazla estática y pública (o ponla en una clase Utils separada)
static bool CheckBraces(const std::string& content);
2. Crea el archivo del test: tests/unit/config/test_ConfigParser_internals.cpp (o similar).

3. Escribe el test (AAA):

cpp
#include "../../lib/catch2/catch.hpp"
#include "ConfigParser.hpp"
TEST_CASE("Validación de llaves (Curly Brackets)", "[config][parser]") {
// SECTION 1: Casos positivos (Happy Path)
SECTION("Debe aceptar bloques bien balanceados") {
// ARRANGE
std::string input = "server { location / { } }";

        // ACT & ASSERT
        REQUIRE( ConfigParser::CheckBraces(input) == true );
    }
    // SECTION 2: Casos negativos (Edge Cases)
    SECTION("Debe rechazar llaves desbalanceadas") {
        REQUIRE( ConfigParser::CheckBraces("server {") == false );       // Falta cierre
        REQUIRE( ConfigParser::CheckBraces("server } {") == false );     // Cierre antes de apertura
        REQUIRE( ConfigParser::CheckBraces("server { }}") == false );    // Sobra cierre
    }
}
Opción 2: Testear a través del API Público (Integración)
Si no quieres tocar el código original (dejarlo privado), tienes que probarlo "desde fuera", llamando a
parse()
. Esto es más lento/tedioso porque requiere crear archivos falsos.

cpp
TEST_CASE("Parser falla con llaves incorrectas", "[config]") {
// ARRANGE: Crear un archivo temporal malo
std::ofstream file("bad_braces.conf");
file << "server { port 80;";
file.close();

    // ACT & ASSERT
    ConfigParser parser("bad_braces.conf");
    REQUIRE_THROWS( parser.parse() );
    
    // Cleanup
    std::remove("bad_braces.conf");
}
Mi recomendación
Usa la Opción 1. Extraer lógica compleja (como validadores de strings) a funciones estáticas públicas (o a un ConfigUtils) es una práctica excelente porque:

Hace el código más reutilizable.
Hace los tests ultra rápidos (no tocan disco).
Te permite probar casos extremos (}}}}) muy fácilmente.