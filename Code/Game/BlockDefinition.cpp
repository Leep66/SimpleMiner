#include "Game/BlockDefinition.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/SpriteDefinition.hpp"
#include "Game/App.hpp"

extern App* g_theApp;

std::vector<BlockDefinition> BlockDefinition::s_blockDefs;

static BlockDefinition MakeUnknownDef()
{
	BlockDefinition def;
	def.m_name = "Unknown";
	def.m_isVisible = false;
	def.m_isSolid = false;
	def.m_isOpaque = false;
	def.m_isLiquid = false;
	def.m_topSpriteCoords = IntVec2(-1, -1);
	def.m_bottomSpriteCoords = IntVec2(-1, -1);
	def.m_sideSpriteCoords = IntVec2(-1, -1);
	def.m_indoorLighting = 0;
	return def;
}


BlockDefinition const& BlockDefinition::GetUnknown()
{
	static BlockDefinition s_unknown = MakeUnknownDef();
	return s_unknown;
}


void BlockDefinition::InitializeBlockDefinitions(const char* path)
{
	XmlDocument doc;
	XmlResult result = doc.LoadFile(path);
	if (result != tinyxml2::XML_SUCCESS)
	{
		ERROR_AND_DIE("Failed to load " + std::string(path));
	}

	XmlElement* root = doc.RootElement();
	if (root == nullptr)
	{
		ERROR_AND_DIE(std::string(path) + " is missing a root element");
	}

	for (XmlElement* elem = root->FirstChildElement("BlockDefinition"); elem != nullptr; elem = elem->NextSiblingElement("BlockDefinition"))
	{
		BlockDefinition def;

		def.m_name = ParseXmlAttribute(*elem, "name", "");
		def.m_isVisible = ParseXmlAttribute(*elem, "isVisible", false);
		def.m_isSolid = ParseXmlAttribute(*elem, "isSolid", false);
		def.m_isOpaque = ParseXmlAttribute(*elem, "isOpaque", false);
		def.m_isLiquid = ParseXmlAttribute(*elem, "isLiquid", false);
		def.m_topSpriteCoords = ParseXmlAttribute(*elem, "topSpriteCoords", IntVec2(7, 7));
		def.m_bottomSpriteCoords = ParseXmlAttribute(*elem, "bottomSpriteCoords", IntVec2(7, 7));
		def.m_sideSpriteCoords = ParseXmlAttribute(*elem, "sideSpriteCoords", IntVec2(7, 7));
		def.m_indoorLighting = ParseXmlAttribute(*elem, "indoorLighting", 0);
		def.m_outdoorLighting = ParseXmlAttribute(*elem, "outdoorLighting", 0);
		def.m_emissiveColor = ParseXmlAttribute(*elem, "color", Rgba8(0, 0, 0, 0));
		SpriteSheet* spriteSheet = g_theApp->GetGame()->m_spriteSheet;
		def.m_topUV = spriteSheet->GetSpriteDef(def.m_topSpriteCoords).GetUVs();
		def.m_botUV = spriteSheet->GetSpriteDef(def.m_bottomSpriteCoords).GetUVs();
		def.m_sideUV = spriteSheet->GetSpriteDef(def.m_sideSpriteCoords).GetUVs();
		s_blockDefs.push_back(def);
	}
}

BlockDefinition const& BlockDefinition::GetDefinition(const std::string& name)
{
	for (auto const& d : s_blockDefs) 
	{
		if (d.m_name == name)
		{
			return d;
		}
	}
	return GetUnknown();
}

BlockDefinition const& BlockDefinition::GetDefinitionByIndex(size_t index)
{
	if (index < s_blockDefs.size())
	{
		return s_blockDefs[index];
	}

	return GetUnknown();
}

uint8_t BlockDefinition::GetIndexByName(const std::string& name)
{
	for (size_t i = 0; i < (int)s_blockDefs.size(); ++i) 
	{
		if (s_blockDefs[i].m_name == name) return static_cast<uint8_t>(i);
	}
	return 0;
}
