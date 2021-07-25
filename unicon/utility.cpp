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
