#include <vector>
#include <stdio.h>
#include "imgui.h"
#include "image.h"
#include "FileDialog.h"
#include "struse/struse.h"
#include "Config.h"
#include "GLFW/glfw3.h"
#include "SourceArt.h"
#include "LevelData.h"
#include "Files.h"

void UpdateScaling(float& scale, float& zoom, float* pos, const ImVec2& mousePos, const ImVec2& tl, const ImVec2& br);

void ShowTemplateImages(Level *level, std::vector<ImTextureID> &aTextureIDs)
{
	ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(main_viewport->GetWorkPos().x + 480, main_viewport->GetWorkPos().y + 48), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);

	if (!ImGui::Begin("Templates", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) { return; }

	ImVec2 winSize = ImGui::GetWindowSize();
	ImVec2 winPos = ImGui::GetWindowPos();
	ImVec2 mousePos = ImGui::GetMousePos();

	int loadedTab = -1;

	ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
	if (ImGui::BeginTabBar("TemplateImages", tab_bar_flags)) {
		if (const char* name = LoadTemplateImageReady()) {
			TemplateImage* map = new TemplateImage();
			level->templateImages.push_back(map);
			map->templateFile.relative_path(level->levelFile, name);
			level->LoadTemplateImage(map);
			if (map->image == nullptr) {
				level->templateImages.pop_back();
			}
			else {
				while (aTextureIDs.size() <= (size_t)map->textureIndex) { aTextureIDs.push_back(ImTextureID(0)); }
				ImTextureID textureID = CreateTexture();
				aTextureIDs[map->textureIndex] = textureID;
				if (textureID) {
					UpdateTextureData(map->width, map->height, map->channels, map->image);
				}
			}
		}
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		for (size_t i = 0, n = level->templateImages.size(); i < n; ++i) {

			if (ImGui::BeginTabItem(level->templateImages[i]->name.c_str(), 0, loadedTab == (int)i ? ImGuiTabItemFlags_SetSelected : 0)) {
				TemplateImage* tmp = level->templateImages[i];
				ImVec2 lp = ImGui::GetCursorPos();
				ImVec2 sp = ImGui::GetCursorScreenPos();
				ImVec2 ep(winPos.x + winSize.x, winPos.y + winSize.y);

				UpdateScaling(tmp->scale, tmp->zoom, tmp->imgPos, mousePos, sp, ep);

				float s = tmp->scale;

				if (tmp->image && aTextureIDs[tmp->textureIndex]) {
					ImGui::SetCursorPos(ImVec2(lp.x - s * tmp->imgPos[0], lp.y - s * tmp->imgPos[1]));
					ImGui::Image(aTextureIDs[tmp->textureIndex], ImVec2(tmp->width * tmp->scale, tmp->height * tmp->scale));
				}
				for (size_t ts = 0, nts = tmp->templateSets.size(); ts < nts; ++ts) {
					TemplateSet* set = tmp->templateSets[ts];
					for (size_t f = 0, nf = set->frames.size(); f < nf; ++f) {
						TemplateFrame* frame = &set->frames[f];
						bool selected = i == level->currTemplateImage && ts == level->currTemplate && f == level->currTemplateFrame;
						ImColor frameCol = selected ? ImColor(255, 255, 0, 255) : ImColor(255, 255, 255, 160);
						ImVec2 tl(sp.x + s * (frame->x - tmp->imgPos[0]), sp.y + s * (frame->y - tmp->imgPos[1]));
						ImVec2 br(tl.x + s * frame->w, tl.y + s * frame->h);
						if (selected) {
							ImU32 col = (120 << IM_COL32_R_SHIFT) | (105 << IM_COL32_G_SHIFT) | (196 << IM_COL32_B_SHIFT) | (80 << IM_COL32_A_SHIFT);
							draw_list->AddRectFilled(tl, br, col);
						}
						if (mousePos.x >= tl.x && mousePos.y >= tl.y && mousePos.x < br.x && mousePos.y < br.y) {
							draw_list->AddRect(tl, br, frameCol);
							if (ImGui::IsMouseClicked(ImGuiMouseButton_::ImGuiMouseButton_Left)) {
								level->currTemplateImage = (int)i;
								level->currTemplate = (int)ts;
								level->currTemplateFrame = (int)f;
								level->currTileMap = -1;
								level->selectedTemplate = -1;
							}
						}
					}
				}
				ImGui::EndTabItem();
			}
		}

		if (ImGui::BeginTabItem("Add Templ. Image")) {
			if (ImGui::Button("Open")) {
				LoadTemplateDialog();
			}
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	ImGui::End();

}
