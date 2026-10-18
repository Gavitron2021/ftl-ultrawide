// The pinned SIL backend already creates a WS_POPUP window without changing
// display modes when the requested size matches the desktop and refresh is 0.
// Keep that path at the exact desktop size and disable fullscreen auto-minimize.
static void installBorderlessDesktop(HANDLE process, uintptr_t base) {
    RemoteCode code(process);
    auto string = [&](const char* text) {
        auto address=code.here();
        do { code.bytes.push_back(static_cast<uint8_t>(*text)); } while (*text++);
        return address;
    };
    const auto minimize=string("fullscreen_minimize_on_focus_loss");
    const auto refresh=string("refresh_rate");
    const auto stub=code.here();
    code.emit({0x60}); // preserve the original display-attribute call's registers
    code.emit({0x6a,0x00,0x68}); code.u32(minimize);
    code.call(base+0x224c50); code.emit({0x83,0xc4,0x08});
    code.emit({0x6a,0x00,0x6a,0x00,0x68}); code.u32(refresh); // vararg double 0
    code.call(base+0x224c50); code.emit({0x83,0xc4,0x0c,0x61});
    code.tail(base+0x224c50); // original window_resizable attribute and return
    code.finish();
    if (readMemory<uint8_t>(process,base+0x2738)!=0xe8 ||
        0x2738+5+readMemory<int32_t>(process,base+0x2739)!=0x224c50)
        fail("Unexpected window setup attribute call");
    writeMemory<uint32_t>(process,base+0x2739,static_cast<uint32_t>(stub-(base+0x2738+5)));

    // FTL normally adds a one-pixel fullscreen fringe. Borderless uses the
    // desktop rectangle exactly; viewport scaling and mouse conversion agree.
    const std::array<uint8_t,7> fringe{0x83,0x83,0xe8,0x34,0,0,0x01};
    if (readMemory<std::array<uint8_t,7>>(process,base+0x27f4)!=fringe)
        fail("Unexpected initial fullscreen fringe");
    writeMemory(process,base+0x27f4,std::array<uint8_t,7>{0x90,0x90,0x90,0x90,0x90,0x90,0x90});
    patch<uint16_t>(process,base,0x2e81,0xc701,0x9090);
    logFile << "Borderless desktop: exact desktop size, default refresh, focus auto-minimize disabled\n";
}
