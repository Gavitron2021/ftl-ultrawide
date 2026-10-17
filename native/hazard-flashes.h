// Shared solar-flare/pulsar overlay in the verified FTL 1.6.9 renderer.
// The stock 500x500 texture is translated (-360,-640) then scaled (4,4).
// Stretch its existing horizontal screen-space mapping by width/1280. The
// native tint, GetFlashOpacity(), texture, timer, and draw order are retained.
static void installHazardFlashes(HANDLE process, uintptr_t base, int width) {
    const std::array<uint8_t,7> originalTranslate{0xc7,0x04,0x24,0x00,0x00,0xb4,0xc3};
    const std::array<uint8_t,7> originalScale{0xc7,0x04,0x24,0x00,0x00,0x80,0x40};
    if (readMemory<std::array<uint8_t,7>>(process,base+0x120f83)!=originalTranslate ||
        readMemory<std::array<uint8_t,7>>(process,base+0x120f9f)!=originalScale)
        fail("Unexpected solar/pulsar flash transform");
    const float ratio = static_cast<float>(width)/1280.f;
    float x = -360.f*ratio, scaleX = 4.f*ratio;
    uint32_t xBits=0, scaleBits=0;
    memcpy(&xBits,&x,4); memcpy(&scaleBits,&scaleX,4);
    patch<uint32_t>(process,base,0x120f86,0xc3b40000,xBits);
    patch<uint32_t>(process,base,0x120fa2,0x40800000,scaleBits);
    logFile << "Solar/pulsar flash widened; native opacity and timing retained\n";
}
