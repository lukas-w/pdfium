// Copyright 2011 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file input format is based loosely on
// Tools/DumpRenderTree/ImageDiff.m

// The exact format of this tool's output to stdout is important, to match
// what the run-webkit-tests script expects.

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/numerics/safe_conversions.h"
#include "core/fxcrt/span_io.h"
#include "testing/png_codec/png_codec.h"
#include "testing/utils/path_service.h"
#include "testing/utils/pixel_diff_util.h"

#if BUILDFLAG(IS_WIN)
#include <windows.h>
#endif

// Return codes used by this utility.
constexpr int kStatusSame = 0;
constexpr int kStatusDifferent = 1;
constexpr int kStatusError = 2;

// Color codes.
constexpr uint32_t RGBA_RED = 0x000000ff;
constexpr uint32_t RGBA_ALPHA = 0xff000000;

class Image {
 public:
  Image() = default;
  Image(int w, int h)
      : w_(w), h_(h), data_(static_cast<size_t>(w) * h * 4, 0) {}
  Image(const Image& image) = default;
  Image& operator=(const Image& other) = default;

  bool has_image() const { return w_ > 0 && h_ > 0; }
  int w() const { return w_; }
  int h() const { return h_; }
  pdfium::span<const uint8_t> span() const { return data_; }

  // Creates the image from the given filename on disk, and returns true on
  // success.
  bool CreateFromFilename(const std::string& path) {
    return CreateFromFilenameImpl(path, /*reverse_byte_order=*/false);
  }

  // Same as CreateFromFilename(), but with BGRA instead of RGBA ordering.
  bool CreateFromFilenameWithReverseByteOrder(const std::string& path) {
    return CreateFromFilenameImpl(path, /*reverse_byte_order=*/true);
  }

  void Clear() {
    w_ = h_ = 0;
    data_.clear();
  }

  // Returns the RGBA value of the pixel at the given location
  uint32_t pixel_at(int x, int y) const {
    if (!pixel_in_bounds(x, y)) {
      return 0;
    }
    return *reinterpret_cast<const uint32_t*>(&(data_[pixel_address(x, y)]));
  }

  void set_pixel_at(int x, int y, uint32_t color) {
    if (!pixel_in_bounds(x, y)) {
      return;
    }

    void* addr = &data_[pixel_address(x, y)];
    *reinterpret_cast<uint32_t*>(addr) = color;
  }

 private:
  bool CreateFromFilenameImpl(const std::string& path,
                              bool reverse_byte_order) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) {
      return false;
    }

    std::vector<uint8_t> compressed;
    const size_t kBufSize = 1024;
    uint8_t buf[kBufSize];
    while (true) {
      auto read_span = fxcrt::spanread(buf, f);
      if (read_span.empty()) {
        break;
      }
      compressed.insert(compressed.end(), read_span.begin(), read_span.end());
    }

    fclose(f);

    data_ = png_codec::Decode(compressed, reverse_byte_order, &w_, &h_);
    if (data_.empty()) {
      Clear();
      return false;
    }
    return true;
  }

  bool pixel_in_bounds(int x, int y) const {
    return x >= 0 && x < w_ && y >= 0 && y < h_;
  }

  size_t pixel_address(int x, int y) const { return (y * w_ + x) * 4; }

  // Pixel dimensions of the image.
  int w_ = 0;
  int h_ = 0;

  std::vector<uint8_t> data_;
};

float CalculateDifferencePercentage(const Image& actual, int pixels_different) {
  // Like the WebKit ImageDiff tool, we define percentage different in terms
  // of the size of the 'actual' bitmap.
  float total_pixels =
      static_cast<float>(actual.w()) * static_cast<float>(actual.h());
  if (total_pixels == 0) {
    // When the bitmap is empty, they are 100% different.
    return 100.0f;
  }
  return 100.0f * pixels_different / total_pixels;
}

