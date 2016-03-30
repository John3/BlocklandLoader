#pragma once

#define LOADER_API extern "C" void __declspec(dllexport) __cdecl

enum LoaderSymbolId {
	SYM_CON_PRINTF,
	SYM_SIM_INIT
};

typedef void *(*sigFind_t)(const char *pattern, const char *mask);

typedef void (*LoaderScanProc_t)(sigFind_t sigFindPtr);
typedef void (*LoaderSymbolProc_t)(LoaderSymbolId symbol, void *pointer);
typedef void (*LoaderVoidProc_t)();

typedef void (*Con__printf_t)(const char *format, ...);
typedef void (*Sim__init_t)(void);