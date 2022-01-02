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

DeviceUBoot::DeviceUBoot()
{
}

DeviceUBoot::~DeviceUBoot()
{
    Close();
}

bool DeviceUBoot::Open(const char *name)
{
    return false;
}

void DeviceUBoot::Close()
{
}

bool DeviceUBoot::Read(void * data, uint64_t offs, uint64_t len)
{
    uint8_t blkbuf[m_blk->blksz];
    const uint64_t blk_start = offs / m_blk->blksz;
    const uint64_t blk_end = (offs + len + 1) / m_blk->blksz;
    uint64_t nblk = blk_end - blk_start;
    uint64_t nbyte = len;
    uint8_t *p = (uint8_t *)data;
    uint64_t blkidx = blk_start;

    if (offs % m_blk->blksz != 0) {
        const uint64_t start_blk_off = offs % m_blk->blksz;
        const uint64_t start_blk_nbyte = m_blk->blksz - start_blk_off;
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
    return 0;
}
