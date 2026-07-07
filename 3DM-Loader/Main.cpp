// Injector.cpp : Defines the entry point for the console application.
//


#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <tlhelp32.h>
#include <set>
#include <string>
#include <vector>
#include <stdlib.h>
#include <time.h>
#include <locale.h>

#include "D3dxIniUtils.hpp"

#define COLOR_DEFAULT (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define COLOR_MUTED   (FOREGROUND_BLUE | FOREGROUND_GREEN)
#define COLOR_INFO    (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define COLOR_SUCCESS (FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define COLOR_WARNING (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define COLOR_ERROR   (FOREGROUND_RED | FOREGROUND_INTENSITY)

static WORD g_default_console_color = COLOR_DEFAULT;

static void SetConsoleColor(WORD color)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

static int console_printf_utf8(const char* format, ...)
{
	char stack_buffer[4096];
	std::vector<char> heap_buffer;
	char* buffer = stack_buffer;
	int size = sizeof(stack_buffer);

	va_list args;
	va_start(args, format);
	int needed = vsnprintf(buffer, size, format, args);
	va_end(args);

	if (needed < 0)
		return needed;

	if (needed >= size) {
		heap_buffer.resize((size_t)needed + 1);
		buffer = heap_buffer.data();
		va_start(args, format);
		needed = vsnprintf(buffer, (size_t)needed + 1, format, args);
		va_end(args);
		if (needed < 0)
			return needed;
	}

	HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD mode = 0;
	if (out != INVALID_HANDLE_VALUE && GetConsoleMode(out, &mode)) {
		int wide_len = MultiByteToWideChar(CP_UTF8, 0, buffer, needed, NULL, 0);
		if (wide_len > 0) {
			std::wstring wide((size_t)wide_len, L'\0');
			MultiByteToWideChar(CP_UTF8, 0, buffer, needed, &wide[0], wide_len);
			DWORD written = 0;
			WriteConsoleW(out, wide.c_str(), (DWORD)wide.size(), &written, NULL);
			return needed;
		}
	}

	fwrite(buffer, 1, needed, stdout);
	return needed;
}

#define printf console_printf_utf8

static void ResetConsoleColor()
{
	SetConsoleColor(g_default_console_color);
}

static void init_console_ui()
{
	setlocale(LC_ALL, ".UTF-8");
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
	SetConsoleTitleW(L"3DMigoto Loader");

	CONSOLE_SCREEN_BUFFER_INFO info;
	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info))
		g_default_console_color = info.wAttributes;
}

static void print_status(const char* label, const char* message, WORD color)
{
	SetConsoleColor(color);
	printf("[%s]", label);
	ResetConsoleColor();
	printf(" %s\n", message);
}

static void print_section(const char* title)
{
	printf("\n");
	SetConsoleColor(COLOR_INFO);
	printf("== %s ==\n", title);
	ResetConsoleColor();
}

static int utf8_display_width(const std::string& text)
{
	int width = 0;
	for (size_t i = 0; i < text.size();) {
		unsigned char c = (unsigned char)text[i];
		if (c < 0x80) {
			width++;
			i++;
		}
		else {
			width += 2;
			do { i++; } while (i < text.size() && (((unsigned char)text[i] & 0xC0) == 0x80));
		}
	}
	return width;
}

static void print_padded_text(const std::string& text, int width)
{
	int text_width = utf8_display_width(text);
	int padding = width - text_width;
	if (padding < 0)
		padding = 0;

	int left = padding / 2;
	int right = padding - left;
	for (int i = 0; i < left; ++i) printf(" ");
	printf("%s", text.c_str());
	for (int i = 0; i < right; ++i) printf(" ");
}

