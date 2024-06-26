// Copyright 2018 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_HELPERS_WRITE_H_
#define TESTING_HELPERS_WRITE_H_

#include <memory>
#include <string>

#include "public/fpdfview.h"

#ifdef PDF_ENABLE_SKIA
class SkPicture;
class SkWStream;
#endif  // PDF_ENABLE_SKIA

std::string WritePpm(const char* pdf_name,
                     int num,
                     void* buffer_void,
                     int stride,
                     int width,
                     int height);
void WriteText(FPDF_TEXTPAGE textpage, const char* pdf_name, int num);
void WriteAnnot(FPDF_PAGE page, const char* pdf_name, int num);
std::string WritePng(const char* pdf_name,
                     int num,
                     void* buffer,
                     int stride,
                     int width,
                     int height);

#ifdef _WIN32
std::string WriteBmp(const char* pdf_name,
                     int num,
                     void* buffer,
                     int stride,
                     int width,
                     int height);
void WriteEmf(FPDF_PAGE page, const char* pdf_name, int num);
void WritePS(FPDF_PAGE page, const char* pdf_name, int num);
#endif  // _WIN32

#ifdef PDF_ENABLE_SKIA
std::unique_ptr<SkWStream> WriteToSkWStream(const std::string& pdf_name,
                                            int num,
                                            const std::string& extension);
std::unique_ptr<SkWStream> WriteToSkWStream(const std::string& pdf_name,
                                            int num,
                                            const std::string& extension,
                                            std::string& filename);
std::string WriteSkp(const char* pdf_name, int num, const SkPicture& picture);
#endif  // PDF_ENABLE_SKIA

void WriteAttachments(FPDF_DOCUMENT doc, const std::string& name);
void WriteImages(FPDF_PAGE page, const char* pdf_name, int page_num);
void WriteRenderedImages(FPDF_DOCUMENT doc,
                         FPDF_PAGE page,
                         const char* pdf_name,
                         int page_num);
void WriteDecodedThumbnailStream(FPDF_PAGE page,
                                 const char* pdf_name,
                                 int page_num);
void WriteRawThumbnailStream(FPDF_PAGE page,
                             const char* pdf_name,
                             int page_num);
void WriteThumbnail(FPDF_PAGE page, const char* pdf_name, int page_num);

#endif  // TESTING_HELPERS_WRITE_H_
