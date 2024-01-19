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
#include <frontend.h>

static struct vxt_pirepheral *disk_create(vxt_allocator *alloc, void *frontend, const char *args) {
    (void)args;
    
    struct frontend_interface *fi = (struct frontend_interface*)frontend;
    if (!fi) return NULL;
    
    struct vxt_pirepheral *p = vxtu_disk_create(alloc, &fi->disk.di);
    if (!p) return NULL;

    if (fi->set_disk_controller) {
		struct frontend_disk_controller c = {
			.device = p,
			.mount = &vxtu_disk_mount,
			.unmount = &vxtu_disk_unmount,
			.set_boot = &vxtu_disk_set_boot_drive
		};
		fi->set_disk_controller(&c);
    }

    vxtu_disk_set_activity_callback(p, fi->disk.activity_callback, fi->disk.userdata);
    return p;
}

VXTU_MODULE_ENTRIES(&disk_create)
