// CreditScreen::OnRender constructs a 23-byte resource name in native code.
// Redirect only this call site to the full panorama and move its origin left
// by the UI padding. The render function uses the texture's actual dimensions.
static void installCreditsBackground(HANDLE process, uintptr_t base, int width) {
    constexpr char oldName[] = "stars/bg_darknebula.png";
    constexpr char newName[] = "ultrawide/credits_1.png";
    static_assert(sizeof(oldName)==sizeof(newName),"Credits name length must stay native");
    const std::array<uint32_t,5> operands{0x159806,0x15980d,0x159814,0x15981b,0x159822};
    for (size_t i=0;i<operands.size();++i) {
        uint32_t expected=0,replacement=0;
        memcpy(&expected,oldName+i*4,4);memcpy(&replacement,newName+i*4,4);
        patch<uint32_t>(process,base,operands[i],expected,replacement);
    }
    patch<int32_t>(process,base,0x1598d5,0,-(width-1280)/2);
    logFile << "Credits background uses full panorama; native text/scroll retained\n";
}
