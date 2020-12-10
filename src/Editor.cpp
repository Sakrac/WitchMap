#include <vector>
#include <stdio.h>
#include <math.h>
#include "imgui.h"
#include "Image.h"
#include "FileDialog.h"
#include "struse/struse.h"
#include "Config.h"
#include "GLFW/glfw3.h"
#include "SourceArt.h"
#include "LevelData.h"
#include "Files.h"

void ShowTileSets(Level* level, std::vector<ImTextureID>& aTextureIDs);
void ShowTemplateImages(Level* level, std::vector<ImTextureID>& aTextureIDs);

static Level sLevel;
static std::vector<ImTextureID> textureIDs;
int mouseTilePrev[2] = { 0, 0 };

void ClearTextureIDs()
{
	for (size_t i = 0, n = textureIDs.size(); i < n; ++i) {
		DestroyTexture(textureIDs[i]);
		textureIDs[i] = 0;
	}
	textureIDs.clear();
}

void PostLoadTextureFixup(Level* level)
{
	textureIDs.clear();
	textureIDs.reserve(level->nextTextureIndex);
	for (int t = 0; t < level->nextTextureIndex; ++t) {
		textureIDs.push_back(ImTextureID(0));
	}
	// textures are in TileSets and Templates
	for (size_t g = 0, n = level->tileSets.size(); g < n; ++g) {
		TileSet* tileSet = level->tileSets[g];
		if (tileSet->image) {
			ImTextureID textureID = CreateTexture();
			textureIDs[tileSet->textureIndex] = textureID;
			if (textureID) {
				UpdateTextureData(tileSet->width, tileSet->height, tileSet->channels, tileSet->image);
			}
		}
	}

	for (size_t t = 0, n = level->templateImages.size(); t < n; ++t) {
		TemplateImage* tmp = level->templateImages[t];
		if (tmp->image) {
			ImTextureID textureID = CreateTexture();
			textureIDs[tmp->textureIndex] = textureID;
			if (textureID) {
				UpdateTextureData(tmp->width, tmp->height, tmp->channels, tmp->image);
			}
		}
	}

}

void SaveLayer(Level* level, UserData& conf, TileLayer& layer)
{
	size_t index = 0;
	for (int y = 0; y < level->mapHeight; ++y) {
		conf.Append("\t");
		for (int x = 0; x < level->mapWidth; ++x) {
			strown<16> value;
			Tile tile = layer[index++];
			if (x) { value.append(' '); }
			value.append_num(tile.set, 2, 16).append_num(tile.tileX, 2, 16).append_num(tile.tileY, 2, 16);
			conf.Append(value);
		}
		conf.Append("\n");
	}
}

void SaveLevel(Level *level)
{
	if (level->levelFile) {
		UserData conf;
		strown<128> arg;
		conf.AddValue("Width", level->mapWidth);
		conf.AddValue("Height", level->mapHeight);
		conf.BeginArray("TileSets");
		for (size_t i = 0, n = level->tileSets.size(); i < n; ++i) {
			TileSet* tileSet = level->tileSets[i];
			conf.BeginStruct("TileSet");
			conf.AddValue("Name", tileSet->name.get_strref());
			conf.AddValue("Image", tileSet->filename.get_strref());
			conf.EndStruct();
		}
		conf.EndArray();
		conf.BeginArray("TemplateFiles");
		for (size_t i = 0, n = level->templateImages.size(); i < n; ++i) {
			TemplateImage* tmp = level->templateImages[i];
			conf.AddValue(tmp->name.get_strref(), tmp->templateFile.get_strref());
		}
		conf.EndArray();

		conf.BeginArray("Layers");
		conf.BeginStruct("Background");
		SaveLayer(level, conf, level->background);
		conf.EndStruct();
		conf.BeginStruct("Properties");
		SaveLayer(level, conf, level->tileProps);
		conf.EndStruct();
		conf.EndArray();
		conf.BeginArray("Templates");
		for (size_t t = 0, n = level->overlay.size(); t < n; ++t) {
			TemplateItem* item = &level->overlay[t];
			if ((size_t)item->image < level->templateImages.size()) {
				TemplateImage* image = level->templateImages[item->image];
				if ((size_t)item->set < image->templateSets.size()) {
					TemplateSet* set = image->templateSets[item->set];
					if ((size_t)item->frame < set->frames.size()) {
						//TemplateFrame* frame = &set->frames[item->frame];
						conf.BeginStruct(set->name);
						conf.AddValue("image", item->image);
						conf.AddValue("set", item->set);
						conf.AddValue("frame", item->frame);
						conf.AddValue("x", item->x);
						conf.AddValue("y", item->y);
						conf.EndStruct();
					}
				}
			}
		}
		conf.EndArray();

		SaveFile(level->levelFile.c_str(), conf.start, conf.curr - conf.start);
	}
}





