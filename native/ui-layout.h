// Presentation stubs for the SHA256-verified x86 executable. They wrap UI draw
// calls rather than replacing the function entries used by Hyperspace hooks.
#include "ui-hook-sites.h"
static void suppressRelocation(HANDLE process, uintptr_t base, uint32_t targetRva) {
    // This launcher can run before the Windows loader relocates the image.
    // Replaced instructions must not retain relocations for deleted operands.
    uint32_t nt = readMemory<uint32_t>(process,base+0x3c);
    uint32_t directory = readMemory<uint32_t>(process,base+nt+24+96+5*8);
    uint32_t length = readMemory<uint32_t>(process,base+nt+24+96+5*8+4);
    for (uint32_t at=0; at<length;) {
        uint32_t page=readMemory<uint32_t>(process,base+directory+at);
        uint32_t size=readMemory<uint32_t>(process,base+directory+at+4);
        if (size<8 || at+size>length) fail("Invalid relocation table");
        for(uint32_t offset=8;offset<size;offset+=2) {
            auto address=base+directory+at+offset;
            uint16_t entry=readMemory<uint16_t>(process,address);
            if ((entry>>12)==3 && page+(entry&0xfff)==targetRva) { writeMemory<uint16_t>(process,address,0); return; }
        }
        at+=size;
    }
    fail("Expected relocation not found");
}
struct RemoteCode {
    HANDLE process;
    uintptr_t address;
    std::vector<uint8_t> bytes;
    std::map<std::string, size_t> labels;
    std::vector<std::pair<size_t, std::string>> links;
    explicit RemoteCode(HANDLE handle) : process(handle) {
        address = reinterpret_cast<uintptr_t>(VirtualAllocEx(process, nullptr, 4096,
            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        if (!address) fail("Cannot allocate UI layout code");
    }
    uintptr_t here() const { return address + bytes.size(); }
    void emit(std::initializer_list<uint8_t> data) { bytes.insert(bytes.end(), data); }
    void u32(uint32_t value) { for (int i = 0; i != 4; ++i) bytes.push_back(static_cast<uint8_t>(value >> (i*8))); }
    void call(uintptr_t target) { emit({0xe8}); u32(static_cast<uint32_t>(target - (here() + 4))); }
    void tail(uintptr_t target) { emit({0xe9}); u32(static_cast<uint32_t>(target - (here() + 4))); }
    void label(const std::string& name) { labels[name] = bytes.size(); }
    void jump(const std::string& name) { emit({0xe9}); links.push_back({bytes.size(), name}); u32(0); }
    void branch(uint8_t condition, const std::string& name) { emit({0x0f,condition}); links.push_back({bytes.size(), name}); u32(0); }
    void finish() {
        for (const auto& link : links) {
            auto found = labels.find(link.second);
            if (found == labels.end()) fail("Missing UI layout label");
            uint32_t displacement = static_cast<uint32_t>(found->second - (link.first + 4));
            memcpy(bytes.data() + link.first, &displacement, 4);
        }
        if (bytes.size() > 4096) fail("UI layout code exceeded its allocation");
        SIZE_T written = 0;
        if (!WriteProcessMemory(process, reinterpret_cast<void*>(address), bytes.data(), bytes.size(), &written) || written != bytes.size()) fail("Cannot initialize UI layout code");
        DWORD previous = 0;
        if (!VirtualProtectEx(process, reinterpret_cast<void*>(address), 4096, PAGE_EXECUTE_READ, &previous)) fail("Cannot protect UI layout code");
    }
};

static void installDisplayChoices(HANDLE process, uintptr_t base, int mode) {
    // Keep rendering at the launch mode while the Options screen edits the
    // saved choice. Framebuffer size and UI layout change together on restart.
    auto active = reinterpret_cast<uintptr_t>(VirtualAllocEx(process, nullptr, 4,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (!active) fail("Cannot allocate active display mode");
    writeMemory(process, active, mode);
    for (auto rva : {0x15d5u,0x1611u,0x1667u,0x1686u,0x2a47u,0x2a5fu,
                     0x2b0bu,0x2b65u,0x2c24u,0x2e37u,0x3669u,0x2f61u,0x2e22u,0x2e79u}) {
        uint32_t preferred = (rva == 0x2f61 || rva == 0x2e22 || rva == 0x2e79) ? 0x8c8ca4 : 0x8c8ca0;
        uint32_t current = readMemory<uint32_t>(process, base + rva);
        uint32_t delta = static_cast<uint32_t>(base - 0x400000);
        bool pending = current == preferred && delta;
        if (current != preferred && current != preferred + delta) fail("Unexpected display mode operand");
        writeMemory(process, base + rva, static_cast<uint32_t>(active - (pending ? delta : 0)));
    }
    RemoteCode code(process);
    // Cycle Windowed -> Classic -> Modded -> Windowed. The unscaled/native
    // variants fail the user's display requirements and remain out of the UI.
    code.emit({0x83,0xf8,0x02}); code.branch(0x84,"choose-wide");
    code.emit({0x83,0xf8,0x03}); code.branch(0x85,"save-choice");
    code.label("choose-wide"); code.emit({0xb8}); code.u32(4);
    code.label("save-choice"); code.emit({0xa3}); code.u32(base + 0x4c8ca0); code.emit({0xc3});
    code.finish();
    const std::array<uint8_t,5> expected{0xc3,0x90,0x8d,0xb6,0x00};
    if (readMemory<std::array<uint8_t,5>>(process, base + 0x181458) != expected) fail("Unexpected display-choice epilogue");
    std::array<uint8_t,5> jump{0xe9,0,0,0,0};
    uint32_t relative = static_cast<uint32_t>(code.address - (base + 0x181458 + 5));
    memcpy(jump.data()+1, &relative, 4);
    writeMemory(process, base + 0x181458, jump);

    // The stock dialog only warns when crossing mode 3. Every mode change now
    // requires rebuilding the framebuffer/UI together, so compare the complete
    // pending enum against the immutable launch enum instead.
    auto replace = [&](uint32_t rva, const std::vector<uint8_t>& expected, const std::vector<uint8_t>& replacement) {
        if (expected.size() != replacement.size()) fail("Display replacement size mismatch");
        for (size_t i = 0; i < expected.size(); ++i) {
            if (readMemory<uint8_t>(process, base+rva+i) != expected[i]) fail("Unexpected display warning instruction");
            writeMemory(process, base+rva+i, replacement[i]);
        }
    };
    // Absolute operands below are newly inserted, with no PE relocation entry.
    std::vector<uint8_t> compare(18, 0x90);
    compare[0] = 0x3b; compare[1] = 0x15; // cmp edx,[active]
    uint32_t pointer = static_cast<uint32_t>(active);
    memcpy(compare.data()+2, &pointer, 4);
    // Preserve the old relocated operand bytes when checking the snapshot.
    std::vector<uint8_t> old{0x83,0xfa,0x03,0x0f,0x94,0xc1,0x83,0x3d,0,0,0,0,0x03,0x0f,0x94,0xc0,0x38,0xc1};
    auto lastOperand = readMemory<uint32_t>(process, base+0x17289e);
    memcpy(old.data()+8, &lastOperand, 4);
    suppressRelocation(process,base,0x17289e);
    replace(0x172896, old, compare);

    // The Options screen's existing restart-required text uses the same test.
    // Use the launch enum immediate here so relocation cannot affect it.
    std::vector<uint8_t> displayCompare(22, 0x90);
    displayCompare[0] = 0xa1;
    uint32_t saved = static_cast<uint32_t>(base+0x4c8ca0);
    memcpy(displayCompare.data()+1, &saved, 4);
    displayCompare[5] = 0x83; displayCompare[6] = 0xf8; displayCompare[7] = static_cast<uint8_t>(mode);
    std::vector<uint8_t> previous(22);
    for (size_t i=0;i<previous.size();++i) previous[i]=readMemory<uint8_t>(process,base+0x171c44+i);
    if (previous[0]!=0x83 || previous[1]!=0x3d || previous[6]!=3 || previous[10]!=0x83 || previous[16]!=3)
        fail("Unexpected display restart text comparison");
    suppressRelocation(process,base,0x171c46);
    suppressRelocation(process,base,0x171c50);
    replace(0x171c44, previous, displayCompare);
    RemoteCode closeCode(process);
    for (auto item : std::array<std::array<uint32_t,3>,2>{{{0x170d0c,0x170c4d,0xce},{0x17360d,0x173370,0xcb}}}) {
        auto stub=closeCode.here();
        closeCode.emit({0xa1}); closeCode.u32(saved);
        closeCode.emit({0x83,0xf8,static_cast<uint8_t>(mode)});
        auto label="close-warning-"+std::to_string(item[0]);
        closeCode.branch(0x85,label); closeCode.tail(base+item[0]+5);
        closeCode.label(label); closeCode.emit({0x60,0x89,static_cast<uint8_t>(item[2])}); // pushad; mov ecx,esi/ebx
        closeCode.emit({0x0f,0xb7,0x81,0x58,0x08,0,0,0x50});
        closeCode.emit({0x66,0xc7,0x81,0x58,0x08,0,0,0x01,0x01,0x51,0x6a,0xff,0x6a,0xff});
        closeCode.call(base+0x1726c0); // shared native close-button warning path, ret 8
        closeCode.emit({0x59,0x58,0x66,0x89,0x81,0x58,0x08,0,0,0x61});
        closeCode.tail(base+item[1]);
        if (readMemory<uint8_t>(process,base+item[0])!=0xa1) fail("Unexpected Options close branch");
        suppressRelocation(process,base,item[0]+1);
        writeMemory<uint8_t>(process,base+item[0],0xe9);
        writeMemory<uint32_t>(process,base+item[0]+1,static_cast<uint32_t>(stub-(base+item[0]+5)));
    }
    closeCode.finish();
    logFile << "Display changes deferred until restart; native warning enabled\n";
}

static void installUiLayout(HANDLE process, uintptr_t base, int width, uintptr_t tooltipBounds) {
    const uint32_t padding = (width - 1280) / 2;
    auto state = reinterpret_cast<uintptr_t>(VirtualAllocEx(process, nullptr, 8,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (!state) fail("Cannot allocate UI layout state");
    RemoteCode code(process);
    const auto settingsStub = code.here();
    code.emit({0xf3,0x0f,0x11,0x81,0x04,0x35,0x00,0x00,0x60}); // original store; pushad
    auto setTooltipBounds = [&](int translation) {
        for (auto item:std::array<std::pair<uintptr_t,float>,2>{{
            {tooltipBounds,static_cast<float>(width-translation-20)},
            {tooltipBounds+4,-static_cast<float>(width-translation-40)}}}) {
            uint32_t bits=0; memcpy(&bits,&item.second,4);
            code.emit({0xc7,0x05}); code.u32(item.first); code.u32(bits);
        }
    };
    setTooltipBounds(0);
    code.emit({0xc6,0x05}); code.u32(state); code.emit({0x00});
    code.emit({0x80,0x79,0x10,0x00}); code.branch(0x85,"main-menu");
    code.emit({0x8b,0x41,0x08,0x85,0xc0}); code.branch(0x84,"settings-done");
    code.emit({0x8b,0xb0,0x0c,0x17,0x00,0x00,0x8b,0xb8,0x10,0x17,0x00,0x00});
    code.label("focus-loop");
    code.emit({0x39,0xfe}); code.branch(0x83,"settings-done");
    code.emit({0x8b,0x06,0x83,0xc6,0x04,0x85,0xc0}); code.branch(0x84,"focus-loop");
    code.emit({0x80,0x78,0x04,0x00}); code.branch(0x84,"focus-loop");
    code.emit({0xc6,0x05}); code.u32(state); code.emit({0x01}); code.jump("translated-tooltip");
    code.label("main-menu");
    code.emit({0xc7,0x81,0xec,0x34,0x00,0x00}); code.u32(padding);
    code.label("translated-tooltip"); setTooltipBounds(padding);
    code.label("settings-done"); code.emit({0x61,0xc3});

    // Translate only input while a game focus window is open. World rendering
    // stays anchored; the matching UI draw wrappers below add the same padding.
    const uint32_t inputSites[] = {0x1949,0x1a08,0x1ac8,0x1b98,0x1c68,0x2d95};
    std::map<uint32_t, uintptr_t> inputStubs;
    for (auto site : inputSites) {
        inputStubs[site] = code.here();
        const auto label = "input-done-" + std::to_string(site);
        code.emit({0x2b,0x81,0xec,0x34,0x00,0x00});
        code.emit({0x80,0x3d}); code.u32(state); code.emit({0x00}); code.branch(0x84,label);
        code.emit({0x2d}); code.u32(padding);
        code.label(label); code.tail(base + site + 6);
    }

    std::map<uint32_t, uintptr_t> wrappers;
    for (const auto& site : uiHookSites) {
        if (wrappers.count(site.target)) continue;
        const std::string suffix = std::to_string(site.target);
        wrappers[site.target] = code.here();
        code.emit({0x80,0x3d}); code.u32(state); code.emit({0}); code.branch(0x84,"plain-" + suffix);
        code.emit({0x80,0x3d}); code.u32(state + 4); code.emit({0}); code.branch(0x85,"plain-" + suffix);
        code.emit({0xc6,0x05}); code.u32(state + 4); code.emit({1});
        code.emit({0x51}); // retain this across CSurface calls
        code.call(base + 0x1913d0); // GL_PushMatrix
        float translation = static_cast<float>(padding); uint32_t bits = 0; memcpy(&bits, &translation, 4);
        code.emit({0x6a,0,0x6a,0,0x68}); code.u32(bits);
        code.call(base + 0x192050); // GL_Translate
        code.emit({0x83,0xc4,0x0c,0x59});
        code.call(base + site.target);
        code.call(base + 0x191450); // GL_PopMatrix
        code.emit({0xc6,0x05}); code.u32(state + 4); code.emit({0,0xc3});
        code.label("plain-" + suffix); code.tail(base + site.target);
    }
    // Extend the actual menu tint operations. These execute for Stats, new-game
    // confirmation, Options, and the hangar ship list, including nested states.
    std::map<uint32_t, uintptr_t> tintWrappers;
    for (auto item : std::array<std::pair<uint32_t,float>,2>{{{0xbfe56,.85f},{0xdf157,.75f}}}) {
        tintWrappers[item.first] = code.here();
        code.emit({0x60}); // preserve all native arguments/registers
        auto pushFloat = [&](float value) { uint32_t bits; memcpy(&bits,&value,4); code.emit({0x68});code.u32(bits); };
        for (float x : {-static_cast<float>(padding),1280.f}) {
            pushFloat(item.second);
            code.emit({0x6a,0,0x6a,0,0x6a,0});
            pushFloat(720.f); pushFloat(static_cast<float>(padding));
            code.emit({0x6a,0}); pushFloat(x);
            code.call(base+0x192900); code.emit({0x83,0xc4,0x20});
        }
        code.emit({0x61}); code.tail(base+0x196fe0);
    }
    code.finish();
    auto redirect = [&](uint32_t rva, uintptr_t target, const std::vector<uint8_t>& expected) {
        for (size_t i = 0; i < expected.size(); ++i)
            if (readMemory<uint8_t>(process, base + rva + i) != expected[i]) fail("Unexpected UI layout hook instruction");
        std::vector<uint8_t> replacement(expected.size(), 0x90);
        replacement[0] = 0xe9;
        uint32_t displacement = static_cast<uint32_t>(target - (base + rva + 5));
        memcpy(replacement.data() + 1, &displacement, 4);
        for (size_t i = 0; i < replacement.size(); ++i) writeMemory(process, base + rva + i, replacement[i]);
    };
    redirect(0x3031, settingsStub, {0xf3,0x0f,0x11,0x81,0x04,0x35,0x00,0x00});
    for (auto site : inputSites)
        redirect(site, inputStubs.at(site), {0x2b,0x81,0xec,0x34,0x00,0x00});
    for (const auto& item : tintWrappers) {
        if (readMemory<uint8_t>(process,base+item.first)!=0xe8 ||
            item.first+5+readMemory<int32_t>(process,base+item.first+1)!=0x196fe0) fail("Unexpected menu tint call");
        writeMemory(process,base+item.first+1,static_cast<uint32_t>(item.second-(base+item.first+5)));
    }
    for (const auto& site : uiHookSites) {
        uintptr_t destination = wrappers.at(site.target);
        if (site.vtable) {
            uint32_t current = readMemory<uint32_t>(process, base + site.rva);
            uint32_t preferred = 0x400000 + site.target;
            uint32_t delta = static_cast<uint32_t>(base - 0x400000);
            bool pending = current == preferred && delta;
            if (current != preferred && current != preferred + delta) fail("Unexpected UI vtable entry");
            writeMemory(process, base + site.rva, static_cast<uint32_t>(destination - (pending ? delta : 0)));
        } else {
            if (readMemory<uint8_t>(process, base + site.rva) != 0xe8) fail("Unexpected UI draw call");
            int32_t current = readMemory<int32_t>(process, base + site.rva + 1);
            if (site.rva + 5 + current != site.target) fail("Unexpected UI draw call target");
            writeMemory(process, base + site.rva + 1, static_cast<uint32_t>(destination - (base + site.rva + 5)));
        }
    }
    logFile << "UI draw/input wrappers installed: " << sizeof(uiHookSites)/sizeof(uiHookSites[0]) << " sites\n";
}