void CountImageSizeMismatchAsPixelDifference(const Image& baseline,
                                             const Image& actual,
                                             int* pixels_different) {
  int w = std::min(baseline.w(), actual.w());
  int h = std::min(baseline.h(), actual.h());

  // Count pixels that are a difference in size as also being different.
  int max_w = std::max(baseline.w(), actual.w());
  int max_h = std::max(baseline.h(), actual.h());
  // These pixels are off the right side, not including the lower right corner.
  *pixels_different += (max_w - w) * h;
  // These pixels are along the bottom, including the lower right corner.
  *pixels_different += (max_h - h) * max_w;
}

// Optional per-comparison measurements, reported by `--metrics`.
struct DiffMetrics {
  int raw_diff_pixels = 0;
  uint8_t max_delta = 0;
  double mse = 0.0;
  double win_mse = 0.0;
};

// Computes the number of pixels considered different under the given limits.
// When `out_metrics` is non-null, the underlying measurements are gathered as
// well, which requires computing values that the limits alone may not need.
int PixelsDifferent(const Image& baseline,
                    const Image& actual,
                    uint8_t max_pixel_per_channel_delta,
                    double max_mean_squared_error,
                    int window_size,
                    double max_window_mean_squared_error,
                    DiffMetrics* out_metrics) {
  int w = std::min(baseline.w(), actual.w());
  int h = std::min(baseline.h(), actual.h());

  int raw_diff_pixels = 0;
  uint8_t max_delta = 0;
  int pixels_different = 0;
  uint64_t total_squared_error = 0;
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const uint32_t baseline_pixel = baseline.pixel_at(x, y);
      const uint32_t actual_pixel = actual.pixel_at(x, y);
      if (baseline_pixel == actual_pixel) {
        continue;
      }

      ++raw_diff_pixels;
      uint8_t delta = MaxPixelPerChannelDelta(baseline_pixel, actual_pixel);
      max_delta = std::max(max_delta, delta);

      if (delta > max_pixel_per_channel_delta) {
        ++pixels_different;
      }
      total_squared_error += PixelSquaredError(baseline_pixel, actual_pixel);
    }
  }

  double mse = 0.0;
  if (w > 0 && h > 0 && (max_mean_squared_error > 0.0 || out_metrics)) {
    mse = static_cast<double>(total_squared_error) / (3.0 * w * h);
    if (max_mean_squared_error > 0.0 && mse > max_mean_squared_error) {
      ++pixels_different;
    }
  }

  double win_mse = 0.0;
  if (w > 0 && h > 0 &&
      ((window_size > 0 && max_window_mean_squared_error > 0.0) ||
       out_metrics)) {
    std::vector<uint32_t> baseline_overlap(w * h);
    std::vector<uint32_t> actual_overlap(w * h);
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        baseline_overlap[y * w + x] = baseline.pixel_at(x, y);
        actual_overlap[y * w + x] = actual.pixel_at(x, y);
      }
    }
    int eff_win_size = window_size > 0 ? window_size : kMaxFuzzyWindowSize;
    win_mse = CalculateMaxWindowMSE(baseline_overlap, static_cast<size_t>(w),
                                    actual_overlap, static_cast<size_t>(w), w,
                                    h, eff_win_size);
    if (max_window_mean_squared_error > 0.0 &&
        win_mse > max_window_mean_squared_error) {
      ++pixels_different;
    }
  }

  CountImageSizeMismatchAsPixelDifference(baseline, actual, &pixels_different);

  if (out_metrics) {
    out_metrics->raw_diff_pixels = raw_diff_pixels;
    out_metrics->max_delta = max_delta;
    out_metrics->mse = mse;
    out_metrics->win_mse = win_mse;
  }

  return pixels_different;
}

