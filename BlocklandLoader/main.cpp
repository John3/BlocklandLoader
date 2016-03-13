#include <Windows.h>
#include <Psapi.h>

#include "detours/detours.h"

using MologieDetours::Detour;

static unsigned long image_base;
static unsigned long image_size;

bool sig_test(char *data, char *pattern, char *mask)
{
	for (; *mask; ++data, ++pattern, ++mask)
	{
		if (*mask == 'x' && *data != *pattern)
			return false;
	}

	return *mask == NULL;
}

void *sig_find(char *pattern, char *mask)
{
	unsigned long i = image_base;
	unsigned long end = i + image_size - strlen(mask);

	for (; i < end; i++)
	{
		if (sig_test((char *)i, pattern, mask))
			return (void *)i;
	}

	return 0;
}

typedef void(*Sim__init_t)(void);
Sim__init_t Sim__init;

typedef void(*Con__printf_t)(const char *format, ...);
Con__printf_t Con__printf;

Detour<Sim__init_t> *detour_Sim__init;

void hook_Sim__init(void)
{
	bool found_any = false;

	const char *moduleDir = "modules\\";
	char moduleSearch[MAX_PATH];
	strcpy_s(moduleSearch, moduleDir);
	strcat_s(moduleSearch, "*.dll");

	WIN32_FIND_DATAA ffd;
	HANDLE hFind = FindFirstFileA(moduleSearch, &ffd);

	if (hFind != INVALID_HANDLE_VALUE) do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		found_any = true;
		Con__printf("Loading %s", ffd.cFileName);

		char moduleName[MAX_PATH];
		strcpy_s(moduleName, moduleDir);
		strcat_s(moduleName, ffd.cFileName);

		HMODULE module = LoadLibraryA(moduleName);

		if (module == NULL)
			Con__printf("   \x03" "Failed to load");
	} while (FindNextFileA(hFind, &ffd) != 0);

	if (!found_any)
	{
		Con__printf("BlocklandLoader found no DLLs in the 'modules' directory");
		Con__printf("--------------------------------------------------------");
	}

	Con__printf("");
	return detour_Sim__init->GetOriginalFunction()();
}

bool __stdcall DllMain(void *, unsigned int reason, void *)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		// GetModuleHandleA("Blockland.exe") would be better, but I don't want to base it on the filename.
		// Any suggestions?
		HMODULE module = (HMODULE)0x400000;

		MODULEINFO info;
		GetModuleInformation(GetCurrentProcess(), module, &info, sizeof MODULEINFO);

		image_base = (unsigned long)info.lpBaseOfDll;
		image_size = info.SizeOfImage;

		Sim__init = (Sim__init_t)sig_find("\x56\x33\xF6\x57\x89\x35", "xxxxxx");
		Con__printf = (Con__printf_t)sig_find("\x8B\x4C\x24\x04\x8D\x44\x24\x08\x50\x6A\x00\x6A\x00\xE8\x00\x00\x00\x00\x83\xC4\x0C\xC3", "xxxxxxxxxxxxxx????xxxx");

		if (Sim__init && Con__printf)
			detour_Sim__init = new Detour<Sim__init_t>(Sim__init, hook_Sim__init);
	}

	return true;
}

extern "C" void __declspec(dllexport) __cdecl loader(void) { }