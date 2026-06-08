#include <vstdlib/IKeyValuesSystem.h>
#include <map>
#include <string>
#include <cstdlib>
#include <cstring>

class CMinimalKeyValuesSystem : public IKeyValuesSystem
{
  public:
	void *AllocKeyValuesMemory(int size) override
	{
		return std::malloc(size);
	}

	void FreeKeyValuesMemory(void *pMem) override
	{
		std::free(pMem);
	}

	HKeySymbol GetSymbolForString(const char *name, bool bCreate) override
	{
		auto it = m_SymbolMap.find(name);
		if (it != m_SymbolMap.end())
			return it->second;

		if (!bCreate)
			return INVALID_KEY_SYMBOL;

		HKeySymbol sym = m_NextSymbol++;
		m_SymbolMap[name]       = sym;
		m_StringMap[sym]        = name;
		return sym;
	}

	const char *GetStringForSymbol(HKeySymbol symbol) override
	{
		auto it = m_StringMap.find(symbol);
		return (it != m_StringMap.end()) ? it->second.c_str() : "";
	}

	void RegisterSizeofKeyValues(int) override {}
	void AddKeyValuesToMemoryLeakList(void *, HKeySymbol) override {}
	void RemoveKeyValuesFromMemoryLeakList(void *) override {}
	void InvalidateCache() override {}
	void InvalidateCacheForFile(const char *, const char *) override {}
	void SetKeyValuesExpressionSymbol(const char *, bool) override {}
	bool GetKeyValuesExpressionSymbol(const char *) override { return false; }
	HKeySymbol GetSymbolForStringCaseSensitive(HKeySymbol &, const char *, bool) override
	{
		return INVALID_KEY_SYMBOL;
	}

	bool LoadFileKeyValuesFromCache(KeyValues *, const char *, const char *,
	                                IBaseFileSystem *) const override
	{
		return false;
	}
	void AddFileKeyValuesToCache(const KeyValues *, const char *, const char *) override {}

  private:
	std::map<std::string, HKeySymbol> m_SymbolMap;
	std::map<HKeySymbol, std::string> m_StringMap;
	HKeySymbol m_NextSymbol = 1;
};

static CMinimalKeyValuesSystem g_KeyValuesSystem;

IKeyValuesSystem *KeyValuesSystem()
{
	return &g_KeyValuesSystem;
}
