#include "pre-processor.hpp"
#include "util.hpp"

#include <fstream>
#include <iostream>
#include "data.hpp"

namespace assembler {
    void read_source_file(pre_processor::Data &data, message::List &msgs) {
        auto &handle = *data.cli_args.input_file;
        data.file_path = handle.path;
        std::string str;

        for (int i = 0; std::getline(handle.stream, str); i++) {
            if (!str.empty())
                data.lines.emplace_back(Location(handle.path, i), str);
        }
    }

    void read_source_file(const std::filesystem::path &filepath, pre_processor::Data &data, message::List &msgs) {
        data.file_path = filepath;
        std::ifstream file(filepath);

        if (!file.is_open()) {
            Location loc(filepath);
            auto msg = std::make_unique<message::Message>(message::Error, loc);
            msg->get() << "cannot read file " << filepath;
            msgs.add(std::move(msg));

            msg = std::make_unique<message::Message>(message::Note, loc);
            msg->get() << "attempted to open file from " << std::filesystem::current_path();
            msgs.add(std::move(msg));

            return;
        }

        std::string str;

        for (int i = 0; std::getline(file, str); i++) {
            if (!str.empty())
                data.lines.emplace_back(Location(filepath, i), str);
        }

        file.close();
    }

    void pre_process(pre_processor::Data &data, message::List &msgs) {
        // Keep track of the current macro (if nullptr we are not in a macro definition)
        std::pair<std::string, pre_processor::Macro> *current_macro = nullptr;

        for (int lines_idx = 0; lines_idx < data.lines.size(); lines_idx++) {
            auto &line = data.lines[lines_idx];

            // Trim leading and trailing whitespace
            trim(line.second);

            // is the line empty?
            if (line.second.empty()) {
                data.lines.erase(data.lines.begin() + lines_idx);
                lines_idx--;
                continue;
            }

            // Remove comments
            bool in_string = false, was_comment = false;

            for (int i = 0; i < line.second.size(); i++) {
                if (line.second[i] == '"') {
                    in_string = !in_string;
                } else if (!in_string && line.second[i] == ';') {
                    line.second = line.second.substr(0, i);
                    was_comment = true;
                    break;
                }
            }

            // Remove any whitespace which may be left over after comment was removed
            if (was_comment) {
                rtrim(line.second);

                if (line.second.empty()) {
                    data.lines.erase(data.lines.begin() + lines_idx);
                    lines_idx--;
                    continue;
                }
            }

            // Section header?
            if (starts_with(line.second, ".section")) {
                continue;
            }

            // Have we found a directive?
            if (line.second[0] == '%') {
                process_directive(data, 1, lines_idx, line, current_macro, msgs);

                if (msgs.has_message_of(message::Error)) {
                    return;
                }

                // Directive has been handled - now remove current line
                data.lines.erase(data.lines.begin() + lines_idx);
                lines_idx--;

                continue;
            }

            // Replace constants in line with their value
            for (const auto &pair: data.constants) {
                size_t index = 0;

                while ((index = line.second.find(pair.first, index)) != std::string::npos) {
                    if (data.cli_args.debug)
                        std::cout << line.first << " CONSTANT: substitute symbol " << pair.first << std::endl;

                    line.second.replace(index, pair.first.size(), pair.second.value);
                    index += pair.second.value.size();
                }
            }

            // If in macro, add to body
            if (current_macro) {
                current_macro->second.lines.push_back(line.second);

                // Remove line from normal program
                data.lines.erase(data.lines.begin() + lines_idx);
                lines_idx--;

                continue;
            }

            // Extract mnemonic
            int i = 0;
            skip_non_whitespace(line.second, i);
            std::string mnemonic = line.second.substr(0, i);

            // Have we a macro?
            auto macro_exists = data.macros.find(mnemonic);

            if (macro_exists != data.macros.end()) {
                if (data.cli_args.debug)
                    std::cout << line.first << " CALL TO MACRO " << mnemonic << std::endl << "\tArgs:";

                // Collect arguments
                std::vector<std::string> arguments;
                int j;

                while (true) {
                    skip_whitespace(line.second, i);

                    // Extract argument data
                    j = i;
                    skip_to_break(line.second, i);

                    // Check if argument is the empty string
                    if (i == j)
                        break;

                    // Add to argument list
                    std::string argument = line.second.substr(j, i - j);
                    arguments.push_back(argument);

                    if (data.cli_args.debug)
                        std::cout << argument << " ";

                    if (line.second[i] == ',')
                        i++;

                    if (i == line.second.size())
                        break;
                }

                if (data.cli_args.debug) {
                    if (arguments.empty()) std::cout << "(none)";
                    std::cout << std::endl;
                }

                // Check that argument sizes match
                if (macro_exists->second.params.size() != arguments.size()) {
                    auto msg = std::make_unique<message::Message>(message::Error, Location(line.first).column(mnemonic.size()));
                    msg->get() << "macro " << mnemonic + " expects " << macro_exists->second.params.size()
                            << " argument(s), received " << arguments.size();
                    msgs.add(std::move(msg));

                    msg = std::make_unique<message::Message>(message::Note, macro_exists->second.loc);
                    msg->get() << "macro \"" << mnemonic << "\" defined here";
                    msgs.add(std::move(msg));
                    return;
                }

                // Remove this line from the program - it will be replaced soon
                auto old_line = line;
                data.lines.erase(data.lines.begin() + lines_idx);

                int insert_idx = lines_idx;

                // Insert macro's lines
                for (auto macro_line: macro_exists->second.lines) {
                    int arg_index = 0;

                    // Replace any parameters with its respective argument value
                    for (const auto &param: macro_exists->second.params) {
                        size_t index = 0;
                        std::string &arg = arguments[arg_index];

                        while ((index = macro_line.find(param, index)) != std::string::npos) {
                            if (data.cli_args.debug) {
                                std::cout << "\tCol " << index << ": EXPANSION: substitute parameter " << param
                                          << " with value \"" <<
                                          arg << "\"\n";
                            }

                            macro_line.replace(index, param.size(), arg);
                            index += arg.size();
                        }

                        arg_index++;
                    }

                    // Add modified macro line to program body
                    data.lines.insert(data.lines.begin() + insert_idx++, {old_line.first, macro_line});
                }

                // We have inserted the macro's body now, so we continue
                lines_idx--;
            }
        }
    }

