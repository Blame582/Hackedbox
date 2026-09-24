// ImageLoader.cpp for Hackedbox - an X Window Manager
// Copyright (c) 2026 Kevin Day [blame582@gmail.com](mailto:blame582@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include "ImageLoader.hpp"

#include <png.h>
#include <jpeglib.h>
#include <webp/decode.h>
#include <cairo/cairo.h>
#include <librsvg/rsvg.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <setjmp.h>
#include <string>
#include <vector>

namespace
{

std::string lowerExtension(
const std::string &filename
)
{
const std::string::size_type slash =
filename.find_last_of("/\\");

const std::string::size_type dot =
filename.find_last_of('.');

if (dot == std::string::npos ||
(slash != std::string::npos && dot < slash))
{
return std::string();
}

std::string extension =
filename.substr(dot);

std::transform(
extension.begin(),
extension.end(),
extension.begin(),
[](unsigned char character)
{
return static_cast<char>(
std::tolower(character)
);
}
);

return extension;
}

struct JpegError
{
jpeg_error_mgr pub;
jmp_buf jump;
};

void jpegErrorExit(
j_common_ptr cinfo
)
{
JpegError *error =
reinterpret_cast<JpegError *>(cinfo->err);

longjmp(
error->jump,
1
);
}

} // namespace

bool HbImageLoader::load(
const std::string &filename,
HbImageData &image,
std::string &error
)
{
image = HbImageData();
error.clear();

if (filename.empty())
{
error = "no image filename specified";
return false;
}

const std::string extension =
lowerExtension(filename);

if (extension == ".png")
{
return loadPNG(
filename,
image,
error
);
}

if (extension == ".jpg" ||
extension == ".jpeg")
{
return loadJPEG(
filename,
image,
error
);
}

if (extension == ".webp")
{
return loadWebP(
filename,
image,
error
);
}

if (extension == ".svg")
{
return loadSVG(
filename,
image,
error
);
}

error =
"unsupported image format '" +
extension +
"'";

return false;
}

