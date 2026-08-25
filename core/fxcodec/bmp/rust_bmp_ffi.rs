// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

use image::codecs::bmp::BmpDecoder;
use image::{ImageDecoder, ImageFormat};
use std::io::Cursor;

#[cxx::bridge(namespace = "fxcodec::rust_bmp")]
mod ffi {
    pub struct BmpHeaderInfo {
        pub width: u32,
        pub height: u32,
        pub components: i32,
    }

    #[derive(Debug, PartialEq)]
    pub enum DecodeStatus {
        Success,
        Continue,
        Error,
    }

    extern "Rust" {
        fn read_bmp_info(src: &[u8], info: &mut BmpHeaderInfo) -> DecodeStatus;
        fn decode_bmp_to_buf(src: &[u8], out_buf: &mut [u8], out_pitch: usize) -> DecodeStatus;
    }
}

fn get_bmp_bpp(src: &[u8]) -> u16 {
    if src.len() < 18 {
        return 24;
    }
    let header_size = u32::from_le_bytes(src[14..18].try_into().unwrap());
    if header_size == 12 {
        if src.len() >= 26 {
            return u16::from_le_bytes(src[24..26].try_into().unwrap());
        }
    } else if header_size >= 16 && src.len() >= 30 {
        return u16::from_le_bytes(src[28..30].try_into().unwrap());
    }
    24
}

const MAX_UNCOMPRESSED_BYTES: u64 = 1024 * 1024 * 1024; // 1 GiB

fn is_32bpp_or_has_alpha(src: &[u8], has_alpha: bool) -> bool {
    has_alpha || get_bmp_bpp(src) == 32
}

fn read_bmp_info(src: &[u8], info: &mut ffi::BmpHeaderInfo) -> ffi::DecodeStatus {
    let decoder = match BmpDecoder::new(Cursor::new(src)) {
        Ok(d) => d,
        Err(image::ImageError::IoError(io_err))
            if io_err.kind() == std::io::ErrorKind::UnexpectedEof =>
        {
            return ffi::DecodeStatus::Continue;
        }
        Err(_) => return ffi::DecodeStatus::Error,
    };
    let (width, height) = decoder.dimensions();
    if width == 0 || height == 0 {
        return ffi::DecodeStatus::Error;
    }
    let components =
        if is_32bpp_or_has_alpha(src, decoder.color_type().has_alpha()) { 4 } else { 3 };
    let uncompressed_size = (width as u64)
        .checked_mul(height as u64)
        .and_then(|pixels| pixels.checked_mul(components as u64));
    if uncompressed_size.map_or(true, |size| size > MAX_UNCOMPRESSED_BYTES) {
        return ffi::DecodeStatus::Error;
    }
    info.width = width;
    info.height = height;
    info.components = components;
    ffi::DecodeStatus::Success
}

fn decode_bmp_to_buf(src: &[u8], dst: &mut [u8], dst_pitch: usize) -> ffi::DecodeStatus {
    let img = match image::load_from_memory_with_format(src, ImageFormat::Bmp) {
        Ok(i) => i,
        Err(image::ImageError::IoError(io_err))
            if io_err.kind() == std::io::ErrorKind::UnexpectedEof =>
        {
            return ffi::DecodeStatus::Continue;
        }
        Err(_) => return ffi::DecodeStatus::Error,
    };
    let width = img.width() as usize;
    let height = img.height() as usize;
    if width == 0 || height == 0 {
        return ffi::DecodeStatus::Error;
    }

    if is_32bpp_or_has_alpha(src, img.color().has_alpha()) {
        let rgba = img.to_rgba8();
        let raw_pixels = rgba.as_raw();
        let row_bytes = width * 4;
        if raw_pixels.len() < height * row_bytes {
            return ffi::DecodeStatus::Error;
        }

        for y in 0..height {
            let src_row = &raw_pixels[y * row_bytes..(y + 1) * row_bytes];
            let dst_start = y * dst_pitch;
            if dst_start + row_bytes > dst.len() {
                return ffi::DecodeStatus::Error;
            }
            let dst_slice = &mut dst[dst_start..dst_start + row_bytes];
            for x in 0..width {
                let r = src_row[x * 4];
                let g = src_row[x * 4 + 1];
                let b = src_row[x * 4 + 2];
                let a = src_row[x * 4 + 3];
                dst_slice[x * 4] = b;
                dst_slice[x * 4 + 1] = g;
                dst_slice[x * 4 + 2] = r;
                dst_slice[x * 4 + 3] = a;
            }
        }
    } else {
        let rgb = img.to_rgb8();
        let raw_pixels = rgb.as_raw();
        let row_bytes = width * 3;
        if raw_pixels.len() < height * row_bytes {
            return ffi::DecodeStatus::Error;
        }

        for y in 0..height {
            let src_row = &raw_pixels[y * row_bytes..(y + 1) * row_bytes];
            let dst_start = y * dst_pitch;
            if dst_start + row_bytes > dst.len() {
                return ffi::DecodeStatus::Error;
            }
            let dst_slice = &mut dst[dst_start..dst_start + row_bytes];
            for x in 0..width {
                let r = src_row[x * 3];
                let g = src_row[x * 3 + 1];
                let b = src_row[x * 3 + 2];
                dst_slice[x * 3] = b;
                dst_slice[x * 3 + 1] = g;
                dst_slice[x * 3 + 2] = r;
            }
        }
    }
    ffi::DecodeStatus::Success
}
