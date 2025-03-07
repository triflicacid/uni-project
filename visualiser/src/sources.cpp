#include <iostream>
#include "sources.hpp"
#include "util.hpp"
#include "processor.hpp"

std::unique_ptr<named_fstream> visualiser::sources::edel_source = nullptr;
std::unique_ptr<named_fstream> visualiser::sources::asm_source = nullptr;
std::unique_ptr<named_fstream> visualiser::sources::s_source = nullptr;
std::map<uint32_t, visualiser::sources::PCLine> visualiser::sources::pc_to_line = {};
std::map<std::filesystem::path, visualiser::sources::File> visualiser::sources::files = {};
Graph<std::pair<std::filesystem::path, int>, visualiser::sources::FileLine*, pair_hash> visualiser::sources::trace;

// read .s file, create binary-to-s links
static void init_s_source() {
  using namespace visualiser::sources;

  // reset file & other state
  auto &stream = s_source->stream;
  stream.clear();
  stream.seekg(std::ios::beg);
  pc_to_line.clear();

  // read each line (save in map entry)
  File* file;
  {
    File file_object{s_source->path, Type::Source, {}, true};
    files.insert({file_object.path, file_object});
    file = &files.at(file_object.path);
  }
  std::string line;
  int idx = 0;

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
    int line_no = std::stoi(debug_info.substr(j + 1)) - 1;
    std::string filepath = debug_info.substr(0, j);

    // register file as assembly, if need to
    if (files.find(filepath) == files.end())
      files.insert({
        filepath,
        File{filepath, Type::Assembly, {}, false}
      });

    // insert into (both) maps
    line = line.substr(0, i);
    pc_to_line.insert({offset, PCLine{offset, line, idx, Location(filepath, line_no)}});
    file->lines.push_back(FileLine{
      file,
      idx,
      line,
      {&pc_to_line.find(offset)->second}
    });

    FileLine* file_line = &file->lines.back();
    auto s_key = std::make_pair(file->path, idx);
    auto asm_key = std::make_pair(filepath, line_no);
    trace.insert(s_key, file_line);
    trace.insert_symmetric(s_key, asm_key);

    idx++;
  }
}

// read .asm file, create links to source file
static void init_asm_source() {
  using namespace visualiser::sources;

  // reset file & other state
  auto &stream = asm_source->stream;
  stream.clear();
  stream.seekg(std::ios::beg);

  // read each line (save in map entry)
  File* file;
  if (auto it = files.find(asm_source->path); it != files.end()) {
    file = &it->second;
  } else {
    File file_object{asm_source->path, Type::Assembly, {}, true};
    files.insert({file_object.path, file_object});
    file = &files.at(file_object.path);
  }

  file->loaded = true;
  std::string line;
  int idx = 0;

  while (std::getline(stream, line)) {
    // insert line into file
    file->lines.push_back(FileLine{
        file,
        idx,
        line,
        {}
    });

    // split into line contents & debug info
    size_t i = line.find_last_of(';');

    // does this line contain a debug comment
    if (i == std::string::npos || i + 1 >= line.length() || line[i + 1] != '@') {
      idx++;
      continue;
    }

    // extract debug info and offset
    std::string debug_info = line.substr(i + 2);
    trim(debug_info);

    // extract filename and line number
    size_t j = debug_info.find_first_of(':'), k = debug_info.find_last_of(':');
    int line_no = -1, col_no = -1;
    if (j == k) {
      try {
        line_no = std::stoi(debug_info.substr(j + 1)) - 1;
      } catch (std::exception& e) { }
    } else {
      try {
        line_no = std::stoi(debug_info.substr(j + 1, k - j)) - 1;
        col_no = std::stoi(debug_info.substr(k + 1)) - 1;
      } catch (std::exception& e) { }
    }
    std::string filepath = debug_info.substr(0, j);

    // register file as high-level, if need to
    if (files.find(filepath) == files.end())
      files.insert({
                       filepath,
                       File{filepath, Type::Language, {}, false}
                   });

    // find PC entry and update lang_origin
    if (line_no > -1) {
      line = line.substr(0, i);
      file->lines.back().line = line;

      // update trace graph
      FileLine* file_line = &file->lines.back();
      auto asm_key = std::make_pair(file->path, idx);
      auto lang_key = std::make_pair(filepath, line_no);
      trace.insert(asm_key, file_line);
      trace.insert_symmetric(lang_key, asm_key);

      // update pc traces
      auto pc_lines = locate_asm_line(file->path, idx);
      for (PCLine* pc_line: pc_lines) {
        pc_line->lang_origin = Location(filepath, line_no, col_no);
      }
      file->lines.back().pc_trace = std::move(pc_lines);
    }

    idx++;
  }
}

