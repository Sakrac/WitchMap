#define STRUSE_IMPLEMENTATION
#include "struse/struse.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "LevelData.h"
#include "Files.h"
#include "Config.h"

#ifdef __linux__
#include <linux/limits.h>
#define PATH_MAX_LEN PATH_MAX
#define sprintf_s sprintf
#else
#define PATH_MAX_LEN _MAX_PATH
#endif

bool AddTemplateFrames(strref data, TemplateSet* tmp)
{
	ConfigParse info(data);
	while (!info.Empty()) {
		strref name, value;
		ConfigParseType type = info.Next(&name, &value);
		if (type == CPT_Struct && name.same_str("Frame")) {
			tmp->frames.push_back(TemplateFrame());
			TemplateFrame *frame = &tmp->frames[tmp->frames.size()-1];
			ConfigParse frameInfo(value);
			while (!frameInfo.Empty()) {
				strref param, num;
				if (frameInfo.Next(&param, &num) == CPT_Value) {
					if (param.same_str("x")) { frame->x = (int)num.atoi(); }
					else if (param.same_str("y")) { frame->y = (int)num.atoi(); }
					else if (param.same_str("w")) { frame->w = (int)num.atoi(); }
					else if (param.same_str("h")) { frame->h = (int)num.atoi(); }
					else { return false; }
				}
				else { return false; }
			}
		}
		else { return false; }
	}
	return true;
}

bool AddTemplate(strref data, TemplateSet* set)
{
	ConfigParse info(data);
	while (!info.Empty()) {
		strref name, value;
		ConfigParseType type = info.Next(&name, &value);
		if (type == CPT_Value) {
			if (name.same_str("Type")) {
				if (value.same_str("chars")) {
					set->type = TemplateSet::chars;
				}
				else {
					set->type = TemplateSet::sprite;
				}
			}
			else if (name.same_str("ID")) {
				set->ID = (TemplateSet::Type)value.atoi();
			}
		}
		else if (type == CPT_Array) {
			if (name.same_str("Frames")) {
				if (!AddTemplateFrames(value, set)) {
					return false;
				}
			}
			else if (name.same_str("Anim")) {
				// don't care about anims in the editor just yet
			}
		}

	}
	return true;
}

bool AddTemplateArray(strref value, TemplateImage* tmp)
{
	ConfigParse info(value);
	while (!info.Empty()) {
		strref name, value;
		ConfigParseType type = info.Next(&name, &value);
		if (type == CPT_Struct) {
			TemplateSet* set = new TemplateSet();
			tmp->templateSets.push_back(set);
			set->name.copy(name);
			if (!AddTemplate(value, set)) {
				return false;
			}
		}
	}
	return true;
}


bool Level::LoadTemplateImage(TemplateImage* map)
{
	size_t size;
	uint8_t* data;
	strown<PATH_MAX_LEN> fullPath;
	if (map->templateFile[1] == ':' || map->templateFile[0] == '/' || map->templateFile[0] == '\\') {
		fullPath.copy(map->templateFile);
	} else {
		fullPath.copy(levelFile.get_substr(0, (strl_t)levelFile.find_last('/', '\\') + 1));
		fullPath.append(map->templateFile.get_strref());
	}
	data = LoadBinary(fullPath.c_str(), size);
	if (data == nullptr) { return false; }
	ConfigParse info(strref((const char*)data, (strl_t)size));
	while (!info.Empty()) {
		strref name, value;
		ConfigParseType type = info.Next(&name, &value);
		if (type == CPT_Value) {
			if (name.same_str("Name")) {
				map->name.copy(value);
			}
			else if (name.same_str("Image")) {
				int width, height, channels;
				//currImage = CurrLoadImage::None;
				map->filename.copy(value);
				uint8_t* image;
				{
					strown<PATH_MAX_LEN> imagePath;
					if (map->filename[1] == ':' || map->filename[0] == '/' || map->filename[0] == '\\') {
						imagePath.copy(map->filename);
					} else {
						imagePath.copy(fullPath.get_substr(0, (strl_t)fullPath.find_last('/', '\\') + 1));
						imagePath.append(map->filename.get_strref());
					}
					image = stbi_load(imagePath.c_str(), &width, &height, &channels, 0);
				}
				if( image ) {
					map->textureIndex = nextTextureIndex++;
					map->width = width;
					map->height = height;
					map->channels = channels;
					map->scale = 1.0f;
					map->image = image;
				}
			}
		}
		else if (type == CPT_Array && name.same_str("Templates")) {
			AddTemplateArray(value, map);
		}
	}
	return true;
}

