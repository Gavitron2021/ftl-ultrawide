// Move the existing native animation along the painted walkway. Coordinates
// describe its center in the 1720x720 panorama; simulation and sprite data stay
// unchanged. Both ends of the loop are fully outside the visible canvas.
static void installHangarPath(HANDLE process, uintptr_t base, int width) {
    const float padding=(width-1280)/2.f;
    const std::array<std::array<float,2>,7> points{{
        {1574,-40},{1574,135},{1510,135},{1486,160},
        {1486,417},{1575,510},{static_cast<float>(width+40),510}
    }};
    // progress, owner pointer, frame delta, total, followed by segment records:
    // begin/end distance, native animation X/Y, unit direction X/Y.
    std::vector<float> values{0,0,0,0};
    float distance=0;
    for(size_t i=1;i<points.size();++i) {
        float dx=points[i][0]-points[i-1][0],dy=points[i][1]-points[i-1][1];
        float length=std::sqrt(dx*dx+dy*dy);
        values.insert(values.end(),{distance,distance+length,
            points[i-1][0]-padding-17.5f,points[i-1][1]-17.5f,dx/length,dy/length});
        distance+=length;
    }
    values[3]=distance;
    auto data=reinterpret_cast<uintptr_t>(VirtualAllocEx(process,nullptr,values.size()*4,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if(!data)fail("Cannot allocate hangar route");
    for(size_t i=0;i<values.size();++i)writeMemory(process,data+i*4,values[i]);
    RemoteCode code(process);
    code.emit({0x60,0x39,0x1d});code.u32(data+4);code.branch(0x84,"owner-ready");
    code.emit({0x89,0x1d});code.u32(data+4);
    code.emit({0xc7,0x05});code.u32(data);code.u32(0);
    code.label("owner-ready");
    code.emit({0xb9});code.u32(base+0x4cb600);code.call(base+0x19a720);
    code.emit({0xd9,0x1d});code.u32(data+8); // fstp frame delta
    auto scalar=[&](std::initializer_list<uint8_t> instruction,uintptr_t address) {
        code.emit(instruction);code.u32(static_cast<uint32_t>(address));
    };
    scalar({0xf3,0x0f,0x10,0x05},data);
    scalar({0xf3,0x0f,0x58,0x05},data+8);
    code.label("wrap-check");scalar({0x0f,0x2e,0x05},data+12);code.branch(0x82,"route-ready");
    scalar({0xf3,0x0f,0x5c,0x05},data+12);code.jump("wrap-check");
    code.label("route-ready");scalar({0xf3,0x0f,0x11,0x05},data);
    for(size_t i=0;i<points.size()-1;++i) {
        scalar({0x0f,0x2e,0x05},data+(4+i*6+1)*4);
        code.branch(0x82,"segment-"+std::to_string(i));
    }
    code.jump("segment-0");
    for(size_t i=0;i<points.size()-1;++i) {
        auto segment=data+(4+i*6)*4;
        code.label("segment-"+std::to_string(i));
        scalar({0xf3,0x0f,0x10,0x0d},segment+8); // xmm1=start x
        code.emit({0x0f,0x28,0xd0}); // xmm2=progress
        scalar({0xf3,0x0f,0x5c,0x15},segment);
        scalar({0xf3,0x0f,0x59,0x15},segment+16);
        code.emit({0xf3,0x0f,0x58,0xca}); // x += along*dx
        scalar({0xf3,0x0f,0x10,0x1d},segment+12); // xmm3=start y
        code.emit({0x0f,0x28,0xd0});
        scalar({0xf3,0x0f,0x5c,0x15},segment);
        scalar({0xf3,0x0f,0x59,0x15},segment+20);
        code.emit({0xf3,0x0f,0x58,0xda});
        code.jump("position-ready");
    }
    code.label("position-ready");
    code.emit({0xf3,0x0f,0x11,0x8b,0x24,0x12,0,0,0xf3,0x0f,0x11,0x9b,0x28,0x12,0,0,0x61});
    code.emit({0xf3,0x0f,0x10,0x8b,0x24,0x12,0,0,0xf3,0x0f,0x10,0x83,0x28,0x12,0,0});
    code.tail(base+0xdf989);code.finish();
    if(readMemory<uint8_t>(process,base+0xdf90b)!=0xb9)fail("Unexpected native worker update");
    auto original=readMemory<uint32_t>(process,base+0xdf90c);
    if(original!=0x8cb600 && original!=base+0x4cb600)fail("Unexpected worker clock operand");
    suppressRelocation(process,base,0xdf90c);
    writeMemory<uint8_t>(process,base+0xdf90b,0xe9);
    writeMemory<uint32_t>(process,base+0xdf90c,static_cast<uint32_t>(code.address-(base+0xdf910)));
    logFile << "Hangar worker follows six painted walkway segments, length=" << distance
            << " route-state=0x" << std::hex << data << std::dec << '\n';
}
