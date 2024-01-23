// Copyright (C) 2023-2024 Intel Corporation
// Under the Apache License v2.0 with LLVM Exceptions. See LICENSE.TXT.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// This file contains tests for UMF pool API

#include "pool.hpp"
#include "poolFixtures.hpp"
#include "provider.hpp"
#include "provider_null.h"
#include "provider_trace.h"
#include "test_helpers.h"

#include "umf/memory_provider.h"

#include <array>
#include <string>
#include <thread>
#include <unordered_map>

using umf_test::test;
using namespace umf_test;

TEST_F(test, memoryPoolTrace) {
    static std::unordered_map<std::string, size_t> poolCalls;
    static std::unordered_map<std::string, size_t> providerCalls;
    auto tracePool = [](const char *name) { poolCalls[name]++; };
    auto traceProvider = [](const char *name) { providerCalls[name]++; };

    auto nullProvider = umf_test::wrapProviderUnique(nullProviderCreate());
    auto tracingProvider = umf_test::wrapProviderUnique(
        traceProviderCreate(nullProvider.get(), traceProvider));
    auto provider = tracingProvider.get();

    auto proxyPool =
        wrapPoolUnique(createPoolChecked(&PROXY_POOL_OPS, provider, nullptr));

    umf_memory_provider_handle_t providerDesc = nullProviderCreate();
    auto tracingPool = umf_test::wrapPoolUnique(
        tracePoolCreate(proxyPool.get(), providerDesc, tracePool));

    size_t pool_call_count = 0;
    size_t provider_call_count = 0;

    umfPoolMalloc(tracingPool.get(), 0);
    ASSERT_EQ(poolCalls["malloc"], 1);
    ASSERT_EQ(poolCalls.size(), ++pool_call_count);

    ASSERT_EQ(providerCalls["alloc"], 1);
    ASSERT_EQ(providerCalls.size(), ++provider_call_count);

    umfPoolFree(tracingPool.get(), nullptr);
    ASSERT_EQ(poolCalls["free"], 1);
    ASSERT_EQ(poolCalls.size(), ++pool_call_count);

    ASSERT_EQ(providerCalls["free"], 1);
    ASSERT_EQ(providerCalls.size(), ++provider_call_count);

    umfPoolCalloc(tracingPool.get(), 0, 0);
    ASSERT_EQ(poolCalls["calloc"], 1);
    ASSERT_EQ(poolCalls.size(), ++pool_call_count);

    ASSERT_EQ(providerCalls["alloc"], 2);
    ASSERT_EQ(providerCalls.size(), provider_call_count);

    umfPoolRealloc(tracingPool.get(), nullptr, 0);
    ASSERT_EQ(poolCalls["realloc"], 1);
    ASSERT_EQ(poolCalls.size(), ++pool_call_count);

    ASSERT_EQ(providerCalls.size(), provider_call_count);

    umfPoolAlignedMalloc(tracingPool.get(), 0, 0);
    ASSERT_EQ(poolCalls["aligned_malloc"], 1);
    ASSERT_EQ(poolCalls.size(), ++pool_call_count);

    ASSERT_EQ(providerCalls["alloc"], 3);
    ASSERT_EQ(providerCalls.size(), provider_call_count);

    umfPoolMallocUsableSize(tracingPool.get(), nullptr);
    ASSERT_EQ(poolCalls["malloc_usable_size"], 1);
    ASSERT_EQ(poolCalls.size(), ++pool_call_count);

    ASSERT_EQ(providerCalls.size(), provider_call_count);

    auto ret = umfPoolGetLastAllocationError(tracingPool.get());
    ASSERT_EQ(ret, UMF_RESULT_SUCCESS);
    ASSERT_EQ(poolCalls["get_last_native_error"], 1);
    ASSERT_EQ(poolCalls.size(), ++pool_call_count);

    umfMemoryProviderDestroy(providerDesc);
}

