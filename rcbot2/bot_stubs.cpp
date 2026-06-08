#include <vstdlib/IKeyValuesSystem.h>

// rcbot2 never uses the KeyValues symbol API.
// This stub satisfies the linker for KeyValues.o in libtf2_tier1.a.
IKeyValuesSystem *KeyValuesSystem()
{
	return nullptr;
}