static void print_banner()
{
	std::vector<std::string> greetings = {
		"祝你天天开心，万事胜意！",
		"愿你眼里的星星，永远闪亮。",
		"今天也要元气满满哦！",
		"代码敲得开心，Bug一扫而空！",
		"愿所有美好，与你不期而遇。",
		"愿你抽卡出金，强化不歪！",
		"游戏启动，烦恼清空！",
		"注入成功，开始享受吧！",
		"编译一次通过，运行没有警告！",
		"Hello World, Hello You!"
	};

	srand((unsigned int)time(NULL));
	const std::string& greeting = greetings[rand() % greetings.size()];
	const int inner_width = 29;

	printf("\n");
	SetConsoleColor(COLOR_ERROR);
	printf("      ******       ******      \n");
	printf("    **********   **********    \n");
	printf("  ************* *************  \n");
	printf(" ***************************** \n");
	printf("*******************************\n");
	printf("*");
	ResetConsoleColor();
	print_padded_text(greeting, inner_width);
	SetConsoleColor(COLOR_ERROR);
	printf("*\n");
	printf("*******************************\n");
	printf(" ***************************** \n");
	printf("  ***************************  \n");
	printf("    ***********************    \n");
	printf("      *******************      \n");
	printf("        ***************        \n");
	printf("          ***********          \n");
	printf("            *******            \n");
	printf("              ***              \n");
	ResetConsoleColor();
	printf("        3DMigoto Loader\n");
	printf("        By 影 & LucyTtk\n\n");
}

static bool is_process_elevated()
{
	HANDLE token = NULL;
	TOKEN_ELEVATION elevation = {};
	DWORD size = sizeof(elevation);

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
		return false;

	bool elevated = GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size) && elevation.TokenIsElevated;
	CloseHandle(token);
	return elevated;
}

static int get_close_delay(D3dxIniUtils& config)
{
	if (config.delay.empty())
		return 5;

	int delay = atoi(config.ToByteString(config.delay).c_str());
	return delay < -1 ? 5 : delay;
}

static void print_loader_summary(D3dxIniUtils& config)
{
	std::string launch = config.launch.empty() ? "未配置" : config.ToByteString(config.launch);

	print_section("配置摘要");
	printf("目标进程   : %s\n", config.ToByteString(config.target).c_str());
	printf("3DM 模块   : %s\n", config.ToByteString(config.module).c_str());
	printf("自动启动   : %s\n", launch.c_str());
	if (!config.launch_args.empty())
		printf("启动参数   : %s\n", config.ToByteString(config.launch_args).c_str());
	printf("额外 DLL   : %zu 个\n", config.extra_dlls.size());
	printf("关闭延迟   : %d 秒\n", get_close_delay(config));
}

static void wait_keypress(std::string msg)
{
	printf("%s\n", msg.c_str());
	getchar();
}

static void wait_exit(int code = 0, std::string msg = "\nPress enter to close...\n")
{
	wait_keypress(msg);
	exit(code);
}

static void exit_usage(const char* msg)
{
	//                                                          80 column limit --------> \n
	printf("The Loader is not configured correctly. Please copy the 3DMigoto d3d11.dll\n"
		"and d3dx.ini into this directory, then edit the d3dx.ini's [Loader] section\n"
		"to set the target executable and 3DMigoto module name.\n"
		"\n"
		"%s", msg);

	wait_exit(EXIT_FAILURE);
}

