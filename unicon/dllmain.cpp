#include "headers.hpp"

#include "patch.hpp"
#include "utility.hpp"

bool patch_if_needed()
{
	const auto ProcessModule = GetModuleHandle({});

	wchar_t Name[MAX_PATH];
	if (!GetModuleFileName(ProcessModule, Name, ARRAYSIZE(Name)))
		throw std::runtime_error("GetModuleFileName");

	if (!is_conhost(Name))
		return true;

	const auto LegacyConhost = GetModuleHandle(L"ConhostV1.dll");
	return patch(LegacyConhost? LegacyConhost : ProcessModule);
}

BOOL APIENTRY DllMain(HMODULE const Module, DWORD const Reason, LPVOID const Reserved)
{
	switch (Reason)
	{
	case DLL_PROCESS_ATTACH:
		if (!patch_if_needed())
			return FALSE;
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