void CheckSaveLevelName(Level *level)
{
	if (const char* file = SaveLevelAsReady()) {
		level->levelFile.copy(file);
		SaveLevel(level);
	}
}

void CheckLoadLevelName()
{
	if (const char* file = LoadLevelReady()) {
		sLevel.NewLevel();
		ClearTextureIDs();
		sLevel.LoadLevel(file);
		ResetStartFolder();
		PostLoadTextureFixup(&sLevel);
	}
}

void MainMenu()
{
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New")) { sLevel.NewLevel(); ClearTextureIDs(); SaveLevelDialog(); }
			if (ImGui::MenuItem("Open")) { LoadLevelDialog(); }
			if (ImGui::MenuItem("Save")) {
				if (!sLevel.levelFile) { SaveLevelDialog();
				} else { SaveLevel(&sLevel); }
			}
			if (ImGui::MenuItem("Save As...")) {
				SaveLevelDialog();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

void UpdateScaling(float &scale, float &zoom, float* pos, const ImVec2 &mousePos, const ImVec2& tl, const ImVec2& br)
{
	float scalePrev = scale;
	if (mousePos.x >= tl.x && mousePos.x < br.x && mousePos.y >= tl.y && mousePos.y < br.y) {
		ImVec2 mouseMove = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle);
		ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);
		pos[0] -= mouseMove.x / scale;
		pos[1] -= mouseMove.y / scale;

		zoom += ImGui::GetIO().MouseWheel;
		zoom = zoom < 0.0f ? 0.0f : (zoom >= ZOOM_SCROLL_STEPS ? (ZOOM_SCROLL_STEPS - 1.0f) : zoom);
	}
	scale = powf(ZOOM_RANGE, float(zoom) / (float)(ZOOM_SCROLL_STEPS / 2) - 1.0f);
	if (scalePrev != scale) {
		ImVec2 mouseCurr = ImVec2((mousePos.x - tl.x) / scale, (mousePos.y - tl.y) / scale);
		ImVec2 mousePrev = ImVec2((mousePos.x - tl.x) / scalePrev, (mousePos.y - tl.y) / scalePrev);
		pos[0] -= mouseCurr.x - mousePrev.x;
		pos[1] -= mouseCurr.y - mousePrev.y;
	}

}

void ShowTemplateFrame(int x, int y, int imageNum, int setNum, int frameNum, Level *level, float mapScale, ImVec2 &spl)
{
	if ((size_t)imageNum < level->templateImages.size()) {
		TemplateImage* image = level->templateImages[imageNum];
		if ((size_t)setNum < image->templateSets.size()) {
			TemplateSet* set = image->templateSets[setNum];
			if ((size_t)frameNum < set->frames.size()) {
				TemplateFrame* frame = &set->frames[frameNum];

				if (set->type == TemplateSet::chars) {
					x &= ~7; y &= ~7;
				}
				ImVec2 tempPos((x - level->mapPos[0]) * mapScale + spl.x,
							   (y - level->mapPos[1]) * mapScale + spl.y);
				ImGui::SetCursorPos(tempPos);

				ImVec2 uv0((float)frame->x / (float)image->width, (float)frame->y / (float)image->height);
				ImVec2 uv1(uv0.x + (float)frame->w / (float)image->width, uv0.y + (float)frame->h / (float)image->height);

				ImGui::Image(textureIDs[image->textureIndex], ImVec2(frame->w * mapScale, frame->h * mapScale), uv0, uv1);
			}
		}
	}
}

