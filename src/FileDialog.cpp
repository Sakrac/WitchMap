#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#endif
#include <stdio.h>
#include "Config.h"
#include "FileDialog.h"
#ifndef _WIN32
#include "LimeOats/L2DFileDialog.h"
#endif
/* L2D File Dialog for ImGui Instructions
Usage
Add L2DFileDialog.h to your C++ project and include it where you use ImGui.
Then, set FileDialog::fileDialogOpen to true to set the open state.
Finally, in your update method, check if FileDialog::fileDialogOpen == true and if so,
call FileDialog::ShowFileDialog, passing in a char array as a buffer to store
the chosen file/folder path.
*/


#define FILE_LOAD_THREAD_STACK 8192

HANDLE hThreadFileDialog = 0;
static bool sFileDialogOpen = false;
static bool sImportImageReady = false;
static bool sLoadAnimReady = false;
static bool sSaveAsAnimReady = false;
static bool sSaveAsLevelReady = false;
static bool sLoadLevelReady = false;
static bool sLoadGrabMapReady = false;
static bool sLoadTemplateImageReady = false;

static char sImportImageFile[MAX_PATH] = {};
static char sLoadGrabFile[MAX_PATH] = {};
static char sLoadAnimFile[MAX_PATH] = {};
static char sSaveLevelFile[MAX_PATH] = {};
static char sLoadLevelFile[MAX_PATH] = {};
static char sLoadTemplateFile[MAX_PATH] = {};


static char sCurrentDir[ MAX_PATH ] = {};

struct FileTypeInfo {
	const char* fileTypes;
	char* fileName;
	bool* doneFlag;
};

static FileTypeInfo aImportInfo = { "Png\0*.png\0BMP\0*.bmp\0TGA\0*.tga\0", sImportImageFile, &sImportImageReady };
static FileTypeInfo aLoadAnimInfo = { "Animation\0*.can\0", sLoadAnimFile, &sLoadAnimReady };
static FileTypeInfo aSaveAsInfo = { "Animation\0*.can\0", sLoadAnimFile, &sSaveAsAnimReady };
static FileTypeInfo aSaveLevelAsInfo = { "Level\0*.txt\0", sSaveLevelFile, &sSaveAsLevelReady };
static FileTypeInfo aLoadLevelInfo = { "Level\0*.txt\0", sLoadLevelFile, &sLoadLevelReady };
static FileTypeInfo aLoadGrabInfo = { "GrabMap\0*.png\0*.bmp\0*.tga\0", sLoadGrabFile, &sLoadGrabMapReady };
static FileTypeInfo aLoadTemplateInfo = { "Template\0*.txt\0", sLoadTemplateFile, &sLoadTemplateImageReady };

void GetStartFolder()
{
	if( GetCurrentDirectory( sizeof( sCurrentDir ), sCurrentDir ) != 0 ) {
		return;
	}
	sCurrentDir[ 0 ] = 0;
}

void ResetStartFolder()
{
	if( sCurrentDir[ 0 ] ) {
		SetCurrentDirectory( sCurrentDir );
	}
}

bool IsFileDialogOpen() { return sFileDialogOpen; }

const char* ImportImageReady()
{
	if (sImportImageReady) {
		sImportImageReady = false;
		return sImportImageFile;
	}
	return nullptr;
}

const char* LoadGrabMapReady()
{
	if (sLoadGrabMapReady) {
		sLoadGrabMapReady = false;
		return sLoadGrabFile;
	}
	return nullptr;
}

const char* LoadTemplateImageReady()
{
	if (sLoadTemplateImageReady) {
		sLoadTemplateImageReady = false;
		return sLoadTemplateFile;
	}
	return nullptr;
}

const char* LoadAnimReady()
{
	if (sLoadAnimReady) {
		sLoadAnimReady = false;
		return sLoadAnimFile;
	}
	return nullptr;
}

const char* SaveAsAnimReady()
{
	if (sSaveAsAnimReady) {
		sSaveAsAnimReady = false;
		return sLoadAnimFile;
	}
	return nullptr;
}

