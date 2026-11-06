// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/cfx_cttnametable.h"

#include <utility>

#include "core/fxcrt/byteorder.h"

namespace {

uint16_t ReadUInt16FromSpanAtOffset(pdfium::span<const uint8_t> data,
                                    size_t offset) {
  return fxcrt::GetUInt16MSBFirst(data.subspan(offset).first<2u>());
}

}  // namespace

CFX_CTTNameTable::CFX_CTTNameTable(pdfium::span<const uint8_t> name_table) {
  if (name_table.size() < 6) {
    return;
  }

  uint16_t nNameCount = ReadUInt16FromSpanAtOffset(name_table, 2);
  uint16_t nStringOffset = ReadUInt16FromSpanAtOffset(name_table, 4);
  if (nStringOffset > name_table.size()) {
    return;
  }

  pdfium::span<const uint8_t> str = name_table.subspan(nStringOffset);
  pdfium::span<const uint8_t> name_record = name_table.subspan<6u>();
  for (uint16_t i = 0; i < nNameCount; ++i) {
    size_t record_offset = static_cast<size_t>(i) * 12;
    if (name_record.size() - record_offset < 12) {
      break;
    }

    pdfium::span<const uint8_t, 12> record =
        name_record.subspan(record_offset).first<12u>();
    uint16_t nPlatformID = ReadUInt16FromSpanAtOffset(record, 0);
    uint16_t nNameID = ReadUInt16FromSpanAtOffset(record, 6);
    if (nNameID != 1) {
      continue;
    }

    uint16_t nNameLength = ReadUInt16FromSpanAtOffset(record, 8);
    uint16_t nNameOffset = ReadUInt16FromSpanAtOffset(record, 10);
    // Avoid out of bounds crashes if the length and/or offset are wrong.
    if (static_cast<size_t>(nNameLength) + nNameOffset > str.size()) {
      continue;
    }

    WideString family;
    if (nPlatformID != 1) {
      family.Reserve(nNameLength / 2);
      for (uint16_t j = 0; j < nNameLength / 2; ++j) {
        family += static_cast<wchar_t>(
            ReadUInt16FromSpanAtOffset(str, nNameOffset + j * 2));
      }
    } else {
      family.Reserve(nNameLength);
      for (uint16_t j = 0; j < nNameLength; ++j) {
        family += static_cast<wchar_t>(str[nNameOffset + j]);
      }
    }
    family_names_.push_back(std::move(family));
  }
}

CFX_CTTNameTable::CFX_CTTNameTable() = default;

CFX_CTTNameTable::CFX_CTTNameTable(CFX_CTTNameTable&&) noexcept = default;

CFX_CTTNameTable& CFX_CTTNameTable::operator=(CFX_CTTNameTable&&) noexcept =
    default;

CFX_CTTNameTable::~CFX_CTTNameTable() = default;

pdfium::span<const WideString> CFX_CTTNameTable::GetFamilyNames() const {
  return family_names_;
}

void CFX_CTTNameTable::AddFamilyName(const WideString& name) {
  family_names_.push_back(name);
}