bool Level::LoadTileSetTo(TileSet* tileSet)
{
	if (tileSet->filename) {
		strown<PATH_MAX_LEN> fullPath;
		if (tileSet->filename[1] == ':' || tileSet->filename[0]=='/' || tileSet->filename[0]=='\\') {
			fullPath.copy(tileSet->filename);
		} else {
			fullPath.copy(levelFile.get_substr(0, (strl_t)levelFile.find_last('/', '\\')+1));
			fullPath.append(tileSet->filename.get_strref());
		}
		int width, height, channels;
		uint8_t* image = stbi_load(fullPath.c_str(), &width, &height, &channels, 0);
		if (image) {
			tileSet->textureIndex = nextTextureIndex++;// textureID;
			tileSet->width = width;
			tileSet->height = height;
			tileSet->channels = channels;
			tileSet->scale = 1.0f;
			tileSet->image = image;
			tileSet->numTilesX = width / tileWidth;
			tileSet->numTilesY = height / tileHeight;
			return true;
		}
	}
	return false;
}

bool Level::LoadTileSets(strref tileSetData)
{
	bool error = false;
	ConfigParse grabs(tileSetData);
	while (!grabs.Empty()) {
		strref name2, value2;
		ConfigParseType type2 = grabs.Next(&name2, &value2);
		if (name2.same_str("TileSet") && type2 == ConfigParseType::CPT_Struct) {
			TileSet *tileSet = new TileSet();
			tileSets.push_back(tileSet);
			tileSet->scale = 1.0f;
			tileSet->width = 0;
			tileSet->height = 0;
			tileSet->image = nullptr;
			ConfigParse details(value2);
			while (!details.Empty()) {
				strref name3, value3;
				ConfigParseType type3 = details.Next(&name3, &value3);
				if (type3 == ConfigParseType::CPT_Value) {
					if (name3.same_str("Name")) { tileSet->name.copy(value3); }
					else if (name3.same_str("Image")) { tileSet->filename.copy(value3); }
				}
			}
			if (tileSet->filename) {
				LoadTileSetTo(tileSet);
			}
			if (tileSet->image == nullptr) {
				tileSets.pop_back();
				error = true;
			}
		}
	}
	return error;
}

bool Level::LoadLayer(strref layerData, TileLayer& layer)
{
	layerData.trim_whitespace();
	if (layerData.get_first() == '{') { ++layerData; layerData.clip(1); }
	size_t index = 0;
	for (int y = 0; y < mapHeight; ++y) {
		for (int x = 0; x < mapWidth; ++x) {
			layerData.skip_whitespace();
			if (layerData.is_empty() || layer.size() <= index) { return true; }
			Tile& tile = layer[index++];
			strref value = layerData.get_word();
			layerData += value.get_len();
			tile.set = (uint8_t)value.get_substr(0, 2).ahextoui();
			tile.tileX = (uint8_t)value.get_substr(2, 2).ahextoui();
			tile.tileY = (uint8_t)value.get_substr(4, 2).ahextoui();
		}
	}
	return false;
}