const char* SaveLevelAsReady()
{
	if (sSaveAsLevelReady) {
		sSaveAsLevelReady = false;
		return sSaveLevelFile;
	}
	return nullptr;
}

const char* LoadLevelReady()
{
	if (sLoadLevelReady) {
		sLoadLevelReady = false;
		return sLoadLevelFile;
	}
	return nullptr;
}


void *FileLoadDialogThreadRun( void *param )
{
	FileTypeInfo* info = (FileTypeInfo*)param;
	OPENFILENAME ofn = {};
	ofn.lStructSize = sizeof( OPENFILENAME );
//	ofn.hInstance = GetPrgInstance();
	ofn.lpstrFile = info->fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = info->fileTypes;// "All\0*.*\0Prg\0*.prg\0Bin\0*.bin\0";
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if( GetOpenFileName( &ofn ) != TRUE )
	{
		DWORD err = GetLastError();
		sFileDialogOpen = false;
		return nullptr;
	}
	sFileDialogOpen = false;
	*info->doneFlag = true;
	return nullptr;
}

void *FileSaveDialogThreadRun(void *param)
{
	FileTypeInfo* info = (FileTypeInfo*)param;
	OPENFILENAME ofn = {};
	ofn.lStructSize = sizeof(OPENFILENAME);
	//	ofn.hInstance = GetPrgInstance();
	ofn.lpstrFile = info->fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = info->fileTypes;// "All\0*.*\0Prg\0*.prg\0Bin\0*.bin\0";
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetSaveFileName(&ofn) != TRUE) {
		DWORD err = GetLastError();
		sFileDialogOpen = false;
		return nullptr;
	}
	sFileDialogOpen = false;
	*info->doneFlag = true;
	return nullptr;
}


void LoadImageDialog()
{
	sImportImageReady = false;
	sFileDialogOpen = true;

	hThreadFileDialog = CreateThread(NULL, FILE_LOAD_THREAD_STACK, (LPTHREAD_START_ROUTINE)FileLoadDialogThreadRun, &aImportInfo,
									 0, NULL);
}

void LoadTemplateDialog()
{
	sLoadTemplateImageReady = false;
	sFileDialogOpen = true;

	hThreadFileDialog = CreateThread(NULL, FILE_LOAD_THREAD_STACK, (LPTHREAD_START_ROUTINE)FileLoadDialogThreadRun, &aLoadTemplateInfo,
		0, NULL);
}

void LoadGrabMapDialog()
{
	sLoadGrabMapReady = false;
	sFileDialogOpen = true;

	hThreadFileDialog = CreateThread(NULL, FILE_LOAD_THREAD_STACK, (LPTHREAD_START_ROUTINE)FileLoadDialogThreadRun, &aLoadGrabInfo,
		0, NULL);
}

void LoadAnimDialog()
{
	sLoadAnimReady = false;
	sFileDialogOpen = true;

	hThreadFileDialog = CreateThread(NULL, FILE_LOAD_THREAD_STACK, (LPTHREAD_START_ROUTINE)FileLoadDialogThreadRun, &aLoadAnimInfo,
									 0, NULL);
}

void SaveAnimDialog()
{
	sLoadAnimReady = false;
	sFileDialogOpen = true;

	hThreadFileDialog = CreateThread(NULL, FILE_LOAD_THREAD_STACK, (LPTHREAD_START_ROUTINE)FileSaveDialogThreadRun, &aSaveAsInfo,
									 0, NULL);
}

void SaveLevelDialog()
{
	sLoadAnimReady = false;
	sFileDialogOpen = true;

	hThreadFileDialog = CreateThread(NULL, FILE_LOAD_THREAD_STACK, (LPTHREAD_START_ROUTINE)FileSaveDialogThreadRun, &aSaveLevelAsInfo,
									 0, NULL);
}

void LoadLevelDialog()
{
	sLoadAnimReady = false;
	sFileDialogOpen = true;

	hThreadFileDialog = CreateThread(NULL, FILE_LOAD_THREAD_STACK, (LPTHREAD_START_ROUTINE)FileLoadDialogThreadRun, &aLoadLevelInfo,
		0, NULL);
}
