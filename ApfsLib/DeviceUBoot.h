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

#if defined(HAS_UBOOT_STUBS) || defined(__UBOOT__)

#include <cstdlib>
#include <cstdint>

#include "Device.h"

enum class intf_type {
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

#ifndef __UBOOT__

#ifdef __UBOOT__
#error stubs used in DeviceUBoot in real u-boot build
#endif

struct udevice {
    const char *name;
};

typedef uint64_t lbaint_t;

struct blk_desc {
    const uint8_t  *__buf;
    lbaint_t        lba;
    unsigned long   blksz;
    unsigned long   (*block_read)(struct blk_desc *block_dev,
                      lbaint_t start,
                      lbaint_t blkcnt,
                      void *buffer);
    unsigned long   (*block_write)(struct blk_desc *block_dev,
                       lbaint_t start,
                       lbaint_t blkcnt,
                       const void *buffer);
    unsigned long   (*block_erase)(struct blk_desc *block_dev,
                       lbaint_t start,
                       lbaint_t blkcnt);
};

#else

// #include <blk.h>

// FIXME (lol)
// #include <dm/device.h>

typedef uint64_t lbaint_t;

struct udevice;

#define CONFIG_LBA48 1
#define CONFIG_BLK 1

#define BLK_VEN_SIZE        40
#define BLK_PRD_SIZE        20
#define BLK_REV_SIZE        8

/*
 * With driver model (CONFIG_BLK) this is uclass platform data, accessible
 * with dev_get_uclass_plat(dev)
 */
struct blk_desc {
    /*
     * TODO: With driver model we should be able to use the parent
     * device's uclass instead.
     */
    int    if_type;    /* type of the interface */
    int     devnum;     /* device number */
    unsigned char   part_type;  /* partition type */
    unsigned char   target;     /* target SCSI ID */
    unsigned char   lun;        /* target LUN */
    unsigned char   hwpart;     /* HW partition, e.g. for eMMC */
    unsigned char   type;       /* device type */
    unsigned char   removable;  /* removable device */
#ifdef CONFIG_LBA48
    /* device can use 48bit addr (ATA/ATAPI v7) */
    unsigned char   lba48;
#endif
    lbaint_t    lba;        /* number of blocks */
    unsigned long   blksz;      /* block size */
    int     log2blksz;  /* for convenience: log2(blksz) */
    char        vendor[BLK_VEN_SIZE + 1]; /* device vendor string */
    char        product[BLK_PRD_SIZE + 1]; /* device product number */
    char        revision[BLK_REV_SIZE + 1]; /* firmware revision */
    int  sig_type;   /* Partition table signature type */
    union {
        uint32_t mbr_sig;   /* MBR integer signature */
        uint8_t guid_sig[16];    /* GPT GUID Signature */
    };
#if CONFIG_BLK
    /*
     * For now we have a few functions which take struct blk_desc as a
     * parameter. This field allows them to look up the associated
     * device. Once these functions are removed we can drop this field.
     */
    struct udevice *bdev;
#else
    unsigned long   (*block_read)(struct blk_desc *block_dev,
                      lbaint_t start,
                      lbaint_t blkcnt,
                      void *buffer);
    unsigned long   (*block_write)(struct blk_desc *block_dev,
                       lbaint_t start,
                       lbaint_t blkcnt,
                       const void *buffer);
    unsigned long   (*block_erase)(struct blk_desc *block_dev,
                       lbaint_t start,
                       lbaint_t blkcnt);
    void        *priv;      /* driver private struct pointer */
#endif
};

#endif

int blk_get_device(int if_type, int devnum, struct udevice **devp);
void *dev_get_uclass_plat(const struct udevice *dev);
struct blk_desc *blk_get_by_device(struct udevice *dev);

unsigned long blk_dread(struct blk_desc *block_dev, lbaint_t start,
            lbaint_t blkcnt, void *buffer);
unsigned long blk_dwrite(struct blk_desc *block_dev, lbaint_t start,
             lbaint_t blkcnt, const void *buffer);
unsigned long blk_derase(struct blk_desc *block_dev, lbaint_t start,
             lbaint_t blkcnt);


} // extern "C"

class DeviceUBoot : public Device
{
public:
    DeviceUBoot();
    ~DeviceUBoot();

    bool Open(const char *name) override;
    bool Open(struct blk_desc *fs_dev_desc);
    void Close() override;

    bool Read(void *data, uint64_t offs, uint64_t len) override;
    uint64_t GetSize() const override;

private:
    struct blk_desc *m_blk;
};

#endif
