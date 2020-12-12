// attempting a home made file dialog in ImGui for a little more portability
#include "struse/struse.h"
#include "FilesView.h"
#include <string.h>
#include <malloc.h>
#include "imgui.h"

#ifdef __linux__
#include <dirent.h>
#include <sys/stat.h>
#elif _WIN32
#endif

void FVFileView::Show(const char *folder)
{
    strovl usr(userPath, sizeof(userPath));
    usr.copy(folder);
    ReadDir(folder); 
    open = true; 
    selectedFile[0] = 0;
}

void FVFileView::Draw(const char *title)
{
    ImGui::SetNextWindowSize(ImVec2(740.0f, 410.0f), ImGuiCond_FirstUseEver);
    if (open && ImGui::Begin(title, &open)) {
        if(ImGui::InputText("Path", userPath, sizeof(userPath), ImGuiInputTextFlags_EnterReturnsTrue)) {
            printf("%s\n",userPath);
        }
        if(ImGui::InputText("File", userFile, sizeof(userFile), ImGuiInputTextFlags_EnterReturnsTrue)) {
            printf("%s\n",userFile);
        }
        ImGui::BeginChild("Directory", ImVec2(0, 0), true);
        for (size_t i=0, n=files.size(); i<n; ++i) {
            strown<PATH_MAX_LEN> fileTxt;
            if( files[i].fileType == FVFileInfo::dir) {
                fileTxt.copy("(dir)");
            } else {
                fileTxt.append_num(files[i].size, 0, 10);
            }
            fileTxt.pad_to(' ', 12);
            fileTxt.append(files[i].name);

            if( ImGui::Selectable(fileTxt.c_str(), i == selectIndex, ImGuiSelectableFlags_AllowDoubleClick)) {
                if (ImGui::IsMouseDoubleClicked(0)) {
                    strown<PATH_MAX_LEN> newPath(path);
                    newPath.append('/').append(files[i].name);
                    newPath.cleanup_path();
                    if( files[i].fileType == FVFileInfo::dir) {
                        strovl usr(userPath, sizeof(userPath));
                        usr.copy(newPath);
                        userFile[0] = 0;
                        ReadDir(newPath.c_str());
                        ImGui::SetScrollHereY(0.0f);
                        printf("entered folder %s\n", newPath.c_str());
                        break;
                    } else {
                        strovl sel(selectedFile, sizeof(selectedFile));
                        sel.copy(newPath);
                        sel.c_str();
                        printf("selected file %s\n", selectedFile);
                        open = false;
                        break;
                    }
                } else {
                    strovl usr(userFile, sizeof(userFile));
                    usr.copy(files[i].name);
                    selectIndex = i;
                }
            }
        }
        ImGui::EndChild();
        ImGui::End();
    }
}



void FVFileInfo::Free()
{
    if( name) { free(name); name = nullptr; }
}

void FVFileList::Clear()
{
    for( std::vector<FVFileInfo>::iterator i=files.begin(); i!=files.end(); ++i) {
        i->Free();
    }
    files.clear();
}


#ifdef __linux__

void FVFileList::ReadDir(const char *full_path)
{
	struct dirent *entry = nullptr;
	DIR *dp = nullptr;

    char* prev_path = path;
    path = strdup(full_path);
    Clear();
    if( prev_path) { free(prev_path);}

	dp = opendir(path);
	if (dp != nullptr) {
		while ((entry = readdir(dp))) {
            if( entry->d_type == DT_DIR || entry->d_type == DT_REG) {
                FVFileInfo info;
                info.name = strdup(entry->d_name);
                info.size = 0;
                if( entry->d_type == DT_DIR) {
                    info.fileType = FVFileInfo::dir;
                } else {
                    info.fileType = FVFileInfo::file;
                    struct stat statbuf;
                    strown<PATH_MAX_LEN> fullFile(path);
                    fullFile.append('/').append(info.name);

                    if (stat(fullFile.c_str(), &statbuf) == -1) {
                    /* check the value of errno */
                    } else {
                        info.size = statbuf.st_size;
                    }
                }
                files.push_back(info);
//    			printf ("%s (%s): %d\n", info.name, info.fileType==FVFileInfo::dir ? "dir" : "file", info.size);
            }
        }
	}

	closedir(dp);
}

#elif _WIN32
#endif

void TestFileView(const char* path)
{
    FVFileList files;
    files.ReadDir(path);
}