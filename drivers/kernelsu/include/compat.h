/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __KSU_COMPAT_H
#define __KSU_COMPAT_H

#include <linux/version.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/types.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0)
#ifndef strncpy_from_user_nofault
#define strncpy_from_user_nofault(dst, src, count) strncpy_from_unsafe(dst, src, count)
#endif
#ifndef copy_from_user_nofault
#define copy_from_user_nofault(dst, src, size) probe_kernel_read(dst, src, size)
#endif
#ifndef copy_to_user_nofault
#define copy_to_user_nofault(dst, src, size) probe_kernel_write(dst, src, size)
#endif
#ifndef copy_to_kernel_nofault
#define copy_to_kernel_nofault(dst, src, size) probe_kernel_write(dst, src, size)
#endif
#ifndef copy_from_kernel_nofault
#define copy_from_kernel_nofault(dst, src, size) probe_kernel_read(dst, src, size)
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
static inline ssize_t ksu_kernel_read_compat(struct file *p, void *buf, size_t count, loff_t *pos)
{
    ssize_t ret = kernel_read(p, *pos, (char *)buf, count);
    if (ret > 0)
        *pos += ret;
    return ret;
}
#define kernel_read ksu_kernel_read_compat
#endif

#ifndef TWA_RESUME
#define TWA_RESUME true
#endif

#endif /* __KSU_COMPAT_H */
