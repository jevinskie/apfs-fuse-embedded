/*
This file is part of apfs-fuse, a read-only implementation of APFS
(Apple File System) for FUSE.
Copyright (C) 2017 Simon Gander
Copyright (C) 2021 Jevin Sweval

Apfs-fuse is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Apfs-fuse is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with apfs-fuse.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "DeviceUBoot.h"

#include <cassert>
#include <cstring>
#include <cstdio>

#include <string_view>

#if defined(HAS_UBOOT_STUBS) || defined(__UBOOT__)

DeviceUBoot::DeviceUBoot()
{
}

DeviceUBoot::~DeviceUBoot()
{
    Close();
}

bool DeviceUBoot::Open(const char *name)
{
    const char *colon = strchr(name, ':');
    if (!colon) {
        printf("DeviceUBoot::Open(\"%s\") no colon, erroring.\n", name);
        return false;
    }
    const auto if_name_sv = std::string_view{name, static_cast<std::size_t>(colon - name)};
    const auto dev_num_cstr = colon + 1;
    const int dev_num = strtol(dev_num_cstr, nullptr, 10);
    if (if_name_sv == "nvme") {
        printf("DeviceUBoot::Open(\"%s\") trying nvme : %d\n", name, dev_num);
        assert(!blk_get_device((int)intf_type::IF_TYPE_NVME, dev_num, &m_dev));
    } else if (if_name_sv == "host") {
        printf("DeviceUBoot::Open(\"%s\") trying host : %d\n", name, dev_num);
        assert(!blk_get_device((int)intf_type::IF_TYPE_HOST, dev_num, &m_dev));
    } else {
        printf("DeviceUBoot::Open(\"%s\") unsupported if_name: \"%.*s\".\n",
                name, (int)if_name_sv.size(), if_name_sv.data());
    }
    m_blk = (struct blk_desc *)dev_get_uclass_plat(m_dev);
    assert(m_blk);
    return true;
}

void DeviceUBoot::Close()
{
}

bool DeviceUBoot::Read(void * data, uint64_t offs, uint64_t len)
{
    printf("Read: offs: %p len: %p\n", (void*)offs, (void*)len);
    uint8_t blkbuf[m_blk->blksz];
    const uint64_t blk_start = offs / m_blk->blksz;
    const uint64_t blk_end = (offs + len - 1) / m_blk->blksz;
    uint64_t nblk = blk_end - blk_start + 1;
    uint64_t nbyte = len;
    uint8_t *p = (uint8_t *)data;
    uint64_t blkidx = blk_start;

    printf("blk_start: %p blk_end: %p nblk: %p nbyte: %p\n", (void*)blk_start, (void*)(void*)blk_end, (void*)nblk, (void*)nbyte);


    if (offs % m_blk->blksz != 0) {
        const uint64_t start_blk_off = offs % m_blk->blksz;
        const uint64_t start_blk_nbyte = std::min(nbyte, m_blk->blksz - start_blk_off);
        printf("start_blk_off: %p start_blk_nbyte: %p\n", (void*)start_blk_off, (void*)start_blk_nbyte);
        if (m_blk->block_read(m_blk, blkidx, 1, blkbuf) != 1) {
            return false;
        }
        nblk -= 1;
        blkidx += 1;
        memcpy(p, blkbuf + start_blk_off, start_blk_nbyte);
        nbyte -= start_blk_nbyte;
        p += start_blk_nbyte;
    }

    const uint64_t ncontigblk = nbyte / m_blk->blksz;
    const uint64_t ncontigbyte = ncontigblk * m_blk->blksz;
    printf("nbyte: %p ncontigblk: %p ncontigbyte: %p\n", (void*)nbyte, (void*)ncontigblk, (void*)ncontigbyte);
    if (m_blk->block_read(m_blk, blkidx, ncontigblk, p) != ncontigblk) {
        return false;
    }
    nblk -= ncontigblk;
    blkidx += ncontigblk;
    nbyte -= ncontigbyte;
    p += ncontigbyte;

    if (nbyte) {
        assert(nbyte < m_blk->blksz);
        if (m_blk->block_read(m_blk, blkidx, 1, blkbuf) != 1) {
            return false;
        }
        nblk -= 1;
        blkidx += 1;
        memcpy(p, blkbuf, nbyte);
        nbyte -= nbyte;
        p += nbyte;
    }

    assert(nblk == 0);
    assert(nbyte == 0);

    return true;
}

uint64_t DeviceUBoot::GetSize() const
{
    return m_blk->lba * m_blk->blksz;
}

#endif
