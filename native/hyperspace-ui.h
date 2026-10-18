// Runtime changes for the hash-verified Hyperspace 1.23.2 Windows DLL.
// Bounds point into this child's allocation, not a DLL-relative address.
static void removeDllRelocation(std::vector<char>& bytes, uint32_t target) {
    auto read32 = [&](size_t at) { uint32_t v; memcpy(&v,bytes.data()+at,4); return v; };
    uint32_t nt=read32(0x3c);
    auto rawOffset = [&](uint32_t rva) -> size_t {
        uint16_t count=0, optional=0;
        memcpy(&count,bytes.data()+nt+6,2); memcpy(&optional,bytes.data()+nt+20,2);
        for (size_t at=nt+24+optional; count--; at+=40) {
            uint32_t va=read32(at+12), length=read32(at+16);
            if (rva>=va && rva-va<length) return read32(at+20)+rva-va;
        }
        fail("Hyperspace relocation RVA is outside a file section"); return 0;
    };
    uint32_t rva=read32(nt+24+96+5*8), length=read32(nt+24+96+5*8+4);
    size_t start=rawOffset(rva); bool found=false;
    for (size_t at=0; at<length;) {
        uint32_t page=read32(start+at), size=read32(start+at+4);
        if (size<8 || at+size>length) fail("Invalid Hyperspace relocation block");
        for (size_t off=8; off<size; off+=2) {
            uint16_t entry=0; auto pos=start+at+off; memcpy(&entry,bytes.data()+pos,2);
            if ((entry>>12)==3 && page+(entry&0xfff)==target) {
                if (found) fail("Duplicate Hyperspace relocation");
                entry=0; memcpy(bytes.data()+pos,&entry,2); found=true;
            }
        }
        at+=size;
    }
    if (!found) fail("Expected Hyperspace tooltip relocation missing");
}

static void patchHyperspaceUi(std::vector<char>& bytes, int width, uintptr_t bounds) {
    // CustomReactor.cpp::OnRender: omit only the per-column price draw.
    // This stdcall wrapper pops 16 argument bytes; its paired caller adjustment
    // must also be omitted to keep the stack balanced. Capacity/next price/MAX
    // use separate calls. Apply this presentation preference in every mode.
    constexpr size_t columnPrice = 0x5b6a11;
    const std::array<uint8_t,15> expectedPrice{
        0xc7,0x04,0x24,0x05,0,0,0,0xe8,0x63,0xa0,0xe3,0xff,0x83,0xec,0x10};
    for (size_t i=0; i<expectedPrice.size(); ++i)
        if (static_cast<uint8_t>(bytes.at(columnPrice+i))!=expectedPrice[i])
            fail("Unexpected Hyperspace reactor price draw");
    memset(bytes.data()+columnPrice+7,0x90,8);

    auto replace = [&](size_t at, std::initializer_list<uint8_t> expected,
                       size_t operand, uint32_t value) {
        size_t i=0;
        for (auto byte:expected) if (static_cast<uint8_t>(bytes.at(at+i++))!=byte)
            fail("Unexpected Hyperspace UI instruction");
        memcpy(bytes.data()+at+operand,&value,4);
    };
    // CustomWeapons.cpp: screen-edge clamp, after ship/mount coordinates have
    // been transformed with combatControl.position. Keep the native 7px inset.
    replace(0x45ac7e,{0x81,0xef,0xf9,0x04,0,0},2,width-7);
    replace(0x45accb,{0x3d,0xf9,0x04,0,0},1,width-7);
    if (width==1280) return;
    // RedesignedTooltips.cpp::RenderTooltip: comparison with width-20 and
    // correction toward width-40. Private floats follow the current outer UI
    // translation; mouse position, hover/click paths and vertical rules stay native.
    replace(0x61bef1,{0x0f,0x2e,0x0d,0xdc,0x28,0x9f,0x10},3,static_cast<uint32_t>(bounds));
    replace(0x61bf11,{0xf3,0x0f,0x58,0x0d,0xe4,0x28,0x9f,0x10},4,static_cast<uint32_t>(bounds+4));
    removeDllRelocation(bytes,0x61c8f4);
    removeDllRelocation(bytes,0x61c915);
}