bool Level::LoadOverlay(strref overlayData)
{
	ConfigParse info(overlayData);
	while (!info.Empty()) {
		strref name, value;
		ConfigParseType type = info.Next(&name, &value);
		if (type != CPT_Struct) { return false; }
		ConfigParse itemInfo(value);
		TemplateItem item;
		while(!itemInfo.Empty()) {
			strref param, num;
			ConfigParseType paramType = itemInfo.Next(&param, &num);
			if (paramType != CPT_Value) { return false; }
			if (param.same_str("x")) { item.x = (int)num.atoi(); }
			if (param.same_str("y")) { item.y = (int)num.atoi(); }
			if (param.same_str("image")) { item.image = (int)num.atoi(); }
			if (param.same_str("set")) { item.set = (int)num.atoi(); }
			if (param.same_str("frame")) { item.frame = (int)num.atoi(); }
		}
		overlay.push_back(item);
	}
	return true;
}

void Level::LoadLevel(const char* file)
{
	//bool error = false;
	size_t size;
	uint8_t* data = LoadBinary(file, size);
	if (data == nullptr) { return; }
	NewLevel();
	levelFile.copy(file);
	ConfigParse info(strref((const char*)data, (strl_t)size));
	while (!info.Empty()) {
		strref name, value;
		ConfigParseType type = info.Next(&name, &value);
		if (type == ConfigParseType::CPT_Value) {
			if (name.same_str("Width")) {
				int wid = (int)value.atoi();
				if (wid) { mapWidth = wid; }
				//else { error = true; }
			}
			else if (name.same_str("Height")) {
				int hgt = (int)value.atoi();
				if (hgt) { mapHeight = hgt; }
				//else { error = true; }
			}
		}
		else if (type == ConfigParseType::CPT_Array) {
			if (name.same_str("TileSets")) {
				LoadTileSets(value);
			} else if (name.same_str("TemplateFiles")) {
				ConfigParse templates(value);
				while (!templates.Empty()) {
					strref name2, value2;
					ConfigParseType type2 = templates.Next(&name2, &value2);
					if (type2 == CPT_Value) {
						TemplateImage* tmp = new TemplateImage();
						tmp->name.copy(name2);
						tmp->templateFile.copy(value2);
						if (LoadTemplateImage(tmp)) {
							templateImages.push_back(tmp);
						} else {
							delete tmp;
						}
					}
				}
			} else if (name.same_str("Layers")) {
				ConfigParse layers(value);
				while (!layers.Empty()) {
					strref name2, value2;
					layers.Next(&name2, &value2);
					if (name2.same_str("Background")) {
						LoadLayer(value2, background);
					}
					else if (name2.same_str("Properties")) {
						LoadLayer(value2, tileProps);
					}
				}
			} else if (name.same_str("Templates")) {
				if (!LoadOverlay(value)) { /*error = true;*/ }
			}
		}
	}
	free(data);
}

void Level::ClearLayer(TileLayer& layer)
{
	layer.clear();
	layer.reserve(size_t(mapWidth) * size_t(mapHeight));
	Tile empty = { 0xff, 0x00, 0x00 };
	for (size_t i = size_t(mapWidth) * size_t(mapHeight); i; --i) {
		layer.push_back(empty);
	}
}

void Level::NewLevel()
{
	sprintf_s(LevelWidthText, "%d", mapWidth);
	sprintf_s(LevelHeightText, "%d", mapHeight);
	levelFile.clear();
	mapPos[0] = 0.0f; mapPos[1] = 0.0f;
	while (tileSets.size()) {
		TileSet* map = tileSets[tileSets.size() - 1];
		if (map->image) { stbi_image_free(map->image); map->image = nullptr; }
		delete map;
		tileSets.pop_back();
	}
	while (templateImages.size()) {
		TemplateImage* tmp = templateImages[templateImages.size() - 1];
		if (tmp->image) { stbi_image_free(tmp->image); tmp->image = nullptr; }
		delete tmp;
		templateImages.pop_back();
	}
	overlay.clear();
	nextTextureIndex = 0;
	ClearLayer(background);
	ClearLayer(tileProps);

}
