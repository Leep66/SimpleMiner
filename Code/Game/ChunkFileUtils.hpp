#pragma once
#include <cstdint>
#include <vector>
#include "Engine/Core/FileUtils.hpp"

class Chunk;

namespace ChunkFileUtils
{
	std::string BuildChunkFilename(const Chunk& chunk);
	bool Save(const Chunk& chunk, const std::string& filename);
	bool Load(Chunk& chunk, const std::string& filename);

}