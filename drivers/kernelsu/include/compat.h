/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __KSU_COMPAT_H
#define __KSU_COMPAT_H

#include <linux/version.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <uapi/linux/eventpoll.h>

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

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0)
#ifndef __poll_t
typedef unsigned int __poll_t;
#define __poll_t __poll_t
#endif
#endif

#ifndef selinux_inode
#define selinux_inode(inode) ((struct inode_security_struct *)(inode)->i_security)
#endif

#ifndef selinux_cred
#define selinux_cred(cred) ((struct task_security_struct *)(cred)->security)
#endif

#ifndef fallthrough
#if defined(__has_attribute)
#if __has_attribute(__fallthrough__)
#define fallthrough __attribute__((__fallthrough__))
#endif
#endif
#ifndef fallthrough
#define fallthrough do {} while (0) /* fallthrough */
#endif
#endif

#ifndef strscpy_pad
static inline ssize_t strscpy_pad(char *dest, const char *src, size_t count)
{
    ssize_t written;

    written = strscpy(dest, src, count);
    if (written < 0)
        return written;

    memset(dest + written, 0, count - written);
    return written;
}
#define strscpy_pad strscpy_pad
#endif

#endif /* __KSU_COMPAT_H */
