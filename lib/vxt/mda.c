// Copyright (c) 2019-2024 Andreas T Jonsson <mail@andreasjonsson.se>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software in
//    a product, an acknowledgment (see the following) in the product
//    documentation is required.
//
//    This product make use of the VirtualXT software emulator.
//    Visit https://virtualxt.org for more information.
//
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.

#include <vxt/vxtu.h>

struct mda_video {
    vxt_byte mem[0x1000];
    bool dirty_cell[0x800];
    bool is_dirty;

    bool cursor_visible;
    int cursor_offset;

    vxt_byte refresh;
    vxt_byte mode_ctrl_reg;
    vxt_byte crt_addr;
    vxt_byte crt_reg[0x100];
};

static vxt_byte read(struct mda_video *m, vxt_pointer addr) {
    return m->mem[(addr - 0xB0000) & 0xFFF];
}

static void write(struct mda_video *m, vxt_pointer addr, vxt_byte data) {
    addr = (addr - 0xB0000) & 0xFFF;
    m->mem[addr] = data;
    m->dirty_cell[addr / 2] = true;
}

static vxt_byte in(struct mda_video *m, vxt_word port) {
    if (port == 0x3BA) {
		m->refresh ^= 0x9;
		return m->refresh;
    } else if (port & 1) { // 0x3B1, 0x3B3, 0x3B5, 0x3B7
		return m->crt_reg[m->crt_addr];
	}
	return 0;
}

static void out(struct mda_video *m, vxt_word port, vxt_byte data) {
    m->is_dirty = true;
    if (port == 0x3B8) {
        m->mode_ctrl_reg = data;
        return;
    } else if (port & 1) { // 0x3B1, 0x3B3, 0x3B5, 0x3B7
		m->crt_reg[m->crt_addr] = data;

        #define DIRTY m->dirty_cell[m->cursor_offset & 0x7FF] = true
		switch (m->crt_addr) {
		case 0xA:
			m->cursor_visible = (data & 0x20) != 0;
            DIRTY;
            break;
		case 0xE:
			DIRTY;
			m->cursor_offset = (m->cursor_offset & 0x00FF) | ((vxt_word)data << 8);
            DIRTY;
            break;
		case 0xF:
			DIRTY;
			m->cursor_offset = (m->cursor_offset & 0xFF00) | (vxt_word)data;
            DIRTY;
            break;
		}
        #undef DIRTY

        return;
    }
	m->crt_addr = data;
}

static vxt_error install(struct mda_video *m, vxt_system *s) {
    struct vxt_pirepheral *p = VXT_GET_PIREPHERAL(m);
    vxt_system_install_mem(s, p, 0xB0000, 0xB7FFF);
    vxt_system_install_io(s, p, 0x3B0, 0x3BF);
    return VXT_NO_ERROR;
}

static vxt_error reset(struct mda_video *m) {
    m->cursor_visible = true;
    m->cursor_offset = 0;
    m->is_dirty = true;

    for (int i = 0; i < 0x800; i++)
        m->dirty_cell[i] = true;

    return VXT_NO_ERROR;
}

static const char *name(struct mda_video *m) {
    (void)m;
    return "MDA Compatible Video Adapter";
}

static enum vxt_pclass pclass(struct mda_video *m) {
    (void)m; return VXT_PCLASS_VIDEO;
}

VXT_API struct vxt_pirepheral *vxtu_mda_create(vxt_allocator *alloc) VXT_PIREPHERAL_CREATE(alloc, mda_video, {
    vxtu_randomize(DEVICE->mem, sizeof(DEVICE->mem), (intptr_t)PIREPHERAL);

    PIREPHERAL->install = &install;
    PIREPHERAL->name = &name;
    PIREPHERAL->pclass = &pclass;
    PIREPHERAL->reset = &reset;
    PIREPHERAL->io.read = &read;
    PIREPHERAL->io.write = &write;
    PIREPHERAL->io.in = &in;
    PIREPHERAL->io.out = &out;
})

VXT_API void vxtu_mda_invalidate(struct vxt_pirepheral *p) { 
    (VXT_GET_DEVICE(mda_video, p))->is_dirty = true;
}

VXT_API int vxtu_mda_traverse(struct vxt_pirepheral *p, int (*f)(int,vxt_byte,enum vxtu_mda_attrib,int,void*), void *userdata) {
    struct mda_video *m = VXT_GET_DEVICE(mda_video, p);
    int cursor = m->cursor_visible ? (m->cursor_offset & 0x7FF) : -1;

    for (int i = 0; i < 0x800; i++) {
        if (m->is_dirty || m->dirty_cell[i]) {
            vxt_byte c = m->mem[i*2];
            vxt_byte a = m->mem[i*2+1];

            enum vxtu_mda_attrib attrib = 0;
            if ((a & 7) == 1)
                attrib |= VXTU_MDA_UNDELINE;
            if (a & 8)
                attrib |= VXTU_MDA_HIGH_INTENSITY;
            if ((a & 0x80) && (m->mode_ctrl_reg & 0x20))
                attrib |= VXTU_MDA_BLINK;

            switch (a) {
                case 0x0:
                case 0x8:
                case 0x80:
                case 0x88:
                    attrib = 0;
                    c = ' ';
                    break;
                case 0x70:
                case 0x78:
                    attrib |= VXTU_MDA_INVERSE;
                    break;
                case 0xF0:
                case 0xF8:
                    attrib |= VXTU_MDA_INVERSE;
                    if (m->mode_ctrl_reg & 0x20)
                        attrib |= VXTU_MDA_BLINK;
                    break;
            }

            int err = f(i, c, attrib, cursor, userdata);
            if (err != 0)
                return err;

            m->dirty_cell[i] = false;
        }
    }

    m->is_dirty = false;
    return 0;
}