int HistogramPixelsDifferent(const Image& baseline, const Image& actual) {
  // TODO(johnme): Consider using a joint histogram instead, as described in
  // "Comparing Images Using Joint Histograms" by Pass & Zabih
  // http://www.cs.cornell.edu/~rdz/papers/pz-jms99.pdf

  int w = std::min(baseline.w(), actual.w());
  int h = std::min(baseline.h(), actual.h());

  // Count occurrences of each RGBA pixel value of baseline in the overlap.
  std::map<uint32_t, int32_t> baseline_histogram;
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      // hash_map operator[] inserts a 0 (default constructor) if key not found.
      ++baseline_histogram[baseline.pixel_at(x, y)];
    }
  }

  // Compute pixels different in the histogram of the overlap.
  int pixels_different = 0;
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      uint32_t actual_rgba = actual.pixel_at(x, y);
      auto it = baseline_histogram.find(actual_rgba);
      if (it != baseline_histogram.end() && it->second > 0) {
        --it->second;
      } else {
        ++pixels_different;
      }
    }
  }

  CountImageSizeMismatchAsPixelDifference(baseline, actual, &pixels_different);
  return pixels_different;
}

void PrintHelp(const std::string& binary_name) {
  fprintf(
      stderr,
      "Usage:\n"
      "  %s OPTIONS <compare_file> <reference_file>\n"
      "    Compares two files on disk, returning 0 when they are the same.\n"
      "    Passing \"--histogram\" additionally calculates a diff of the\n"
      "    RGBA value histograms (which is resistant to shifts in layout).\n"
      "    Passing \"--reverse-byte-order\" additionally assumes the\n"
      "    compare file has BGRA byte ordering.\n"
      "    Passing \"--fuzzy[=delta,mse,win_mse]\" additionally allows\n"
      "    individual pixels and windows to differ within specified limits.\n"
      "    Passing \"--metrics\" additionally prints a machine-readable line\n"
      "    of measurements.\n"
      "    Under \"--histogram\" or \"--metrics\", the comparison result is\n"
      "    reported in the output rather than through the exit code, which\n"
      "    then indicates only whether the comparison could be performed.\n\n"
      "  %s --diff <compare_file> <reference_file> <output_file>\n"
      "    Compares two files on disk, and if they differ, outputs an image\n"
      "    to <output_file> that visualizes the differing pixels as red\n"
      "    dots.\n\n"
      "  %s --subtract <compare_file> <reference_file> <output_file>\n"
      "    Compares two files on disk, and if they differ, outputs an image\n"
      "    to <output_file> that visualizes the difference as a scaled\n"
      "    subtraction of pixel values.\n",
      binary_name.c_str(), binary_name.c_str(), binary_name.c_str());
}

int CompareImages(const std::string& binary_name,
                  const std::string& file1,
                  const std::string& file2,
                  bool compare_histograms,
                  bool reverse_byte_order,
                  bool report_metrics,
                  uint8_t max_pixel_per_channel_delta,
                  double max_mean_squared_error,
                  int window_size,
                  double max_window_mean_squared_error) {
  Image actual_image;
  Image baseline_image;

  bool actual_load_result =
      reverse_byte_order
          ? actual_image.CreateFromFilenameWithReverseByteOrder(file1)
          : actual_image.CreateFromFilename(file1);
  if (!actual_load_result) {
    fprintf(stderr, "%s: Unable to open file \"%s\"\n", binary_name.c_str(),
            file1.c_str());
    return kStatusError;
  }
  if (!baseline_image.CreateFromFilename(file2)) {
    fprintf(stderr, "%s: Unable to open file \"%s\"\n", binary_name.c_str(),
            file2.c_str());
    return kStatusError;
  }

  if (compare_histograms) {
    int pixels_different =
        HistogramPixelsDifferent(actual_image, baseline_image);
    float percent =
        CalculateDifferencePercentage(actual_image, pixels_different);
    const char* passed = percent > 0.0 ? "failed" : "passed";
    UNSAFE_TODO(printf("histogram diff: %01.2f%% %s (%d pixels differ)\n",
                       percent, passed, pixels_different));
  }

  const char* const diff_name = compare_histograms ? "exact diff" : "diff";
  DiffMetrics metrics;
  int pixels_different = PixelsDifferent(
      actual_image, baseline_image, max_pixel_per_channel_delta,
      max_mean_squared_error, window_size, max_window_mean_squared_error,
      report_metrics ? &metrics : nullptr);

  float percent = CalculateDifferencePercentage(actual_image, pixels_different);
  const char* const passed = percent > 0.0 ? "failed" : "passed";

  if (report_metrics) {
    int overlap_w = std::min(actual_image.w(), baseline_image.w());
    int overlap_h = std::min(actual_image.h(), baseline_image.h());
    const size_t last_separator = file1.rfind(PATH_SEPARATOR);
    std::string base_name = last_separator != std::string::npos
                                ? file1.substr(last_separator + 1)
                                : file1;

    printf(
        "%s: actual_w=%d actual_h=%d expected_w=%d expected_h=%d pixels=%d "
        "total=%d max_delta=%u mse=%.6f win_mse=%.6f c_result=%s\n",
        base_name.c_str(), actual_image.w(), actual_image.h(),
        baseline_image.w(), baseline_image.h(), metrics.raw_diff_pixels,
        overlap_w * overlap_h, metrics.max_delta, metrics.mse, metrics.win_mse,
        percent > 0.0 ? "FAIL" : "PASS");
  }

  UNSAFE_TODO(printf("%s: %01.2f%% %s (%d pixels differ)\n", diff_name, percent,
                     passed, pixels_different));

  if (compare_histograms || report_metrics) {
    // Measurement modes: the result is reported in the output above, leaving
    // the exit code to indicate only whether the comparison could be
    // performed at all.
    return kStatusSame;
  }
  if (percent > 0.0) {
    // failure: The WebKit version also writes the difference image to
    // stdout, which seems excessive for our needs.
    return kStatusDifferent;
  }
  // success
  return kStatusSame;
}

