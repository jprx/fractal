#include <fractal.h>
#include <filesys/fileio.h>
#include <oxide/catalog.h>
#include <oxide/logo.h>

// Asset catalog for Oxide window manager
// Every image asset should be stored in the filesystem and loaded by the catalog into a fixed-size buffer
// Better to use fixed-size buffers rather than dynamic allocation for GUI stuff because it lives for the life of the kernel
#ifdef USE_GRAPHICS
u32 bootscreen_mountain[1920*1080];
u32 bootscreen_release[1920*1080];
u32 bootscreen_green[1920*1080];
u32 cursor[64*64];
u32 school_logo[130*59];
u32 lab_logo[130*59];
u32 xbutton_hover[16*16];
u32 xbutton[16*16];
u32 arch_icon_x86[194*194];
u32 arch_icon_arm[194*194];
u32 arch_icon_riscv[194*194];
u32 icon_console[100*100];
u32 icon_info[100*100];
u32 icon_prefs[100*100];
u32 icon_textedit[100*100];
u32 icon_fractal[100*100];
u32 border_topleft[32*32];
u32 border_topright[32*32];
u32 border_bottomleft[32*32];
u32 border_bottomright[32*32];
u32 border_topleft_mask[32*32];
u32 border_topright_mask[32*32];
u32 border_bottomleft_mask[32*32];
u32 border_bottomright_mask[32*32];
u32 logo_graphic[LOGO_WIDTH*LOGO_HEIGHT];
#else // USE_GRPAHICS
u32 bootscreen_mountain[1];
u32 bootscreen_release[1];
u32 bootscreen_green[1];
u32 cursor[1];
u32 school_logo[1];
u32 lab_logo[1];
u32 xbutton_hover[1];
u32 xbutton[1];
u32 arch_icon_x86[1];
u32 arch_icon_arm[1];
u32 arch_icon_riscv[1];
u32 icon_console[1];
u32 icon_info[1];
u32 icon_prefs[1];
u32 icon_textedit[1];
u32 icon_fractal[1];
u32 border_topleft[1];
u32 border_topright[1];
u32 border_bottomleft[1];
u32 border_bottomright[1];
u32 border_topleft_mask[1];
u32 border_topright_mask[1];
u32 border_bottomleft_mask[1];
u32 border_bottomright_mask[1];
u32 logo_graphic[LOGO_WIDTH * LOGO_HEIGHT];
#endif // ! USE_GRAPHICS

extern bool oxide_initialized;

#define LOAD_ASSET(n) load_asset("/assets/" #n ".bin", n, sizeof(n));

void load_asset(char *abspath, void *dest, usize sz) {
    void *h = filesys_open_absolute(abspath);
    if (!h) panic("Asset catalog missing resource %s", abspath);
    if (filesys_filelen(h) < sz) panic("Asset %s too small", abspath);
    if (sz != filesys_read(h, dest, sz, 0)) panic("Error reading asset %s into catalog", abspath);
}

// Load all assets into buffers
// Ensure that the initrd is discovered and initialized before calling this!
void init_catalog() {
#ifdef USE_GRAPHICS
    LOAD_ASSET(bootscreen_release)
    LOAD_ASSET(bootscreen_mountain)
    LOAD_ASSET(bootscreen_green)
    LOAD_ASSET(cursor)
    LOAD_ASSET(school_logo)
    LOAD_ASSET(lab_logo)
    LOAD_ASSET(xbutton)
    LOAD_ASSET(xbutton_hover)
    LOAD_ASSET(arch_icon_x86)
    LOAD_ASSET(arch_icon_arm)
    LOAD_ASSET(arch_icon_riscv)
    LOAD_ASSET(icon_console)
    LOAD_ASSET(icon_info)
    LOAD_ASSET(icon_prefs)
    LOAD_ASSET(icon_textedit)
    LOAD_ASSET(icon_fractal)
    LOAD_ASSET(border_topleft)
    LOAD_ASSET(border_topright)
    LOAD_ASSET(border_bottomleft)
    LOAD_ASSET(border_bottomright)
    LOAD_ASSET(border_topleft_mask)
    LOAD_ASSET(border_topright_mask)
    LOAD_ASSET(border_bottomleft_mask)
    LOAD_ASSET(border_bottomright_mask)
    LOAD_ASSET(logo_graphic)
    oxide_initialized = true;
#endif // USE_GRAPHICS
}
