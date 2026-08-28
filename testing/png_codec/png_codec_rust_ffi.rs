// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

use std::io::Cursor;

#[cxx::bridge(namespace = "png_codec::rust_png")]
mod ffi {
    extern "Rust" {
        fn decode_png(
            src: &[u8],
            reverse_byte_order: bool,
            width: &mut i32,
            height: &mut i32,
        ) -> Vec<u8>;

        fn encode_bgr(input: &[u8], width: i32, height: i32, row_byte_width: usize) -> Vec<u8>;

        fn encode_rgba(input: &[u8], width: i32, height: i32, row_byte_width: usize) -> Vec<u8>;

        fn encode_bgra(
            input: &[u8],
            width: i32,
            height: i32,
            row_byte_width: usize,
            discard_transparency: bool,
        ) -> Vec<u8>;

        fn encode_gray(input: &[u8], width: i32, height: i32, row_byte_width: usize) -> Vec<u8>;
    }
}

fn decode_png(
    src: &[u8],
    reverse_byte_order: bool,
    width_out: &mut i32,
    height_out: &mut i32,
) -> Vec<u8> {
    *width_out = 0;
    *height_out = 0;

    let cursor = Cursor::new(src);
    let mut decoder = png::Decoder::new(cursor);
    decoder.set_transformations(png::Transformations::EXPAND | png::Transformations::STRIP_16);
    let mut reader = match decoder.read_info() {
        Ok(r) => r,
        Err(_) => return Vec::new(),
    };
    let buf_size = match reader.output_buffer_size() {
        Some(s) => s,
        None => return Vec::new(),
    };
    let mut buf = vec![0; buf_size];
    let output_info = match reader.next_frame(&mut buf) {
        Ok(info) => info,
        Err(_) => return Vec::new(),
    };
    let width = output_info.width as usize;
    let height = output_info.height as usize;
    if width == 0 || height == 0 {
        return Vec::new();
    }
    let line_size = output_info.line_size;
    if line_size < width {
        return Vec::new();
    }
    let bytes_per_pixel = line_size / width;
    if bytes_per_pixel == 0 || bytes_per_pixel > 4 {
        return Vec::new();
    }
    if let Some(total_src_bytes) = line_size.checked_mul(height) {
        if total_src_bytes > buf.len() {
            return Vec::new();
        }
    } else {
        return Vec::new();
    }

    let out_size = match width.checked_mul(height).and_then(|wh| wh.checked_mul(4)) {
        Some(s) => s,
        None => return Vec::new(),
    };
    let mut out = vec![0u8; out_size];

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
        let dst_start = y * width * 4;
        let dst_slice = &mut out[dst_start..dst_start + width * 4];

        for x in 0..width {
            let (r, g, b, a) = match bytes_per_pixel {
                1 => {
                    let gray = gamma_lut[src_row[x] as usize];
                    (gray, gray, gray, 255)
                }
                2 => {
                    let gray = gamma_lut[src_row[x * 2] as usize];
                    let alpha = src_row[x * 2 + 1];
                    (gray, gray, gray, alpha)
                }
                3 => {
                    let red = gamma_lut[src_row[x * 3] as usize];
                    let green = gamma_lut[src_row[x * 3 + 1] as usize];
                    let blue = gamma_lut[src_row[x * 3 + 2] as usize];
                    (red, green, blue, 255)
                }
                4 => {
                    let red = gamma_lut[src_row[x * 4] as usize];
                    let green = gamma_lut[src_row[x * 4 + 1] as usize];
                    let blue = gamma_lut[src_row[x * 4 + 2] as usize];
                    let alpha = src_row[x * 4 + 3];
                    (red, green, blue, alpha)
                }
                _ => return Vec::new(),
            };

            if reverse_byte_order {
                dst_slice[x * 4] = b;
                dst_slice[x * 4 + 1] = g;
                dst_slice[x * 4 + 2] = r;
                dst_slice[x * 4 + 3] = a;
            } else {
                dst_slice[x * 4] = r;
                dst_slice[x * 4 + 1] = g;
                dst_slice[x * 4 + 2] = b;
                dst_slice[x * 4 + 3] = a;
            }
        }
    }

    *width_out = width as i32;
    *height_out = height as i32;
    out
}

fn write_png(data: &[u8], width: u32, height: u32, color_type: png::ColorType) -> Vec<u8> {
    let mut out = Vec::new();
    {
        let mut encoder = png::Encoder::new(&mut out, width, height);
        encoder.set_color(color_type);
        encoder.set_depth(png::BitDepth::Eight);
        let mut writer = match encoder.write_header() {
            Ok(w) => w,
            Err(_) => return Vec::new(),
        };
        if writer.write_image_data(data).is_err() {
            return Vec::new();
        }
    }
    out
}

