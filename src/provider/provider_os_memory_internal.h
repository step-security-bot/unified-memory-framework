/*
 * Copyright (C) 2023 Intel Corporation
 *
 * Under the Apache License v2.0 with LLVM Exceptions. See LICENSE.TXT.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
*/

#ifndef UMF_OS_MEMORY_PROVIDER_INTERNAL_H
#define UMF_OS_MEMORY_PROVIDER_INTERNAL_H

#include "utils_common.h"
#include <umf/providers/provider_os_memory.h>

#ifdef __cplusplus
extern "C" {
#endif

int os_translate_flags(unsigned in_flags, unsigned max,
                       int (*translate_flag)(unsigned));

int os_translate_mem_protection_flags(unsigned protection);

int os_translate_mem_visibility(umf_mem_visibility_t visibility);

int os_mmap_aligned(void *hint_addr, size_t length, size_t alignment,
                    size_t page_size, int prot, int flags, int fd, long offset,
                    void **out_addr);

int os_munmap(void *addr, size_t length);

int os_purge(void *addr, size_t length, int advice);

size_t os_get_page_size(void);

void os_strerror(int errnum, char *buf, size_t buflen);

#ifdef __cplusplus
}
#endif

#endif /* UMF_OS_MEMORY_PROVIDER_INTERNAL_H */
