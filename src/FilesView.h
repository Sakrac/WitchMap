#pragma once

#include <vector>
#ifdef __linux__
#include <linux/limits.h>
#define PATH_MAX_LEN PATH_MAX
#elif _WIN32
#define PATH_MAX_LEN _MAX_PATH
#endif

struct FVFileInfo {
	enum type {
		none,
		file,
		dir
	};
	char *name;
	size_t size;
	type fileType;
	void Free();
};

struct FVFileList {
	std::vector<FVFileInfo> files;
	char *filter;
	char *path;
	void ReadDir(const char *full_path, const char *filter = nullptr);
	void Clear();
	void InsertAlphabetically(FVFileInfo& info);

	FVFileList() : filter(nullptr), path(nullptr) {}
};

class FVFileView : protected FVFileList {
protected:
	bool open;
	size_t selectIndex;
	char userPath[PATH_MAX_LEN];
	char userFile[PATH_MAX_LEN];
	char selectedFile[PATH_MAX_LEN];
public:
	void Draw(const char *title);
	void Show(const char *folder, const char *filter = nullptr);
	bool IsOpen() const { return open; }
	const char* GetSelectedFile() const { return selectedFile; }
};
