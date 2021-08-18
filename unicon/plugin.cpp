#include "headers.hpp"

#include "plugin.hpp"
#include "inject.hpp"

#define EXPORT_THIS __pragma(comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__))

// {1A795D4B-DC68-4C77-8CE9-F4CAB04B8E99}
static const GUID MainUUID = { 0x1a795d4b, 0xdc68, 0x4c77, { 0x8c, 0xe9, 0xf4, 0xca, 0xb0, 0x4b, 0x8e, 0x99 } };

extern "C" void WINAPI GetGlobalInfoW(GlobalInfo* Info)
{
	EXPORT_THIS

	Info->StructSize = sizeof(GlobalInfo);
	Info->MinFarVersion = MAKEFARVERSION(3, 0, 0, 5000, VS_RELEASE);
	Info->Version = MAKEFARVERSION(1, 0, 0, 0, VS_RELEASE);
	Info->Guid = MainUUID;
	Info->Title = L"unicon";
	Info->Description = L"Conhost Unicode Fixer";
	Info->Author = L"Alex Alabuzhev";
}

extern "C" void WINAPI GetPluginInfoW(PluginInfo* Info)
{
	EXPORT_THIS

	Info->StructSize = sizeof(*Info);
	Info->Flags = PF_PRELOAD;
}

extern "C" void WINAPI SetStartupInfoW(const struct PluginStartupInfo* PSInfo)
{
	EXPORT_THIS

	try
	{
		inject();
	}
	catch(std::exception const& e)
	{
		std::string StrNarrow(e.what());
		// Good enough for now
		std::wstring Str(StrNarrow.begin(), StrNarrow.end());

		const wchar_t* Items[]
		{
			L"Unicon: error",
			Str.data()
		};

		PSInfo->Message(&MainUUID, &MainUUID, FMSG_WARNING | FMSG_MB_OK, {}, Items, std::size(Items), 0);
	}
}