static bool verify_injection(PROCESSENTRY32* pe, const wchar_t* module, bool log_name)
{
	HANDLE snapshot;
	MODULEENTRY32 me;
	const wchar_t* basename = wcsrchr(module, '\\');
	bool rc = false;
	static std::set<DWORD> pids;
	wchar_t exe_path[MAX_PATH], mod_path[MAX_PATH];

	if (basename)
		basename++;
	else
		basename = module;

	do {
		snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pe->th32ProcessID);
	} while (snapshot == INVALID_HANDLE_VALUE && GetLastError() == ERROR_BAD_LENGTH);

	if (snapshot == INVALID_HANDLE_VALUE) {
		DWORD lastError = GetLastError();

		if (lastError == ERROR_ACCESS_DENIED) {
			if (!pids.count(pe->th32ProcessID)) {
				printf("%d: target process found, but it don't want us to inject, whatever, we don't care. :)\n", pe->th32ProcessID);
				pids.insert(pe->th32ProcessID);
			}
			return true; // 假设注入成功
		}

		printf("%S (%d): Unable to verify if 3DMigoto was successfully loaded: %d\n",
			pe->szExeFile, pe->th32ProcessID, lastError);
		return false;
	}

	me.dwSize = sizeof(MODULEENTRY32);
	if (!Module32First(snapshot, &me)) {
		DWORD lastError = GetLastError();

		// 同样处理Module32First的访问拒绝
		if (lastError == ERROR_ACCESS_DENIED) {
			if (!pids.count(pe->th32ProcessID)) {
				printf("%d: Unable to verify 3DMigoto loading status due to access denied - assuming success :)\n", pe->th32ProcessID);
				pids.insert(pe->th32ProcessID);
			}
			CloseHandle(snapshot);
			return true; // 假设注入成功
		}

		printf("%S (%d): Unable to verify if 3DMigoto was successfully loaded: %d\n",
			pe->szExeFile, pe->th32ProcessID, lastError);
		goto out_close;
	}

	// First module is the executable, and this is how we get the full path:
	if (log_name)
		printf("Target process found (%i): %S\n", pe->th32ProcessID, me.szExePath);
	wcscpy_s(exe_path, MAX_PATH, me.szExePath);

	rc = false;
	while (Module32Next(snapshot, &me)) {
		if (_wcsicmp(me.szModule, basename))
			continue;

		if (!_wcsicmp(me.szExePath, module)) {
			if (!pids.count(pe->th32ProcessID)) {
				printf("%d: 3DMigoto loaded :)\n", pe->th32ProcessID);
				pids.insert(pe->th32ProcessID);
			}
			rc = true;
		}
		else {
			wcscpy_s(mod_path, MAX_PATH, me.szExePath);
			wcsrchr(exe_path, L'\\')[1] = '\0';
			wcsrchr(mod_path, L'\\')[1] = '\0';
			if (!_wcsicmp(exe_path, mod_path)) {
				printf("\n\n\n"
					"WARNING: Found a second copy of 3DMigoto loaded from the game directory:\n"
					"%S\n"
					"This may crash - please remove the copy in the game directory and try again\n\n\n",
					me.szExePath);
				wait_exit(EXIT_FAILURE);
			}
		}
	}

out_close:
	CloseHandle(snapshot);
	return rc;
}

static bool check_for_running_target(wchar_t* target, const wchar_t* module)
{
	// https://docs.microsoft.com/en-us/windows/desktop/ToolHelp/taking-a-snapshot-and-viewing-processes
	HANDLE snapshot;
	PROCESSENTRY32 pe;
	bool rc = false;
	wchar_t* basename = wcsrchr(target, '\\');
	static std::set<DWORD> pids;

	if (basename)
		basename++;
	else
		basename = target;

	snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE) {
		printf("Unable to verify if 3DMigoto was successfully loaded: %d\n", GetLastError());
		return false;
	}

	pe.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(snapshot, &pe)) {
		printf("Unable to verify if 3DMigoto was successfully loaded: %d\n", GetLastError());
		goto out_close;
	}

	do {
		if (_wcsicmp(pe.szExeFile, basename))
			continue;

		rc = verify_injection(&pe, module, !pids.count(pe.th32ProcessID)) || rc;
		pids.insert(pe.th32ProcessID);
	} while (Process32Next(snapshot, &pe));

out_close:
	CloseHandle(snapshot);
	return rc;
}

// 前向声明
static DWORD find_target_pid(const wchar_t* target);
static bool inject_dll_into_process(DWORD pid, const wchar_t* dll_path);

static void wait_for_target(const char* target_a, const wchar_t* module_path, bool wait, int delay, bool launched)
{
	wchar_t target_w[MAX_PATH];

	if (!MultiByteToWideChar(CP_UTF8, 0, target_a, -1, target_w, MAX_PATH))
		return;

	for (int seconds = 0; wait || delay == -1; seconds++) {
		if (check_for_running_target(target_w, module_path) && delay != -1)
			break;
		Sleep(1000);

		if (launched && seconds == 3) {
			printf("\n仍在等待游戏启动...\n"
				"如果游戏没有自动启动，请保持本窗口打开并手动运行游戏。\n"
				"您也可以在 d3dx.ini 中调整或移除 [Loader] launch= 选项。\n\n");
		}
	}

	for (int i = delay; i > 0; i--) {
		printf("注入器将在 %i 秒后关闭...\r", i);
		Sleep(1000);
		check_for_running_target(target_w, module_path);
	}
	printf("\n");
}

