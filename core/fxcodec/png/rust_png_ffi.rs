// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

use std::io::Cursor;

#[cxx::bridge(namespace = "fxcodec::rust_png")]
mod ffi {
    pub struct PngHeaderInfo {
        pub width: u32,
        pub height: u32,
    }

    extern "Rust" {
        fn read_png_info(src: &[u8], info: &mut PngHeaderInfo) -> bool;
        fn decode_png_to_buf(src: &[u8], dst: &mut [u8], dst_pitch: usize) -> bool;
    }
}

fn read_png_info(src: &[u8], info: &mut ffi::PngHeaderInfo) -> bool {
    if src.len() >= 24
        && &src[0..8] == b"\x89PNG\r\n\x1a\n"
        && &src[12..16] == b"IHDR"
        && let (Ok(w), Ok(h)) = (src[16..20].try_into(), src[20..24].try_into())
    {
        let width = u32::from_be_bytes(w);
        let height = u32::from_be_bytes(h);
        if width > 0 && height > 0 {
            info.width = width;
            info.height = height;
            return true;
        }
    }

    let cursor = Cursor::new(src);
    let decoder = png::Decoder::new(cursor);
    let reader = match decoder.read_info() {
        Ok(r) => r,
        Err(_) => return false,
    };
    let header = reader.info();
    if header.width == 0 || header.height == 0 {
        return false;
    }
    info.width = header.width;
    info.height = header.height;
    true
}

fn decode_png_to_buf(src: &[u8], dst: &mut [u8], dst_pitch: usize) -> bool {
    let cursor = Cursor::new(src);
    let mut decoder = png::Decoder::new(cursor);
    decoder.set_transformations(png::Transformations::EXPAND | png::Transformations::STRIP_16);
    let mut reader = match decoder.read_info() {
        Ok(r) => r,
        Err(_) => return false,
    };
    let buf_size = match reader.output_buffer_size() {
        Some(s) => s,
        None => return false,
    };
    let mut buf = vec![0; buf_size];
    let output_info = match reader.next_frame(&mut buf) {
        Ok(info) => info,
        Err(_) => return false,
    };
    let width = output_info.width as usize;
    let height = output_info.height as usize;
    if width == 0 || height == 0 {
        return false;
    }
    let line_size = output_info.line_size;
    if line_size < width {
        return false;
    }
    let bytes_per_pixel = line_size / width;
    if bytes_per_pixel == 0 || bytes_per_pixel > 4 {
        return false;
    }
    if let Some(total_src_bytes) = line_size.checked_mul(height) {
        if total_src_bytes > buf.len() {
            return false;
        }
    } else {
        return false;
    }

    let row_dst_bytes = match width.checked_mul(4) {
        Some(b) => b,
        None => return false,
    };

    // Standard default file gamma for sRGB (1.0 / 2.2).
    const DEFAULT_FILE_GAMMA: f64 = 0.45455;
    let file_gamma: f64 = if reader.info().srgb.is_some() {
        DEFAULT_FILE_GAMMA
    } else if let Some(gama) = reader.info().gama_chunk {
        let val = gama.into_value() as f64;
        if val > 0.0 {
            val
        } else {
            DEFAULT_FILE_GAMMA
        }
    } else {
        DEFAULT_FILE_GAMMA
    };

    const DISPLAY_GAMMA: f64 = 2.2;
    let mut gamma_lut = [0u8; 256];
    let exponent = 1.0 / (file_gamma * DISPLAY_GAMMA);
    if (exponent - 1.0).abs() < 1e-4 {
        for (i, item) in gamma_lut.iter_mut().enumerate() {
            *item = i as u8;
        }
    } else {
        for (i, item) in gamma_lut.iter_mut().enumerate() {
            let v = (i as f64) / 255.0;
            *item = (v.powf(exponent) * 255.0).round().clamp(0.0, 255.0) as u8;
        }
    }

    for y in 0..height {
        let src_start = y * line_size;
        let src_row = &buf[src_start..src_start + line_size];
        let dst_start = match y.checked_mul(dst_pitch) {
            Some(s) => s,
            None => return false,
        };
        let dst_end = match dst_start.checked_add(row_dst_bytes) {
            Some(e) => e,
            None => return false,
        };
        if dst_end > dst.len() {
            return false;
        }
        let dst_slice = &mut dst[dst_start..dst_end];
        if bytes_per_pixel == 1 {
            for x in 0..width {
                let g = gamma_lut[src_row[x] as usize];
                dst_slice[x * 4] = g;
                dst_slice[x * 4 + 1] = g;
                dst_slice[x * 4 + 2] = g;
                dst_slice[x * 4 + 3] = 255;
            }
        } else if bytes_per_pixel == 2 {
            for x in 0..width {
                let g = gamma_lut[src_row[x * 2] as usize];
                let a = src_row[x * 2 + 1];
                dst_slice[x * 4] = g;
                dst_slice[x * 4 + 1] = g;
                dst_slice[x * 4 + 2] = g;
                dst_slice[x * 4 + 3] = a;
            }
        } else if bytes_per_pixel == 3 {
            for x in 0..width {
                let r = gamma_lut[src_row[x * 3] as usize];
                let g = gamma_lut[src_row[x * 3 + 1] as usize];
                let b = gamma_lut[src_row[x * 3 + 2] as usize];
                dst_slice[x * 4] = b;
                dst_slice[x * 4 + 1] = g;
                dst_slice[x * 4 + 2] = r;
                dst_slice[x * 4 + 3] = 255;
            }
        } else if bytes_per_pixel == 4 {
            for x in 0..width {
                let r = gamma_lut[src_row[x * 4] as usize];
                let g = gamma_lut[src_row[x * 4 + 1] as usize];
                let b = gamma_lut[src_row[x * 4 + 2] as usize];
                let a = src_row[x * 4 + 3];
                dst_slice[x * 4] = b;
                dst_slice[x * 4 + 1] = g;
                dst_slice[x * 4 + 2] = r;
                dst_slice[x * 4 + 3] = a;
            }
        } else {
            return false;
        }
    }
    true
}
