#pragma once
#include <cstdint>
#include "Engine/Core/Rgba8.hpp"

struct Block
{
	uint8_t m_typeIndex = 0;
	uint8_t m_light = 0;
	uint8_t m_flags = 0;

	Rgba8   m_indoorLightRGB = Rgba8(0, 0, 0, 255);
	Rgba8   m_outdoorLightRGB = Rgba8(0, 0, 0, 255);

	static constexpr uint8_t OUTDOOR_MASK = 0xF0; 
	static constexpr uint8_t INDOOR_MASK = 0x0F; 

	static inline uint8_t ClampLight(uint8_t v)
	{
		return v > 15 ? 15 : v;
	}

	inline uint8_t GetOutdoorLight() const
	{
		return (m_light & OUTDOOR_MASK) >> 4;
	}

	inline void SetOutdoorLight(uint8_t value)
	{
		value = ClampLight(value);
		m_light = (m_light & ~OUTDOOR_MASK) | (value << 4);
	}

	inline uint8_t GetIndoorLight() const
	{
		return (m_light & INDOOR_MASK);
	}

	inline void SetIndoorLight(uint8_t value)
	{
		value = ClampLight(value);
		m_light = (m_light & ~INDOOR_MASK) | value;
	}

	inline void ClearRGBLight()
	{
		m_indoorLightRGB = Rgba8(0, 0, 0, 0);
		m_outdoorLightRGB = Rgba8(0, 0, 0, 0);
	}

	enum : uint8_t
	{
		BLOCK_BIT_IS_SKY = 1 << 0, 
		BLOCK_BIT_IS_LIGHT_DIRTY = 1 << 1, 
		BLOCK_BIT_IS_FULL_OPAQUE = 1 << 2, 
		BLOCK_BIT_IS_SOLID = 1 << 3,
		BLOCK_BIT_IS_VISIBLE = 1 << 4, 
	};

	inline bool GetFlag(uint8_t mask) const
	{
		return (m_flags & mask) != 0;
	}

	inline void SetFlag(uint8_t mask, bool enabled)
	{
		if (enabled)
			m_flags |= mask;
		else
			m_flags &= ~mask;
	}

	inline bool IsSky() const { return GetFlag(BLOCK_BIT_IS_SKY); }
	inline void SetIsSky(bool v) { SetFlag(BLOCK_BIT_IS_SKY, v); }

	inline bool IsLightDirty() const { return GetFlag(BLOCK_BIT_IS_LIGHT_DIRTY); }
	inline void SetIsLightDirty(bool v) { SetFlag(BLOCK_BIT_IS_LIGHT_DIRTY, v); }

	inline bool IsFullOpaque() const { return GetFlag(BLOCK_BIT_IS_FULL_OPAQUE); }
	inline void SetIsFullOpaque(bool v) { SetFlag(BLOCK_BIT_IS_FULL_OPAQUE, v); }

	inline bool IsSolid() const { return GetFlag(BLOCK_BIT_IS_SOLID); }
	inline void SetIsSolid(bool v) { SetFlag(BLOCK_BIT_IS_SOLID, v); }

	inline bool IsVisible() const { return GetFlag(BLOCK_BIT_IS_VISIBLE); }
	inline void SetIsVisible(bool v) { SetFlag(BLOCK_BIT_IS_VISIBLE, v); }
};

