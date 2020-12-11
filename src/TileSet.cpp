#include <vector>
#include <stdio.h>
#include "imgui.h"
#include "Image.h"
#include "FileDialog.h"
#include "struse/struse.h"
#include "Config.h"
#include "GLFW/glfw3.h"
#include "SourceArt.h"
#include "LevelData.h"

void UpdateScaling(float& scale, float& zoom, float* pos, const ImVec2& mousePos, const ImVec2& tl, const ImVec2& br);

void ShowTileSets(Level* level, std::vector<ImTextureID> &aTextureIDs)
{
	ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(main_viewport->GetWorkPos().x + 480, main_viewport->GetWorkPos().y + 300), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);

	if (!ImGui::Begin("TileSet", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) { return; }

	ImVec2 winSize = ImGui::GetWindowSize();
	ImVec2 winPos = ImGui::GetWindowPos();
	ImVec2 mousePos = ImGui::GetMousePos();

	int loadedTab = -1;

	ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
	if (ImGui::BeginTabBar("TileSetImages", tab_bar_flags)) {
		if (const char* name = LoadGrabMapReady()) {
			TileSet* tileSet = new TileSet();
			level->tileSets.push_back(tileSet);
			tileSet->filename.relative_path(level->levelFile, name);
			tileSet->name.copy(strref(name).before_last('.').after_last_or_full('/', '\\'));
			level->LoadTileSetTo(tileSet);
			if (tileSet->image == nullptr) {
				level->tileSets.pop_back();
			} else {
				while (aTextureIDs.size() <= (size_t)tileSet->textureIndex) { aTextureIDs.push_back(ImTextureID(0)); }
				ImTextureID textureID = CreateTexture();
				aTextureIDs[tileSet->textureIndex] = textureID;
				if (textureID) {
					UpdateTextureData(tileSet->width, tileSet->height, tileSet->channels, tileSet->image);
					loadedTab = (int)level->tileSets.size() - 1;
				}
			}
		}
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		for (size_t i = 0, n = level->tileSets.size(); i < n; ++i) {
			if (ImGui::BeginTabItem(level->tileSets[i]->name.c_str(), 0, loadedTab == (int)i ? ImGuiTabItemFlags_SetSelected : 0)) {
				TileSet* tileSet = level->tileSets[i];
				ImVec2 pos = ImGui::GetCursorScreenPos();
				ImVec2 lp = ImGui::GetCursorPos();
				ImVec2 ep(winPos.x + winSize.x, winPos.y + winSize.y);

				UpdateScaling(tileSet->scale, tileSet->zoom, tileSet->imgPos, mousePos, pos, ep);

				float s = tileSet->scale;
				if (tileSet->image && aTextureIDs[tileSet->textureIndex]) {
					ImGui::SetCursorPos(ImVec2(lp.x - s * tileSet->imgPos[0], lp.y - s * tileSet->imgPos[1]));
					ImGui::Image(aTextureIDs[tileSet->textureIndex], ImVec2(tileSet->width * s, tileSet->height * s));
				}

				if (mousePos.x >= pos.x && mousePos.y >= pos.y && mousePos.x < ep.x && mousePos.y < ep.y) {
					float mx = mousePos.x - pos.x + s * tileSet->imgPos[0], my = mousePos.y - pos.y + s * tileSet->imgPos[1];
					if (mx >= 0.0f && my >= 0.0f && mx < (tileSet->width * s) && my < (tileSet->height * s)) {
						int tileX = (int)(mx / (s * level->tileWidth)), tileY = (int)(my / (s * level->tileHeight));

						ImVec2 p0((tileX * level->tileWidth - tileSet->imgPos[0]) * s + pos.x, (tileY * level->tileHeight - tileSet->imgPos[1]) * s + pos.y);
						ImVec2 p1(p0.x + level->tileWidth * s, p0.y + level->tileHeight * s);
						ImColor col(255, 255, 255, 128);
						draw_list->AddRect(p0, p1, col);

						if (ImGui::IsMouseClicked(ImGuiMouseButton_::ImGuiMouseButton_Left)) {
							level->currTileMap = (int)i;
							level->currTileX = tileX;
							level->currTileY = tileY;
							level->currTemplateImage = -1;
							level->selectedTemplate = -1;
						}
					}
				}

				if (level->currTileMap == (int)i) {
					int tx = level->currTileX;
					int ty = level->currTileY;
					ImVec2 p0((tx * level->tileWidth - tileSet->imgPos[0]) * s + pos.x, (ty * level->tileHeight - tileSet->imgPos[1]) * s + pos.y);
					ImVec2 p1(p0.x + level->tileWidth * s, p0.y + level->tileHeight * s);
					ImU32 col = (120 << IM_COL32_R_SHIFT) | (105 << IM_COL32_G_SHIFT) | (196 << IM_COL32_B_SHIFT) | (80 << IM_COL32_A_SHIFT);

					draw_list->AddRectFilled(p0, p1, col);
				}
				ImGui::EndTabItem();
			}
		}

		if (ImGui::BeginTabItem("Add Map")) {
			if (ImGui::Button("Open")) {
				LoadGrabMapDialog();
			}
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	ImGui::End();
}

