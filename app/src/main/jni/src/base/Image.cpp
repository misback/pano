#include "Image.h"
#include <string>
#include <ctype.h>
#include "png.h"
#include "jpeglib.h"
#include <stdlib.h>
#include <string.h>
typedef struct{
    const unsigned char *data;
    ssize_t size;
    int offset;
}tImageSource;
static void pngReadCallback(png_structp png_ptr, png_bytep data, png_size_t length){
    tImageSource* isource = (tImageSource*)png_get_io_ptr(png_ptr);
    if((int)(isource->offset + length) <= isource->size){
        memcpy(data, isource->data+isource->offset, length);
        isource->offset += length;
    }else{
        png_error(png_ptr, "pngReaderCallback failed");
    }
}
PixelFormat getDevicePixelFormat(PixelFormat format){
    return format;
}
//////////////////////////////////////////////////////////////////////////
// Implement Image
//////////////////////////////////////////////////////////////////////////
bool Image::PNG_PREMULTIPLIED_ALPHA_ENABLED = true;
Image::Image()
: _data(nullptr)
, _dataLen(0)
, _width(0)
, _height(0)
, _unpack(false)
, _fileType(Format::UNKNOWN)
, _renderFormat(PixelFormat::NONE)
, _hasPremultipliedAlpha(false){
}
Image::~Image(){
    delete []_data, _data= nullptr;
}
void Image::clear(){
    _width = 0;
    _height = 0;
    _dataLen = 0;
    delete []_data, _data= nullptr;
}
bool Image::initWithImageData(const unsigned char * data, ssize_t dataLen){
    bool ret = false;
    do{
        if(! data || dataLen <= 0){
            break;
        }
        unsigned char* unpackedData = nullptr;
        ssize_t unpackedLen = 0;
        unpackedData = const_cast<unsigned char*>(data);
        unpackedLen = dataLen;
        _fileType = detectFormat(unpackedData, unpackedLen);
        switch (_fileType)
        {
        case Format::PNG:
            ret = initWithPngData(unpackedData, unpackedLen);
            break;
        case Format::JPG:
            ret = initWithJpgData(unpackedData, unpackedLen);
            break;
        }
        if(unpackedData != data){
            free(unpackedData);
        }
    } while (0);
    return ret;
}

