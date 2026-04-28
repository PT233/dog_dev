#pragma once

#include <fstream>
#include <string>
#include <vector>

namespace project_shared {

inline bool load_trimmed_lines(const std::string& path, std::vector<std::string>* lines) {
  if (lines == nullptr) {
    return false;
  }

  std::ifstream file(path);
  if (!file.is_open()) {
    lines->clear();
    return false;
  }

  lines->clear();

  std::string line;
  while (std::getline(file, line)) {
    size_t start = line.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
      continue;
    }

    size_t end = line.find_last_not_of(" \t\r\n");
    lines->push_back(line.substr(start, end - start + 1));
  }

  return true;
}

}  // namespace project_shared
