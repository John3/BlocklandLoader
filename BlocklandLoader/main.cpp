#include <Windows.h>
#include <Psapi.h>

#include <map>
#include <string>

#include "detours/detours.h"
#include "api/BlocklandLoader.hpp"

using MologieDetours::Detour;

using std::map;
using std::string;

static unsigned long imageBase;
static unsigned long imageSize;

map<string, HMODULE> moduleTable;

const char *moduleDir = "modules\\";

bool sigTest(const char *data, const char *pattern, const char *mask)
{
	for (; *mask; ++data, ++pattern, ++mask)
	{
		if (*mask == 'x' && *data != *pattern)
			return false;
	}

	return *mask == NULL;
}

void *sigFind(const char *pattern, const char *mask)
{
	unsigned long i = imageBase;
	unsigned long end = i + imageSize - strlen(mask);

	for (; i < end; i++)
	{
		if (sigTest((char *)i, pattern, mask))
			return (void *)i;
	}

	return 0;
}

typedef bool(*BoolCallback)(void *obj, int argc, const char* argv[]);

void *StringTable;

typedef const char *(__thiscall *StringTableInsert_t)(void *this_, const char *str, const bool caseSensitive);
StringTableInsert_t StringTableInsert;

typedef void (__thiscall *AddBoolCommand_t)(void *ns, const char* name, BoolCallback cb, const char *usage, int minArgs, int maxArgs);
AddBoolCommand_t AddBoolCommand;

void ConsoleFunction(const char* nameSpace, const char* name, BoolCallback callBack, const char* usage, int minArgs, int maxArgs)
{
	AddBoolCommand(Con::lookupNamespace(nameSpace), StringTableInsert(StringTable, name, false), callBack, usage, minArgs, maxArgs);
}

typedef void(*LoaderScanProc_t)(sigFind_t sigFindPtr);
typedef void(*LoaderSymbolProc_t)(LoaderSymbolId symbol, void *pointer);
typedef void(*LoaderVoidProc_t)();

bool doDetachModule(string name)
{
	HMODULE module = moduleTable[name];

	if (module == NULL)
		return false;

	Con::printf("Detaching module '%s'", name.c_str());

	LoaderVoidProc_t detachProc = (LoaderVoidProc_t)GetProcAddress(module, "LoaderDetach");
	if (detachProc != NULL)
		detachProc();

	if (FreeLibrary(module))
	{
		moduleTable[name] = NULL;
		return true;
	}

	DWORD dw = GetLastError();
	LPVOID lpMsgBuf;

	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (LPSTR)&lpMsgBuf, 0, NULL);

	Con::printf("   \x03" "Failed to detach: %s", lpMsgBuf);
	LocalFree(lpMsgBuf);

	return false;
}

HMODULE doAttachModule(string name)
{
	doDetachModule(name);

	Con::printf("Attaching module '%s'", name.c_str());

	char filename[MAX_PATH];
	strcpy_s(filename, moduleDir);
	strcat_s(filename, name.c_str());

	HMODULE module = LoadLibraryA(filename);

	if (module == NULL)
	{
		DWORD dw = GetLastError();
		LPVOID lpMsgBuf;

		FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (LPSTR)&lpMsgBuf, 0, NULL);

		Con::printf("   \x03" "Failed to attach: %s", lpMsgBuf);
		LocalFree(lpMsgBuf);
	}
	else
	{
		moduleTable[name] = module;

		LoaderSymbolProc_t symbolProc = (LoaderSymbolProc_t)GetProcAddress(module, "LoaderSymbol");
		if (symbolProc != NULL)
		{
			symbolProc(SYM_CON_PRINTF, Con::printf);
			symbolProc(SYM_SIM_INIT, Sim::init);
		}

		LoaderScanProc_t scanProc = (LoaderScanProc_t)GetProcAddress(module, "LoaderScan");
		if (scanProc != NULL)
			scanProc(sigFind);

		LoaderVoidProc_t attachProc = (LoaderVoidProc_t)GetProcAddress(module, "LoaderAttach");
		if (attachProc != NULL)
			attachProc();
	}

	return module;
}

