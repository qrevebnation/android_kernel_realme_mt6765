/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_COMPILER_TYPES_H
#define __LINUX_COMPILER_TYPES_H

/* Compatibility shim for Linux 4.9: forward to linux/compiler.h */
#include <linux/compiler.h>

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

#endif /* __LINUX_COMPILER_TYPES_H */