// 等待目标进程出现并返回其 PID（不做倒计时，仅等待发现进程）
static DWORD wait_for_target_pid(const char* target_a, bool launched)
{
	wchar_t target_w[MAX_PATH];
	if (!MultiByteToWideChar(CP_UTF8, 0, target_a, -1, target_w, MAX_PATH))
		return 0;

	const wchar_t* basename = wcsrchr(target_w, L'\\');
	if (basename)
		basename++;
	else
		basename = target_w;

	for (int seconds = 0; ; seconds++) {
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (snapshot != INVALID_HANDLE_VALUE) {
			PROCESSENTRY32 pe;
			pe.dwSize = sizeof(PROCESSENTRY32);
			if (Process32First(snapshot, &pe)) {
				do {
					if (!_wcsicmp(pe.szExeFile, basename)) {
						DWORD pid = pe.th32ProcessID;
						CloseHandle(snapshot);
						return pid;
					}
				} while (Process32Next(snapshot, &pe));
			}
			CloseHandle(snapshot);
		}

		Sleep(1000);

		if (launched && seconds == 3) {
			printf("\n仍在等待游戏启动...\n"
				"如果游戏没有自动启动，请保持本窗口打开并手动运行游戏。\n\n");
		}
	}

	return 0;
}

wchar_t* deduce_working_directory(wchar_t* setting, wchar_t dir[MAX_PATH])
{
	DWORD ret;
	wchar_t* file_part = NULL;

	ret = GetFullPathName(setting, MAX_PATH, dir, &file_part);
	if (!ret || ret >= MAX_PATH)
		return NULL;

	ret = GetFileAttributes(dir);
	if (ret == INVALID_FILE_ATTRIBUTES)
		return NULL;

	if (!(ret & FILE_ATTRIBUTE_DIRECTORY) && file_part)
		*file_part = '\0';

	printf("Using working directory: \"%S\"\n", dir);

	return dir;
}

static DWORD find_target_pid(const wchar_t* target)
{
	HANDLE snapshot;
	PROCESSENTRY32 pe;
	DWORD pid = 0;
	const wchar_t* basename = wcsrchr(target, L'\\');

	if (basename)
		basename++;
	else
		basename = target;

	printf("[UnlockerIsland] 正在搜索进程: %S\n", basename);

	snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE) {
		printf("[UnlockerIsland] CreateToolhelp32Snapshot 失败: %d\n", GetLastError());
		return 0;
	}

	pe.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(snapshot, &pe)) {
		printf("[UnlockerIsland] Process32First 失败: %d\n", GetLastError());
		CloseHandle(snapshot);
		return 0;
	}

	do {
		if (!_wcsicmp(pe.szExeFile, basename)) {
			pid = pe.th32ProcessID;
			printf("[UnlockerIsland] 找到目标进程: %S (PID: %d)\n", pe.szExeFile, pid);
			break;
		}
	} while (Process32Next(snapshot, &pe));

	if (pid == 0) {
		printf("[UnlockerIsland] 未在进程列表中找到目标进程\n");
	}

	CloseHandle(snapshot);
	return pid;
}

