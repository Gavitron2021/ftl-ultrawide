#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincrypt.h>
#include <shellapi.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
static std::ofstream logFile;
static void fail(const std::string& message) { throw std::runtime_error(message); }

static std::string hashFile(const fs::path& path) {
    HCRYPTPROV provider = 0;
    HCRYPTHASH hash = 0;
    if (!CryptAcquireContextW(&provider, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) ||
        !CryptCreateHash(provider, CALG_SHA_256, 0, 0, &hash)) fail("Cannot initialize SHA-256");
    std::ifstream file(path, std::ios::binary);
    if (!file) fail("Cannot read the game file");
    std::array<char, 65536> buffer{};
    while (file.read(buffer.data(), buffer.size()) || file.gcount()) {
        if (!CryptHashData(hash, reinterpret_cast<BYTE*>(buffer.data()), static_cast<DWORD>(file.gcount()), 0)) fail("SHA-256 failed");
    }
    std::array<BYTE, 32> digest{};
    DWORD length = static_cast<DWORD>(digest.size());
    if (!CryptGetHashParam(hash, HP_HASHVAL, digest.data(), &length, 0)) fail("Cannot finish SHA-256");
    CryptDestroyHash(hash);
    CryptReleaseContext(provider, 0);
    std::ostringstream result;
    for (auto byte : digest) result << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned>(byte);
    return result.str();
}

template<typename T> static T readMemory(HANDLE process, uintptr_t address) {
    T value{};
    SIZE_T count = 0;
    if (!ReadProcessMemory(process, reinterpret_cast<void*>(address), &value, sizeof(value), &count) || count != sizeof(value)) fail("Cannot read child game memory");
    return value;
}

template<typename T> static void writeMemory(HANDLE process, uintptr_t address, const T& value) {
    DWORD previous = 0;
    if (!VirtualProtectEx(process, reinterpret_cast<void*>(address), sizeof(value), PAGE_EXECUTE_READWRITE, &previous)) fail("Cannot make display patch writable");
    SIZE_T count = 0;
    BOOL ok = WriteProcessMemory(process, reinterpret_cast<void*>(address), &value, sizeof(value), &count);
    DWORD ignored = 0;
    VirtualProtectEx(process, reinterpret_cast<void*>(address), sizeof(value), previous, &ignored);
    if (!ok || count != sizeof(value)) fail("Cannot write display patch");
}

template<typename T> static void patch(HANDLE process, uintptr_t base, uint32_t rva, T expected, T replacement) {
    if (readMemory<T>(process, base + rva) != expected) {
        std::ostringstream message;
        message << "Unexpected display instruction at RVA 0x" << std::hex << rva << "; no game was started";
        fail(message.str());
    }
    writeMemory<T>(process, base + rva, replacement);
    logFile << "patch rva=0x" << std::hex << rva << " bytes=" << std::dec << sizeof(T) << '\n';
}

static int configuredMode(const fs::path& path) {
    std::ifstream settings(path);
    std::string line;
    while (std::getline(settings, line)) if (line.rfind("fullscreen=", 0) == 0) return std::stoi(line.substr(11));
    return 1;
}

static void setMode(const fs::path& path, int mode) {
    std::ifstream settings(path);
    if (!settings) fail("settings.ini is missing; restore the mod installation first");
    std::string text, line;
    bool found = false;
    while (std::getline(settings, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.rfind("fullscreen=", 0) == 0) { line = "fullscreen=" + std::to_string(mode); found = true; }
        text += line + "\n";
    }
    if (!found) text = "fullscreen=" + std::to_string(mode) + "\n" + text;
    settings.close();
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output << text;
    if (!output) fail("Cannot update the display setting");
}

#include "hyperspace-ui.h"
static void prepareHyperspace(const fs::path& directory, int width, uintptr_t tooltipBounds) {
    const auto baseline = directory / L"Hyperspace.classic.dll";
    if (hashFile(baseline) != "e61145d7b7c18a33c738305bece9445b09d4f86d925a36b7cceeecdaf0cf4697")
        fail("Hyperspace baseline does not match the verified renderer/logger fixes");
    std::ifstream source(baseline, std::ios::binary);
    std::vector<char> bytes((std::istreambuf_iterator<char>(source)), std::istreambuf_iterator<char>());
    // Hyperspace 1.23.2 SystemPositionManager::subSystemOffset. Its static UI
    // layout uses the same value for the subsystem boxes, frame and hit regions.
    constexpr size_t subsystemOffset = 0xaa1d24;
    uint32_t expected = 0;
    memcpy(&expected, bytes.data() + subsystemOffset, 4);
    if (expected != 1015) fail("Unexpected Hyperspace subsystem anchor");
    uint32_t replacement = 1015 + width - 1280;
    memcpy(bytes.data() + subsystemOffset, &replacement, 4);
    // MoreInfoButton.cpp initializes and positions the same Button twice.
    // Moving both shared origins preserves rendering, hover and click behavior.
    for (size_t offset : {size_t(0x5fce4a),size_t(0x5fce8d)}) {
        uint32_t anchor = 0; memcpy(&anchor,bytes.data()+offset,4);
        if (anchor!=1207) fail("Unexpected Hyperspace information button anchor");
        anchor += width-1280; memcpy(bytes.data()+offset,&anchor,4);
    }
    patchHyperspaceUi(bytes,width,tooltipBounds);
    std::ofstream output(directory / L"Hyperspace.dll", std::ios::binary | std::ios::trunc);
    output.write(bytes.data(), bytes.size());
    if (!output) fail("Cannot prepare Hyperspace; close any previous FTL test instance");
}

