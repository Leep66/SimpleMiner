#include "Game/ChunkFileUtils.hpp"
#include "Game/Chunk.hpp"
#include "Game/ChunkCoords.hpp"
#include <cstdio>
#include <filesystem>
#include <system_error>
#include "Engine/Core/EngineCommon.hpp"

namespace fs = std::filesystem;

static std::string& SavesDir()
{
	static std::string s;
	if (s.empty()) {
		std::filesystem::path p = std::filesystem::current_path() / "Saves";
		const std::string folder = p.string();

		if (!FolderExists(folder)) 
		{
			if (!CreateFolder(folder))
			{
				ERROR_RECOVERABLE(Stringf("Failed to ensure Saves folder: \"%s\"", folder.c_str()));
			}
		}
		s = folder;
	}
	return s;
}



std::string ChunkFileUtils::BuildChunkFilename(const Chunk& c)
{
	char name[64];
	std::snprintf(name, sizeof(name), "Chunk(%d,%d).chunk", c.m_chunkCoords.x, c.m_chunkCoords.y);
	std::string out; out.reserve(SavesDir().size() + 1 + std::strlen(name));
	out = SavesDir(); out.push_back('/'); out += name;
	return out;
}


bool ChunkFileUtils::Save(const Chunk& chunk, const std::string& filename)
{
	std::vector<uint8_t> buffer;
	buffer.reserve(8 + (BLOCKS_PER_CHUNK / 2));

	buffer.push_back('G'); buffer.push_back('C');
	buffer.push_back('H'); buffer.push_back('K');
	buffer.push_back(CHUNK_FILE_VERSION);
	buffer.push_back(CHUNK_BITS_X);
	buffer.push_back(CHUNK_BITS_Y);
	buffer.push_back(CHUNK_BITS_Z);

	static_assert(BLOCKS_PER_CHUNK > 0, "BLOCKS_PER_CHUNK must be > 0 for RLE save.");

	uint8_t curType = chunk.GetTypeAtLocal(0);
	uint8_t runLen = 0;
	for (int i = 0; i < BLOCKS_PER_CHUNK; ++i) {
		const uint8_t t = chunk.GetTypeAtLocal(i);
		if (t == curType && runLen < 255) ++runLen;
		else { buffer.push_back(curType); buffer.push_back(runLen); curType = t; runLen = 1; }
	}
	if (runLen > 0) { buffer.push_back(curType); buffer.push_back(runLen); }

	int written = FileWriteFromBuffer(buffer, filename);
	return written == static_cast<int>(buffer.size());
}




bool ChunkFileUtils::Load(Chunk& chunk, const std::string& filename)
{
	std::vector<uint8_t> buffer;
	int readBytes = FileReadToBuffer(buffer, filename);
	if (readBytes < 0) return false;
	if (static_cast<size_t>(readBytes) < 8u) return false; 

	uint8_t* cur = buffer.data();
	uint8_t* end = buffer.data() + buffer.size();


#pragma pack(push, 1)
	struct Header {
		char    magic[4]; // 'G','C','H','K'
		uint8_t version;
		uint8_t bitsX;
		uint8_t bitsY;
		uint8_t bitsZ;
	};
	struct Run {
		uint8_t type;
		uint8_t length;   // 1..255
	};
#pragma pack(pop)


	if (static_cast<size_t>(end - cur) < sizeof(Header)) return false;
	const Header* hdr = reinterpret_cast<const Header*>(cur);

	if (std::memcmp(hdr->magic, "GCHK", 4) != 0) return false;
	if (hdr->version != static_cast<uint8_t>(CHUNK_FILE_VERSION)) return false;
	if (hdr->bitsX != static_cast<uint8_t>(CHUNK_BITS_X))       return false;
	if (hdr->bitsY != static_cast<uint8_t>(CHUNK_BITS_Y))       return false;
	if (hdr->bitsZ != static_cast<uint8_t>(CHUNK_BITS_Z))       return false;

	cur += sizeof(Header);

	if ((end - cur) < 0) return false; 
	const size_t bodyBytes = static_cast<size_t>(end - cur);
	if ((bodyBytes % sizeof(Run)) != 0) return false;
	const size_t runCount = bodyBytes / sizeof(Run);

	int writeIdx = 0;
	for (size_t i = 0; i < runCount; ++i)
	{
		const Run* r = reinterpret_cast<const Run*>(cur);
		cur += sizeof(Run);

		if (r->length == 0) return false;
		if (writeIdx + r->length > BLOCKS_PER_CHUNK) return false;

		for (uint8_t k = 0; k < r->length; ++k) 
		{
			chunk.SetTypeAtLocal(writeIdx++, r->type);
		}
	}

	if (writeIdx != BLOCKS_PER_CHUNK) return false;

	return true;
}