TEST_F(test, memoryPoolWithCustomProvider) {
    umf_memory_provider_handle_t hProvider = nullProviderCreate();

    struct pool : public umf_test::pool_base_t {
        umf_result_t
        initialize(umf_memory_provider_handle_t provider) noexcept {
            EXPECT_NE_NOEXCEPT(provider, nullptr);
            return UMF_RESULT_SUCCESS;
        }
    };
    umf_memory_pool_ops_t pool_ops = umf::poolMakeCOps<pool, void>();

    umf_memory_pool_handle_t hPool;
    auto ret = umfPoolCreate(&pool_ops, hProvider, nullptr, 0, &hPool);
    ASSERT_EQ(ret, UMF_RESULT_SUCCESS);
    ASSERT_NE(hPool, nullptr);

    umfPoolDestroy(hPool);
    umfMemoryProviderDestroy(hProvider);
}

TEST_F(test, retrieveMemoryProvider) {
    umf_memory_provider_handle_t provider = (umf_memory_provider_handle_t)0x1;

    auto pool =
        wrapPoolUnique(createPoolChecked(&PROXY_POOL_OPS, provider, nullptr));

    umf_memory_provider_handle_t retProvider;
    auto ret = umfPoolGetMemoryProvider(pool.get(), &retProvider);
    ASSERT_EQ(ret, UMF_RESULT_SUCCESS);
    ASSERT_EQ(retProvider, provider);
}

INSTANTIATE_TEST_SUITE_P(
    mallocPoolTest, umfPoolTest,
    ::testing::Values(poolCreateExtParams{&MALLOC_POOL_OPS, nullptr,
                                          &UMF_NULL_PROVIDER_OPS, nullptr},
                      poolCreateExtParams{&PROXY_POOL_OPS, nullptr,
                                          &MALLOC_PROVIDER_OPS, nullptr}));

INSTANTIATE_TEST_SUITE_P(mallocMultiPoolTest, umfMultiPoolTest,
                         ::testing::Values(poolCreateExtParams{
                             &PROXY_POOL_OPS, nullptr, &MALLOC_PROVIDER_OPS,
                             nullptr}));

struct umfPoolCreateFlagsTest
    : umf_test::test,
      ::testing::WithParamInterface<umf_pool_create_flags_t> {
    void SetUp() override {
        test::SetUp();
        flags = this->GetParam();
    }
    umf_pool_create_flags_t flags;
};

TEST_P(umfPoolCreateFlagsTest, umfPoolCreateFlagsTestSuccess) {
    umf_memory_provider_handle_t provider = nullptr;
    umf_result_t ret =
        umfMemoryProviderCreate(&UMF_NULL_PROVIDER_OPS, nullptr, &provider);
    ASSERT_EQ(ret, UMF_RESULT_SUCCESS);
    ASSERT_NE(provider, nullptr);

    umf_memory_pool_handle_t pool = nullptr;
    ret = umfPoolCreate(&PROXY_POOL_OPS, provider, nullptr, flags, &pool);
    ASSERT_EQ(ret, UMF_RESULT_SUCCESS);
    ASSERT_NE(pool, nullptr);

    umfPoolDestroy(pool);

    if (!(flags & UMF_POOL_CREATE_FLAG_OWN_PROVIDER)) {
        umfMemoryProviderDestroy(provider);
    }
}

TEST_P(umfPoolCreateFlagsTest, umfPoolCreateFlagsCountProviderCalls) {
    auto nullProvider = umf_test::wrapProviderUnique(nullProviderCreate());

    static std::unordered_map<std::string, size_t> providerCalls;
    providerCalls.clear();
    auto traceCb = [](const char *name) { providerCalls[name]++; };

    umf_provider_trace_params_t provider_params = {nullProvider.get(), traceCb};

    umf_memory_provider_handle_t provider = nullptr;
    umf_result_t ret = umfMemoryProviderCreate(&UMF_TRACE_PROVIDER_OPS,
                                               &provider_params, &provider);
    ASSERT_EQ(ret, UMF_RESULT_SUCCESS);
    ASSERT_NE(provider, nullptr);

    umf_memory_pool_handle_t pool = nullptr;
    ret = umfPoolCreate(&PROXY_POOL_OPS, provider, nullptr, flags, &pool);
    ASSERT_EQ(ret, UMF_RESULT_SUCCESS);
    ASSERT_NE(pool, nullptr);

    size_t provider_call_count = 0;

    umfPoolMalloc(pool, 0);
    ASSERT_EQ(providerCalls["alloc"], 1);
    ASSERT_EQ(providerCalls.size(), ++provider_call_count);

    umfPoolFree(pool, 0);
    ASSERT_EQ(providerCalls["free"], 1);
    ASSERT_EQ(providerCalls.size(), ++provider_call_count);

    umfPoolCalloc(pool, 0, 0);
    ASSERT_EQ(providerCalls["alloc"], 2);
    ASSERT_EQ(providerCalls.size(), provider_call_count);

    umfPoolAlignedMalloc(pool, 0, 0);
    ASSERT_EQ(providerCalls["alloc"], 3);
    ASSERT_EQ(providerCalls.size(), provider_call_count);

    umfPoolDestroy(pool);

    if (!(flags & UMF_POOL_CREATE_FLAG_OWN_PROVIDER)) {
        umfMemoryProviderDestroy(provider);
    }
}

