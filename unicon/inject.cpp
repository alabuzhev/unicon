#include "headers.hpp"

#include "utility.hpp"

template<typename F>
class scope_guard
{
public:
	scope_guard(F f) :
		m_f(std::move(f))
	{

	}

	~scope_guard()
	{
		m_f();
	}

private:
	F m_f;
};

class make_scope_guard
{
public:
	template<typename F>
	[[nodiscard]]
	auto operator<<(F f) { return scope_guard<F>(std::move(f)); }
};

#define DETAIL_CONCATENATE_IMPL(s1, s2) s1 ## s2
#define CONCATENATE(s1, s2) DETAIL_CONCATENATE_IMPL(s1, s2)
#define ANONYMOUS_VARIABLE(str) CONCATENATE(str, __LINE__)

#define SCOPE_EXIT const auto ANONYMOUS_VARIABLE(scope_guard) = make_scope_guard() << [&]()

static unsigned GetProcessConsoleHostProcessId(HANDLE const Process)
{
	const auto NtDll = GetModuleHandle(L"ntdll");
	if (!NtDll)
		throw std::runtime_error("ntdll");

	using NtQueryInformationProcessType = decltype(&NtQueryInformationProcess);
	const auto NtQueryInformationProcessPtr = reinterpret_cast<NtQueryInformationProcessType>(GetProcAddress(NtDll, "NtQueryInformationProcess"));
	if (!NtQueryInformationProcessPtr)
		throw std::runtime_error("NtQueryInformationProcessPtr");

	const auto ProcessConsoleHostProcess = static_cast<PROCESSINFOCLASS>(49);

	ULONG_PTR ConsoleHostProcess;
	const auto Status = NtQueryInformationProcessPtr(Process, ProcessConsoleHostProcess, &ConsoleHostProcess, sizeof(ConsoleHostProcess), {});
	if (!NT_SUCCESS(Status))
		throw std::runtime_error("NtQueryInformationProcess");

	return ConsoleHostProcess & ~3;
}

extern "C" IMAGE_DOS_HEADER __ImageBase;

inline HMODULE GetCurrentModuleHandle()
{
	return reinterpret_cast<HMODULE>(&__ImageBase);
}

__declspec(dllexport) void inject(unsigned Pid)
{
	BOOL IsWow64{};
	if (IsWow64Process(GetCurrentProcess(), &IsWow64) && IsWow64)
		throw std::runtime_error("WOW64 is not supported. Please use the x64 version.");

	if (!Pid)
	{
		Pid = GetProcessConsoleHostProcessId(GetCurrentProcess());
	}

	wchar_t FullDllPath[MAX_PATH];
	if (!GetModuleFileNameEx(GetCurrentProcess(), GetCurrentModuleHandle(), FullDllPath, ARRAYSIZE(FullDllPath)))
		throw std::runtime_error("GetModuleFileNameEx");

	const auto Process = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, {}, Pid);
	if (!Process)
		throw std::runtime_error("OpenProcess");;

	SCOPE_EXIT{ CloseHandle(Process); };

	wchar_t Name[MAX_PATH];
	auto NameSize = static_cast<DWORD>(std::size(Name));
	if (!QueryFullProcessImageName(Process, {}, Name, &NameSize))
		throw std::runtime_error("QueryFullProcessImageName");

	// It if's not conhost, it's either OpenConsole (which shouldn't need this already) or csrss (which doesn't need this yet).
	if (!is_conhost(Name))
		return;

	const auto FullDllPathSize = (wcslen(FullDllPath) + 1) * sizeof(wchar_t);

	const auto DllPathAddress = VirtualAllocEx(Process, {}, FullDllPathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!DllPathAddress)
		throw std::runtime_error("VirtualAllocEx");

	SCOPE_EXIT{ VirtualFreeEx(Process, DllPathAddress, 0, MEM_RELEASE); };

	if (!WriteProcessMemory(Process, DllPathAddress, FullDllPath, FullDllPathSize, {}))
		throw std::runtime_error("WriteProcessMemory");

	const auto LoadLibraryAddress = GetProcAddress(GetModuleHandle(L"kernel32"), "LoadLibraryW");

	const auto RemoteThread = CreateRemoteThread(Process, {}, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(LoadLibraryAddress), DllPathAddress, 0, {});
	if (!RemoteThread)
		throw std::runtime_error("CreateRemoteThread");

	SCOPE_EXIT{ CloseHandle(RemoteThread); };

	WaitForSingleObject(RemoteThread, INFINITE);

	DWORD ExitCode;
	if (!GetExitCodeThread(RemoteThread, &ExitCode))
		throw std::runtime_error("GetExitCodeThread");

	if (!ExitCode)
		throw std::runtime_error("Patch failed");
}
