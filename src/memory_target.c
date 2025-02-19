/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * Under the Apache License v2.0 with LLVM Exceptions. See LICENSE.TXT.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */

#include <assert.h>
#include <stdlib.h>

#include "base_alloc_global.h"
#include "libumf.h"
#include "memory_target.h"
#include "memory_target_ops.h"
#include "utils_concurrency.h"

umf_result_t umfMemoryTargetCreate(const umf_memory_target_ops_t *ops,
                                   void *params,
                                   umf_memory_target_handle_t *memoryTarget) {
    libumfInit();
    if (!ops || !memoryTarget) {
        return UMF_RESULT_ERROR_INVALID_ARGUMENT;
    }

    umf_ba_pool_t *base_allocator =
        umf_ba_get_pool(sizeof(umf_memory_target_t));
    if (!base_allocator) {
        return UMF_RESULT_ERROR_OUT_OF_HOST_MEMORY;
    }

    umf_memory_target_handle_t target =
        (umf_memory_target_t *)umf_ba_alloc(base_allocator);
    if (!target) {
        return UMF_RESULT_ERROR_OUT_OF_HOST_MEMORY;
    }

    assert(ops->version == UMF_VERSION_CURRENT);

    target->base_allocator = base_allocator;
    target->ops = ops;

    void *target_priv;
    umf_result_t ret = ops->initialize(params, &target_priv);
    if (ret != UMF_RESULT_SUCCESS) {
        umf_ba_free(base_allocator, target);
        return ret;
    }

    target->priv = target_priv;

    *memoryTarget = target;

    return UMF_RESULT_SUCCESS;
}

void umfMemoryTargetDestroy(umf_memory_target_handle_t memoryTarget) {
    assert(memoryTarget);
    memoryTarget->ops->finalize(memoryTarget->priv);
    umf_ba_free(memoryTarget->base_allocator, memoryTarget);
}
