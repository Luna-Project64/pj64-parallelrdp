#include "ini.h"
#include <fcntl.h>
#include <Shlobj.h>
#include <shlwapi.h>
#include <windows.h>
#include <stdbool.h>

extern char gPluginConfigDir[MAX_PATH];
char ini_file[MAX_PATH] = { 0 };

#pragma comment(lib, "Shlwapi.lib")

void ini_init()
{
	if ('\0' != *ini_file)
		return;

	if (*gPluginConfigDir)
	{
		strncpy_s(ini_file, sizeof(ini_file), gPluginConfigDir, MAX_PATH);
	}
	else
	{
		SHGetFolderPath(NULL,
			CSIDL_APPDATA,
			NULL,
			0,
			ini_file);
	}

	PathAppend(ini_file, "LParallel");
	CreateDirectory(ini_file, NULL); // can fail, ignore errors
	PathAppend(ini_file, "cfg.ini");
}

bool ini_set_value(const char* key, int value)
{
	char num_str[10];
	itoa(value, num_str, 10);
    return WritePrivateProfileStringA("Settings", key, num_str, ini_file);
}

bool ini_get_value(const char* key, int* value)
{
    char buf[MAX_PATH];
    GetPrivateProfileStringA("Settings", key, NULL, buf, MAX_PATH, ini_file);
    if (buf == NULL)
    {
    	*value = 0;
        return false;
    }

    *value = atoi(buf);
    return true;
}

