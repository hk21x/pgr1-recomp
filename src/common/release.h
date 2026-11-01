// The release an executable is built for. Each disc's default.xbe is another compile of the game, every address elsewhere;
// CMakeLists.txt sets PGR1_RELEASE from assets/default.xbe's SHA-1: 0 Europe 1.1 (the addresses the comments cite), 1
// Europe and Australia 1.0, 2 USA. The hooks give a function's address for each, PGR1_FN(11, 10, usa) the function to
// replace and PGR1_ORIG(...) its recompiled body; PGR1_VA(...) a code address as a value; pgr1::Va(1.1's address) a
// data address. The others are 1.1's ported by sdk/tools/symbols/xbe_port.py.
#pragma once
#include <cstdint>

#ifndef PGR1_RELEASE
#define PGR1_RELEASE 0
#endif

#define PGR1_CAT_(a, b) a##b
#define PGR1_CAT(a, b) PGR1_CAT_(a, b)
#if PGR1_RELEASE == 2
#define PGR1_PICK(e11, e10, usa) usa
#elif PGR1_RELEASE == 1
#define PGR1_PICK(e11, e10, usa) e10
#else
#define PGR1_PICK(e11, e10, usa) e11
#endif
#define PGR1_FN(e11, e10, usa) PGR1_CAT(sub_, PGR1_PICK(e11, e10, usa))
#define PGR1_ORIG(e11, e10, usa) PGR1_CAT(__imp__sub_, PGR1_PICK(e11, e10, usa))
#define PGR1_VA(e11, e10, usa) PGR1_PICK(e11, e10, usa)

namespace pgr1 {
// a data address of Europe 1.1 in this release: .rdata on, and the D3D section's data, each move by one amount
constexpr uint32_t Va(uint32_t e11) {
#if PGR1_RELEASE == 2
    // USA's data blocks moved by different amounts; each range holds the addresses the hooks and achievements use
    return e11 < 0x150000   ? e11 - 0x32E0
           : e11 < 0x15C000 ? e11 - 0x29B8
           : e11 < 0x170000 ? e11 - 0x2800
           : e11 < 0x1A0000 ? e11 - 0x3A08
           : e11 < 0x2A0000 ? e11 - 0x3A90
           : e11 < 0x2E8000 ? e11 - 0x3B80
           : e11 < 0x2EB000 ? e11 - 0x3BA0
                            : e11 - 0x4820;
#elif PGR1_RELEASE == 1
    return e11 - 0xE0;
#else
    return e11;
#endif
}
}  // namespace pgr1
