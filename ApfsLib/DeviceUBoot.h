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

#pragma once

#include "Device.h"

enum class if_type {
    IF_TYPE_UNKNOWN = 0,
    IF_TYPE_IDE,
    IF_TYPE_SCSI,
    IF_TYPE_ATAPI,
    IF_TYPE_USB,
    IF_TYPE_DOC,
    IF_TYPE_MMC,
    IF_TYPE_SD,
    IF_TYPE_SATA,
    IF_TYPE_HOST,
    IF_TYPE_NVME,
    IF_TYPE_EFI,
    IF_TYPE_PVBLOCK,
    IF_TYPE_VIRTIO,

    IF_TYPE_COUNT,          /* Number of interface types */
};

extern "C" {

struct udevice;

int blk_get_device(int if_type, int devnum, struct udevice **devp);

struct blk_desc *blk_get_by_device(struct udevice *dev);

} // extern "C"

class DeviceUBoot : public Device
{
public:
    DeviceUBoot();
    ~DeviceUBoot();

    bool Open(const char *name) override;
    void Close() override;

    bool Read(void *data, uint64_t offs, uint64_t len) override;
    uint64_t GetSize() const override;

private:
};
