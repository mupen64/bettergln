#ifndef GLN64_H
#define GLN64_H
#include <filesystem>

extern HWND hWnd;
extern HWND hStatusBar;
extern HWND hToolBar;
extern HINSTANCE hInstance;

extern void (*CheckInterrupts)(void);
extern std::filesystem::path screenDirectory;

#define PLUGIN_VERSION L"1.4-rc2"

#ifdef _M_X64
#define PLUGIN_ARCH L"-x64"
#else
#define PLUGIN_ARCH L"-x86"
#endif

#ifdef _DEBUG
#define PLUGIN_TARGET L"-debug"
#else
#define PLUGIN_TARGET L"-release"
#endif

#define PLUGIN_NAME L"bettergln " PLUGIN_VERSION PLUGIN_ARCH PLUGIN_TARGET

#endif
