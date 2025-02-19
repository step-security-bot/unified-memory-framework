/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * Under the Apache License v2.0 with LLVM Exceptions. See LICENSE.TXT.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */

#ifndef UMF_MEMSPACE_H
#define UMF_MEMSPACE_H 1

#include <umf/base.h>
#include <umf/memory_pool.h>
#include <umf/memory_provider.h>
#include <umf/memspace_policy.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct umf_memspace_t *umf_memspace_handle_t;

///
/// \brief Creates new memory pool from memspace and policy.
/// \param hMemspace handle to memspace
/// \param hPolicy handle to policy
/// \param hPool [out] handle to the newly created memory pool
/// \return UMF_RESULT_SUCCESS on success or appropriate error code on failure.
///
umf_result_t umfPoolCreateFromMemspace(umf_memspace_handle_t hMemspace,
                                       umf_memspace_policy_handle_t hPolicy,
                                       umf_memory_pool_handle_t *hPool);

///
/// \brief Creates new memory provider from memspace and policy.
/// \param hMemspace handle to memspace
/// \param hPolicy handle to policy
/// \param hProvider [out] handle to the newly created memory provider
/// \return UMF_RESULT_SUCCESS on success or appropriate error code on failure.
///
umf_result_t
umfMemoryProviderCreateFromMemspace(umf_memspace_handle_t hMemspace,
                                    umf_memspace_policy_handle_t hPolicy,
                                    umf_memory_provider_handle_t *hProvider);

#ifdef __cplusplus
}
#endif

#endif /* UMF_MEMSPACE_H */