    void process_directive(pre_processor::Data &data, int i, int line_idx, pre_processor::Line &line,
                           std::pair<std::string, pre_processor::Macro> *&current_macro, message::List &msgs) {
        // Extract directive name
        int j = i;
        skip_alphanum(line.second, i);
        std::string directive = line.second.substr(j, i - j);
        to_lowercase(directive);

        if (data.cli_args.debug) {
            std::cout << line.first << " DIRECTIVE: '" << directive << "'\n";
        }

        // If we are in a macro, our options are limited
        if (current_macro) {
            if (directive == "end") {
                if (data.cli_args.debug) {
                    std::cout << "\tEnd definition of " << current_macro->first << " - "
                              << current_macro->second.lines.size() << " lines" << std::endl;
                }

                current_macro = nullptr;
            } else {
                auto error = std::make_unique<message::Message>(message::Error, line.first);
                error->get() << "unknown/invalid directive in %macro body: %" << directive;
                msgs.add(std::move(error));
                return;
            }
        } else {
            if (directive == "define") {
                // %define [SYMBOL] [VALUE] - creates a new constant
                skip_alpha(line.second, i);
                skip_whitespace(line.second, i);

                // Extract constant name
                j = i;
                skip_non_whitespace(line.second, i);
                std::string constant = line.second.substr(j, i - j);

                if (data.cli_args.debug) {
                    std::cout << "\tConstant: " << constant;
                }

                // Get value
                skip_whitespace(line.second, i);
                std::string value = line.second.substr(i);

                if (data.cli_args.debug) {
                    std::cout << "; Value = \"" << value << "\"\n";
                }

                // Check if constant already exists
                auto exists = data.constants.find(constant);

                if (exists != data.constants.end()) {
                    // Warn user of potential mishap
                    auto msg = std::make_unique<message::Message>(message::Warning, Location(line.first).column(j));
                    msg->get() << "Re-definition of constant " << constant << " (previously defined at "
                                     << exists->second.loc << ')';
                    msgs.add(std::move(msg));

                    // Update value.
                    exists->second.value = value;
                    exists->second.loc = line.first.copy().column(j);
                } else {
                    // Add to constant dictionary
                    data.constants.insert({constant, {line.first.copy().column(j), value}});
                }
            } else if (directive == "include") {
                // %include [FILEPATH]
                skip_non_whitespace(line.second, i);
                skip_whitespace(line.second, i);

                // Extract file path
                std::string file_path = line.second.substr(i);

                if (data.cli_args.debug) {
                    std::cout << "\tFile path '" + file_path + "'\n";
                }

                // Get file's full path
                std::filesystem::path full_path;

                if (starts_with(file_path, "lib:")) {
                    full_path = data.cli_args.lib_path / std::filesystem::path(file_path.substr(4) + ".asm");
                } else {
                    full_path = data.file_path.parent_path() / std::filesystem::path(file_path + ".asm");
                }

                if (data.cli_args.debug) {
                    std::cout << "\tFull path " << full_path << std::endl;
                }

                // Set-up pre-processing data
                pre_processor::Data include_data(data.cli_args);
                message::List include_messages;

                // Read included file
                read_source_file(full_path.string(), include_data, include_messages);

                if (include_messages.has_message_of(message::Error)) {
                    msgs.add(include_messages);

                    auto msg = std::make_unique<message::Message>(message::Note, line.first.copy().column(i));
                    msg->get() << "attempted to %include file here";
                    msgs.add(std::move(msg));
                    return;
                }

                // Now we know the file exists update data path
                include_data.file_path = full_path.string();

                // Check if the file has already been included
                auto canonical_path = std::filesystem::canonical(full_path);
                auto circular_include = data.included_files.find(canonical_path);

                if (circular_include != data.included_files.end()) {
                    auto msg = std::make_unique<message::Message>(message::Error, line.first.copy().column(i));
                    msg->get() << "circular %include: " << full_path;
                    msgs.add(std::move(msg));

                    msg = std::make_unique<message::Message>(message::Note, circular_include->second);
                    msg->get() << "file " << canonical_path << " previously included here";
                    msgs.add(std::move(msg));

                    return;
                }

                // Add to circular references map
                include_data.included_files.insert(data.included_files.begin(), data.included_files.end());
                include_data.included_files.insert({canonical_path, line.first.copy().column(i)});

                // Pre-process included file
                pre_process(include_data, include_messages);

                if (include_messages.has_message_of(message::Error)) {
                    msgs.add(include_messages);
                    return;
                }

                // Merge data
                data.merge(include_data, line_idx + 1);
            } else if (directive == "macro") {
                // %macro [NAME] <args...>
                skip_alpha(line.second, i);
                skip_whitespace(line.second, i);

                // Extract macro's name
                j = i;
                int macro_name_index = i;
                skip_non_whitespace(line.second, i);
                std::string macro_name = line.second.substr(j, i - j);

                if (data.cli_args.debug) {
                    std::cout << "\tName: '" << macro_name << "'; Args: ";
                }

                // Check if name is valid
                if (!is_valid_label_name(macro_name)) {
                    auto error = std::make_unique<message::Message>(message::Error, line.first);
                    error->get() << "invalid macro name \"" << macro_name << "\"";
                    msgs.add(std::move(error));
                    return;
                }

                // Check if name already exists
                auto macro_exists = data.macros.find(macro_name);

                if (macro_exists != data.macros.end()) {
                    // Warn user of potential mishap
                    auto msg = std::make_unique<message::Message>(message::Warning, line.first.copy().column(j));
                    msg->get() << "re-definition of macro " << macro_name;
                    msgs.add(std::move(msg));

                    msg = std::make_unique<message::Message>(message::Note, macro_exists->second.loc);
                    msg->get() << "previously defined here";
                    msgs.add(std::move(msg));

                    // Update value.
                    macro_exists->second.loc = line.first;
                }

                // Collect parameters
                std::vector<std::string> macro_params;

                while (true) {
                    skip_whitespace(line.second, i);

                    if (i == line.second.size())
                        break;

                    // Extract parameter name
                    j = i;
                    skip_non_whitespace(line.second, i);
                    std::string parameter = line.second.substr(j, i - j);

                    // Check if name is valid
                    if (!is_valid_label_name(parameter)) {
                        auto msg = std::make_unique<message::Message>(message::Error, line.first);
                        msg->get() << "invalid parameter name \"" << parameter << '"';
                        msgs.add(std::move(msg));

                        msg = std::make_unique<message::Message>(message::Note, line.first.copy().column(macro_name_index));
                        msg->get() << "in definition of macro \"" << macro_name << '"';
                        msgs.add(std::move(msg));
                        return;
                    }

                    // Duplicate parameter?
                    auto param_exists = std::ranges::find(macro_params.begin(), macro_params.end(), parameter);

                    if (param_exists != macro_params.end()) {
                        auto msg = std::make_unique<message::Message>(message::Error, line.first.copy().column(j));
                        msg->get() << "duplicate parameter \"" << parameter << '"';
                        msgs.add(std::move(msg));

                        msg = std::make_unique<message::Message>(message::Note, line.first.copy().column(macro_name_index));
                        msg->get() << "in definition of macro \"" << macro_name + '"';
                        msgs.add(std::move(msg));
                        return;
                    }

                    // Add top parameter list
                    macro_params.push_back(parameter);

                    if (data.cli_args.debug)
                        std::cout << parameter << " ";

                    // Break form the loop?
                    if (i > line.second.size())
                        break;
                }

                if (data.cli_args.debug)
                    std::cout << std::endl;

                // Insert/update macro
                if (macro_exists == data.macros.end()) {
                    data.macros.insert({macro_name, pre_processor::Macro(line.first.copy().column(macro_name_index), macro_params)});
                    macro_exists = data.macros.find(macro_name);
                } else {
                    macro_exists->second.params = macro_params;
                }

                // Set current_macro
                current_macro = reinterpret_cast<std::pair<std::string, pre_processor::Macro> *>(&*macro_exists);
            } else if (directive == "rm") {
                // %rm: act as a comment
                if (data.cli_args.debug) {
                    std::cout << "\tIgnoring this line.\n";
                }
            } else if (directive == "stop") {
                // %stop: halt the pre-processor, remove all lines after this one
                if (data.cli_args.debug) {
                    std::cout << "\tRemoving all lines past line " << line_idx << "\n";
                }

                while (data.lines.size() != line_idx) {
                    data.lines.pop_back();
                }
            } else {
                auto msg = std::make_unique<message::Message>(message::Error, line.first);
                msg->get() << "unknown directive %" << directive;
                msgs.add(std::move(msg));
                return;
            }
        }
    }
}
