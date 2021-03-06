//
//  test-SymbolIdentifier.cc
//  snowcrash
//
//  Created by Zdenek Nemec on 2/7/14.
//  Copyright (c) 2014 Apiary Inc. All rights reserved.
//

#include "snowcrash.h"
#include "snowcrashtest.h"

using namespace snowcrash;
using namespace snowcrashtest;

TEST_CASE("Punctuation in identifiers", "[symbol_identifier]")
{
    mdp::ByteBuffer source = "# Parcel's sticker @#!$%^&*=-?><,.~`\"' [/]\n";

    ParseResult<Blueprint> blueprint;
    parse(source, 0, blueprint);

    REQUIRE(blueprint.report.error.code == Error::OK);
    REQUIRE(blueprint.report.warnings.empty());

    REQUIRE(blueprint.node.resourceGroups.size() == 1);
    REQUIRE(blueprint.node.resourceGroups[0].resources.size() == 1);
    REQUIRE(blueprint.node.resourceGroups[0].resources[0].name == "Parcel's sticker @#!$%^&*=-?><,.~`\"'");
    REQUIRE(blueprint.node.resourceGroups[0].resources[0].uriTemplate == "/");
    REQUIRE(blueprint.node.resourceGroups[0].resources[0].actions.empty());
}

TEST_CASE("Non ASCII characters in identifiers", "[symbol_identifier]")
{
    // Blueprint in question:
    //R"(
    //# Kategorii [/]
    //");

    // "Kategorii in Russian"
    mdp::ByteBuffer source = "# \xD0\x9A\xD0\xB0\xD1\x82\xD0\xB5\xD0\xB3\xD0\xBE\xD1\x80\xD0\xB8\xD0\xB8 [/]\n";

    ParseResult<Blueprint> blueprint;
    parse(source, 0, blueprint);

    REQUIRE(blueprint.report.error.code == Error::OK);
    REQUIRE(blueprint.report.warnings.empty());

    REQUIRE(blueprint.node.resourceGroups.size() == 1);
    REQUIRE(blueprint.node.resourceGroups[0].resources.size() == 1);
    REQUIRE(blueprint.node.resourceGroups[0].resources[0].name == "\xD0\x9A\xD0\xB0\xD1\x82\xD0\xB5\xD0\xB3\xD0\xBE\xD1\x80\xD0\xB8\xD0\xB8");
    REQUIRE(blueprint.node.resourceGroups[0].resources[0].uriTemplate == "/");
    REQUIRE(blueprint.node.resourceGroups[0].resources[0].actions.empty());
}
