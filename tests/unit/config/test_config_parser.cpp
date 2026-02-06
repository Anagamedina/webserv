#include <fstream>

#include "../../lib/catch2/catch.hpp"
#include "../../src/config/ConfigParser.hpp"

TEST_CASE("VALIDACION DE LLAVES(CURLY BRACKETS", "[config][parser]")
{

	//	Arrange
	std::ofstream file("bad_braces.conf");
	file<< "server { port 80";
	file.close();

	// ACT && ASSERT
	ConfigParser parser("bad_braces.conf");
	REQUIRE_THROWS(parser.parse());

	//Cleanup
	std::remove("bad_braces.conf");
}