void ShowTemplateFrame(TemplateItem* item, Level* level, float mapScale, ImVec2& spl)
{
	ShowTemplateFrame(item->x, item->y, item->image, item->set, item->frame, level, mapScale, spl);
}

TemplateFrame* GetTemplateFrameFromItem(TemplateItem& item, Level *level)
{
	if ((size_t)item.image < level->templateImages.size()) {
		TemplateImage* image = level->templateImages[item.image];
		if ((size_t)item.set < image->templateSets.size()) {
			TemplateSet* set = image->templateSets[item.set];
			if ((size_t)item.frame < set->frames.size()) {
				return &set->frames[item.frame];
			}
		}
	}
	return nullptr;
}

#ifndef _MSC_VER
#define sprintf_s sprintf
#endif 

void ShowLevelMap(Level *level)
{
	static float mapScale = 1.0f;

	ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(main_viewport->GetWorkPos().x + 32, main_viewport->GetWorkPos().y + 32), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(640, 480), ImGuiCond_FirstUseEver);

	if (!ImGui::Begin("Level Map", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) { return; }

	ImVec2 winSize = ImGui::GetWindowSize();
	ImVec2 winPos = ImGui::GetWindowPos();

	ImGui::Columns(2, "LevelMapHeaderColumns");

	if (ImGui::InputText("Width", level->LevelWidthText, sizeof(level->LevelWidthText), ImGuiInputTextFlags_CharsDecimal)) {
		int newWidth = atoi(level->LevelWidthText);
		if (newWidth) {
			sprintf_s(level->LevelWidthText, "%d", level->mapWidth);
		}
	}
	if (ImGui::InputText("Height", level->LevelHeightText, sizeof(level->LevelHeightText), ImGuiInputTextFlags_CharsDecimal)) {
		int newHeight = atoi(level->LevelWidthText);
		if (newHeight) {
			sprintf_s(level->LevelHeightText, "%d", level->mapHeight);
		}
	}

	ImGui::NextColumn();

	//static int e = 0;
	ImGui::RadioButton("background", &level->currentLayer, 0);
	ImGui::SameLine();
	ImGui::Checkbox("show##bg", &level->showBackground);
	ImGui::RadioButton("properties", &level->currentLayer, 1);
	ImGui::SameLine();
	ImGui::Checkbox("show##props", &level->showProperties);
	ImGui::RadioButton("overlay", &level->currentLayer, 2);
	ImGui::SameLine();
	ImGui::Checkbox("show##ol", &level->showOverlay);

	ImGui::Columns(1);

	ImDrawList* draw_list = ImGui::GetWindowDrawList();


	//	draw_list->AddRectFilled(p0, p1, col);
	ImVec2 sp = ImGui::GetCursorScreenPos();
	ImVec2 spl = ImGui::GetCursorPos();
	ImVec2 ep(winPos.x + winSize.x, winPos.y + winSize.y);

	float tl[2] = { -level->mapPos[0] * mapScale, -level->mapPos[1] * mapScale };
	float br[2] = { tl[0] + level->mapWidth * level->tileWidth * mapScale, tl[1] + level->mapHeight * level->tileHeight * mapScale };
	if (tl[0] < 0.0f) { tl[0] = 0.0f; }
	if (tl[1] < 0.0f) { tl[1] = 0.0f; }
	if (br[0] < 0.0f) { br[0] = 0.0f; }
	if (br[1] < 0.0f) { br[1] = 0.0f; }

	ImU32 col = (120 << IM_COL32_R_SHIFT) | (105 << IM_COL32_G_SHIFT) | (196 << IM_COL32_B_SHIFT) | (80 << IM_COL32_A_SHIFT);

	draw_list->AddRectFilled(ImVec2(tl[0] + sp.x, tl[1] + sp.y), ImVec2(br[0] + sp.x, br[1] + sp.y), col);

	// pixels left in window
	ImVec2 winLeft(winSize.x + winPos.x, winSize.y + winPos.y);
	//float mapWindowArea[2] = { winLeft.x - spl.x, winLeft.y - spl.y };

	ImVec2 mousePos = ImGui::GetMousePos();

	// draw the background
	ImVec2 winMin(tl[0] + sp.x, tl[1] + sp.y);
	ImVec2 winMax(br[0] + sp.x, br[1] + sp.y);
	ImGui::PushClipRect(winMin, winMax, true);

	ImVec2 winExt(winPos.x + winSize.x, winPos.y + winSize.y);


	UpdateScaling(mapScale, level->mapZoom, level->mapPos, mousePos, winPos, winExt);

	// feels ok with this speed regardless of zoom level
	if (ImGui::IsKeyDown(GLFW_KEY_UP)) { level->mapPos[1] -= 1.0f; }
	if (ImGui::IsKeyDown(GLFW_KEY_DOWN)) { level->mapPos[1] += 1.0f; }
	if (ImGui::IsKeyDown(GLFW_KEY_LEFT)) { level->mapPos[0] -= 1.0f; }
	if (ImGui::IsKeyDown(GLFW_KEY_RIGHT)) { level->mapPos[0] += 1.0f; }
	if (ImGui::IsKeyPressed(GLFW_KEY_ESCAPE)) {
		level->currTemplateImage = -1;
		level->currTileMap = -1;
		level->selectedTemplate = -1;
	}
	if (ImGui::IsKeyPressed(GLFW_KEY_DELETE)) {
		if (level->currentLayer == 2 && level->selectedTemplate >= 0) {
			if (level->selectedTemplate < (int)level->overlay.size()) {
				level->overlay.erase(level->overlay.begin() + level->selectedTemplate);
			}
			level->selectedTemplate = -1;
		}
	}

	// just draw all the tiles and let ImGui sort out the outside ones for now
	TileLayer* layers[] = { &level->background, &level->tileProps };
	for (size_t l = 0; l < 2; ++l) {
		if (l == 0 && !level->showBackground) { continue; }
		else if (l == 1 && !level->showProperties) { continue; }
		TileLayer* layer = layers[l];
		for (size_t y = 0, h = (size_t)level->mapHeight; y < h; ++y) {
			for (size_t x = 0, w = (size_t)level->mapWidth; x < w; ++x) {
				size_t i = y * w + x;
				Tile& tile = (*layer)[i];
				uint8_t grab = tile.set;
				if (grab != 0xff && (size_t)grab < level->tileSets.size()) {
					TileSet* tileSet = level->tileSets[grab];
					uint8_t tx = tile.tileX, ty = tile.tileY;
					float tileUV[2] = { (float)level->tileWidth / (float)(tileSet->width) ,
						(float)level->tileHeight / (float)tileSet->height };
					ImGui::SetCursorPos(ImVec2((x * level->tileWidth - level->mapPos[0]) * mapScale + spl.x,
						(y * level->tileHeight - level->mapPos[1]) * mapScale + spl.y));
					ImGui::Image(textureIDs[tileSet->textureIndex], ImVec2(level->tileWidth * mapScale, level->tileHeight * mapScale),
						ImVec2(tx * tileUV[0], ty * tileUV[1]),
						ImVec2((tx + 1) * tileUV[0], (ty + 1) * tileUV[1]));
				}
			}
		}
	}

	// draw all the templates in the overlay
	if (level->showOverlay) {
		for (size_t t = 0, n = level->overlay.size(); t < n; ++t) {
			TemplateItem* item = &level->overlay[t];
			ShowTemplateFrame(item, level, mapScale, spl);
		}
	}

	static int dragTemplate = -1;
	if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
		dragTemplate = -1;
	}

	// select templates in the overlay if not holding one
	if (level->currentLayer == 2 && level->currTemplateImage < 0) {
		if (level->selectedTemplate >= 0 && level->selectedTemplate < (int)level->overlay.size()) {
			TemplateItem& item = level->overlay[level->selectedTemplate];
			if (TemplateFrame* frame = GetTemplateFrameFromItem(item, level)) {
				ImVec2 itemPos((item.x - level->mapPos[0]) * mapScale + sp.x,
							   (item.y - level->mapPos[1]) * mapScale + sp.y);
				ImVec2 itemSize(frame->w * mapScale, frame->h * mapScale);
				draw_list->AddRectFilled(itemPos, ImVec2(itemPos.x + itemSize.x, itemPos.y + itemSize.y),
								   ImColor(255, 160, 32, 160));
			}
		}

		int currHoverItem = -1;
		ImVec2 currItemPos, currItemSize;
		for (size_t t = 0, n = level->overlay.size(); t < n; ++t) {
			TemplateItem& item = level->overlay[t];
			if( TemplateFrame* frame = GetTemplateFrameFromItem(item, level)) {
				ImVec2 itemPos((item.x - level->mapPos[0]) * mapScale + sp.x,
								(item.y - level->mapPos[1]) * mapScale + sp.y);
				ImVec2 itemSize(frame->w * mapScale, frame->h * mapScale);
				if (mousePos.x >= itemPos.x && mousePos.y >= itemPos.y &&
					mousePos.x < (itemPos.x + itemSize.x) && mousePos.y < (itemPos.y + itemSize.y)) {
					currHoverItem = (int)t;
					currItemPos = itemPos;
					currItemSize = itemSize;
				}
			}
		}
		static ImVec2 dragMouseStart;
		static int dragItemStart[2];

		if (dragTemplate >= 0) {
			TemplateItem& item = level->overlay[dragTemplate];
			TemplateImage* itemImg = level->templateImages[item.image];
			TemplateSet* itemSet = itemImg->templateSets[item.set];

			int dragCurrX = (int)((mousePos.x - sp.x - dragMouseStart.x)/mapScale);
			int dragCurrY = (int)((mousePos.y - sp.y - dragMouseStart.y)/mapScale);
			int newX = (dragItemStart[0] + dragCurrX);
			int newY = (dragItemStart[1] + dragCurrY);
			if (itemSet->type == TemplateSet::chars) {
				newX = (newX + 3) & ~7; newY = (newY + 3) & ~7;
			}
			item.x = newX; item.y = newY;
		}


		if (currHoverItem >= 0) {
			draw_list->AddRect(currItemPos, ImVec2(currItemPos.x + currItemSize.x,
												   currItemPos.y + currItemSize.y),
							   ImColor(255, 160, 32, 255));
			if(ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				level->selectedTemplate = currHoverItem;
				dragTemplate = currHoverItem;
				dragMouseStart = ImVec2(mousePos.x - sp.x, mousePos.y - sp.y);
				dragItemStart[0] = level->overlay[currHoverItem].x;
				dragItemStart[1] = level->overlay[currHoverItem].y;
			}
		}

	}

	// if current holding a template frame show it on the map
	if (level->currTemplateImage >= 0 && mousePos.x > sp.x && mousePos.y > sp.y && mousePos.x < ep.x && mousePos.y < ep.y ) {
		if ((size_t)level->currTemplateImage < level->templateImages.size()) {
			int x = (int)((mousePos.x - sp.x) / mapScale + level->mapPos[0]);
			int y = (int)((mousePos.y - sp.y) / mapScale + level->mapPos[1]);
			ShowTemplateFrame(x, y, level->currTemplateImage, level->currTemplate, level->currTemplateFrame,
							  level, mapScale, spl);
			if (mousePos.x > sp.x && mousePos.y > sp.y && mousePos.x < ep.x && mousePos.y < ep.y &&
				ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				level->currentLayer = 2;	// overlay layer
				TemplateImage* itemImg = level->templateImages[level->currTemplateImage];
				TemplateSet* itemSet = itemImg->templateSets[level->currTemplate];
				if (itemSet->type == TemplateSet::chars) { x = x & ~7; y = y & ~7; }
				TemplateItem item = { x, y, (uint8_t)level->currTemplateImage,
									  (uint8_t)level->currTemplate, (uint8_t)level->currTemplateFrame };
				level->overlay.push_back(item);
			}
		}
	}

	// current tile under mouse
	ImVec2 mouseOnMap = ImVec2(mousePos.x - sp.x, mousePos.y - sp.y);
	int mouseTile[2] = { (int)((mouseOnMap.x + level->mapPos[0] * mapScale) / (level->tileWidth * mapScale)),
						 (int)((mouseOnMap.y + level->mapPos[1] * mapScale) / (level->tileHeight * mapScale)) };
	if( mouseTile[0]>=0 && mouseTile[0] < level->mapWidth && mouseTile[1]>=0 && mouseTile[1] < level->mapHeight ) {
		// draw tile under cursor
		if (level->currTileMap >= 0 && level->currTileMap < (int)level->tileSets.size() &&
			level->currTileX >= 0 && level->currTileY >= 0) {
			TileSet *tileSet = level->tileSets[level->currTileMap];
			float tileUV[2] = { (float)level->tileWidth / (float)(tileSet->width), (float)level->tileHeight / (float)tileSet->height };
			if (level->currTileX < tileSet->numTilesX && level->currTileY < tileSet->numTilesY) {
				ImVec2 currTilePos((mouseTile[0] * level->tileWidth - level->mapPos[0])* mapScale + spl.x,
					(mouseTile[1] * level->tileHeight - level->mapPos[1])* mapScale + spl.y);
				ImGui::SetCursorPos(currTilePos);
				ImGui::Image(textureIDs[tileSet->textureIndex], ImVec2(level->tileWidth * mapScale, level->tileHeight * mapScale),
							 ImVec2(level->currTileX * tileUV[0], level->currTileY * tileUV[1]),
							 ImVec2((level->currTileX + 1) * tileUV[0], (level->currTileY + 1) * tileUV[1]));

				// comsider plonking down a tile here in the current layer
				bool mouseMoved = mouseTile[0] != mouseTilePrev[0] || mouseTile[1] != mouseTilePrev[1];
				TileLayer *layer = level->currentLayer==0 ? &level->background : (level->currentLayer==1 ? &level->tileProps : nullptr);
				if (layer && mousePos.x > sp.x && mousePos.y > sp.y && mousePos.x < ep.x && mousePos.y < ep.y) {
					if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || (ImGui::IsMouseDown(ImGuiMouseButton_Left) && mouseMoved)) {
						size_t index = size_t(level->mapWidth) * size_t(mouseTile[1]) + size_t(mouseTile[0]);
						if (index < level->background.size()) {
							Tile& tile = (*layer)[index];
							tile.set = (uint8_t)level->currTileMap;
							tile.tileX = (uint8_t)level->currTileX;
							tile.tileY = (uint8_t)level->currTileY;
						}
					}
				}
			}
		}
		mouseTilePrev[0] = mouseTile[0]; mouseTilePrev[1] = mouseTilePrev[1];
	} else {
		mouseTilePrev[0] = -1; mouseTilePrev[1] = -1;
	}
	ImGui::PopClipRect();

	ImGui::End();
}


void WitchEdInit()
{
	sLevel.NewLevel();
	ClearTextureIDs();
}

void WitchEdWindow()
{
	CheckSaveLevelName(&sLevel);
	CheckLoadLevelName();
	MainMenu();
	ShowTileSets(&sLevel, textureIDs);
	ShowTemplateImages(&sLevel, textureIDs);
	ShowLevelMap(&sLevel);
}