INSTANTIATE_TEST_SUITE_P(umfPoolCreateFlagsTest, umfPoolCreateFlagsTest,
                         ::testing::Values(0,
                                           UMF_POOL_CREATE_FLAG_OWN_PROVIDER));

////////////////// Negative test cases /////////////////

TEST_P(umfPoolCreateFlagsTest, umfPoolCreateFlagsNullOps) {
    umf_memory_provider_handle_t provider = nullptr;
    umf_result_t ret =
        umfMemoryProviderCreate(&UMF_NULL_PROVIDER_OPS, nullptr, &provider);
    ASSERT_EQ(ret, UMF_RESULT_SUCCESS);
    ASSERT_NE(provider, nullptr);

    umf_memory_pool_handle_t pool = nullptr;
    ret = umfPoolCreate(nullptr, provider, nullptr, flags, &pool);
    ASSERT_EQ(ret, UMF_RESULT_ERROR_INVALID_ARGUMENT);

    umfMemoryProviderDestroy(provider);
}

TEST_P(umfPoolCreateFlagsTest, umfPoolCreateFlagsNullPoolHandle) {
    umf_memory_provider_handle_t provider = nullptr;
    umf_result_t ret =
        umfMemoryProviderCreate(&UMF_NULL_PROVIDER_OPS, nullptr, &provider);
    ASSERT_EQ(ret, UMF_RESULT_SUCCESS);
    ASSERT_NE(provider, nullptr);

    ret = umfPoolCreate(&PROXY_POOL_OPS, provider, nullptr, flags, nullptr);
    ASSERT_EQ(ret, UMF_RESULT_ERROR_INVALID_ARGUMENT);

    umfMemoryProviderDestroy(provider);
}

TEST_P(umfPoolCreateFlagsTest, umfPoolCreateFlagsInvalidProviders) {
    umf_memory_pool_handle_t hPool;
    auto ret = umfPoolCreate(&MALLOC_POOL_OPS, nullptr, nullptr, flags, &hPool);
    ASSERT_EQ(ret, UMF_RESULT_ERROR_INVALID_ARGUMENT);
}

struct poolInitializeTest : umf_test::test,
                            ::testing::WithParamInterface<umf_result_t> {};

INSTANTIATE_TEST_SUITE_P(
    poolInitializeTest, poolInitializeTest,
    ::testing::Values(UMF_RESULT_ERROR_OUT_OF_HOST_MEMORY,
                      UMF_RESULT_ERROR_MEMORY_PROVIDER_SPECIFIC,
                      UMF_RESULT_ERROR_INVALID_ARGUMENT,
                      UMF_RESULT_ERROR_UNKNOWN));

TEST_P(poolInitializeTest, errorPropagation) {
    auto nullProvider = umf_test::wrapProviderUnique(nullProviderCreate());
    umf_memory_provider_handle_t hProvider = nullProvider.get();

    struct pool : public umf_test::pool_base_t {
        umf_result_t
        initialize([[maybe_unused]] umf_memory_provider_handle_t provider,
                   umf_result_t *errorToReturn) noexcept {
            return *errorToReturn;
        }
    };
    umf_memory_pool_ops_t pool_ops = umf::poolMakeCOps<pool, umf_result_t>();

    umf_memory_pool_handle_t hPool;
    auto ret = umfPoolCreate(&pool_ops, hProvider, (void *)&this->GetParam(), 0,
                             &hPool);
    ASSERT_EQ(ret, this->GetParam());
}

