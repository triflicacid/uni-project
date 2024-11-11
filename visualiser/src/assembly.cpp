#include <iostream>
#include "assembly.hpp"
#include "data.hpp"
#include "util.h"
#include "util.hpp"

std::set<std::string> visualiser::assembly::file_extensions = {".s", ".S", ".asm"};

std::unique_ptr<visualiser::assembly::Data> visualiser::compile_assembly(const visualiser::Data &data) {
    // create command to run the assembler
    std::stringstream cmd;
    cmd << data.cli_args.assembler_path
        << " " << data.cli_args.source->path
        << " -o " << (data.cli_args.out_dir / "program")
        << " -r " << (data.cli_args.out_dir / "program.s");

    if (data.cli_args.assembler_lib) cmd << " -l \"" << data.cli_args.assembler_lib.value() << '"';
    if (data.cli_args.debug) {
        cmd << " -d";
        cmd << " 1> " << data.cli_args.out_dir / "assembler.stdout.log";
        cmd << " 2> " << data.cli_args.out_dir / "assembler.stderr.log";

        // write command to file
        std::ofstream file(data.cli_args.out_dir / "assembler.sh");
        file << "#!/bin/bash" << std::endl << cmd.str() << std::endl;
        file.close();
    }

    // run the command as a new process
    FILE *pipe = popen(cmd.str().c_str(), "r");

    if (!pipe) {
        std::cout << ANSI_RED "error creating pipe for executing the assembler" ANSI_RESET << std::endl;
        perror(nullptr);
        return nullptr;
    }

    // get exit code
    int exit_code = pclose(pipe);
    exit_code = WIFEXITED(exit_code) ? WEXITSTATUS(exit_code) : -1;

    // exit OK?
    if (exit_code != EXIT_SUCCESS) {
        std::cout << ANSI_RED "assembler exited with code " << exit_code << ANSI_RED << std::endl;
        std::cout << ANSI_BLUE "note" << ANSI_RESET << ": command was '" << cmd.str() << '\'';
        return nullptr;
    }

    if (data.cli_args.debug) std::cout << "file successfully assembled" << std::endl;

    // create assembly data
    auto assembly_data = std::make_unique<assembly::Data>(*data.cli_args.source);
    assembly_data->reconstruction = named_fstream::open(data.cli_args.out_dir / "program.s", std::ios::in);

    return assembly_data;
}

void visualiser::assembly::Data::populate() {
    // reset file & other state
    auto &stream = reconstruction->stream;
    stream.clear();
    stream.seekg(std::ios::beg);
    lines.clear();

    // read each line
    std::string line;
    while (std::getline(stream, line)) {
        // split into line contents & debug info
        size_t i = line.find(';');

        // extract debug info and offset
        std::string debug_info = line.substr(i + 1);
        trim(debug_info);
        size_t j = debug_info.find('+');
        uint32_t offset = std::stoul(debug_info.substr(j + 1));

        // extract filename and line number
        debug_info = debug_info.substr(0, j);
        trim(debug_info);
        j = debug_info.find(':');
        int line_no = std::stoi(debug_info.substr(j + 1));
        std::string filepath = debug_info.substr(0, j);

        // insert into map
        lines.insert({offset, {Location(filepath, line_no), line.substr(0, i)}});
    }
}
