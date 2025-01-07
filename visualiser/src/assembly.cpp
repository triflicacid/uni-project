#include <iostream>
#include "assembly.hpp"
#include "util.hpp"

std::unique_ptr<named_fstream> visualiser::assembly::source = nullptr;
std::map<uint32_t, visualiser::assembly::PCEntry> visualiser::assembly::pc_to_line = {};
std::map<std::filesystem::path, std::vector<std::string>> visualiser::assembly::files = {};

void visualiser::assembly::init() {
  // reset file & other state
  auto &stream = source->stream;
  stream.clear();
  stream.seekg(std::ios::beg);
  pc_to_line.clear();

  // read each line (save in map entry)
  std::pair<std::filesystem::path, std::vector<std::string>> map_entry = {source->path, {}};

  std::string line;
  int idx = 0;
  while (std::getline(stream, line)) {
    idx++;

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

    // insert into (both) maps
    line = line.substr(0, i);
    pc_to_line.insert({offset, PCEntry{offset, line, idx, Location(filepath, line_no), Location("fake")}});
    map_entry.second.push_back(line);
  }

  // add map entry to file dictionary
  files.insert(map_entry);
}

visualiser::assembly::PCEntry *visualiser::assembly::locate_pc(uint64_t pc) {
  if (auto it = pc_to_line.find(pc); it != pc_to_line.end()) {
    return &it->second;
  }

  return nullptr;
}

const std::vector<std::string> &visualiser::assembly::get_file_lines(const std::filesystem::path &path) {
  // check if file is already in the dictionary
  if (auto it = files.find(path); it != files.end()) {
    return it->second;
  }

  // open file and read line-by-line
  std::ifstream file(path);
  std::pair<std::filesystem::path, std::vector<std::string>> map_entry = {path, {}};
  std::string line;

  while (std::getline(file, line)) {
    map_entry.second.push_back(line);
  }

  // insert into dictionary and return
  files.insert(map_entry);
  file.close();
  return files.find(path)->second;
}

const visualiser::assembly::PCEntry *visualiser::assembly::locate_line(int line) {
  for (const auto &[pc, entry]: pc_to_line) {
    if (entry.line_no == line) {
      return &entry;
    }
  }

  return nullptr;
}

std::vector<const visualiser::assembly::PCEntry *>
visualiser::assembly::locate_asm_line(const std::filesystem::path &path, int line) {
  std::vector<const visualiser::assembly::PCEntry *> entries;

  for (const auto &[pc, entry]: pc_to_line) {
    if (entry.asm_origin.path() == path && entry.asm_origin.line() == line) {
      entries.push_back(&entry);
    }
  }

  return entries;
}
