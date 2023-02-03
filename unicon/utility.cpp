#include "headers.hpp"

#include "utility.hpp"

[[nodiscard]]
inline bool ends_with(const std::wstring_view Str, const std::wstring_view Suffix) noexcept
{
	return Str.size() >= Suffix.size() && Str.substr(Str.size() - Suffix.size()) == Suffix;
}

bool is_conhost(const wchar_t* ProcessName)
{
	return ends_with(ProcessName, L"\\conhost.exe");
}

bool is_windows_11_or_greater()
{
	OSVERSIONINFOEXW osvi
	{
		sizeof(osvi),
		HIBYTE(_WIN32_WINNT_WIN10),
		LOBYTE(_WIN32_WINNT_WIN10),
		22000
	};

	const auto ConditionMask =
		VerSetConditionMask(
			VerSetConditionMask(
				VerSetConditionMask(
					0,
					VER_MAJORVERSION,
					VER_GREATER_EQUAL
				),
				VER_MINORVERSION,
				VER_GREATER_EQUAL
			),
			VER_BUILDNUMBER,
			VER_GREATER_EQUAL
		);

	return VerifyVersionInfoW(
		&osvi,
		VER_MAJORVERSION |
		VER_MINORVERSION |
		VER_BUILDNUMBER,
		ConditionMask
	) != FALSE;
}
