#pragma once
#include "Game/Chunk.hpp"
#include "Game/ChunkCoords.hpp"
#include "Engine/Math/IntVec3.hpp"

class BlockIterator {
public:
	Chunk* m_chunk = nullptr;
	int    m_blockIdx = -1;

public:
	BlockIterator() = default;

	BlockIterator(Chunk* chunk, int blockIdx)
		: m_chunk(chunk)
		, m_blockIdx(blockIdx)
	{
	}

	static BlockIterator Invalid() { return BlockIterator(nullptr, -1); }
	inline bool IsValid() const { return m_chunk != nullptr && m_blockIdx >= 0; }
	inline bool IsOpaque() const
	{
		const BlockDefinition& def = BlockDefinition::GetDefinitionByIndex(m_blockIdx);
		return def.m_isOpaque;
	}


	inline int GetLocalX() const
	{
		return (m_blockIdx & CHUNK_MASK_X);
	}

	inline int GetLocalY() const
	{
		return (m_blockIdx & CHUNK_MASK_Y) >> CHUNK_BITS_X;
	}

	inline int GetLocalZ() const 
	{
		return (m_blockIdx & CHUNK_MASK_Z) >> (CHUNK_BITS_X + CHUNK_BITS_Y);
	}

	inline IntVec3 GetLocalCoords() const 
	{
		return IntVec3(GetLocalX(), GetLocalY(), GetLocalZ());
	}


	inline bool HasWest() const { return m_chunk != nullptr && (GetLocalX() > 0 || m_chunk->m_west != nullptr); }
	inline bool HasEast() const { return m_chunk != nullptr && (GetLocalX() < CHUNK_MAX_X || m_chunk->m_east != nullptr); }
	inline bool HasSouth() const { return m_chunk != nullptr && (GetLocalY() > 0 || m_chunk->m_south != nullptr); }
	inline bool HasNorth() const { return m_chunk != nullptr && (GetLocalY() < CHUNK_MAX_Y || m_chunk->m_north != nullptr); }


	inline BlockIterator GetWestNeighbor() const 
	{
		if (!m_chunk) return Invalid();

		int x = GetLocalX();
		if (x > 0) 
		{
			return BlockIterator(m_chunk, m_blockIdx - STEP_X);
		}

		Chunk* west = m_chunk->m_west;
		if (!west) return Invalid();

		int newIdx = (m_blockIdx & ~CHUNK_MASK_X) | (CHUNK_MAX_X);
		return BlockIterator(west, newIdx);
	}

	inline BlockIterator GetEastNeighbor() const 
	{
		if (!m_chunk) return Invalid();

		int x = GetLocalX();
		if (x < CHUNK_MAX_X)
		{
			return BlockIterator(m_chunk, m_blockIdx + STEP_X);
		}

		Chunk* east = m_chunk->m_east;
		if (!east) return Invalid();

		int newIdx = (m_blockIdx & ~CHUNK_MASK_X);
		return BlockIterator(east, newIdx);
	}

	inline BlockIterator GetSouthNeighbor() const 
	{
		if (!m_chunk) return Invalid();

		int y = GetLocalY();
		if (y > 0) 
		{
			return BlockIterator(m_chunk, m_blockIdx - STEP_Y);
		}

		Chunk* south = m_chunk->m_south;
		if (!south) return Invalid();

		int newIdx = (m_blockIdx & ~CHUNK_MASK_Y) | (CHUNK_MAX_Y << CHUNK_BITS_X);
		return BlockIterator(south, newIdx);
	}

	inline BlockIterator GetNorthNeighbor() const 
	{
		if (!m_chunk) return Invalid();

		int y = GetLocalY();
		if (y < CHUNK_MAX_Y) 
		{
			return BlockIterator(m_chunk, m_blockIdx + STEP_Y);
		}

		Chunk* north = m_chunk->m_north;
		if (!north) return Invalid();

		int newIdx = (m_blockIdx & ~CHUNK_MASK_Y);
		return BlockIterator(north, newIdx);
	}

	inline BlockIterator GetBelowNeighbor() const 
	{
		if (!m_chunk) return Invalid();

		int z = GetLocalZ();
		if (z > 0) 
		{
			return BlockIterator(m_chunk, m_blockIdx - STEP_Z);
		}

		Chunk* below = m_chunk->m_below;
		if (!below) return Invalid();

		int newIdx = (m_blockIdx & ~CHUNK_MASK_Z) | (CHUNK_MAX_Z << (CHUNK_BITS_X + CHUNK_BITS_Y));
		return BlockIterator(below, newIdx);
	}

	inline BlockIterator GetAboveNeighbor() const 
	{
		if (!m_chunk) return Invalid();

		int z = GetLocalZ();
		if (z < CHUNK_MAX_Z) 
		{
			return BlockIterator(m_chunk, m_blockIdx + STEP_Z);
		}

		Chunk* above = m_chunk->m_above;
		if (!above) return Invalid();

		int newIdx = (m_blockIdx & ~CHUNK_MASK_Z);
		return BlockIterator(above, newIdx);
	}
};
