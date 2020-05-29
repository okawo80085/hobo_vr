#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

TEST_CASE( "bob", "[f]" ) {
    REQUIRE( 1 == -1 );
}

TEST_CASE( "2", "[f]" ) {
    REQUIRE( 1 == -1 );
}