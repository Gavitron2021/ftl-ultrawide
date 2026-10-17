// ResourceControl has its own startup/preload renderer, before Lua/main-menu
// hooks are active. Center its logo and fill together and use the same canvas
// and framebuffer presentation as the normal wide renderer.
static void installLoadingScreen(HANDLE process, uintptr_t base, int width) {
    // Selected Shiny loading PNGs are 256x128. Both visible alpha envelopes
    // center on (103,56), so the texture canvas needs (+25,+8) compensation.
    // Outline and fill use the same origin; native text-bar Y nudges are removed.
    patch<int32_t>(process,base,0x1a1cd9,640,width/2+25);
    patch<int32_t>(process,base,0x1a1f68,640,width/2+25);
    patch<int32_t>(process,base,0x1a1d06,337,368);
    float oldLeft=537.f,newLeft=static_cast<float>(width/2-103);
    uint32_t oldBits=0,newBits=0,widthBits=0;
    memcpy(&oldBits,&oldLeft,4);memcpy(&newBits,&newLeft,4);
    patch<uint32_t>(process,base,0x1a1e80,oldBits,newBits);
    patch<uint32_t>(process,base,0x1a21f4,1280,width);
    const float logicalWidth=static_cast<float>(width);
    memcpy(&widthBits,&logicalWidth,4);
    patch<uint32_t>(process,base,0x1a2230,0x44a00000,widthBits);
    patch<uint32_t>(process,base,0x1a2324,0x44a00000,widthBits);
    std::array<float,5> ratios{logicalWidth/720.f,720.f/logicalWidth,0.f,150.f,28.f};
    auto remote=reinterpret_cast<uintptr_t>(VirtualAllocEx(process,nullptr,sizeof(ratios),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if (!remote) fail("Cannot allocate loading-screen aspect constants");
    SIZE_T written=0;
    if (!WriteProcessMemory(process,reinterpret_cast<void*>(remote),ratios.data(),sizeof(ratios),&written) || written!=sizeof(ratios))
        fail("Cannot initialize loading-screen aspect constants");
    for (auto item:std::array<std::array<uint32_t,3>,7>{{
        {0x1a21ae,0x833ff4,0},{0x1a2354,0x833ff4,0},{0x1a21bc,0x833ffc,4},
        {0x1a1dae,0x833fe0,8},{0x1a1e39,0x833fe4,8},
        {0x1a1e6d,0x833fe8,12},{0x1a1eba,0x833fec,16}}}) {
        auto existing=readMemory<uint32_t>(process,base+item[0]);
        auto delta=static_cast<uint32_t>(base-0x400000);
        bool pending=existing==item[1] && delta;
        if (existing!=item[1] && existing!=item[1]+delta) fail("Unexpected loading-screen aspect operand");
        writeMemory<uint32_t>(process,base+item[0],static_cast<uint32_t>(remote+item[2]-(pending?delta:0)));
    }
    logFile << "Startup loading logo and progress fill centered on wide canvas\n";
}
