//
//  snowcrash.cc
//  snowcrash
//
//  Created by Zdenek Nemec on 4/27/13.
//  Copyright (c) 2013 Apiary Inc. All rights reserved.
//

#include <iostream>
#include <sstream>
#include <fstream>
#include "snowcrash.h"
#include "SerializeJSON.h"
#include "SerializeYAML.h"
#include "cmdline.h"
#include "Version.h"

using snowcrash::SourceAnnotation;
using snowcrash::Error;

static const std::string OutputArgument = "output";
static const std::string FormatArgument = "format";
static const std::string RenderArgument = "render";
static const std::string SourcemapArgument = "sourcemap";
static const std::string ValidateArgument = "validate";
static const std::string VersionArgument = "version";

/// \enum Snow Crash AST output format.
enum SerializationFormat {
    YAMLSerializationFormat,
    JSONSerializationFormat
};

/// \brief Print Markdown source annotation.
/// \param prefix A string prefix for the annotation
/// \param annotation An annotation to print
void PrintAnnotation(const std::string& prefix, const snowcrash::SourceAnnotation& annotation)
{
    std::cerr << prefix;

    if (annotation.code != SourceAnnotation::OK) {
        std::cerr << " (" << annotation.code << ") ";
    }

    if (!annotation.message.empty()) {
        std::cerr << " " << annotation.message;
    }

    if (!annotation.location.empty()) {
        for (mdp::CharactersRangeSet::const_iterator it = annotation.location.begin();
             it != annotation.location.end();
             ++it) {
            std::cerr << ((it == annotation.location.begin()) ? " :" : ";");
            std::cerr << it->location << ":" << it->length;
        }
    }

    std::cerr << std::endl;
}

/// \brief Print parser report to stderr.
/// \param report A parser report to print
void PrintReport(const snowcrash::Report& report)
{
    std::cerr << std::endl;

    if (report.error.code == Error::OK) {
        std::cerr << "OK.\n";
    }
    else {
        PrintAnnotation("error:", report.error);
    }

    for (snowcrash::Warnings::const_iterator it = report.warnings.begin(); it != report.warnings.end(); ++it) {
        PrintAnnotation("warning:", *it);
    }
}

int main(int argc, const char *argv[])
{
    cmdline::parser argumentParser;

    argumentParser.set_program_name("snowcrash");
    std::stringstream ss;

    ss << "<input file>\n\n";
    ss << "API Blueprint Parser\n";
    ss << "If called without <input file>, 'snowcrash' will listen on stdin.\n";

    argumentParser.footer(ss.str());

    argumentParser.add<std::string>(OutputArgument, 'o', "save output AST into file", false);
    argumentParser.add<std::string>(FormatArgument, 'f', "output AST format", false, "yaml", cmdline::oneof<std::string>("yaml", "json"));
    argumentParser.add<std::string>(SourcemapArgument, 's', "export sourcemap AST into file", false);
    // TODO: argumentParser.add("render", 'r', "render markdown descriptions");
    argumentParser.add("help", 'h', "display this help message");
    argumentParser.add(VersionArgument, 'v', "print Snow Crash version");
    argumentParser.add(ValidateArgument, 'l', "validate input only, do not print AST");

    argumentParser.parse_check(argc, argv);

    // Check arguments
    if (argumentParser.rest().size() > 1) {
        std::cerr << "one input file expected, got " << argumentParser.rest().size() << std::endl;
        exit(EXIT_FAILURE);
    }

    // Version query
    if (argumentParser.exist(VersionArgument)) {
        std::cout << SNOWCRASH_VERSION_STRING << std::endl;
        exit(EXIT_SUCCESS);
    }

    // Input
    std::stringstream inputStream;
    if (argumentParser.rest().empty()) {
        // Read stdin
        inputStream << std::cin.rdbuf();
    }
    else {
        // Read from file
        std::ifstream inputFileStream;
        std::string inputFileName = argumentParser.rest().front();
        inputFileStream.open(inputFileName.c_str());

        if (!inputFileStream.is_open()) {
            std::cerr << "fatal: unable to open input file '" << inputFileName << "'\n";
            exit(EXIT_FAILURE);
        }

        inputStream << inputFileStream.rdbuf();
        inputFileStream.close();
    }

    // Initialize
    snowcrash::BlueprintParserOptions options = 0;  // Or snowcrash::RequireBlueprintNameOption
    snowcrash::ParseResult<snowcrash::Blueprint> blueprint;

    if (argumentParser.exist(SourcemapArgument)) {
        options |= snowcrash::ExportSourcemapOption;
    }

    // Parse
    snowcrash::parse(inputStream.str(), options, blueprint);

    // Output
    if (!argumentParser.exist(ValidateArgument)) {

        std::stringstream outputStream;
        std::stringstream sourcemapOutputStream;

        if (argumentParser.get<std::string>(FormatArgument) == "json") {
            SerializeJSON(blueprint.node, outputStream);
            SerializeSourceMapJSON(blueprint.sourceMap, sourcemapOutputStream);
        }
        else if (argumentParser.get<std::string>(FormatArgument) == "yaml") {
            SerializeYAML(blueprint.node, outputStream);
            SerializeSourceMapYAML(blueprint.sourceMap, sourcemapOutputStream);
        }

        std::string outputFileName = argumentParser.get<std::string>(OutputArgument);
        std::string sourcemapOutputFileName = argumentParser.get<std::string>(SourcemapArgument);

        if (!outputFileName.empty()) {
            // Serialize to file
            std::ofstream outputFileStream;
            outputFileStream.open(outputFileName.c_str());

            if (!outputFileStream.is_open()) {
                std::cerr << "fatal: unable to write to file '" <<  outputFileName << "'\n";
                exit(EXIT_FAILURE);
            }

            outputFileStream << outputStream.rdbuf();
            outputFileStream.close();
        }
        else {
            // Serialize to stdout
            std::cout << outputStream.rdbuf();
        }

        if (!sourcemapOutputFileName.empty()) {
            // Serialize to file
            std::ofstream sourcemapOutputFileStream;
            sourcemapOutputFileStream.open(sourcemapOutputFileName.c_str());

            if (!sourcemapOutputFileStream.is_open()) {
                std::cerr << "fatal: unable to write to file '" << sourcemapOutputFileName << "'\n";
                exit(EXIT_FAILURE);
            }

            sourcemapOutputFileStream << sourcemapOutputStream.rdbuf();
            sourcemapOutputFileStream.close();
        }
    }

    // report
    PrintReport(blueprint.report);
    return blueprint.report.error.code;
}