static bool inject_dll_into_process(DWORD pid, const wchar_t* dll_path)
{
	printf("[UnlockerIsland] --- 开始 DLL 注入 ---\n");
	printf("[UnlockerIsland] 目标 PID: %d\n", pid);
	printf("[UnlockerIsland] DLL 路径 (原始值): %S\n", dll_path);

	// 检查文件是否存在
	DWORD fileAttr = GetFileAttributesW(dll_path);
	if (fileAttr == INVALID_FILE_ATTRIBUTES) {
		DWORD err = GetLastError();
		printf("[UnlockerIsland] DLL 文件未找到！GetFileAttributesW 错误: %d\n", err);
		if (err == ERROR_FILE_NOT_FOUND)
			printf("[UnlockerIsland]   -> ERROR_FILE_NOT_FOUND: 请检查文件路径是否正确\n");
		else if (err == ERROR_PATH_NOT_FOUND)
			printf("[UnlockerIsland]   -> ERROR_PATH_NOT_FOUND: 请检查目录是否存在\n");
		return false;
	}
	printf("[UnlockerIsland] DLL 文件存在 (属性: 0x%08X)\n", fileAttr);

	// 获取 DLL 的完整路径
	wchar_t full_path[MAX_PATH];
	DWORD ret = GetFullPathNameW(dll_path, MAX_PATH, full_path, NULL);
	if (!ret || ret >= MAX_PATH) {
		printf("[UnlockerIsland] GetFullPathNameW 失败: ret=%d, error=%d\n", ret, GetLastError());
		return false;
	}
	printf("[UnlockerIsland] DLL 完整路径: %S\n", full_path);

	// 打开目标进程
	HANDLE hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
		PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid);
	if (!hProcess) {
		DWORD err = GetLastError();
		printf("[UnlockerIsland] OpenProcess 失败！错误: %d\n", err);
		if (err == ERROR_ACCESS_DENIED)
			printf("[UnlockerIsland]   -> ERROR_ACCESS_DENIED: 目标进程可能受反作弊保护。\n"
				   "[UnlockerIsland]      请确保注入器以管理员身份运行。\n");
		else if (err == ERROR_INVALID_PARAMETER)
			printf("[UnlockerIsland]   -> ERROR_INVALID_PARAMETER: 进程可能已退出\n");
		return false;
	}
	printf("[UnlockerIsland] OpenProcess 成功 (句柄: 0x%p)\n", hProcess);

	// 分配远程内存
	size_t size = (wcslen(full_path) + 1) * sizeof(wchar_t);
	printf("[UnlockerIsland] 正在远程进程中分配 %zu 字节内存...\n", size);
	LPVOID remoteMem = VirtualAllocEx(hProcess, NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!remoteMem) {
		printf("[UnlockerIsland] VirtualAllocEx 失败: %d\n", GetLastError());
		CloseHandle(hProcess);
		return false;
	}
	printf("[UnlockerIsland] VirtualAllocEx 成功 (远程地址: 0x%p)\n", remoteMem);

	// 写入 DLL 路径
	if (!WriteProcessMemory(hProcess, remoteMem, full_path, size, NULL)) {
		printf("[UnlockerIsland] WriteProcessMemory 失败: %d\n", GetLastError());
		VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return false;
	}
	printf("[UnlockerIsland] WriteProcessMemory 成功\n");

	// 获取 LoadLibraryW 地址
	HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
	if (!hKernel32) {
		printf("[UnlockerIsland] GetModuleHandleW(kernel32.dll) 失败: %d\n", GetLastError());
		VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return false;
	}
	LPTHREAD_START_ROUTINE loadLibrary = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryW");
	if (!loadLibrary) {
		printf("[UnlockerIsland] GetProcAddress(LoadLibraryW) 失败: %d\n", GetLastError());
		VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return false;
	}
	printf("[UnlockerIsland] LoadLibraryW 地址: 0x%p\n", loadLibrary);

	// 创建远程线程
	printf("[UnlockerIsland] 正在创建远程线程...\n");
	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, loadLibrary, remoteMem, 0, NULL);
	if (!hThread) {
		DWORD err = GetLastError();
		printf("[UnlockerIsland] CreateRemoteThread 失败！错误: %d\n", err);
		if (err == ERROR_ACCESS_DENIED)
			printf("[UnlockerIsland]   -> ERROR_ACCESS_DENIED: 反作弊可能阻止了远程线程创建\n");
		VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return false;
	}
	printf("[UnlockerIsland] 远程线程已创建 (句柄: 0x%p)，正在等待...\n", hThread);

	// 等待远程线程完成
	DWORD waitResult = WaitForSingleObject(hThread, 10000);
	if (waitResult == WAIT_TIMEOUT) {
		printf("[UnlockerIsland] 警告: 远程线程在 10 秒内未完成！\n");
		VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
		CloseHandle(hThread);
		CloseHandle(hProcess);
		return false;
	} else if (waitResult == WAIT_FAILED) {
		printf("[UnlockerIsland] WaitForSingleObject 失败: %d\n", GetLastError());
		VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
		CloseHandle(hThread);
		CloseHandle(hProcess);
		return false;
	}

	DWORD exitCode = 0;
	GetExitCodeThread(hThread, &exitCode);
	printf("[UnlockerIsland] 远程线程完成。退出码 (LoadLibraryW 返回值): 0x%08X\n", exitCode);

	VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
	CloseHandle(hThread);
	CloseHandle(hProcess);

	if (exitCode == 0) {
		printf("[UnlockerIsland] LoadLibraryW 返回 NULL - DLL 在目标进程中加载失败！\n");
		printf("[UnlockerIsland]   可能的原因:\n");
		printf("[UnlockerIsland]   - DLL 依赖项缺失\n");
		printf("[UnlockerIsland]   - DLL 架构不匹配 (x86 vs x64)\n");
		printf("[UnlockerIsland]   - DllMain 返回了 FALSE\n");
		printf("[UnlockerIsland]   - 目标进程无法访问该路径\n");
		return false;
	}

	printf("[UnlockerIsland] --- DLL 注入成功 ---\n");
	return true;
}

