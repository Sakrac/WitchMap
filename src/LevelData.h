#pragma once

#include <stdint.h>
#include <vector>

#define ZOOM_SCROLL_STEPS 32
#define ZOOM_RANGE 8.0f

// holds original art that can be either a tileset or a template image
struct SourceArt {
	strown<256> filename;
	strown<64> name;
	int textureIndex;	// don't require ImGUI for the level loader code, reference texture id from a separate array
	uint8_t* image;
	float scale; // updated from zoom value
	float zoom;
	float imgPos[2];
	int width, height, channels;

	SourceArt() : textureIndex(-1), image(nullptr), scale(2.0f), zoom(ZOOM_SCROLL_STEPS / 2) {
		imgPos[0] = 0.0f; imgPos[1] = 0.0f;
	}
};

// this is an area of a template set that represents an animation frame
// (you can place any frame of the animation in the map)
struct TemplateFrame {
	int x, y, w, h;
};

// this is a group of templates that loosely defines a set of template animation frames
struct TemplateSet {
	enum Type {
		chars = 0,
		sprite
	};
	Type type; // editor type
	int ID;	// game type ID
	std::vector<TemplateFrame> frames;
	strown<64> name;
};

struct TileSet : public SourceArt {
	int numTilesX, numTilesY;
};

struct TemplateImage : public SourceArt {
	strown<256> templateFile;
	std::vector<TemplateSet*> templateSets;

	// also anims..
};

struct Tile {
	uint8_t	set, tileX, tileY;
};

struct TileLayer : public std::vector<Tile> {
};

struct TemplateItem {
	int x, y;	// in pixels => chars must be placed in increments of 8
	uint8_t image, set, frame;
};

struct TemplateLayer : public std::vector<TemplateItem> {

};

struct Level {
	strown<256> levelFile;
	char LevelWidthText[32];
	char LevelHeightText[32];
	TileLayer background;
	TileLayer tileProps;
	TemplateLayer overlay;
	std::vector<TileSet*> tileSets;
	std::vector<TemplateImage*> templateImages;
	int currTileMap, currTileX, currTileY;
	int mapWidth;
	int mapHeight;
	int tileWidth;
	int tileHeight;
	int currentLayer;
	int selectedTemplate;
	int nextTextureIndex;
	int currTemplateImage, currTemplate, currTemplateFrame;
	float mapZoom;
	float mapPos[2];
	bool showBackground, showProperties, showOverlay;

	Level() : currTileMap(-1), currTileX(-1), currTileY(-1), mapWidth(10), mapHeight(32),
		tileWidth(32), tileHeight(32), currentLayer(0), mapZoom(ZOOM_SCROLL_STEPS / 2),
		showBackground(true), showProperties(true), showOverlay(true) {
		mapPos[0] = 0; mapPos[1] = 0;
		LevelWidthText[0] = 0;
		LevelHeightText[0] = 0;
		currTemplateImage = -1;
	}

	void NewLevel();
	bool LoadTemplateImage(TemplateImage* tmp);
	bool LoadTileSetTo(TileSet* tileSet);
	bool LoadTileSets(strref tileSetData);
	bool LoadLayer(strref layerData, TileLayer& layer);
	bool LoadOverlay(strref overlayData);
	void LoadLevel(const char* file);
	void ShowLevelMap();
	void ClearLayer(TileLayer& layer);
};