bool tsAttachModule(void *obj, int argc, const char *argv[])
{
	return doAttachModule(argv[1]) != NULL;
}

bool tsDetachModule(void *obj, int argc, const char *argv[])
{
	return doDetachModule(argv[1]);
}

Detour<Sim::init_t> *detour_Sim__init;

void hook_Sim__init(void)
{
	Con::printf = (Con::printf_t)sigFind("\x8B\x4C\x24\x04\x8D\x44\x24\x08\x50\x6A\x00\x6A\x00\xE8\x00\x00\x00\x00\x83\xC4\x0C\xC3", "xxxxxxxxxxxxxx????xxxx");

	if (Con::printf == NULL)
		Con::printf = (Con::printf_t)printf; // This is a bad idea... probably

	Con::lookupNamespace = (Con::lookupNamespace_t)sigFind("\x8B\x44\x24\x04\x85\xC0\x75\x05", "xxxxxxxx");
	StringTableInsert = (StringTableInsert_t)sigFind("\x53\x8B\x5C\x24\x08\x55\x56\x57\x53", "xxxxxxxxx");
	StringTable = (void *)(*(unsigned int *)(*(unsigned int *)((unsigned int)Con::lookupNamespace + 15)));

	AddBoolCommand = (AddBoolCommand_t)sigFind(
		"\x8B\x44\x24\x04\x56\x50\xE8\x00\x00\x00\x00\x8B\xF0\xA1\x00\x00\x00\x00\x40\xB9\x00\x00\x00\x00\xA3"
		"\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x8B\x4C\x24\x10\x8B\x54\x24\x14\x8B\x44\x24\x18\x89\x4E\x18\x8B"
		"\x4C\x24\x0C\x89\x56\x10\x89\x46\x14\xC7\x46\x0C\x05\x00\x00\x00\x89\x4E\x28\x5E\xC2\x14\x00",
		"xxxxxxx????xxx????xx????x????x????xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");

	// TODO: Inform the user if the above searches fail

	ConsoleFunction(NULL, "attachModule", tsAttachModule, "attachModule(string filename)", 2, 2);
	ConsoleFunction(NULL, "detachModule", tsDetachModule, "detachModule(string filename)", 2, 2);

	bool foundAny = false;

	char moduleSearch[MAX_PATH];
	strcpy_s(moduleSearch, moduleDir);
	strcat_s(moduleSearch, "*.dll");

	WIN32_FIND_DATAA ffd;
	HANDLE hFind = FindFirstFileA(moduleSearch, &ffd);

	if (hFind != INVALID_HANDLE_VALUE) do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		foundAny = true;
		doAttachModule(ffd.cFileName);
	} while (FindNextFileA(hFind, &ffd) != 0);

	if (!foundAny)
	{
		Con::printf("BlocklandLoader found no DLLs in the 'modules' directory");
		Con::printf("--------------------------------------------------------");
	}

	Con::printf("");

	return detour_Sim__init->GetOriginalFunction()();
}

bool __stdcall DllMain(void *, unsigned int reason, void *)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		MODULEINFO info;
		GetModuleInformation(GetCurrentProcess(), GetModuleHandle(NULL), &info, sizeof MODULEINFO);

		imageBase = (unsigned long)info.lpBaseOfDll;
		imageSize = info.SizeOfImage;

		Sim::init = (Sim::init_t)sigFind("\x56\x33\xF6\x57\x89\x35", "xxxxxx");

		if (Sim::init)
			detour_Sim__init = new Detour<Sim::init_t>(Sim::init, hook_Sim__init);
		else
			return false;
	}

	return true;
}

LOADER_API loader() {}