bool CreateImageDiff(const Image& image1, const Image& image2, Image* out) {
  int union_w = std::max(image1.w(), image2.w());
  int union_h = std::max(image1.h(), image2.h());
  int inter_w = std::min(image1.w(), image2.w());
  int inter_h = std::min(image1.h(), image2.h());
  *out = Image(union_w, union_h);
  bool same = (image1.w() == image2.w()) && (image1.h() == image2.h());

  for (int y = 0; y < union_h; ++y) {
    for (int x = 0; x < union_w; ++x) {
      if (x < inter_w && y < inter_h) {
        uint32_t base_pixel = image1.pixel_at(x, y);
        if (base_pixel != image2.pixel_at(x, y)) {
          // Set differing pixels red.
          out->set_pixel_at(x, y, RGBA_RED | RGBA_ALPHA);
          same = false;
        } else {
          // Set same pixels as faded.
          uint32_t alpha = base_pixel & RGBA_ALPHA;
          uint32_t new_pixel = base_pixel - ((alpha / 2) & RGBA_ALPHA);
          out->set_pixel_at(x, y, new_pixel);
        }
      } else {
        // Set out-of-bounds pixels red.
        out->set_pixel_at(x, y, RGBA_RED | RGBA_ALPHA);
        same = false;
      }
    }
  }

  return same;
}

bool SubtractImages(const Image& image1, const Image& image2, Image* out) {
  int union_w = std::max(image1.w(), image2.w());
  int union_h = std::max(image1.h(), image2.h());
  int inter_w = std::min(image1.w(), image2.w());
  int inter_h = std::min(image1.h(), image2.h());
  *out = Image(union_w, union_h);
  bool same = (image1.w() == image2.w()) && (image1.h() == image2.h());

  for (int y = 0; y < union_h; ++y) {
    for (int x = 0; x < union_w; ++x) {
      if (x < inter_w && y < inter_h) {
        uint32_t pixel1 = image1.pixel_at(x, y);
        int32_t r1 = pixel1 & 0xff;
        int32_t g1 = (pixel1 >> 8) & 0xff;
        int32_t b1 = (pixel1 >> 16) & 0xff;

        uint32_t pixel2 = image2.pixel_at(x, y);
        int32_t r2 = pixel2 & 0xff;
        int32_t g2 = (pixel2 >> 8) & 0xff;
        int32_t b2 = (pixel2 >> 16) & 0xff;

        int32_t delta_r = r1 - r2;
        int32_t delta_g = g1 - g2;
        int32_t delta_b = b1 - b2;
        same &= (delta_r == 0 && delta_g == 0 && delta_b == 0);

        delta_r = std::clamp(128 + delta_r * 8, 0, 255);
        delta_g = std::clamp(128 + delta_g * 8, 0, 255);
        delta_b = std::clamp(128 + delta_b * 8, 0, 255);

        uint32_t new_pixel = RGBA_ALPHA;
        new_pixel |= delta_r;
        new_pixel |= (delta_g << 8);
        new_pixel |= (delta_b << 16);
        out->set_pixel_at(x, y, new_pixel);
      } else {
        out->set_pixel_at(x, y, RGBA_RED | RGBA_ALPHA);
        same = false;
      }
    }
  }
  return same;
}

