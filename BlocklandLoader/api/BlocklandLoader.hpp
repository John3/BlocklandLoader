#pragma once

#define LOADER_API extern "C" void __declspec(dllexport) __cdecl

enum LoaderSymbolId {
	SYM_CON_PRINTF,
	SYM_CON_LOOKUPNAMESPACE,
	SYM_SIM_INIT
};

typedef void *(*sigFind_t)(const char *pattern, const char *mask);

#define LOADER_FUNC_DECL(type, convention, name, ...) \
	typedef type (convention*name##_t)(__VA_ARGS__); \
	extern name##_t name;

namespace Con
{
	LOADER_FUNC_DECL(void, , printf, const char *format, ...);
	LOADER_FUNC_DECL(void *, , lookupNamespace, const char *nsName);
}

namespace Sim
{
	LOADER_FUNC_DECL(void, , init, void);
}

typedef const char *(*StringCallback)(void *obj, int argc, const char *argv[]);
typedef int (*IntCallback)(void *obj, int argc, const char *argv[]);
typedef float (*FloatCallback)(void *obj, int argc, const char *argv[]);
typedef void (*VoidCallback)(void *obj, int argc, const char *argv[]);
typedef bool (*BoolCallback)(void *obj, int argc, const char *argv[]);

void ConsoleFunction(const char *nsName, const char *name, StringCallback callBack, const char* usage, int minArgs, int maxArgs);
void ConsoleFunction(const char *nsName, const char *name,    IntCallback callBack, const char* usage, int minArgs, int maxArgs);
void ConsoleFunction(const char *nsName, const char *name,  FloatCallback callBack, const char* usage, int minArgs, int maxArgs);
void ConsoleFunction(const char *nsName, const char *name,   VoidCallback callBack, const char* usage, int minArgs, int maxArgs);
void ConsoleFunction(const char *nsName, const char *name,   BoolCallback callBack, const char* usage, int minArgs, int maxArgs);

#ifndef BLOCKLAND_LOADER_INTERNAL
LOADER_API LoaderSymbol(LoaderSymbolId symbol, void *pointer);
#endif