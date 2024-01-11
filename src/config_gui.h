#ifndef CONFIG_GUI
#define CONFIG_GUI

#ifdef __cplusplus
extern "C" {
#endif

	extern HMODULE config_gui_hInstance;
	extern void config_gui_open(HWND hParent);

#ifdef __cplusplus
}
#endif

#endif // CONFIG_GUI