int DiffImages(const std::string& binary_name,
               const std::string& file1,
               const std::string& file2,
               const std::string& out_file,
               bool do_subtraction,
               bool reverse_byte_order) {
  Image actual_image;
  Image baseline_image;

  bool actual_load_result =
      reverse_byte_order
          ? actual_image.CreateFromFilenameWithReverseByteOrder(file1)
          : actual_image.CreateFromFilename(file1);

  if (!actual_load_result) {
    fprintf(stderr, "%s: Unable to open file \"%s\"\n", binary_name.c_str(),
            file1.c_str());
    return kStatusError;
  }
  if (!baseline_image.CreateFromFilename(file2)) {
    fprintf(stderr, "%s: Unable to open file \"%s\"\n", binary_name.c_str(),
            file2.c_str());
    return kStatusError;
  }

  Image diff_image;
  bool same = do_subtraction
                  ? SubtractImages(baseline_image, actual_image, &diff_image)
                  : CreateImageDiff(baseline_image, actual_image, &diff_image);
  if (same) {
    return kStatusSame;
  }

  std::vector<uint8_t> png_encoding = png_codec::EncodeRGBA(
      diff_image.span(), diff_image.w(), diff_image.h(), diff_image.w() * 4);
  if (png_encoding.empty()) {
    return kStatusError;
  }

  FILE* f = fopen(out_file.c_str(), "wb");
  if (!f) {
    return kStatusError;
  }

  if (fxcrt::spanwrite(png_encoding, f) != png_encoding.size()) {
    return kStatusError;
  }

  return kStatusDifferent;
}

// Parses `str` as a whole into `value`, returning false if it contains
// anything the conversion does not consume.
bool ParseUnsigned(const std::string& str, unsigned int* value) {
  int consumed = 0;
  return UNSAFE_TODO(sscanf(str.c_str(), "%u%n", value, &consumed)) == 1 &&
         static_cast<size_t>(consumed) == str.size();
}

bool ParseDouble(const std::string& str, double* value) {
  int consumed = 0;
  return UNSAFE_TODO(sscanf(str.c_str(), "%lf%n", value, &consumed)) == 1 &&
         static_cast<size_t>(consumed) == str.size();
}

// Parses the limits in a "--fuzzy=<delta>,<mse>,<win_mse>" argument. Each
// field may be omitted, leaving the corresponding limit unchanged. Returns
// false if a field is present but malformed or out of range. Must agree with
// the parsing that `testing/tools/pngdiffer.py` performs on the same flag.
bool ParseFuzzyOption(const std::string& arg,
                      uint8_t* max_pixel_per_channel_delta,
                      double* max_mean_squared_error,
                      double* max_window_mean_squared_error) {
  const std::string value = arg.substr(strlen("--fuzzy="));
  std::vector<std::string> fields;
  for (size_t start = 0;;) {
    size_t comma = value.find(',', start);
    if (comma == std::string::npos) {
      fields.push_back(value.substr(start));
      break;
    }
    fields.push_back(value.substr(start, comma - start));
    start = comma + 1;
  }
  if (fields.size() > 3) {
    return false;
  }

  if (!fields[0].empty()) {
    unsigned int delta = 0;
    if (!ParseUnsigned(fields[0], &delta) || delta > 255) {
      return false;
    }
    *max_pixel_per_channel_delta = static_cast<uint8_t>(delta);
  }
  if (fields.size() > 1 && !fields[1].empty() &&
      !ParseDouble(fields[1], max_mean_squared_error)) {
    return false;
  }
  if (fields.size() > 2 && !fields[2].empty() &&
      !ParseDouble(fields[2], max_window_mean_squared_error)) {
    return false;
  }
  return true;
}