int main()
{
	init_console_ui();
	wchar_t setting_w[MAX_PATH], working_dir[MAX_PATH], * working_dir_p = NULL;
	wchar_t module_full_path[MAX_PATH];
	int rc = EXIT_FAILURE;
	HMODULE module;
	int hook_proc;
	FARPROC fn;
	HHOOK hook;
	bool launch;

	CreateMutexA(0, FALSE, "Local\\3DMigotoLoader");
	if (GetLastError() == ERROR_ALREADY_EXISTS)
		wait_exit(EXIT_FAILURE, "ERROR: Another instance of the 3DMigoto Loader is already running. Please close it and try again\n");

	print_banner();


	D3dxIniUtils d3dxIniUtils(L"d3dx.ini");
	if (!d3dxIniUtils.parse_error.empty()) {
		std::string error = "配置错误: " + d3dxIniUtils.ToByteString(d3dxIniUtils.parse_error);
		print_status("错误", error.c_str(), COLOR_ERROR);
		wait_exit(EXIT_FAILURE, "\n请检查 d3dx.ini 的 [Loader] 配置后重试。\n按 Enter 关闭...\n");
	}

	print_loader_summary(d3dxIniUtils);
	if (d3dxIniUtils.require_admin && !is_process_elevated()) {
		print_status("警告", "配置要求管理员权限，建议右键以管理员身份运行本程序。", COLOR_WARNING);
	}

	module = LoadLibraryA(d3dxIniUtils.ToByteString(d3dxIniUtils.module).c_str());
	if (!module) {
		std::string error = "无法加载 3DMigoto 模块: " + d3dxIniUtils.ToByteString(d3dxIniUtils.module);
		print_status("错误", error.c_str(), COLOR_ERROR);
		wait_exit(EXIT_FAILURE);
	}

	GetModuleFileName(module, module_full_path, MAX_PATH);
	SetConsoleColor(COLOR_SUCCESS);
	printf("[OK]");
	ResetConsoleColor();
	printf(" 已加载 3DMigoto: %S\n\n", module_full_path);


	fn = GetProcAddress(module, "CBTProc");
	if (!fn) {
		wait_exit(EXIT_FAILURE, "Module does not support injection method\n"
			"Make sure this is a recent 3DMigoto d3d11.dll\n");
	}

	//We don't need to read it ,we just use WH_CBT.
	//hook_proc = find_ini_int_lite(ini_section, "hook_proc", WH_CBT);

	hook_proc = WH_CBT;

	//WH_SHELL is also works good,but since we always use WH_CBT, we will keep use WH_CBT until some thing happens.
	//hook_proc = WH_SHELL;

	hook = SetWindowsHookEx(hook_proc, (HOOKPROC)fn, module, 0);
	if (!hook)
		wait_exit(EXIT_FAILURE, "Error installing hook\n");

	rc = EXIT_SUCCESS;


	launch = d3dxIniUtils.launch != L"";
	if (launch) {
		std::string launch_path = d3dxIniUtils.ToByteString(d3dxIniUtils.launch);
		print_section("启动目标");
		printf("正在启动: %s\n", launch_path.c_str());
		if (!d3dxIniUtils.launch_args.empty())
			printf("启动参数: %s\n", d3dxIniUtils.ToByteString(d3dxIniUtils.launch_args).c_str());

		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

		if (!MultiByteToWideChar(CP_UTF8, 0, launch_path.c_str(), -1, setting_w, MAX_PATH))
			wait_exit(EXIT_FAILURE, "Invalid launch setting\n");

		working_dir_p = deduce_working_directory(setting_w, working_dir);

		HINSTANCE shell_result = ShellExecuteW(NULL, NULL, setting_w,
			d3dxIniUtils.launch_args.empty() ? NULL : d3dxIniUtils.launch_args.c_str(),
			working_dir_p, SW_SHOWNORMAL);
		if ((INT_PTR)shell_result <= 32) {
			print_status("错误", "启动目标失败，请检查 launch 路径和权限。", COLOR_ERROR);
		}
		else {
			print_status("成功", "启动命令已发送，正在等待目标进程。", COLOR_SUCCESS);
		}
	}
	else {
		print_status("就绪", "3DMigoto 已就绪，请启动游戏。", COLOR_SUCCESS);
	}

	// 如果配置了额外的 DLL（unlocker_dll 和/或 extra_dll），等待目标进程出现并逐一注入
	if (!d3dxIniUtils.extra_dlls.empty()) {
		print_section("额外 DLL 注入");
		printf("[DLL注入] 共发现 %zu 个额外 DLL 需要注入\n", d3dxIniUtils.extra_dlls.size());
		printf("[DLL注入] target 配置值: %s\n", d3dxIniUtils.ToByteString(d3dxIniUtils.target).c_str());

		for (size_t i = 0; i < d3dxIniUtils.extra_dlls.size(); i++) {
			printf("[DLL注入]   [%zu] %s\n", i + 1, d3dxIniUtils.ToByteString(d3dxIniUtils.extra_dlls[i]).c_str());
		}

		wchar_t cwd[MAX_PATH];
		if (GetCurrentDirectoryW(MAX_PATH, cwd)) {
			printf("[DLL注入] 当前工作目录: %S\n", cwd);
		}

		printf("[DLL注入] 正在等待目标进程出现...\n");
		DWORD target_pid = wait_for_target_pid(d3dxIniUtils.ToByteString(d3dxIniUtils.target).c_str(), launch);
		if (target_pid != 0) {
			printf("[DLL注入] 找到目标进程 (PID: %d)\n", target_pid);

			int success_count = 0;
			int fail_count = 0;
			for (size_t i = 0; i < d3dxIniUtils.extra_dlls.size(); i++) {
				const std::wstring& dll = d3dxIniUtils.extra_dlls[i];
				printf("\n[DLL注入] --- 正在注入第 %zu/%zu 个 DLL ---\n", i + 1, d3dxIniUtils.extra_dlls.size());
				printf("[DLL注入] DLL: %s\n", d3dxIniUtils.ToByteString(dll).c_str());

				if (inject_dll_into_process(target_pid, dll.c_str())) {
					print_status("成功", "DLL 注入成功。", COLOR_SUCCESS);
					success_count++;
				} else {
					print_status("失败", "DLL 注入失败。", COLOR_ERROR);
					fail_count++;
				}

				// 注入多个 DLL 时，在每次注入之间稍作等待，避免目标进程过载
				if (i + 1 < d3dxIniUtils.extra_dlls.size()) {
					Sleep(500);
				}
			}

			printf("\n[DLL注入] 注入完成: 成功 %d 个, 失败 %d 个\n", success_count, fail_count);
		} else {
			printf("[DLL注入] 未找到目标进程！\n");
		}
		printf("\n");
	}

	// 等待 3DMigoto 注入验证 + 5秒倒计时关闭
	wait_for_target(d3dxIniUtils.ToByteString(d3dxIniUtils.target).c_str(), module_full_path, true, get_close_delay(d3dxIniUtils), launch);

	UnhookWindowsHookEx(hook);

	return rc;
}




