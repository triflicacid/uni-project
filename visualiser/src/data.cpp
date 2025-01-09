#include <iostream>
#include "data.hpp"
#include "util.hpp"

std::unique_ptr<named_fstream> visualiser::source = nullptr;
std::map<uint32_t, visualiser::PCLine> visualiser::pc_to_line = {};
std::map<std::filesystem::path, visualiser::File> visualiser::files = {};

void visualiser::init() {
  // reset file & other state
  auto &stream = source->stream;
  stream.clear();
  stream.seekg(std::ios::beg);
  pc_to_line.clear();

  // read each line (save in map entry)
  File file = {source->path, Type::Source, {}, true};
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

    // register file as assembly, if need to
    if (files.find(filepath) == files.end())
      files.insert({
        filepath,
        File{filepath, Type::Assembly, {}, false}
      });

    // insert into (both) maps
    line = line.substr(0, i);
    pc_to_line.insert({offset, PCLine{offset, line, idx, Location(filepath, line_no), Location("fake")}});
    file.lines.push_back(FileLine{
      idx,
      line,
      {&pc_to_line.find(offset)->second}
    });
  }

  // add map entry to file dictionary
  files.insert({file.path, file});
}

const visualiser::PCLine* visualiser::locate_pc(uint64_t pc) {
  if (auto it = pc_to_line.find(pc); it != pc_to_line.end()) {
    return &it->second;
  }

  return nullptr;
}

std::vector<const visualiser::PCLine*> visualiser::locate_asm_line(const std::filesystem::path &path, int line) {
  std::vector<const visualiser::PCLine *> entries;

  for (const auto &[pc, entry]: visualiser::pc_to_line) {
    if (entry.asm_origin.path() == path && entry.asm_origin.line() == line) {
      entries.push_back(&entry);
    }
  }

  return entries;
}

std::vector<const visualiser::PCLine*> visualiser::locate_lang_line(const std::filesystem::path &path, int line) {
  std::vector<const visualiser::PCLine *> entries;

  for (const auto &[pc, entry]: visualiser::pc_to_line) {
    if (entry.lang_origin.path() == path && entry.lang_origin.line() == line) {
      entries.push_back(&entry);
    }
  }

  return entries;
}

const visualiser::File* visualiser::get_file(const std::filesystem::path &path) {
  File* file = &files.find(path)->second;

  // if already loaded, return
  if (file->loaded) return file;

  // open file and read line-by-line
  std::ifstream stream(path);
  std::pair<std::filesystem::path, std::vector<visualiser::FileLine>> map_entry = {path, {}};
  std::string line;
  int n = 1;

  while (std::getline(stream, line)) {
    file->lines.push_back({n, line, locate_asm_line(path, n++)});
  }

  // insert into dictionary and return
  file->loaded = true;
  stream.close();
  return file;
}

const visualiser::PCLine *visualiser::locate_line(int line) {
  for (const auto &[pc, entry]: pc_to_line) {
    if (entry.line_no == line) {
      return &entry;
    }
  }

  return nullptr;
}

std::string visualiser::File::to_string() const {
  std::stringstream stream;
  for (auto& line : lines)
    stream << line.line << std::endl;

  return stream.str();
}