int main(int argc, const char* argv[]) {
  bool histograms = false;
  bool produce_diff_image = false;
  bool produce_image_subtraction = false;
  bool reverse_byte_order = false;
  bool report_metrics = false;
  uint8_t max_pixel_per_channel_delta = 0;
  double max_mean_squared_error = 0.0;
  int window_size = 0;
  double max_window_mean_squared_error = 0.0;
  std::string filename1;
  std::string filename2;
  std::string diff_filename;

  // Strip the path from the first arg
  const char* last_separator = UNSAFE_TODO(strrchr(argv[0], PATH_SEPARATOR));
  std::string binary_name =
      last_separator ? UNSAFE_TODO(last_separator + 1) : argv[0];

  int i;
  for (i = 1; i < argc; ++i) {
    const char* arg = UNSAFE_TODO(argv[i]);
    if (UNSAFE_TODO(strstr(arg, "--")) != arg) {
      break;
    }
    if (UNSAFE_TODO(strcmp(arg, "--histogram")) == 0) {
      histograms = true;
    } else if (UNSAFE_TODO(strcmp(arg, "--diff")) == 0) {
      produce_diff_image = true;
    } else if (UNSAFE_TODO(strcmp(arg, "--subtract")) == 0) {
      produce_image_subtraction = true;
    } else if (UNSAFE_TODO(strcmp(arg, "--reverse-byte-order")) == 0) {
      reverse_byte_order = true;
    } else if (UNSAFE_TODO(strcmp(arg, "--metrics")) == 0) {
      report_metrics = true;
    } else if (UNSAFE_TODO(strcmp(arg, "--fuzzy")) == 0) {
      max_pixel_per_channel_delta = kMaxFuzzyPixelDelta;
      max_mean_squared_error = kMaxFuzzyMeanSquaredError;
      window_size = kMaxFuzzyWindowSize;
      max_window_mean_squared_error = kMaxFuzzyWindowMeanSquaredError;
    } else if (UNSAFE_TODO(strstr(arg, "--fuzzy=")) == arg) {
      max_pixel_per_channel_delta = kMaxFuzzyPixelDelta;
      max_mean_squared_error = kMaxFuzzyMeanSquaredError;
      window_size = kMaxFuzzyWindowSize;
      max_window_mean_squared_error = kMaxFuzzyWindowMeanSquaredError;
      const std::string fuzzy_arg = arg;
      if (!ParseFuzzyOption(fuzzy_arg, &max_pixel_per_channel_delta,
                            &max_mean_squared_error,
                            &max_window_mean_squared_error)) {
        fprintf(stderr, "%s: Invalid fuzzy option \"%s\"\n",
                binary_name.c_str(), fuzzy_arg.c_str());
        return kStatusError;
      }
    }
  }
  if (i < argc) {
    filename1 = UNSAFE_TODO(argv[i++]);
  }
  if (i < argc) {
    filename2 = UNSAFE_TODO(argv[i++]);
  }
  if (i < argc) {
    diff_filename = UNSAFE_TODO(argv[i++]);
  }

  if (produce_diff_image || produce_image_subtraction) {
    if (!diff_filename.empty()) {
      return DiffImages(binary_name, filename1, filename2, diff_filename,
                        produce_image_subtraction, reverse_byte_order);
    }
  } else if (!filename2.empty()) {
    return CompareImages(binary_name, filename1, filename2, histograms,
                         reverse_byte_order, report_metrics,
                         max_pixel_per_channel_delta, max_mean_squared_error,
                         window_size, max_window_mean_squared_error);
  }

  PrintHelp(binary_name);
  return kStatusError;
}
