#pragma once

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define WIN32_NO_STATUS
#include <windows.h>
#include <winternl.h>
#include <psapi.h>
#include <rpc.h>
#include <tlhelp32.h>
