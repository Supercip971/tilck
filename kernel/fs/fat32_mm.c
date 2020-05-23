/* SPDX-License-Identifier: BSD-2-Clause */

#include <tilck/common/basic_defs.h>
#include <tilck/common/printk.h>

#include <tilck/kernel/errno.h>
#include <tilck/kernel/fs/vfs.h>
#include <tilck/kernel/fs/fat32.h>
#include <tilck/kernel/process.h>
#include <tilck/kernel/process_mm.h>
#include <tilck/kernel/system_mmap.h>

int fat_ramdisk_mm_fixes(struct fat_hdr *hdr, size_t rd_size)
{
   if (system_mmap_merge_rd_extra_region_if_any(hdr)) {

      /*
       * Typical case: the extra 4k region after our ramdisk, survived the
       * overlap handling, meaning that the was at least 4k of usable (regular)
       * memory just after our ramdisk. This will help in the corner case below.
       */

      rd_size += PAGE_SIZE;
   }

   if (fat_is_first_data_sector_aligned(hdr, PAGE_SIZE))
      return 0; /* Typical case: nothing to do */

   /*
    * The code here below will almost never be triggered as it handles a very
    * ugly use case. Let me explain.
    *
    * In order to fat_mmap() work in the simple and direct way as implemented
    * in Tilck, the FAT clusters must be aligned at page boundary. That is true
    * when just the first data sector is aligned. In our build system, the
    * fathack build app is used to make that happen. Fathack substantially calls
    * fat_align_first_data_sector() which adds more reserved sectors to the
    * partition by shifting all the data by the necessary amount of bytes.
    *
    * Now, when our FAT ramdisk is build by our build system, because of fathack
    * we never have to worry about such alignment. That is true even if the
    * fat ramdisk is later modified by an external tool to add/remove files from
    * it: the number of reserved sectors won't changed. BUT, in the unlikely
    * case when the external tools reformats the whole partition, we'll loose
    * that alignment and it would be nice if Tilck itself could handle that case
    * too. That's what the following code does.
    *
    * Note: in order to the code below to work (corner case, like explained) we
    * need at least one of the following conditions to be true:
    *
    *    - boot the OS using one of Tilck's bootloaders OR
    *    - have 1 page avail at the end of the ramdisk mem region (very likely)
    *
    * In summary, the code below won't work only in the 1 in billion case where
    * all of the following statements are true:
    *
    *    - the fatpart was NOT generated by our build system (why doing that?)
    *    - the OS was NOT booted using Tilck's bootloaders (e.g. using GRUB)
    *    - according to the firmware, the next 4k after the ramdisk belong
    *      to a reserved memory region (extremely unlucky case)
    */

   const u32 used = fat_calculate_used_bytes(hdr);
   pdir_t *const pdir = get_kernel_pdir();
   char *const va_begin = (char *)hdr;
   char *const va_end = va_begin + rd_size;
   VERIFY(rd_size >= used);

   if (rd_size - used < PAGE_SIZE) {
      printk("WARNING: [fat ramdisk] cannot align first data sector\n");
      return -1;
   }

   for (char *va = va_begin; va < va_end; va += PAGE_SIZE)
      set_page_rw(pdir, va, true);

   fat_align_first_data_sector(hdr, PAGE_SIZE);

   for (char *va = va_begin; va < va_end; va += PAGE_SIZE)
      set_page_rw(pdir, va, false);

   printk("[fat ramdisk]: align of ramdisk was necessary\n");
   return 0;
}

int fat_mmap(struct user_mapping *um, bool register_only)
{
   struct process *pi = get_curr_proc();
   struct fatfs_handle *fh = um->h;
   struct fat_fs_device_data *d = fh->fs->device_data;

   if (d->cluster_size < PAGE_SIZE)
      return -ENODEV; /* We do NOT support mmap in this case */

   (void)pi;
   NOT_IMPLEMENTED();
}

int fat_munmap(fs_handle h, void *vaddrp, size_t len)
{
   struct fatfs_handle *fh = h;
   struct fat_fs_device_data *d = fh->fs->device_data;

   if (d->cluster_size < PAGE_SIZE)
      return -ENODEV; /* We do NOT support mmap in this case */

   NOT_IMPLEMENTED();
}
