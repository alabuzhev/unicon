#include "headers.hpp"

namespace replacement
{
	static BOOL WINAPI PolyTextOutW(HDC const Dc, POLYTEXTW const* const PolyText, int const Count)
	{
		for (int i = 0; i != Count; ++i)
		{
			const auto& Text = PolyText[i];
			if (!ExtTextOutW(Dc, Text.x, Text.y, Text.uiFlags, &Text.rcl, Text.lpstr, Text.n, Text.pdx))
				return FALSE;
		}

		return TRUE;
	}
}

static void* FindByName(const char* Name)
{
	if (!lstrcmpA(Name, "PolyTextOutW"))
		return reinterpret_cast<void*>(replacement::PolyTextOutW);

	return {};
}

template <class type>
auto FromRva(HMODULE const Module, DWORD const Rva)
{
	return reinterpret_cast<type>(PBYTE(Module) + Rva);
}

static bool write_memory(void const** const To, const void* const From)
{
	MEMORY_BASIC_INFORMATION Info;
	if (!VirtualQuery(To, &Info, sizeof(Info)))
		return false;

	DWORD Protection;

	switch (Info.Protect)
	{
	case PAGE_READWRITE:
	case PAGE_EXECUTE_READWRITE:
		*To = From;
		return true;

	case PAGE_READONLY:
		Protection = PAGE_READWRITE;
		break;

	default:
		Protection = PAGE_EXECUTE_READWRITE;
		break;
	}

	if (!VirtualProtect(Info.BaseAddress, Info.RegionSize, Protection, &Protection))
		return false;

	*To = From;

	return VirtualProtect(Info.BaseAddress, Info.RegionSize, Info.Protect, &Protection);
}

bool patch(HMODULE const Module)
{
	const auto Headers = reinterpret_cast<PIMAGE_NT_HEADERS>(PBYTE(Module) + PIMAGE_DOS_HEADER(Module)->e_lfanew);
	if (Headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size <= 0)
		return false;

	bool AnyPatched{};

	const auto Imports = Headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	if (Imports.VirtualAddress && Imports.Size)
	{
		for (auto ImportIterator = FromRva<PIMAGE_IMPORT_DESCRIPTOR>(Module, Imports.VirtualAddress); ImportIterator->OriginalFirstThunk; ++ImportIterator)
		{
			const auto FirstUnbound = FromRva<IMAGE_THUNK_DATA const*>(Module, ImportIterator->OriginalFirstThunk);
			const auto FirstBound = FromRva<IMAGE_THUNK_DATA*>(Module, ImportIterator->FirstThunk);

			for (size_t i = 0; FirstBound[i].u1.Function; ++i)
			{
				if (IMAGE_SNAP_BY_ORDINAL(FirstUnbound[i].u1.Ordinal))
					continue;

				const auto ImageImportByName = FromRva<IMAGE_IMPORT_BY_NAME const*>(Module, DWORD(UINT_PTR(FirstUnbound[i].u1.AddressOfData)));
				const auto FunctionName = reinterpret_cast<const char*>(ImageImportByName->Name);
				const auto Function = FindByName(FunctionName);

				if (Function && write_memory(reinterpret_cast<void const**>(FirstBound + i), Function))
					AnyPatched = true;
			}
		}
	}

	const auto DelayedImports = Headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT];
	if (DelayedImports.VirtualAddress && DelayedImports.Size)
	{
		for (auto ImportIterator = FromRva<PIMAGE_DELAYLOAD_DESCRIPTOR>(Module, DelayedImports.VirtualAddress); ImportIterator->DllNameRVA; ++ImportIterator)
		{
			const auto FirstUnbound = FromRva<IMAGE_THUNK_DATA const*>(Module, ImportIterator->ImportNameTableRVA);
			const auto FirstBound = FromRva<IMAGE_THUNK_DATA*>(Module, ImportIterator->ImportAddressTableRVA);

			for (size_t i = 0; FirstBound[i].u1.Function; ++i)
			{
				if (IMAGE_SNAP_BY_ORDINAL(FirstUnbound[i].u1.Ordinal))
					continue;

				const auto ImageImportByName = FromRva<IMAGE_IMPORT_BY_NAME const*>(Module, DWORD(UINT_PTR(FirstUnbound[i].u1.AddressOfData)));
				const auto FunctionName = reinterpret_cast<const char*>(ImageImportByName->Name);
				const auto Function = FindByName(FunctionName);

				if (Function && write_memory(reinterpret_cast<void const**>(FirstBound + i), Function))
					AnyPatched = true;
			}
		}
	}

	return AnyPatched;
}
