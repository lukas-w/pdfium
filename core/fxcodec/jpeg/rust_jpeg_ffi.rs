// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

use zune_jpeg::zune_core::bytestream::ZCursor;
use zune_jpeg::zune_core::colorspace::ColorSpace;
use zune_jpeg::JpegDecoder;

#[cxx::bridge(namespace = "fxcodec::rust_jpeg")]
mod ffi {
    pub struct JpegHeaderInfo {
        pub width: u32,
        pub height: u32,
        pub num_components: i32,
        pub bits_per_component: i32,
        pub color_transform: bool,
        pub x_density: u16,
        pub y_density: u16,
        pub density_unit: u8,
        pub max_h_samp: u8,
        pub max_v_samp: u8,
    }

    extern "Rust" {
        fn read_jpeg_info(src: &[u8], info: &mut JpegHeaderInfo) -> bool;
        fn decode_jpeg_to_buf(
            src: &[u8],
            out_buf: &mut [u8],
            out_pitch: usize,
            scale_denom: u32,
        ) -> bool;
    }
}

fn parse_jfif_density(src: &[u8]) -> (u16, u16, u8) {
    let mut pos = 0;
    while pos + 1 < src.len() {
        if src[pos] == 0xff && src[pos + 1] == 0xd8 {
            pos += 2;
            break;
        }
        pos += 1;
    }
    while pos + 3 < src.len() {
        if src[pos] != 0xff {
            break;
        }
        let marker = src[pos + 1];
        if marker == 0xff {
            pos += 1;
            continue;
        }
        pos += 2;
        if marker == 0xd8 || marker == 0xd9 || marker == 0x00 {
            continue;
        }
        if pos + 2 > src.len() {
            break;
        }
        let len = u16::from_be_bytes([src[pos], src[pos + 1]]) as usize;
        if marker == 0xe0 && len >= 16 && pos + len <= src.len() {
            let payload = &src[pos + 2..pos + len];
            if payload.starts_with(b"JFIF\0") && payload.len() >= 12 {
                let density_unit = payload[7];
                let x_density = u16::from_be_bytes([payload[8], payload[9]]);
                let y_density = u16::from_be_bytes([payload[10], payload[11]]);
                return (x_density, y_density, density_unit);
            }
        }
        pos += len;
    }
    (0, 0, 0)
}

fn read_jpeg_info(src: &[u8], info: &mut ffi::JpegHeaderInfo) -> bool {
    let cursor = ZCursor::new(src);
    let mut decoder = JpegDecoder::new(cursor);
    if decoder.decode_headers().is_err() {
        return false;
    }
    let img_info = match decoder.info() {
        Some(i) => i,
        None => return false,
    };
    info.width = img_info.width as u32;
    info.height = img_info.height as u32;
    info.num_components = img_info.components as i32;
    info.bits_per_component = 8;
    info.color_transform =
        matches!(decoder.input_colorspace(), Some(ColorSpace::YCbCr) | Some(ColorSpace::YCCK));
    let (max_h_samp, max_v_samp) = match img_info.sample_ratio {
        zune_jpeg::SampleRatios::None => (1, 1),
        zune_jpeg::SampleRatios::H => (2, 1),
        zune_jpeg::SampleRatios::V => (1, 2),
        zune_jpeg::SampleRatios::HV => (2, 2),
        zune_jpeg::SampleRatios::Generic(h, v) => (h as u8, v as u8),
    };
    info.max_h_samp = max_h_samp;
    info.max_v_samp = max_v_samp;
    let (x_density, y_density, density_unit) = parse_jfif_density(src);
    info.x_density = x_density;
    info.y_density = y_density;
    info.density_unit = density_unit;
    true
}

fn ycck_to_cmyk_inplace(pixels: &mut [u8]) {
    for chunk in pixels.chunks_exact_mut(4) {
        let y = chunk[0] as i32;
        let cb = chunk[1] as i32 - 128;
        let cr = chunk[2] as i32 - 128;
        let k = chunk[3];

        // 16-bit fixed-point scaling (SCALE = 65536, ONE_HALF = 32768).
        let r = y + ((91881 * cr + 32768) >> 16);
        let g = y + ((-22554 * cb - 46802 * cr + 32768) >> 16);
        let b = y + ((116130 * cb + 32768) >> 16);

        chunk[0] = (255 - r).clamp(0, 255) as u8;
        chunk[1] = (255 - g).clamp(0, 255) as u8;
        chunk[2] = (255 - b).clamp(0, 255) as u8;
        chunk[3] = k;
    }
}

fn decode_jpeg_to_buf(src: &[u8], out_buf: &mut [u8], out_pitch: usize, scale_denom: u32) -> bool {
    let cursor = ZCursor::new(src);
    let mut decoder = JpegDecoder::new(cursor);
    if decoder.decode_headers().is_err() {
        return false;
    }
    let img_info = match decoder.info() {
        Some(i) => i,
        None => return false,
    };
    let is_ycck = decoder.input_colorspace() == Some(ColorSpace::YCCK);
    if img_info.components == 1 {
        let options = decoder.options().jpeg_set_out_colorspace(ColorSpace::Luma);
        decoder.set_options(options);
    } else if img_info.components == 4 {
        let out_cs = if is_ycck { ColorSpace::YCCK } else { ColorSpace::CMYK };
        let options = decoder.options().jpeg_set_out_colorspace(out_cs);
        decoder.set_options(options);
    }
    let output_colorspace = match decoder.output_colorspace() {
        Some(cs) => cs,
        None => return false,
    };
    let mut pixels = match decoder.decode() {
        Ok(p) => p,
        Err(_) => return false,
    };
    if is_ycck {
        ycck_to_cmyk_inplace(&mut pixels);
    }
    let orig_width = img_info.width as usize;
    let orig_height = img_info.height as usize;
    let num_comps = output_colorspace.num_components();
    let src_row_len = orig_width * num_comps;

    if pixels.len() < orig_height * src_row_len {
        return false;
    }

    let scale = (scale_denom as usize).max(1);
    let out_width = orig_width.div_ceil(scale);
    let out_height = orig_height.div_ceil(scale);
    let dst_row_len = out_width * num_comps;

    if scale == 1 {
        for y in 0..out_height {
            let src_row = &pixels[y * src_row_len..(y + 1) * src_row_len];
            let dst_start = y * out_pitch;
            if dst_start + dst_row_len > out_buf.len() {
                return false;
            }
            out_buf[dst_start..dst_start + dst_row_len].copy_from_slice(src_row);
        }
    } else {
        for out_y in 0..out_height {
            let dst_start = out_y * out_pitch;
            if dst_start + dst_row_len > out_buf.len() {
                return false;
            }
            let dst_row = &mut out_buf[dst_start..dst_start + dst_row_len];
            let y_start = out_y * scale;
            let y_end = (y_start + scale).min(orig_height);

            for out_x in 0..out_width {
                let x_start = out_x * scale;
                let x_end = (x_start + scale).min(orig_width);
                let count = (y_end - y_start) * (x_end - x_start);
                let dst_pixel = &mut dst_row[out_x * num_comps..(out_x + 1) * num_comps];

                if count == 0 {
                    continue;
                }

                for c in 0..num_comps {
                    let mut sum: u32 = 0;
                    for sy in y_start..y_end {
                        let row_offset = sy * src_row_len;
                        for sx in x_start..x_end {
                            sum += pixels[row_offset + sx * num_comps + c] as u32;
                        }
                    }
                    dst_pixel[c] = ((sum + (count as u32 / 2)) / count as u32) as u8;
                }
            }
        }
    }
    true
}