bool HbImageLoader::loadPNG(
const std::string &filename,
HbImageData &image,
std::string &error
)
{
FILE *file =
std::fopen(
filename.c_str(),
"rb"
);

if (!file)
{
error =
"unable to open file";

return false;
}

png_byte signature[8];

if (std::fread(
signature,
1,
sizeof(signature),
file) != sizeof(signature))
{
std::fclose(file);

error =
"unable to read PNG signature";

return false;
}

if (png_sig_cmp(
signature,
0,
sizeof(signature)) != 0)
{
std::fclose(file);

error =
"invalid PNG signature";

return false;
}

png_structp png =
png_create_read_struct(
PNG_LIBPNG_VER_STRING,
nullptr,
nullptr,
nullptr
);

if (!png)
{
std::fclose(file);

error =
"unable to initialize libpng";

return false;
}

png_infop info =
png_create_info_struct(png);

if (!info)
{
png_destroy_read_struct(
&png,
nullptr,
nullptr
);

std::fclose(file);

error =
"unable to initialize PNG information";

return false;
}

if (setjmp(png_jmpbuf(png)))
{
png_destroy_read_struct(
&png,
&info,
nullptr
);

std::fclose(file);

image = HbImageData();

error =
"libpng failed while decoding image";

return false;
}

png_init_io(
png,
file
);

png_set_sig_bytes(
png,
sizeof(signature)
);

png_read_info(
png,
info
);

png_uint_32 width = 0;
png_uint_32 height = 0;
int bit_depth = 0;
int color_type = 0;
int interlace_type = 0;
int compression_type = 0;
int filter_method = 0;

png_get_IHDR(
png,
info,
&width,
&height,
&bit_depth,
&color_type,
&interlace_type,
&compression_type,
&filter_method
);

if (width == 0 ||
height == 0)
{
png_destroy_read_struct(
&png,
&info,
nullptr
);

std::fclose(file);

error =
"PNG has invalid dimensions";

return false;
}

if (bit_depth == 16)
png_set_strip_16(png);

if (color_type == PNG_COLOR_TYPE_PALETTE)
png_set_palette_to_rgb(png);

if (color_type == PNG_COLOR_TYPE_GRAY &&
bit_depth < 8)
{
png_set_expand_gray_1_2_4_to_8(png);
}

if (png_get_valid(
png,
info,
PNG_INFO_tRNS))
{
png_set_tRNS_to_alpha(png);
}

if (color_type == PNG_COLOR_TYPE_RGB ||
color_type == PNG_COLOR_TYPE_GRAY)
{
png_set_filler(
png,
0xff,
PNG_FILLER_AFTER
);
}

if (color_type == PNG_COLOR_TYPE_PALETTE)
{
if (!png_get_valid(
png,
info,
PNG_INFO_tRNS))
{
png_set_filler(
png,
0xff,
PNG_FILLER_AFTER
);
}
}

if (color_type == PNG_COLOR_TYPE_GRAY ||
color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
{
png_set_gray_to_rgb(png);
}

png_read_update_info(
png,
info
);

const png_size_t rowbytes =
png_get_rowbytes(
png,
info
);

if (rowbytes < static_cast<png_size_t>(width) * 4)
{
png_destroy_read_struct(
&png,
&info,
nullptr
);

std::fclose(file);

error =
"PNG decoder returned an invalid row size";

return false;
}

try
{
image.width =
static_cast<unsigned int>(width);

image.height =
static_cast<unsigned int>(height);

image.stride =
image.width * 4;

image.pixels.resize(
static_cast<size_t>(image.stride) *
static_cast<size_t>(image.height)
);

std::vector<png_bytep> rows(
image.height
);

for (unsigned int y = 0;
y < image.height;
++y)
{
rows[y] =
image.pixels.data() +
static_cast<size_t>(y) *
image.stride;
}

png_read_image(
png,
rows.data()
);

png_read_end(
png,
nullptr
);
}
catch (...)
{
png_destroy_read_struct(
&png,
&info,
nullptr
);

std::fclose(file);

image = HbImageData();

error =
"unable to allocate memory for PNG";

return false;
}

png_destroy_read_struct(
&png,
&info,
nullptr
);

std::fclose(file);

return true;
}

bool HbImageLoader::loadJPEG(
const std::string &filename,
HbImageData &image,
std::string &error
)
{
FILE *file =
std::fopen(
filename.c_str(),
"rb"
);

if (!file)
{
error =
"unable to open file";

return false;
}

jpeg_decompress_struct jpeg;
JpegError jpeg_error;

std::memset(
&jpeg,
0,
sizeof(jpeg)
);

std::memset(
&jpeg_error,
0,
sizeof(jpeg_error)
);

jpeg.err =
jpeg_std_error(
&jpeg_error.pub
);

jpeg_error.pub.error_exit =
jpegErrorExit;

if (setjmp(jpeg_error.jump))
{
jpeg_destroy_decompress(&jpeg);
std::fclose(file);

image = HbImageData();

error =
"libjpeg failed while decoding image";

return false;
}

jpeg_create_decompress(
&jpeg
);

jpeg_stdio_src(
&jpeg,
file
);

jpeg_read_header(
&jpeg,
TRUE
);

jpeg.out_color_space =
JCS_RGB;

jpeg_start_decompress(
&jpeg
);

if (jpeg.output_width == 0 ||
jpeg.output_height == 0)
{
jpeg_finish_decompress(&jpeg);
jpeg_destroy_decompress(&jpeg);
std::fclose(file);

error =
"JPEG has invalid dimensions";

return false;
}

image.width =
static_cast<unsigned int>(
jpeg.output_width
);

image.height =
static_cast<unsigned int>(
jpeg.output_height
);

image.stride =
image.width * 4;

try
{
image.pixels.resize(
static_cast<size_t>(image.stride) *
static_cast<size_t>(image.height)
);
}
catch (...)
{
jpeg_finish_decompress(&jpeg);
jpeg_destroy_decompress(&jpeg);
std::fclose(file);

image = HbImageData();

error =
"unable to allocate memory for JPEG";

return false;
}

const unsigned int source_stride =
image.width * 3;

std::vector<unsigned char> row;

try
{
row.resize(source_stride);
}
catch (...)
{
jpeg_finish_decompress(&jpeg);
jpeg_destroy_decompress(&jpeg);
std::fclose(file);

image = HbImageData();

error =
"unable to allocate JPEG scanline";

return false;
}

while (jpeg.output_scanline <
jpeg.output_height)
{
JSAMPROW scanline =
row.data();

jpeg_read_scanlines(
&jpeg,
&scanline,
1
);

const unsigned int y =
static_cast<unsigned int>(
jpeg.output_scanline - 1
);

unsigned char *destination =
image.pixels.data() +
static_cast<size_t>(y) *
image.stride;

for (unsigned int x = 0;
x < image.width;
++x)
{
const unsigned char *source =
row.data() +
static_cast<size_t>(x) * 3;

destination[
static_cast<size_t>(x) * 4
] = source[0];

destination[
static_cast<size_t>(x) * 4 + 1
] = source[1];

destination[
static_cast<size_t>(x) * 4 + 2
] = source[2];

destination[
static_cast<size_t>(x) * 4 + 3
] = 255;
}
}

jpeg_finish_decompress(
&jpeg
);

jpeg_destroy_decompress(
&jpeg
);

std::fclose(file);

return true;
}

bool HbImageLoader::loadWebP(
const std::string &filename,
HbImageData &image,
std::string &error
)
{
FILE *file =
std::fopen(
filename.c_str(),
"rb"
);

if (!file)
{
error =
"unable to open file";

return false;
}

if (std::fseek(
file,
0,
SEEK_END
) != 0)
{
std::fclose(file);

error =
"unable to seek WebP file";

return false;
}

const long file_size =
std::ftell(file);

if (file_size <= 0)
{
std::fclose(file);

error =
"invalid WebP file size";

return false;
}

if (std::fseek(
file,
0,
SEEK_SET
) != 0)
{
std::fclose(file);

error =
"unable to rewind WebP file";

return false;
}

std::vector<unsigned char> data;

try
{
data.resize(
static_cast<size_t>(file_size)
);
}
catch (...)
{
std::fclose(file);

error =
"unable to allocate memory for WebP";

return false;
}

if (std::fread(
data.data(),
1,
data.size(),
file
) != data.size())
{
std::fclose(file);

error =
"unable to read WebP file";

return false;
}

std::fclose(file);

int width = 0;
int height = 0;

if (!WebPGetInfo(
data.data(),
data.size(),
&width,
&height
))
{
error =
"invalid WebP image";

return false;
}

if (width <= 0 ||
height <= 0)
{
error =
"WebP has invalid dimensions";

return false;
}

int stride = 0;

unsigned char *decoded =
WebPDecodeRGBA(
data.data(),
data.size(),
&width,
&height
);

if (!decoded)
{
error =
"libwebp failed while decoding image";

return false;
}

try
{
image.width =
static_cast<unsigned int>(width);

image.height =
static_cast<unsigned int>(height);

image.stride =
static_cast<unsigned int>(width)*4;

image.pixels.resize(
static_cast<size_t>(image.stride) *
static_cast<size_t>(image.height)
);

for (unsigned int y = 0;
y < image.height;
++y)
{
const unsigned char *source =
decoded +
static_cast<size_t>(y) *
static_cast<size_t>(stride);

unsigned char *destination =
image.pixels.data() +
static_cast<size_t>(y) *
image.stride;

std::memcpy(
destination,
source,
image.stride
);
}
}
catch (...)
{
WebPFree(decoded);

image = HbImageData();

error =
"unable to allocate memory for WebP";

return false;
}

WebPFree(decoded);

return true;
}

bool HbImageLoader::loadSVG(
const std::string &filename,
HbImageData &image,
std::string &error
)
{
GError *svg_error = nullptr;

RsvgHandle *handle =
rsvg_handle_new_from_file(
filename.c_str(),
&svg_error
);

if (!handle)
{
if (svg_error)
{
error = svg_error->message;
g_error_free(svg_error);
}
else
{
error =
"librsvg failed to load SVG";
}

return false;
}

RsvgRectangle viewport;

viewport.x = 0.0;
viewport.y = 0.0;
viewport.width = 0.0;
viewport.height = 0.0;

/*
 * Obtain the SVG's intrinsic dimensions.
 */
RsvgRectangle ink_rect;
RsvgRectangle logical_rect;

if (!rsvg_handle_get_geometry_for_element(
handle,
nullptr,
&ink_rect,
&logical_rect,
&svg_error
))
{
if (svg_error)
{
error = svg_error->message;
g_error_free(svg_error);
}

g_object_unref(handle);

if (error.empty())
{
error =
"unable to determine SVG dimensions";
}

return false;
}

if (logical_rect.width <= 0.0 ||
logical_rect.height <= 0.0)
{
g_object_unref(handle);

error =
"SVG has invalid dimensions";

return false;
}

/*
 * Menu icons are ultimately rendered through the same RGBA
 * image path as PNG/JPEG/WebP. Render the SVG at its intrinsic
 * dimensions.
 */
const unsigned int width =
static_cast<unsigned int>(
logical_rect.width + 0.5
);

const unsigned int height =
static_cast<unsigned int>(
logical_rect.height + 0.5
);

if (width == 0 ||
height == 0)
{
g_object_unref(handle);

error =
"SVG has invalid dimensions";

return false;
}

cairo_surface_t *surface =
cairo_image_surface_create(
CAIRO_FORMAT_ARGB32,
static_cast<int>(width),
static_cast<int>(height)
);

if (!surface)
{
g_object_unref(handle);

error =
"unable to create Cairo surface";

return false;
}

cairo_status_t status =
cairo_surface_status(surface);

if (status != CAIRO_STATUS_SUCCESS)
{
cairo_surface_destroy(surface);
g_object_unref(handle);

error =
"unable to initialize Cairo SVG surface";

return false;
}

cairo_t *cr =
cairo_create(surface);

if (!cr)
{
cairo_surface_destroy(surface);
g_object_unref(handle);

error =
"unable to create Cairo context";

return false;
}

viewport.width =
static_cast<double>(width);

viewport.height =
static_cast<double>(height);

if (!rsvg_handle_render_document(
handle,
cr,
&viewport,
&svg_error
))
{
if (svg_error)
{
error = svg_error->message;
g_error_free(svg_error);
}
else
{
error =
"librsvg failed while rendering SVG";
}

cairo_destroy(cr);
cairo_surface_destroy(surface);
g_object_unref(handle);

return false;
}

cairo_surface_flush(surface);

const unsigned char *source =
cairo_image_surface_get_data(surface);

const int source_stride =
cairo_image_surface_get_stride(surface);

try
{
image.width = width;
image.height = height;
image.stride = width * 4;

image.pixels.resize(
static_cast<size_t>(image.stride) *
static_cast<size_t>(image.height)
);

for (unsigned int y = 0;
y < image.height;
++y)
{
const unsigned char *src =
source +
static_cast<size_t>(y) *
static_cast<size_t>(source_stride);

unsigned char *dst =
image.pixels.data() +
static_cast<size_t>(y) *
static_cast<size_t>(image.stride);

for (unsigned int x = 0;
x < image.width;
++x)
{
/*
 * Cairo ARGB32 is stored as native-endian
 * premultiplied ARGB. On the little-endian
 * systems Hackedbox targets, the byte order is
 * BGRA. Convert to straight RGBA.
 */
const unsigned char blue =
src[static_cast<size_t>(x) * 4];

const unsigned char green =
src[static_cast<size_t>(x) * 4 + 1];

const unsigned char red =
src[static_cast<size_t>(x) * 4 + 2];

const unsigned char alpha =
src[static_cast<size_t>(x) * 4 + 3];

unsigned char *pixel =
dst +
static_cast<size_t>(x) * 4;

if (alpha == 0)
{
pixel[0] = 0;
pixel[1] = 0;
pixel[2] = 0;
pixel[3] = 0;
}
else
{
pixel[0] =
static_cast<unsigned char>(
(std::min(
static_cast<unsigned int>(red) * 255U,
static_cast<unsigned int>(alpha) * 255U
)) /
static_cast<unsigned int>(alpha)
);

pixel[1] =
static_cast<unsigned char>(
(std::min(
static_cast<unsigned int>(green) * 255U,
static_cast<unsigned int>(alpha) * 255U
)) /
static_cast<unsigned int>(alpha)
);

pixel[2] =
static_cast<unsigned char>(
(std::min(
static_cast<unsigned int>(blue) * 255U,
static_cast<unsigned int>(alpha) * 255U
)) /
static_cast<unsigned int>(alpha)
);

pixel[3] = alpha;
}
}
}
}
catch (...)
{
cairo_destroy(cr);
cairo_surface_destroy(surface);
g_object_unref(handle);

image = HbImageData();

error =
"unable to allocate memory for SVG";

return false;
}

cairo_destroy(cr);
cairo_surface_destroy(surface);
g_object_unref(handle);

return true;
}

bool HbImageLoader::hasExtension(
const std::string &filename,
const char *extension
)
{
return lowerExtension(filename) == extension;
}