void visualiser::sources::init() {
  init_s_source();
  init_asm_source();

  // ensure language file exists
  if (auto it = files.find(edel_source->path); it == files.end()) {
    File file{edel_source->path, Type::Language, {}, false};
    files.insert({file.path, file});
  }
  get_file(edel_source->path);
  return;
}

const visualiser::sources::PCLine* visualiser::sources::locate_pc(uint64_t pc) {
  if (auto it = pc_to_line.find(pc); it != pc_to_line.end()) {
    return &it->second;
  }

  return nullptr;
}

std::vector<visualiser::sources::PCLine*> visualiser::sources::locate_asm_line(const std::filesystem::path &path, int line) {
  std::vector<visualiser::sources::PCLine *> entries;

  for (auto &[pc, entry]: visualiser::sources::pc_to_line) {
    if (entry.asm_origin.path() == path && entry.asm_origin.line() == line) {
      entries.push_back(&entry);
    }
  }

  return entries;
}

std::vector<visualiser::sources::PCLine*> visualiser::sources::locate_lang_line(const std::filesystem::path &path, int line) {
  std::vector<visualiser::sources::PCLine *> entries;

  for (auto &[pc, entry]: visualiser::sources::pc_to_line) {
    if (auto origin = entry.lang_origin; origin && origin->path() == path && origin->line() == line) {
      entries.push_back(&entry);
    }
  }

  return entries;
}

const visualiser::sources::File* visualiser::sources::get_file(const std::filesystem::path &path) {
  File* file = &files.find(path)->second;

  // if already loaded, return
  if (file->loaded) return file;

  // open file and read line-by-line
  std::ifstream stream(path);
  std::pair<std::filesystem::path, std::vector<visualiser::sources::FileLine>> map_entry = {path, {}};
  std::string line;
  int n = 0;

  const std::string extension = path.extension().string();
  const bool is_asm_file = extension == ".s" || extension == ".S" || extension == ".asm";

  while (std::getline(stream, line)) {
    FileLine file_line{file, n, line, is_asm_file ? locate_asm_line(path, n) : locate_lang_line(path, n)};
    file->lines.push_back(file_line);
    trace.insert(std::make_pair(path, n), &file->lines.back());
    n++;
  }

  // insert into dictionary and return
  file->loaded = true;
  stream.close();
  return file;
}

const visualiser::sources::PCLine *visualiser::sources::locate_line(int line) {
  for (const auto &[pc, entry]: pc_to_line) {
    if (entry.line_no == line) {
      return &entry;
    }
  }

  return nullptr;
}

std::string visualiser::sources::File::to_string() const {
  std::stringstream stream;
  for (const FileLine& line : lines)
    stream << line.line << std::endl;

  return stream.str();
}

int visualiser::sources::File::count_breakpoints() const {
  int count = 0;
  for (const FileLine& line : lines)
    count += line.has_breakpoint();

  return count;
}

bool visualiser::sources::FileLine::has_breakpoint() const {
  return !pc_trace.empty() && std::any_of(pc_trace.begin(), pc_trace.end(), [](auto* line) { return line->has_breakpoint(); });
}

bool visualiser::sources::FileLine::contains_pc(uint64_t pc) const {
  return !pc_trace.empty() && std::any_of(pc_trace.begin(), pc_trace.end(), [pc](auto* line) { return line->pc == pc; });
}

std::optional<uint64_t> visualiser::sources::FileLine::pc() const {
  if (pc_trace.empty()) return {};
  return pc_trace.front()->pc;
}

bool visualiser::sources::PCLine::has_breakpoint() const {
  return processor::breakpoints.find(this) != visualiser::processor::breakpoints.end();
}

void visualiser::sources::PCLine::toggle_breakpoint() const {
  set_breakpoint(!has_breakpoint());
}

void visualiser::sources::PCLine::set_breakpoint(bool b) const {
  if (b) {
    processor::breakpoints.insert(this);
  } else {
    processor::breakpoints.erase(this);
  }
}