bool Image::isPng(const unsigned char * data, ssize_t dataLen){
    if (dataLen <= 8){
        return false;
    }
    static const unsigned char PNG_SIGNATURE[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
    return memcmp(PNG_SIGNATURE, data, sizeof(PNG_SIGNATURE)) == 0;
}
bool Image::isJpg(const unsigned char * data, ssize_t dataLen){
    if (dataLen <= 4){
        return false;
    }
    static const unsigned char JPG_SOI[] = {0xFF, 0xD8};
    return memcmp(data, JPG_SOI, 2) == 0;
}
Image::Format Image::detectFormat(const unsigned char * data, ssize_t dataLen){
    if (isPng(data, dataLen)){
        return Format::PNG;
    }else if (isJpg(data, dataLen)){
        return Format::JPG;
    }else{
        return Format::UNKNOWN;
    }
}
bool Image::hasAlpha(){
    return _renderFormat == PixelFormat::BGRA8888 || _renderFormat == PixelFormat::RGBA8888 || _renderFormat == PixelFormat::A8 || _renderFormat == PixelFormat::AI88
    || _renderFormat == PixelFormat::RGBA4444 || _renderFormat == PixelFormat::RGB5A1;
}

/*
 * ERROR HANDLING:
 *
 * The JPEG library's standard error handler (jerror.c) is divided into
 * several "methods" which you can override individually.  This lets you
 * adjust the behavior without duplicating a lot of code, which you might
 * have to update with each future release.
 *
 * We override the "error_exit" method so that control is returned to the
 * library's caller when a fatal error occurs, rather than calling exit()
 * as the standard error_exit method does.
 *
 * We use C's setjmp/longjmp facility to return control.  This means that the
 * routine which calls the JPEG library must first execute a setjmp() call to
 * establish the return point.  We want the replacement error_exit to do a
 * longjmp().  But we need to make the setjmp buffer accessible to the
 * error_exit routine.  To do this, we make a private extension of the
 * standard JPEG error handler object.  (If we were using C++, we'd say we
 * were making a subclass of the regular error handler.)
 *
 * Here's the extended error handler struct:
 */
struct MyErrorMgr
{
    struct jpeg_error_mgr pub;  /* "public" fields */
    jmp_buf setjmp_buffer;  /* for return to caller */
};

typedef struct MyErrorMgr * MyErrorPtr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

METHODDEF(void)
myErrorExit(j_common_ptr cinfo){
    /* cinfo->err really points to a MyErrorMgr struct, so coerce pointer */
    MyErrorPtr myerr = (MyErrorPtr) cinfo->err;

    /* Always display the message. */
    /* We could postpone this until after returning, if we chose. */
    /* internal message function can't show error message in some platforms, so we rewrite it here.
     * edit it if has version conflict.
     */
    //(*cinfo->err->output_message) (cinfo);
    char buffer[JMSG_LENGTH_MAX];
    (*cinfo->err->format_message) (cinfo, buffer);
    LOGE("jpeg error: %s", buffer);
    /* Return control to the setjmp point */
    longjmp(myerr->setjmp_buffer, 1);
}
bool Image::initWithJpgData(const unsigned char * data, ssize_t dataLen){
    /* these are standard libjpeg structures for reading(decompression) */
    struct jpeg_decompress_struct cinfo;
    /* We use our private extension JPEG error handler.
     * Note that this struct must live as long as the main JPEG parameter
     * struct, to avoid dangling-pointer problems.
     */
    struct MyErrorMgr jerr;
    /* libjpeg data structure for storing one row, that is, scanline of an image */
    JSAMPROW row_pointer[1] = {0};
    unsigned long location = 0;
    bool ret = false;
    do {
        /* We set up the normal JPEG error routines, then override error_exit. */
        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = myErrorExit;
        /* Establish the setjmp return context for MyErrorExit to use. */
        if (setjmp(jerr.setjmp_buffer)){
            /* If we get here, the JPEG code has signaled an error.
             * We need to clean up the JPEG object, close the input file, and return.
             */
            jpeg_destroy_decompress(&cinfo);
            break;
        }
        /* setup decompression process and source, then read JPEG header */
        jpeg_create_decompress( &cinfo );
#ifndef CC_TARGET_QT5
        jpeg_mem_src(&cinfo, const_cast<unsigned char*>(data), dataLen);
#endif /* CC_TARGET_QT5 */
        /* reading the image header which contains image information */
#if (JPEG_LIB_VERSION >= 90)
        // libjpeg 0.9 adds stricter types.
        jpeg_read_header(&cinfo, TRUE);
#else
        jpeg_read_header(&cinfo, TRUE);
#endif
        // we only support RGB or grayscale
        if (cinfo.jpeg_color_space == JCS_GRAYSCALE){
            _renderFormat = PixelFormat::I8;
        }else{
            cinfo.out_color_space = JCS_RGB;
            _renderFormat = PixelFormat::RGB888;
        }
        /* Start decompression jpeg here */
        jpeg_start_decompress( &cinfo );
        /* init image info */
        _width  = cinfo.output_width;
        _height = cinfo.output_height;
        _dataLen = cinfo.output_width*cinfo.output_height*cinfo.output_components;
        if(_data != nullptr){
            delete[] _data , _data = nullptr;
        }
        _data = static_cast<unsigned char*>(malloc(_dataLen * sizeof(unsigned char)));
        if(! _data){
            break;
        }
        /* now actually read the jpeg into the raw buffer */
        /* read one scan line at a time */
        while (cinfo.output_scanline < cinfo.output_height){
            row_pointer[0] = _data + location;
            location += cinfo.output_width*cinfo.output_components;
            jpeg_read_scanlines(&cinfo, row_pointer, 1);
        }
    /* When read image file with broken data, jpeg_finish_decompress() may cause error.
     * Besides, jpeg_destroy_decompress() shall deallocate and release all memory associated
     * with the decompression object.
     * So it doesn't need to call jpeg_finish_decompress().
     */
    //jpeg_finish_decompress( &cinfo );
        jpeg_destroy_decompress( &cinfo );
        /* wrap up decompression, destroy objects, free pointers and close open files */
        ret = true;
    } while (0);
    return ret;
}

bool Image::initWithPngData(const unsigned char * data, ssize_t dataLen){
    // length of bytes to check if it is a valid png file
#define PNGSIGSIZE  8
    bool ret = false;
    png_byte        header[PNGSIGSIZE]   = {0};
    png_structp     png_ptr     =   0;
    png_infop       info_ptr    = 0;
    do {
        // png header len is 8 bytes
        if(dataLen < PNGSIGSIZE){
            break;
        }
        // check the data is png or not
        memcpy(header, data, PNGSIGSIZE);
        CC_BREAK_IF(png_sig_cmp(header, 0, PNGSIGSIZE));
        // init png_struct
        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
        CC_BREAK_IF(! png_ptr);
        // init png_info
        info_ptr = png_create_info_struct(png_ptr);
        CC_BREAK_IF(!info_ptr);
        CC_BREAK_IF(setjmp(png_jmpbuf(png_ptr)));
        // set the read call back function
        tImageSource imageSource;
        imageSource.data    = (unsigned char*)data;
        imageSource.size    = dataLen;
        imageSource.offset  = 0;
        png_set_read_fn(png_ptr, &imageSource, pngReadCallback);
        // read png header info
        // read png file info
        png_read_info(png_ptr, info_ptr);
        _width = png_get_image_width(png_ptr, info_ptr);
        _height = png_get_image_height(png_ptr, info_ptr);
        png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);
        png_uint_32 color_type = png_get_color_type(png_ptr, info_ptr);
        //CCLOG("color type %u", color_type);
        // force palette images to be expanded to 24-bit RGB
        // it may include alpha channel
        if (color_type == PNG_COLOR_TYPE_PALETTE){
            png_set_palette_to_rgb(png_ptr);
        }
        // low-bit-depth grayscale images are to be expanded to 8 bits
        if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8){
            bit_depth = 8;
            png_set_expand_gray_1_2_4_to_8(png_ptr);
        }
        // expand any tRNS chunk data into a full alpha channel
        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)){
            png_set_tRNS_to_alpha(png_ptr);
        }  
        // reduce images with 16-bit samples to 8 bits
        if (bit_depth == 16){
            png_set_strip_16(png_ptr);            
        }
        // Expanded earlier for grayscale, now take care of palette and rgb
        if (bit_depth < 8){
            png_set_packing(png_ptr);
        }
        // update info
        png_read_update_info(png_ptr, info_ptr);
        bit_depth = png_get_bit_depth(png_ptr, info_ptr);
        color_type = png_get_color_type(png_ptr, info_ptr);
        switch (color_type){
        case PNG_COLOR_TYPE_GRAY:
            _renderFormat = PixelFormat::I8;
            break;
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            _renderFormat = PixelFormat::AI88;
            break;
        case PNG_COLOR_TYPE_RGB:
            _renderFormat = PixelFormat::RGB888;
            break;
        case PNG_COLOR_TYPE_RGB_ALPHA:
            _renderFormat = PixelFormat::RGBA8888;
            break;
        default:
            break;
        }
        // read png data
        png_size_t rowbytes;
        png_bytep* row_pointers = (png_bytep*)malloc( sizeof(png_bytep) * _height );
        rowbytes = png_get_rowbytes(png_ptr, info_ptr);
        _dataLen = rowbytes * _height;
        if(_data != nullptr){
            delete[] _data , _data = nullptr;
        }
        _data = static_cast<unsigned char*>(malloc(_dataLen * sizeof(unsigned char)));
        if (!_data){
            if (row_pointers != nullptr){
                free(row_pointers);
            }
            break;
        }
        for (unsigned short i = 0; i < _height; ++i){
            row_pointers[i] = _data + i*rowbytes;
        }
        png_read_image(png_ptr, row_pointers);
        png_read_end(png_ptr, nullptr);
        // premultiplied alpha for RGBA8888
        if (PNG_PREMULTIPLIED_ALPHA_ENABLED && color_type == PNG_COLOR_TYPE_RGB_ALPHA){
            premultipliedAlpha();
        }
        if (row_pointers != nullptr){
            free(row_pointers);
        }
        ret = true;
    } while (0);
    if (png_ptr){
        png_destroy_read_struct(&png_ptr, (info_ptr) ? &info_ptr : 0, 0);
    }
    return ret;
}

