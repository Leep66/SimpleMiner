#pragma once

enum class Biome 
{
	Ocean, 
	DeepOcean, 
	FrozenOcean,
	Beach, 
	SnowyBeach,
	Desert, 
	Savanna,
	Plains, 
	SnowyPlains,
	Forest, 
	Jungle,
	Taiga, 
	SnowyTaiga,
	StonyPeaks, 
	SnowyPeaks,
	Unknown
};


enum class Region { DeepOcean, Ocean, Coast, NearInland, MidInland, FarInland };
enum class ErosionBand { E0, E1, E2, E3, E4, E5, E6 };
enum class PVBand { Valleys, Low, Mid, High, Peaks };
enum class TBand { T0, T1, T2, T3, T4 };
enum class HBand { H0, H1, H2, H3, H4 };

static inline Region BucketRegion(float c) 
{
	if (c < -1.05f) return Region::DeepOcean;
	if (c < -0.455f) return Region::DeepOcean;
	if (c < -0.19f)  return Region::Ocean;
	if (c < -0.11f)  return Region::Coast;
	if (c < 0.03f)  return Region::NearInland;
	if (c < 0.30f)  return Region::MidInland;
	return Region::FarInland;
}

static inline ErosionBand BucketE(float e)
{
	if (e < -0.78f)      return ErosionBand::E0;        
	if (e < -0.375f)     return ErosionBand::E1;        
	if (e < -0.2225f)    return ErosionBand::E2;        
	if (e < 0.05f)      return ErosionBand::E3;         
	if (e < 0.45f)      return ErosionBand::E4;         
	if (e < 0.55f)      return ErosionBand::E5;         
	return ErosionBand::E6;                             
}

static inline PVBand BucketPV(float pv)
{
	if (pv < -0.85f) return PVBand::Valleys;
	if (pv < -0.2f)  return PVBand::Low;
	if (pv < 0.2f)  return PVBand::Mid;
	if (pv < 0.7f)  return PVBand::High;
	return PVBand::Peaks;
}

static inline TBand BucketT(float t)
{
	if (t < -0.45f) return TBand::T0;                   
	if (t < -0.15f) return TBand::T1;                   
	if (t < 0.20f) return TBand::T2;                    
	if (t < 0.55f) return TBand::T3;                    
	return TBand::T4;                                   
}

static inline HBand BucketH(float h) 
{
	if (h < -0.35f) return HBand::H0;
	if (h < -0.10f) return HBand::H1;
	if (h < 0.10f) return HBand::H2;
	if (h < 0.30f) return HBand::H3;
	return HBand::H4;
}


static inline Biome PickMiddle(TBand T, HBand H) 
{
	switch (H) 
	{
	case HBand::H0:
		switch (T)
		{
		case TBand::T0: return Biome::SnowyPlains;
		case TBand::T1: return Biome::Plains;
		case TBand::T2: return Biome::Savanna;
		case TBand::T3: return Biome::Desert;
		case TBand::T4: return Biome::Desert;
		}
		break;
	case HBand::H1:
		switch (T) 
		{
		case TBand::T0: return Biome::SnowyTaiga;
		case TBand::T1: return Biome::Forest;
		case TBand::T2: return Biome::Plains;
		case TBand::T3: return Biome::Plains;
		case TBand::T4: return Biome::Desert;
		}
		break;
	case HBand::H2:
		switch (T) 
		{
		case TBand::T0: return Biome::SnowyTaiga;
		case TBand::T1: return Biome::Forest;
		case TBand::T2: return Biome::Plains;
		case TBand::T3: return Biome::Plains;
		case TBand::T4: return Biome::Desert;
		}
		break;
	case HBand::H3:
		switch (T) 
		{
		case TBand::T0: return Biome::Taiga;
		case TBand::T1: return Biome::Forest;
		case TBand::T2: return Biome::Jungle;
		case TBand::T3: return Biome::Jungle;
		case TBand::T4: return Biome::Jungle; 
		}
		break;
	case HBand::H4:
		switch (T)
		{
		case TBand::T0: return Biome::Taiga;
		case TBand::T1: return Biome::Taiga;
		case TBand::T2: return Biome::Forest;
		case TBand::T3: return Biome::Jungle;
		case TBand::T4: return Biome::Jungle;
		}
		break;
	}
	return Biome::Plains;
}

static inline Biome PickBeach(TBand T) 
{
	if (T == TBand::T0) return Biome::SnowyBeach;
	if (T == TBand::T4) return Biome::Desert;
	return Biome::Beach;
}

static inline Biome PickBadland(HBand H)
{
	switch (H) 
	{
	case HBand::H3: case HBand::H4: return Biome::Savanna;
	default: return Biome::Desert;
	}
}

static inline Biome PickOceanLike(Region r, TBand T) 
{
	const bool deep = (r == Region::DeepOcean);
	if (T == TBand::T0) return deep ? Biome::FrozenOcean : Biome::FrozenOcean;
	return deep ? Biome::DeepOcean : Biome::Ocean;                              
}

static inline bool IsSnowyPeaks(TBand T) { return (T == TBand::T0 || T == TBand::T1 || T == TBand::T2); }

static Biome GetBiomeForParams(float temperature, float humidity, float erosion, float continentalness, float peaksValleys)
{
	const float t = temperature * 2.0f - 1.0f;
	const float h = humidity * 2.0f - 1.0f;
	const float e = erosion * 2.0f - 1.0f;
	const float c = continentalness * 2.0f - 1.0f;
	const float v = peaksValleys * 2.0f - 1.0f;

	Region r = BucketRegion(c);
	ErosionBand E = BucketE(e);
	PVBand PV = BucketPV(v);
	TBand T = BucketT(t);
	HBand H = BucketH(h);

	if (r == Region::DeepOcean || r == Region::Ocean)
		return PickOceanLike(r, T);

	if (r == Region::Coast)
		return PickBeach(T);

	if ((PV == PVBand::High || PV == PVBand::Peaks) && E == ErosionBand::E0)
	{
		return IsSnowyPeaks(T) ? Biome::SnowyPeaks : Biome::StonyPeaks;
	}

	if (T == TBand::T4)
	{
		return PickBadland(H);
	}

	if ((r == Region::NearInland || r == Region::MidInland) && PV == PVBand::Mid && E == ErosionBand::E5)
	{
		return PickBeach(T);
	}

	return PickMiddle(T, H);
}



BlockType GetSurfaceBlockForBiome(Biome biome)
{
	switch (biome)
	{
	case Biome::Ocean:        return BlockType::SAND;
	case Biome::DeepOcean:    return BlockType::SAND;
	case Biome::FrozenOcean:  return BlockType::ICE;
	case Biome::Beach:        return BlockType::SAND;
	case Biome::SnowyBeach:   return BlockType::SNOW;
	case Biome::Desert:       return BlockType::SAND;
	case Biome::Savanna:      return BlockType::GRASSYELLOW;
	case Biome::Plains:       return BlockType::GRASSLIGHT;
	case Biome::SnowyPlains:  return BlockType::SNOW;
	case Biome::Forest:       return BlockType::GRASS;
	case Biome::Jungle:       return BlockType::GRASSDARK;
	case Biome::Taiga:        return BlockType::GRASSLIGHT;
	case Biome::SnowyTaiga:   return BlockType::SNOW;
	case Biome::StonyPeaks:   return BlockType::STONE;
	case Biome::SnowyPeaks:   return BlockType::SNOW;
	default:                  return BlockType::GRASS;
	}
}


