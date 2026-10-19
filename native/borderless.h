// Use SIL's windowed backend with a desktop-sized popup. Merely disabling
// fullscreen auto-minimize leaves both SIL and CApp in their fullscreen paths.
static void installBorderlessDesktop(HANDLE process, uintptr_t base) {
    // CApp::SetupWindow still chooses the desktop dimensions, but requests
    // graphics_set_display_attr("window", 1) and records windowed state.
    patch<uint32_t>(process,base,0x27ff,0,1);
    patch<uint32_t>(process,base,0x2832,1,0);

    // SIL's window_resizable attribute normally computes WS_OVERLAPPEDWINDOW.
    // Keep its existing window lifecycle and DC/context ownership; only replace
    // the computed style with WS_POPUP | WS_MINIMIZEBOX for this child.
    // This avoids all fullscreen display-mode and minimize/restore transitions.
    patch<uint32_t>(process,base,0x279781,0xfffb0000,0);
    patch<uint32_t>(process,base,0x279787,0x00cf0000,0x80020000);


    // FTL normally adds a one-pixel fullscreen fringe. Borderless uses the
    // desktop rectangle exactly; viewport scaling and mouse conversion agree.
    const std::array<uint8_t,7> fringe{0x83,0x83,0xe8,0x34,0,0,0x01};
    if (readMemory<std::array<uint8_t,7>>(process,base+0x27f4)!=fringe)
        fail("Unexpected initial fullscreen fringe");
    writeMemory(process,base+0x27f4,std::array<uint8_t,7>{0x90,0x90,0x90,0x90,0x90,0x90,0x90});
    patch<uint16_t>(process,base,0x2e81,0xc701,0x9090);
    logFile << "Borderless desktop: windowed backend and focus handling, desktop popup, no display-mode switching\n";
}
