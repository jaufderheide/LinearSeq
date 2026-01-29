#pragma once

#include <string>

#include "core/Types.h"

namespace linearseq::SongJson {

std::string toJson(const Song& song);
bool saveToFile(const Song& song, const std::string& path);
bool loadFromFile(const std::string& path, Song& song);

} // namespace linearseq::SongJson
