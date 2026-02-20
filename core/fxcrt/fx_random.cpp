// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcrt/fx_random.h"

#include <stdint.h>

#include <array>

#include "build/build_config.h"
#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/fx_system.h"

#define MT_M 456
#define MT_Matrix_A 0x9908b0df
#define MT_Upper_Mask 0x80000000
#define MT_Lower_Mask 0x7fffffff

#if BUILDFLAG(IS_WIN)
#include <wincrypt.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

namespace {

bool g_bHaveGlobalSeed = false;
uint32_t g_nGlobalSeed = 0;

#if BUILDFLAG(IS_WIN)
bool GenerateSeedFromCryptoRandom(uint32_t* pSeed) {
  HCRYPTPROV hCP = 0;
  if (!::CryptAcquireContext(&hCP, nullptr, nullptr, PROV_RSA_FULL, 0) ||
      !hCP) {
    return false;
  }
  ::CryptGenRandom(hCP, sizeof(uint32_t), reinterpret_cast<uint8_t*>(pSeed));
  ::CryptReleaseContext(hCP, 0);
  return true;
}
#endif

uint32_t GenerateSeedFromEnvironment() {
  char c;
  uintptr_t p = reinterpret_cast<uintptr_t>(&c);
  uint32_t seed = ~static_cast<uint32_t>(p >> 3);
#if BUILDFLAG(IS_WIN)
  SYSTEMTIME st;
  GetSystemTime(&st);
  seed ^= static_cast<uint32_t>(st.wSecond) * 1000000;
  seed ^= static_cast<uint32_t>(st.wMilliseconds) * 1000;
  seed ^= GetCurrentProcessId();
#else
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  seed ^= static_cast<uint32_t>(tv.tv_sec) * 1000000;
  seed ^= static_cast<uint32_t>(tv.tv_usec);
  seed ^= static_cast<uint32_t>(getpid());
#endif
  return seed;
}

uint32_t GetNextGlobalSeed() {
  if (!g_bHaveGlobalSeed) {
#if BUILDFLAG(IS_WIN)
    if (!GenerateSeedFromCryptoRandom(&g_nGlobalSeed)) {
      g_nGlobalSeed = GenerateSeedFromEnvironment();
    }
#else
    g_nGlobalSeed = GenerateSeedFromEnvironment();
#endif
    g_bHaveGlobalSeed = true;
  }
  return ++g_nGlobalSeed;
}

}  // namespace

FX_Random::FX_Random(uint32_t seed) {
  context_.mt[0] = seed;
  for (uint32_t i = 1; i < kN; i++) {
    const uint32_t prev = context_.mt[i - 1];
    context_.mt[i] = (1812433253UL * (prev ^ (prev >> 30)) + i);
  }
  context_.mti = kN;
}

FX_Random::~FX_Random() = default;

// static
void FX_Random::Fill(pdfium::span<uint32_t> buffer) {
  FX_Random next_generator(GetNextGlobalSeed());
  for (uint32_t& val : buffer) {
    val = next_generator.Generate();
  }
}

uint32_t FX_Random::Generate() {
  uint32_t v;
  if (context_.mti >= kN) {
    static constexpr std::array<uint32_t, 2> mag = {{0, MT_Matrix_A}};
    uint32_t kk;
    for (kk = 0; kk < kN - MT_M; kk++) {
      v = (context_.mt[kk] & MT_Upper_Mask) |
          (context_.mt[kk + 1] & MT_Lower_Mask);
      context_.mt[kk] = context_.mt[kk + MT_M] ^ (v >> 1) ^ mag[v & 1];
    }
    for (; kk < kN - 1; kk++) {
      v = (context_.mt[kk] & MT_Upper_Mask) |
          (context_.mt[kk + 1] & MT_Lower_Mask);
      // `MT_M - kN` underflows, but this is safe because unsigned underflow is
      // well-defined.
      context_.mt[kk] = context_.mt[kk + (MT_M - kN)] ^ (v >> 1) ^ mag[v & 1];
    }
    v = (context_.mt[kN - 1] & MT_Upper_Mask) |
        (context_.mt[0] & MT_Lower_Mask);
    context_.mt[kN - 1] = context_.mt[MT_M - 1] ^ (v >> 1) ^ mag[v & 1];
    context_.mti = 0;
  }
  v = context_.mt[context_.mti++];
  v ^= (v >> 11);
  v ^= (v << 7) & 0x9d2c5680UL;
  v ^= (v << 15) & 0xefc60000UL;
  v ^= (v >> 18);
  return v;
}