bool Image::saveToFile(const std::string& filename, bool isToRGB){
    //only support for PixelFormat::RGB888 or PixelFormat::RGBA8888 uncompressed data
    if ((_renderFormat != PixelFormat::RGB888 && _renderFormat != PixelFormat::RGBA8888)){
        LOGE("cocos2d: Image: saveToFile is only support for PixelFormat::RGB888 or PixelFormat::RGBA8888 uncompressed data for now");
        return false;
    }
    std::string fileExtension = getFileExtension(filename);
    if (fileExtension == ".png"){
        return saveImageToPNG(filename, isToRGB);
    }else if (fileExtension == ".jpg"){
        return saveImageToJPG(filename);
    }else{
        LOGE("cocos2d: Image: saveToFile no support file extension(only .png or .jpg) for file: %s", filename.c_str());
        return false;
    }
}
bool Image::saveImageToPNG(unsigned char* _imageData, bool isAlpha, bool isToRGB, int _imageWidth, int _imageHeight, const std::string& filePath){
    bool ret = false;
    do{
        FILE *fp;
        png_structp png_ptr;
        png_infop info_ptr;
        png_colorp palette;
        png_bytep *row_pointers;
        fp = fopen(filePath.c_str(), "wb");
        CC_BREAK_IF(nullptr == fp);
        png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (nullptr == png_ptr){
            fclose(fp);
            break;
        }
        info_ptr = png_create_info_struct(png_ptr);
        if (nullptr == info_ptr){
            fclose(fp);
            png_destroy_write_struct(&png_ptr, nullptr);
            break;
        }
        if (setjmp(png_jmpbuf(png_ptr))){
            fclose(fp);
            png_destroy_write_struct(&png_ptr, &info_ptr);
            break;
        }
        png_init_io(png_ptr, fp);
        if (!isToRGB && hasAlpha()){
            png_set_IHDR(png_ptr, info_ptr, _imageWidth, _imageHeight, 8, PNG_COLOR_TYPE_RGB_ALPHA,
                PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        } else{
            png_set_IHDR(png_ptr, info_ptr, _imageWidth, _imageHeight, 8, PNG_COLOR_TYPE_RGB,
                PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        }
        palette = (png_colorp)png_malloc(png_ptr, PNG_MAX_PALETTE_LENGTH * sizeof (png_color));
        png_set_PLTE(png_ptr, info_ptr, palette, PNG_MAX_PALETTE_LENGTH);
        png_write_info(png_ptr, info_ptr);
        png_set_packing(png_ptr);
        row_pointers = (png_bytep *)malloc(_imageHeight * sizeof(png_bytep));
        if(row_pointers == nullptr){
            fclose(fp);
            png_destroy_write_struct(&png_ptr, &info_ptr);
            break;
        }
        if (!isAlpha){
            for (int i = 0; i < (int)_imageHeight; i++){
                row_pointers[i] = (png_bytep)_imageData + i * _imageWidth * 3;
            }
            png_write_image(png_ptr, row_pointers);
            free(row_pointers);
            row_pointers = nullptr;
        }else{
            if (isToRGB){
                unsigned char *tempData = static_cast<unsigned char*>(malloc(_imageWidth * _imageHeight * 3 * sizeof(unsigned char)));
                if (nullptr == tempData){
                    fclose(fp);
                    png_destroy_write_struct(&png_ptr, &info_ptr);
                    free(row_pointers);
                    row_pointers = nullptr;
                    break;
                }
                for (int i = 0; i < _imageHeight; ++i){
                    for (int j = 0; j < _imageWidth; ++j){
                        tempData[(i * _imageWidth + j) * 3] = _imageData[(i * _imageWidth + j) * 4];
                        tempData[(i * _imageWidth + j) * 3 + 1] = _imageData[(i * _imageWidth + j) * 4 + 1];
                        tempData[(i * _imageWidth + j) * 3 + 2] = _imageData[(i * _imageWidth + j) * 4 + 2];
                    }
                }
                for (int i = 0; i < (int)_imageHeight; i++){
                    row_pointers[i] = (png_bytep)tempData + i * _imageWidth * 3;
                }
                png_write_image(png_ptr, row_pointers);
                free(row_pointers);
                row_pointers = nullptr;
                if (tempData != nullptr){
                    free(tempData);
                }
            }else{
                for (int i = 0; i < (int)_imageHeight; i++){
                    row_pointers[i] = (png_bytep)_imageData + i * _imageWidth * 4;
                }
                png_write_image(png_ptr, row_pointers);
                free(row_pointers);
                row_pointers = nullptr;
            }
        }
        png_write_end(png_ptr, info_ptr);
        png_free(png_ptr, palette);
        palette = nullptr;
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        ret = true;
    } while (0);
    return ret;
}
bool Image::saveImageToPNG(const std::string& filePath, bool isToRGB){
    bool ret = false;
    do{
        FILE *fp;
        png_structp png_ptr;
        png_infop info_ptr;
        png_colorp palette;
        png_bytep *row_pointers;
        fp = fopen(filePath.c_str(), "wb");
        CC_BREAK_IF(nullptr == fp);
        png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (nullptr == png_ptr){
            fclose(fp);
            break;
        }
        info_ptr = png_create_info_struct(png_ptr);
        if (nullptr == info_ptr){
            fclose(fp);
            png_destroy_write_struct(&png_ptr, nullptr);
            break;
        }
        if (setjmp(png_jmpbuf(png_ptr))){
            fclose(fp);
            png_destroy_write_struct(&png_ptr, &info_ptr);
            break;
        }
        png_init_io(png_ptr, fp);
        if (!isToRGB && hasAlpha()){
            png_set_IHDR(png_ptr, info_ptr, _width, _height, 8, PNG_COLOR_TYPE_RGB_ALPHA,
                PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        } else{
            png_set_IHDR(png_ptr, info_ptr, _width, _height, 8, PNG_COLOR_TYPE_RGB,
                PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        }
        palette = (png_colorp)png_malloc(png_ptr, PNG_MAX_PALETTE_LENGTH * sizeof (png_color));
        png_set_PLTE(png_ptr, info_ptr, palette, PNG_MAX_PALETTE_LENGTH);
        png_write_info(png_ptr, info_ptr);
        png_set_packing(png_ptr);
        row_pointers = (png_bytep *)malloc(_height * sizeof(png_bytep));
        if(row_pointers == nullptr){
            fclose(fp);
            png_destroy_write_struct(&png_ptr, &info_ptr);
            break;
        }
        if (!hasAlpha()){
            for (int i = 0; i < (int)_height; i++){
                row_pointers[i] = (png_bytep)_data + i * _width * 3;
            }
            png_write_image(png_ptr, row_pointers);
            free(row_pointers);
            row_pointers = nullptr;
        }else{
            if (isToRGB){
                unsigned char *tempData = static_cast<unsigned char*>(malloc(_width * _height * 3 * sizeof(unsigned char)));
                if (nullptr == tempData){
                    fclose(fp);
                    png_destroy_write_struct(&png_ptr, &info_ptr);
                    free(row_pointers);
                    row_pointers = nullptr;
                    break;
                }
                for (int i = 0; i < _height; ++i){
                    for (int j = 0; j < _width; ++j){
                        tempData[(i * _width + j) * 3] = _data[(i * _width + j) * 4];
                        tempData[(i * _width + j) * 3 + 1] = _data[(i * _width + j) * 4 + 1];
                        tempData[(i * _width + j) * 3 + 2] = _data[(i * _width + j) * 4 + 2];
                    }
                }
                for (int i = 0; i < (int)_height; i++){
                    row_pointers[i] = (png_bytep)tempData + i * _width * 3;
                }
                png_write_image(png_ptr, row_pointers);
                free(row_pointers);
                row_pointers = nullptr;
                if (tempData != nullptr){
                    free(tempData);
                }
            }else{
                for (int i = 0; i < (int)_height; i++){
                    row_pointers[i] = (png_bytep)_data + i * _width * 4;
                }
                png_write_image(png_ptr, row_pointers);
                free(row_pointers);
                row_pointers = nullptr;
            }
        }
        png_write_end(png_ptr, info_ptr);
        png_free(png_ptr, palette);
        palette = nullptr;
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        ret = true;
    } while (0);
    return ret;
}
bool Image::saveImageToJPG(unsigned char* _imageData, bool isAlpha, int _imageWidth, int _imageHeight, const std::string& filePath){
     bool ret = false;
     do {
         struct jpeg_compress_struct cinfo;
         struct jpeg_error_mgr jerr;
         FILE * outfile;                 /* target file */
         JSAMPROW row_pointer[1];        /* pointer to JSAMPLE row[s] */
         int     row_stride;          /* physical row width in image buffer */
         cinfo.err = jpeg_std_error(&jerr);
         /* Now we can initialize the JPEG compression object. */
         jpeg_create_compress(&cinfo);
         outfile =fopen(filePath.c_str(), "wb");
         jpeg_stdio_dest(&cinfo, outfile);
         cinfo.image_width = _imageWidth;    /* image width and height, in pixels */
         cinfo.image_height = _imageHeight;
         cinfo.input_components = 3;       /* # of color components per pixel */
         cinfo.in_color_space = JCS_RGB;       /* colorspace of input image */
         jpeg_set_defaults(&cinfo);
          // 指定亮度及色度质量
         cinfo.q_scale_factor[0] = jpeg_quality_scaling(100);
         cinfo.q_scale_factor[1] = jpeg_quality_scaling(100);
         // 图像采样率，默认为2 * 2
         cinfo.comp_info[0].v_samp_factor = 1;
         cinfo.comp_info[0].h_samp_factor = 2;
         jpeg_set_quality(&cinfo, 100, TRUE);
         jpeg_start_compress(&cinfo, TRUE);
         row_stride = _imageWidth * 3; /* JSAMPLEs per row in image_buffer */
         if (isAlpha){
             unsigned char *tempData = static_cast<unsigned char*>(malloc(_imageWidth * _imageHeight * 3 * sizeof(unsigned char)));
             if (nullptr == tempData){
                 jpeg_finish_compress(&cinfo);
                 jpeg_destroy_compress(&cinfo);
                 fclose(outfile);
                 break;
             }
             for (int i = 0; i < _imageHeight; ++i){
                 for (int j = 0; j < _imageWidth; ++j){
                     tempData[(i * _imageWidth + j) * 3] = _imageData[(i * _imageWidth + j) * 4];
                     tempData[(i * _imageWidth + j) * 3 + 1] = _imageData[(i * _imageWidth + j) * 4 + 1];
                     tempData[(i * _imageWidth + j) * 3 + 2] = _imageData[(i * _imageWidth + j) * 4 + 2];
                 }
             }
             while (cinfo.next_scanline < cinfo.image_height){
                 row_pointer[0] = & tempData[cinfo.next_scanline * row_stride];
                 (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
             }
             if (tempData != nullptr){
                 free(tempData);
             }
         } else{
             while (cinfo.next_scanline < cinfo.image_height) {
                 row_pointer[0] = & _imageData[cinfo.next_scanline * row_stride];
                 (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
             }
         }
         jpeg_finish_compress(&cinfo);
         fclose(outfile);
         jpeg_destroy_compress(&cinfo);
         ret = true;
     } while (0);
     return ret;
 }
 bool Image::saveImageToJPG_FlipY(unsigned char* _imageData, bool isAlpha, int _imageWidth, int _imageHeight, const std::string& filePath){
     bool ret = false;
     do {
         struct jpeg_compress_struct cinfo;
         struct jpeg_error_mgr jerr;
         FILE * outfile;                 /* target file */
         JSAMPROW row_pointer[1];        /* pointer to JSAMPLE row[s] */
         int     row_stride;          /* physical row width in image buffer */
         cinfo.err = jpeg_std_error(&jerr);
         /* Now we can initialize the JPEG compression object. */
         jpeg_create_compress(&cinfo);
         outfile =fopen(filePath.c_str(), "wb");
         jpeg_stdio_dest(&cinfo, outfile);
         cinfo.image_width = _imageWidth;    /* image width and height, in pixels */
         cinfo.image_height = _imageHeight;
         cinfo.input_components = 3;       /* # of color components per pixel */
         cinfo.in_color_space = JCS_RGB;       /* colorspace of input image */
         jpeg_set_defaults(&cinfo);
          // 指定亮度及色度质量
         cinfo.q_scale_factor[0] = jpeg_quality_scaling(100);
         cinfo.q_scale_factor[1] = jpeg_quality_scaling(100);
         // 图像采样率，默认为2 * 2
         cinfo.comp_info[0].v_samp_factor = 1;
         cinfo.comp_info[0].h_samp_factor = 2;
         jpeg_set_quality(&cinfo, 100, TRUE);
         jpeg_start_compress(&cinfo, TRUE);
         row_stride = _imageWidth * 3; /* JSAMPLEs per row in image_buffer */
         if (isAlpha){
             unsigned char *tempData = static_cast<unsigned char*>(malloc(_imageWidth * _imageHeight * 3 * sizeof(unsigned char)));
             if (nullptr == tempData){
                 jpeg_finish_compress(&cinfo);
                 jpeg_destroy_compress(&cinfo);
                 fclose(outfile);
                 break;
             }
             for (int i = 0; i < _imageHeight; ++i){
                 for (int j = 0; j < _imageWidth; ++j){
                     tempData[(i * _imageWidth + j) * 3] = _imageData[((_imageHeight-i-1) * _imageWidth + j) * 4];
                     tempData[(i * _imageWidth + j) * 3 + 1] = _imageData[((_imageHeight-i-1) * _imageWidth + j) * 4 + 1];
                     tempData[(i * _imageWidth + j) * 3 + 2] = _imageData[((_imageHeight-i-1) * _imageWidth + j) * 4 + 2];
                 }
             }
             while (cinfo.next_scanline < cinfo.image_height){
                 row_pointer[0] = & tempData[cinfo.next_scanline * row_stride];
                 (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
             }
             if (tempData != nullptr){
                 free(tempData);
             }
         } else{
             while (cinfo.next_scanline < cinfo.image_height) {
                 row_pointer[0] = & _imageData[cinfo.next_scanline * row_stride];
                 (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
             }
         }
         jpeg_finish_compress(&cinfo);
         fclose(outfile);
         jpeg_destroy_compress(&cinfo);
         ret = true;
     } while (0);
     return ret;
 }
bool Image::saveImageToJPG(const std::string& filePath){
    bool ret = false;
    do {
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr jerr;
        FILE * outfile;                 /* target file */
        JSAMPROW row_pointer[1];        /* pointer to JSAMPLE row[s] */
        int     row_stride;          /* physical row width in image buffer */
        cinfo.err = jpeg_std_error(&jerr);
        /* Now we can initialize the JPEG compression object. */
        jpeg_create_compress(&cinfo);
        outfile =fopen(filePath.c_str(), "wb");
        jpeg_stdio_dest(&cinfo, outfile);
        cinfo.image_width = _width;    /* image width and height, in pixels */
        cinfo.image_height = _height;
        cinfo.input_components = 3;       /* # of color components per pixel */
        cinfo.in_color_space = JCS_RGB;       /* colorspace of input image */
        jpeg_set_defaults(&cinfo);
         // 指定亮度及色度质量
        cinfo.q_scale_factor[0] = jpeg_quality_scaling(100);
        cinfo.q_scale_factor[1] = jpeg_quality_scaling(100);
        // 图像采样率，默认为2 * 2
        cinfo.comp_info[0].v_samp_factor = 1;
        cinfo.comp_info[0].h_samp_factor = 2;
        jpeg_set_quality(&cinfo, 100, TRUE);
        jpeg_start_compress(&cinfo, TRUE);
        row_stride = _width * 3; /* JSAMPLEs per row in image_buffer */
        if (hasAlpha()){
            unsigned char *tempData = static_cast<unsigned char*>(malloc(_width * _height * 3 * sizeof(unsigned char)));
            if (nullptr == tempData){
                jpeg_finish_compress(&cinfo);
                jpeg_destroy_compress(&cinfo);
                fclose(outfile);
                break;
            }
            for (int i = 0; i < _height; ++i){
                for (int j = 0; j < _width; ++j){
                    tempData[(i * _width + j) * 3] = _data[(i * _width + j) * 4];
                    tempData[(i * _width + j) * 3 + 1] = _data[(i * _width + j) * 4 + 1];
                    tempData[(i * _width + j) * 3 + 2] = _data[(i * _width + j) * 4 + 2];
                }
            }
            while (cinfo.next_scanline < cinfo.image_height){
                row_pointer[0] = & tempData[cinfo.next_scanline * row_stride];
                (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
            }
            if (tempData != nullptr){
                free(tempData);
            }
        } else{
            while (cinfo.next_scanline < cinfo.image_height) {
                row_pointer[0] = & _data[cinfo.next_scanline * row_stride];
                (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
            }
        }
        jpeg_finish_compress(&cinfo);
        fclose(outfile);
        jpeg_destroy_compress(&cinfo);
        ret = true;
    } while (0);
    return ret;
}

void Image::premultipliedAlpha(){
    //CCASSERT(_renderFormat == PixelFormat::RGBA8888, "The pixel format should be RGBA8888!");
    unsigned int* fourBytes = (unsigned int*)_data;
    for(int i = 0; i < _width * _height; i++){
        unsigned char* p = _data + i * 4;
        fourBytes[i] = CC_RGB_PREMULTIPLY_ALPHA(p[0], p[1], p[2], p[3]);
    }
    _hasPremultipliedAlpha = true;
}