TEST_F(test, retrieveMemoryProvidersError) {
    umf_memory_provider_handle_t provider = (umf_memory_provider_handle_t)0x1;

    auto pool =
        wrapPoolUnique(createPoolChecked(&PROXY_POOL_OPS, provider, nullptr));

    auto ret = umfPoolGetMemoryProvider(pool.get(), nullptr);
    ASSERT_EQ(ret, UMF_RESULT_ERROR_INVALID_ARGUMENT);
}

// TODO: extend test for different functions (not only alloc)
TEST_F(test, getLastFailedMemoryProvider) {
    static constexpr size_t allocSize = 8;
    static umf_result_t allocResult = UMF_RESULT_SUCCESS;

    struct memory_provider : public umf_test::provider_base_t {
        umf_result_t initialize(const char *providerName) {
            name = providerName;
            return UMF_RESULT_SUCCESS;
        }

        umf_result_t alloc(size_t size, size_t, void **ptr) noexcept {
            if (allocResult == UMF_RESULT_SUCCESS) {
                *ptr = malloc(size);
            } else {
                *ptr = nullptr;
            }

            return allocResult;
        }

        umf_result_t free(void *ptr, [[maybe_unused]] size_t size) noexcept {
            ::free(ptr);
            return UMF_RESULT_SUCCESS;
        }

        const char *get_name() noexcept { return name; }

        const char *name;
    };
    umf_memory_provider_ops_t provider_ops =
        umf::providerMakeCOps<memory_provider, char>();

    auto providerUnique1 = wrapProviderUnique(
        createProviderChecked(&provider_ops, (void *)"provider1"));
    auto providerUnique2 = wrapProviderUnique(
        createProviderChecked(&provider_ops, (void *)"provider2"));

    auto hProvider = providerUnique1.get();
    auto pool =
        wrapPoolUnique(createPoolChecked(&PROXY_POOL_OPS, hProvider, nullptr));

    auto ptr = umfPoolMalloc(pool.get(), allocSize);
    ASSERT_NE(ptr, nullptr);
    umfPoolFree(pool.get(), ptr);

    // make provider return an error during allocation
    allocResult = UMF_RESULT_ERROR_UNKNOWN;
    ptr = umfPoolMalloc(pool.get(), allocSize);
    ASSERT_EQ(ptr, nullptr);
    ASSERT_EQ(std::string_view(
                  umfMemoryProviderGetName(umfGetLastFailedMemoryProvider())),
              "provider1");

    auto ret =
        umfMemoryProviderAlloc(providerUnique2.get(), allocSize, 0, &ptr);
    ASSERT_NE(ret, UMF_RESULT_SUCCESS);
    ASSERT_EQ(ptr, nullptr);
    ASSERT_EQ(std::string_view(
                  umfMemoryProviderGetName(umfGetLastFailedMemoryProvider())),
              "provider2");

    // successful provider should not be returned by umfGetLastFailedMemoryProvider
    allocResult = UMF_RESULT_SUCCESS;
    ptr = umfPoolMalloc(pool.get(), allocSize);
    ASSERT_NE(ptr, nullptr);
    ASSERT_EQ(std::string_view(
                  umfMemoryProviderGetName(umfGetLastFailedMemoryProvider())),
              "provider2");
    umfPoolFree(pool.get(), ptr);

    // error in another thread should not impact umfGetLastFailedMemoryProvider on this thread
    allocResult = UMF_RESULT_ERROR_UNKNOWN;
    std::thread t([&, hPool = pool.get()] {
        ptr = umfPoolMalloc(hPool, allocSize);
        ASSERT_EQ(ptr, nullptr);
        ASSERT_EQ(std::string_view(umfMemoryProviderGetName(
                      umfGetLastFailedMemoryProvider())),
                  "provider1");
    });
    t.join();

    ASSERT_EQ(std::string_view(
                  umfMemoryProviderGetName(umfGetLastFailedMemoryProvider())),
              "provider2");
}
