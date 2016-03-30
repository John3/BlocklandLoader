#include "BlocklandLoader.hpp"

#define LOADER_FUNC(name) name##_t name;

namespace Con
{
	LOADER_FUNC(printf);
	LOADER_FUNC(lookupNamespace);
}

namespace Sim
{
	LOADER_FUNC(init);
}

#ifndef BLOCKLAND_LOADER_INTERNAL
#define SYM_CASE(id, name) case SYM_##id: name = (name##_t)symbol; break;
LOADER_API LoaderSymbol(LoaderSymbolId symbol, void *pointer)
{
	switch (symbol)
	{
		SYM_CASE(CON_PRINTF, Con::printf);
		SYM_CASE(CON_LOOKUPNAMESPACE, Con::lookupNamespace);
		SYM_CASE(SIM_INIT, Sim::init);
	}
}
#endif