#include "ui-layout.h"
#include "hangar-path.h"
#include "hazard-flashes.h"
#include "credits-background.h"
#include "loading-screen.h"

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    PROCESS_INFORMATION child{};
    try {
        wchar_t ownPath[MAX_PATH]{};
        if (!GetModuleFileNameW(nullptr, ownPath, MAX_PATH)) fail("Cannot locate launcher");
        fs::path directory = fs::path(ownPath).parent_path();
        const bool steamEntry = _wcsicmp(fs::path(ownPath).filename().c_str(), L"FTLGame.exe") == 0;
        if (!fs::exists(directory / L"FTLGame.exe") && !fs::exists(directory / L"FTLGame-1.6.9.exe")) directory /= L"runtime";
        logFile.open(directory / L"ultrawide-launch.log", std::ios::trunc);
        auto game = directory / L"FTLGame-1.6.9.exe";
        if (!fs::exists(game)) {
            if (steamEntry) fail("FTLGame-1.6.9.exe is missing. Restore the mod installation before launching from Steam.");
            game = directory / L"FTLGame.exe";
        }
        logFile << "game=" << game.filename().string() << " Steam entry=" << steamEntry << '\n';
        constexpr auto expectedHash = "76df0aeddada8be9cabefd917964c9603c1bb5d4c08943362a463050996a746a";
        auto actualHash = hashFile(game);
        logFile << "FTL SHA256=" << actualHash << '\n';
        if (actualHash != expectedHash) fail("FTL Ultrawide requires the verified Steam 1.6.9 executable. Restore the mod installation before launching.");
        if (!fs::exists(directory / L"Hyperspace.dll")) fail("Hyperspace.dll is missing");

        int argc = 0;
        auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        int mode = configuredMode(directory / L"settings.ini");
        bool trace = false;
        for (int i = 1; i < argc; ++i) {
            if (std::wstring(argv[i]) == L"--wide") { mode = 4; setMode(directory / L"settings.ini", mode); }
            else if (std::wstring(argv[i]) == L"--classic") { mode = 1; setMode(directory / L"settings.ini", mode); }
            else if (std::wstring(argv[i]) == L"--trace") trace = true;
            else fail("Supported launcher arguments: --wide, --classic, --trace");
        }
        LocalFree(argv);
        if (mode == 2 || mode == 3) { mode = 1; setMode(directory / L"settings.ini", mode); }
        DEVMODEW display{};
        display.dmSize = sizeof(display);
        if (!EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &display)) fail("Cannot read primary display size");
        const bool wide = mode == 4;
        int width = wide ? static_cast<int>(720.0 * display.dmPelsWidth / display.dmPelsHeight + 0.5) : 1280;
        if (width < 1280 || width > 4096) fail("Unsupported desktop aspect ratio for FTL Ultrawide");
        // The approved scenery and hangar route cover precisely 1720x720.
        // Resolution may scale uniformly; different aspect ratios need more art
        // and layout validation before they can be offered as supported modes.
        if (wide && width != 1720)
            fail("This beta's Modded mode requires the 3440x1440 aspect ratio (43:18). Higher resolutions at that same ratio can scale, but are untested. Use --classic for other aspect ratios.");
        logFile << "mode=" << mode << " desktop=" << display.dmPelsWidth << 'x' << display.dmPelsHeight
                << " logical=" << width << "x720\n";

        // Create only our own verified game process, suspended before its entry
        // point. Patches affect this child process; the executable on disk stays
        // identical to the community rollback target.
        std::ofstream runtimeConfig(directory / L"ultrawide-runtime.cfg", std::ios::trunc);
        runtimeConfig << "enabled=" << (wide ? 1 : 0) << "\nwidth=" << width << "\n";
        runtimeConfig.close();
        std::wstring command = L"\"" + game.wstring() + L"\" --force-opengl";
        STARTUPINFOW startup{};
        startup.cb = sizeof(startup);
        if (!CreateProcessW(game.c_str(), command.data(), nullptr, nullptr, FALSE, CREATE_SUSPENDED | (trace ? DEBUG_ONLY_THIS_PROCESS : 0),
                            nullptr, directory.c_str(), &startup, &child)) fail("Cannot start FTL");
        CONTEXT context{};
        context.ContextFlags = CONTEXT_INTEGER;
        if (!GetThreadContext(child.hThread, &context)) fail("Cannot locate the suspended game image");
        const uintptr_t base = readMemory<uint32_t>(child.hProcess, context.Ebx + 8);
        if (readMemory<uint16_t>(child.hProcess, base) != 0x5a4d) fail("Invalid child image base");
        logFile << "image base=0x" << std::hex << base << std::dec << '\n';
        auto tooltipBounds=reinterpret_cast<uintptr_t>(VirtualAllocEx(child.hProcess,nullptr,8,
            MEM_COMMIT | MEM_RESERVE,PAGE_READWRITE));
        if (!tooltipBounds) fail("Cannot allocate tooltip viewport bounds");
        writeMemory<float>(child.hProcess,tooltipBounds,static_cast<float>(width-20));
        writeMemory<float>(child.hProcess,tooltipBounds+4,-static_cast<float>(width-40));
        // The child is still suspended before DLL initialization. Build its
        // active DLL with guarded operands referencing this child's allocation.
        prepareHyperspace(directory,width,tooltipBounds);

        // Add Modded to the verified Windowed / Classic display cycle.
        patch<uint8_t>(child.hProcess, base, 0x18144f, 3, 4);
        installDisplayChoices(child.hProcess, base, mode);
        if (wide) {
            // Enable framebuffer scaling for the new enum value 4.
            patch<uint8_t>(child.hProcess, base, 0x2f65, 1, 4);
            patch<uint32_t>(child.hProcess, base, 0x30d4, 1280, width); // viewport
            patch<uint32_t>(child.hProcess, base, 0x316c, 1280, width); // clip rectangle
            patch<uint32_t>(child.hProcess, base, 0x36ab, 1280, width); // framebuffer allocation
            // CommandGui::LinkShip passes this UI origin to CombatControl::OnInit.
            // That initializer positions the enemy frame, render target and
            // native mouse regions together; ship simulation coordinates stay
            // unchanged.
            patch<uint32_t>(child.hProcess, base, 0x11d46a, 747, 747 + width - 1280);
            // Pause artwork is cached when the language/UI is initialized.
            patch<uint32_t>(child.hProcess, base, 0x123594, 640, width / 2);
            float logicalWidth = static_cast<float>(width);
            uint32_t widthBits = 0;
            memcpy(&widthBits, &logicalWidth, sizeof(widthBits));
            patch<uint32_t>(child.hProcess, base, 0x3110, 0x44a00000, widthBits); // projection
            patch<uint32_t>(child.hProcess, base, 0x3328, 0x44a00000, widthBits); // framebuffer presentation

            // Private constants keep ship/projectile/simulation constants intact.
            // The operands already have PE relocations. Account for whether the
            // Windows loader has applied those relocations before this snapshot.
            std::array<float, 3> values{logicalWidth / 720.0f, 720.0f / logicalWidth, logicalWidth};
            auto remote = reinterpret_cast<uintptr_t>(VirtualAllocEx(child.hProcess, nullptr, sizeof(values), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
            if (!remote) fail("Cannot allocate private display constants");
            SIZE_T written = 0;
            if (!WriteProcessMemory(child.hProcess, reinterpret_cast<void*>(remote), values.data(), sizeof(values), &written) || written != sizeof(values)) fail("Cannot initialize private display constants");
            for (auto item : std::array<std::array<uint32_t, 3>, 4>{{
                {0x2fd3, 0x82a4e8, 0}, {0x2fdd, 0x82a4e8, 0},
                {0x3044, 0x82a4f0, 4}, {0x3007, 0x82a4f4, 8}}}) {
                uint32_t existing = readMemory<uint32_t>(child.hProcess, base + item[0]);
                const uint32_t delta = static_cast<uint32_t>(base - 0x400000);
                bool pendingRelocation = existing == item[1] && delta != 0;
                if (existing != item[1] && existing != item[1] + delta) fail("Unexpected aspect-ratio operand");
                uint32_t target = static_cast<uint32_t>(remote + item[2] - (pendingRelocation ? delta : 0));
                writeMemory(child.hProcess, base + item[0], target);
            }
            // FTL's filtering heuristic divides by stock width. For an exact
            // integer scale, use its existing height test instead.
            if (display.dmPelsWidth % width == 0 && display.dmPelsHeight % 720 == 0)
                patch<uint8_t>(child.hProcess, base, 0x3310, 0x74, 0xeb);

            installUiLayout(child.hProcess, base, width, tooltipBounds);
            installHangarPath(child.hProcess,base,width);
            installHazardFlashes(child.hProcess,base,width);
            installCreditsBackground(child.hProcess,base,width);
            installLoadingScreen(child.hProcess,base,width);
            // The expanded framebuffer owns the whole clipping rectangle,
            // including the background extensions outside the centered UI.
            const std::array<uint8_t, 6> originalClip{0x8b,0x83,0xec,0x34,0x00,0x00};
            if (readMemory<std::array<uint8_t, 6>>(child.hProcess, base + 0x3174) != originalClip) fail("Unexpected clip origin instruction");
            writeMemory(child.hProcess, base + 0x3174, std::array<uint8_t, 6>{0x31,0xc0,0x90,0x90,0x90,0x90});
        }
        FlushInstructionCache(child.hProcess, nullptr, 0);
        logFile << "display patches validated; resuming PID " << child.dwProcessId << '\n';
        logFile.flush();
        ResumeThread(child.hThread);
        if (trace) {
            std::ofstream diagnostic(directory / L"load-debug.log", std::ios::trunc);
            diagnostic << "game-base=0x" << std::hex << base << '\n';
            bool finished = false;
            while (!finished) {
                DEBUG_EVENT event{};
                if (!WaitForDebugEvent(&event, INFINITE)) fail("Debug event wait failed");
                DWORD disposition = DBG_CONTINUE;
                if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
                    wchar_t name[1024]{};
                    GetFinalPathNameByHandleW(event.u.LoadDll.hFile, name, 1024, FILE_NAME_NORMALIZED);
                    diagnostic << "module=0x" << reinterpret_cast<uintptr_t>(event.u.LoadDll.lpBaseOfDll) << ' ' << fs::path(name).filename().string() << '\n';
                    if (event.u.LoadDll.hFile) CloseHandle(event.u.LoadDll.hFile);
                } else if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
                    if (event.u.CreateProcessInfo.hFile) CloseHandle(event.u.CreateProcessInfo.hFile);
                } else if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
                    const auto& record = event.u.Exception.ExceptionRecord;
                    disposition = record.ExceptionCode == EXCEPTION_BREAKPOINT ? DBG_CONTINUE : DBG_EXCEPTION_NOT_HANDLED;
                    if (record.ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
                        CONTEXT registers{}; registers.ContextFlags = CONTEXT_FULL;
                        HANDLE thread = OpenThread(THREAD_GET_CONTEXT, FALSE, event.dwThreadId);
                        if (thread && GetThreadContext(thread, &registers)) {
                            diagnostic << "fault=0x" << registers.Eip << " accessed=0x" << record.ExceptionInformation[1]
                                << " first=" << event.u.Exception.dwFirstChance << '\n'
                                << "eax=" << registers.Eax << " ebx=" << registers.Ebx << " ecx=" << registers.Ecx << " edx=" << registers.Edx
                                << " esi=" << registers.Esi << " edi=" << registers.Edi << " ebp=" << registers.Ebp << " esp=" << registers.Esp << '\n';
                            for (unsigned offset = 0; offset < 1024; offset += 4) {
                                uint32_t word = 0; SIZE_T count = 0;
                                if (!ReadProcessMemory(child.hProcess, reinterpret_cast<void*>(registers.Esp + offset), &word, 4, &count)) break;
                                diagnostic << offset << ": " << word << '\n';
                            }
                            diagnostic.flush();
                        }
                        if (thread) CloseHandle(thread);
                    }
                } else if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) finished = true;
                ContinueDebugEvent(event.dwProcessId, event.dwThreadId, disposition);
            }
        }
        // Keep Steam's entry process alive for the lifetime of the real game,
        // so Play/Stop and playtime tracking follow the complete session.
        if (steamEntry && !trace) {
            if (WaitForSingleObject(child.hProcess, INFINITE) == WAIT_FAILED) fail("Cannot wait for the game session");
            DWORD exitCode = 0;
            GetExitCodeProcess(child.hProcess, &exitCode);
            logFile << "game exited with code " << exitCode << '\n';
        }
        CloseHandle(child.hThread);
        CloseHandle(child.hProcess);
        return 0;
    } catch (const std::exception& error) {
        if (child.hProcess) { TerminateProcess(child.hProcess, 1); CloseHandle(child.hProcess); }
        if (child.hThread) CloseHandle(child.hThread);
        if (logFile) { logFile << "ERROR: " << error.what() << '\n'; logFile.flush(); }
        MessageBoxA(nullptr, error.what(), "FTL Ultrawide", MB_OK | MB_ICONERROR);
        return 1;
    }
}