fn encode_bgr(input: &[u8], width: i32, height: i32, row_byte_width: usize) -> Vec<u8> {
    if width <= 0 || height <= 0 {
        return Vec::new();
    }
    let w = width as usize;
    let h = height as usize;
    if row_byte_width < w * 3 {
        return Vec::new();
    }
    if let Some(total_input) = row_byte_width.checked_mul(h) {
        if total_input > input.len() {
            return Vec::new();
        }
    } else {
        return Vec::new();
    }

    let mut rgb = vec![0u8; w * h * 3];
    for y in 0..h {
        let src_row = &input[y * row_byte_width..y * row_byte_width + w * 3];
        let dst_row = &mut rgb[y * w * 3..(y + 1) * w * 3];
        for x in 0..w {
            dst_row[x * 3] = src_row[x * 3 + 2]; // R
            dst_row[x * 3 + 1] = src_row[x * 3 + 1]; // G
            dst_row[x * 3 + 2] = src_row[x * 3]; // B
        }
    }
    write_png(&rgb, width as u32, height as u32, png::ColorType::Rgb)
}

fn encode_rgba(input: &[u8], width: i32, height: i32, row_byte_width: usize) -> Vec<u8> {
    if width <= 0 || height <= 0 {
        return Vec::new();
    }
    let w = width as usize;
    let h = height as usize;
    if row_byte_width < w * 4 {
        return Vec::new();
    }
    if let Some(total_input) = row_byte_width.checked_mul(h) {
        if total_input > input.len() {
            return Vec::new();
        }
    } else {
        return Vec::new();
    }

    if row_byte_width == w * 4 {
        return write_png(&input[..w * h * 4], width as u32, height as u32, png::ColorType::Rgba);
    }

    let mut rgba = vec![0u8; w * h * 4];
    for y in 0..h {
        let src_row = &input[y * row_byte_width..y * row_byte_width + w * 4];
        let dst_row = &mut rgba[y * w * 4..(y + 1) * w * 4];
        dst_row.copy_from_slice(src_row);
    }
    write_png(&rgba, width as u32, height as u32, png::ColorType::Rgba)
}

fn encode_bgra(
    input: &[u8],
    width: i32,
    height: i32,
    row_byte_width: usize,
    discard_transparency: bool,
) -> Vec<u8> {
    if width <= 0 || height <= 0 {
        return Vec::new();
    }
    let w = width as usize;
    let h = height as usize;
    if row_byte_width < w * 4 {
        return Vec::new();
    }
    if let Some(total_input) = row_byte_width.checked_mul(h) {
        if total_input > input.len() {
            return Vec::new();
        }
    } else {
        return Vec::new();
    }

    if discard_transparency {
        let mut rgb = vec![0u8; w * h * 3];
        for y in 0..h {
            let src_row = &input[y * row_byte_width..y * row_byte_width + w * 4];
            let dst_row = &mut rgb[y * w * 3..(y + 1) * w * 3];
            for x in 0..w {
                dst_row[x * 3] = src_row[x * 4 + 2]; // R
                dst_row[x * 3 + 1] = src_row[x * 4 + 1]; // G
                dst_row[x * 3 + 2] = src_row[x * 4]; // B
            }
        }
        write_png(&rgb, width as u32, height as u32, png::ColorType::Rgb)
    } else {
        let mut rgba = vec![0u8; w * h * 4];
        for y in 0..h {
            let src_row = &input[y * row_byte_width..y * row_byte_width + w * 4];
            let dst_row = &mut rgba[y * w * 4..(y + 1) * w * 4];
            for x in 0..w {
                dst_row[x * 4] = src_row[x * 4 + 2]; // R
                dst_row[x * 4 + 1] = src_row[x * 4 + 1]; // G
                dst_row[x * 4 + 2] = src_row[x * 4]; // B
                dst_row[x * 4 + 3] = src_row[x * 4 + 3]; // A
            }
        }
        write_png(&rgba, width as u32, height as u32, png::ColorType::Rgba)
    }
}

fn encode_gray(input: &[u8], width: i32, height: i32, row_byte_width: usize) -> Vec<u8> {
    if width <= 0 || height <= 0 {
        return Vec::new();
    }
    let w = width as usize;
    let h = height as usize;
    if row_byte_width < w {
        return Vec::new();
    }
    if let Some(total_input) = row_byte_width.checked_mul(h) {
        if total_input > input.len() {
            return Vec::new();
        }
    } else {
        return Vec::new();
    }

    if row_byte_width == w {
        return write_png(&input[..w * h], width as u32, height as u32, png::ColorType::Grayscale);
    }

    let mut gray = vec![0u8; w * h];
    for y in 0..h {
        let src_row = &input[y * row_byte_width..y * row_byte_width + w];
        let dst_row = &mut gray[y * w..(y + 1) * w];
        dst_row.copy_from_slice(src_row);
    }
    write_png(&gray, width as u32, height as u32, png::ColorType::Grayscale)
}
