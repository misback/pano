//
// Created by roading on 2016/7/22.
//

#include "ImageUtil.h"
#include "jpeglib.h"
#include <setjmp.h>
#include "Common.h"
#include "png.h"
#include <string.h>
// premultiply alpha, or the effect will wrong when want to use other pixel format in Texture2D,
// such as RGB888, RGB5A1
#define CC_RGB_PREMULTIPLY_ALPHA(vr, vg, vb, va) \
    (unsigned)(((unsigned)((unsigned char)(vr) * ((unsigned char)(va) + 1)) >> 8) | \
    ((unsigned)((unsigned char)(vg) * ((unsigned char)(va) + 1) >> 8) << 8) | \
    ((unsigned)((unsigned char)(vb) * ((unsigned char)(va) + 1) >> 8) << 16) | \
    ((unsigned)(unsigned char)(va) << 24))
    #define CC_BREAK_IF(cond)           if(cond) break
bool saveToJPG(unsigned char* _data, bool bAlpha, int _width, int _height, const std::string& filePath){
    bool ret = false;
    do
    {
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr jerr;
        FILE * outfile;                 /* target file */
        JSAMPROW row_pointer[1];        /* pointer to JSAMPLE row[s] */
        int     row_stride;          /* physical row width in image buffer */
        cinfo.err = jpeg_std_error(&jerr);
        /* Now we can initialize the JPEG compression object. */
        jpeg_create_compress(&cinfo);
        outfile = fopen(filePath.c_str(), "wb");
        jpeg_stdio_dest(&cinfo, outfile);
        cinfo.image_width = _width;    /* image width and height, in pixels */
        cinfo.image_height = _height;
        cinfo.input_components = 3;       /* # of color components per pixel */
        cinfo.in_color_space = JCS_RGB;       /* colorspace of input image */
        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, 100, TRUE);
        jpeg_start_compress(&cinfo, TRUE);
        row_stride = _width * 3; /* JSAMPLEs per row in image_buffer */
        if (bAlpha){
            unsigned char *tempData = static_cast<unsigned char*>(malloc(_width * _height * 3 * sizeof(unsigned char)));
            if (nullptr == tempData){
                jpeg_finish_compress(&cinfo);
                jpeg_destroy_compress(&cinfo);
                fclose(outfile);
                break;
            }
            for (int i = 0; i < _height; ++i){
                for (int j = 0; j < _width; ++j){
                    tempData[(i * _width + j) * 3]     = _data[(i * _width + j) * 4];
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
        }else{
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

bool saveToJPG_FlipY(unsigned char* _data, bool bAlpha, int _width, int _height, const std::string& filePath){
    bool ret = false;
    do
    {
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr jerr;
        FILE * outfile;                 /* target file */
        JSAMPROW row_pointer[1];        /* pointer to JSAMPLE row[s] */
        int     row_stride;          /* physical row width in image buffer */
        cinfo.err = jpeg_std_error(&jerr);
        /* Now we can initialize the JPEG compression object. */
        jpeg_create_compress(&cinfo);
        outfile = fopen(filePath.c_str(), "wb");
        jpeg_stdio_dest(&cinfo, outfile);
        cinfo.image_width = _width;    /* image width and height, in pixels */
        cinfo.image_height = _height;
        cinfo.input_components = 3;       /* # of color components per pixel */
        cinfo.in_color_space = JCS_RGB;       /* colorspace of input image */
        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, 100, TRUE);
        jpeg_start_compress(&cinfo, TRUE);
        row_stride = _width * 3; /* JSAMPLEs per row in image_buffer */
        if (bAlpha){
            unsigned char *tempData = static_cast<unsigned char*>(malloc(_width * _height * 3 * sizeof(unsigned char)));
            if (nullptr == tempData){
                jpeg_finish_compress(&cinfo);
                jpeg_destroy_compress(&cinfo);
                fclose(outfile);
                break;
            }
            for (int i = 0; i < _height; ++i){
                for (int j = 0; j < _width; ++j){
                    tempData[(i * _width + j) * 3]     = _data[((_height-i-1) * _width + j) * 4];
                    tempData[(i * _width + j) * 3 + 1] = _data[((_height-i-1) * _width + j) * 4 + 1];
                    tempData[(i * _width + j) * 3 + 2] = _data[((_height-i-1) * _width + j) * 4 + 2];
                }
            }
            while (cinfo.next_scanline < cinfo.image_height){
                row_pointer[0] = & tempData[cinfo.next_scanline * row_stride];
                (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
            }

            if (tempData != nullptr){
                free(tempData);
            }
        }else{
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

struct MyErrorMgr
{
    struct jpeg_error_mgr pub;  /* "public" fields */
    jmp_buf setjmp_buffer;  /* for return to caller */
};

typedef struct MyErrorMgr * MyErrorPtr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

METHODDEF(void)myErrorExit(j_common_ptr cinfo)
{
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

    /* Return control to the setjmp point */
    longjmp(myerr->setjmp_buffer, 1);
}


bool read_JPEG_file_data (unsigned char*imageData, ssize_t unpackedLen, unsigned char* &_data, int& _width, int& _height, ssize_t& _dataLen) {
    struct jpeg_decompress_struct cinfo;
    struct MyErrorMgr jerr;
    JSAMPROW row_pointer[1] = {0};
    unsigned long location = 0;
    bool ret = false;
    do {
        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = myErrorExit;
        if (setjmp(jerr.setjmp_buffer)) {
            jpeg_destroy_decompress(&cinfo);
            LOGE("1122233333");
            break;
        }
        jpeg_create_decompress( &cinfo );
#ifndef CC_TARGET_QT5
        jpeg_mem_src(&cinfo, const_cast<unsigned char*>(imageData), unpackedLen);
#endif /* CC_TARGET_QT5 */
        /* reading the image header which contains image information */
#if (JPEG_LIB_VERSION >= 90)
        // libjpeg 0.9 adds stricter types.
        jpeg_read_header(&cinfo, TRUE);
#else
        jpeg_read_header(&cinfo, TRUE);
#endif
        /* Start decompression jpeg here */
        jpeg_start_decompress( &cinfo );
        /* init image info */
        _width  = cinfo.output_width;
        _height = cinfo.output_height;
        _dataLen = cinfo.output_width*cinfo.output_height*cinfo.output_components;
        _data = static_cast<unsigned char*>(malloc(_dataLen * sizeof(unsigned char)));
        /* now actually read the jpeg into the raw buffer */
        /* read one scan line at a time */
        while (cinfo.output_scanline < cinfo.output_height) {
            row_pointer[0] = _data + location;
            location += cinfo.output_width*cinfo.output_components;
            jpeg_read_scanlines(&cinfo, row_pointer, 1);
        }
        jpeg_destroy_decompress( &cinfo );
        /* wrap up decompression, destroy objects, free pointers and close open files */
        ret = true;
    } while (0);
    delete[] imageData;
    return ret;
}
bool read_JPEG_file (const char* filePath, unsigned char* &_data, int& _width, int& _height, int& _dataLen) {
    auto fp        = fopen(filePath, "rb");
    unsigned char* unpackedData = nullptr;
    ssize_t unpackedLen =   0;
    if (fp != nullptr) {
        fseek(fp,0,SEEK_END);
        unpackedLen = ftell(fp);
        unpackedData  =   new unsigned char[unpackedLen];
        fseek(fp,0,SEEK_SET);
        fread(unpackedData, 1, unpackedLen, fp);
        fflush(fp);
        fclose(fp);
    } else {
        LOGE("read_JPEG_file%s failed!", filePath);
        fclose(fp);
        return false;
    }
    struct jpeg_decompress_struct cinfo;
    struct MyErrorMgr jerr;
    JSAMPROW row_pointer[1] = {0};
    unsigned long location = 0;
    bool ret = false;
    do {
        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = myErrorExit;
        if (setjmp(jerr.setjmp_buffer)) {
            jpeg_destroy_decompress(&cinfo);
            LOGE("1122233333");
            break;
        }
        jpeg_create_decompress( &cinfo );
#ifndef CC_TARGET_QT5
        jpeg_mem_src(&cinfo, const_cast<unsigned char*>(unpackedData), unpackedLen);
#endif /* CC_TARGET_QT5 */
        /* reading the image header which contains image information */
#if (JPEG_LIB_VERSION >= 90)
        // libjpeg 0.9 adds stricter types.
        jpeg_read_header(&cinfo, TRUE);
#else
        jpeg_read_header(&cinfo, TRUE);
#endif
        /* Start decompression jpeg here */
        jpeg_start_decompress( &cinfo );
        /* init image info */
        _width  = cinfo.output_width;
        _height = cinfo.output_height;
        _dataLen = cinfo.output_width*cinfo.output_height*cinfo.output_components;
        _data = static_cast<unsigned char*>(malloc(_dataLen * sizeof(unsigned char)));
        /* now actually read the jpeg into the raw buffer */
        /* read one scan line at a time */
        while (cinfo.output_scanline < cinfo.output_height) {
            row_pointer[0] = _data + location;
            location += cinfo.output_width*cinfo.output_components;
            jpeg_read_scanlines(&cinfo, row_pointer, 1);
        }
        jpeg_destroy_decompress( &cinfo );
        /* wrap up decompression, destroy objects, free pointers and close open files */
        ret = true;
    } while (0);
    delete[] unpackedData;
    return ret;
}
    typedef struct
    {
        const unsigned char * data;
        ssize_t size;
        int offset;
    }tImageSource;
     void pngReadCallback(png_structp png_ptr, png_bytep data, png_size_t length)
    {
        tImageSource* isource = (tImageSource*)png_get_io_ptr(png_ptr);

        if((int)(isource->offset + length) <= isource->size)
        {
            memcpy(data, isource->data+isource->offset, length);
            isource->offset += length;
        }
        else
        {
            png_error(png_ptr, "pngReaderCallback failed");
        }
    }
bool read_PNG_file (const char* filePath, unsigned char* &_data, int& _width, int& _height, int& _dataLen) {
    auto fp        = fopen(filePath, "rb");
    unsigned char* data = nullptr;
    ssize_t dataLen =   0;
    if (fp != nullptr) {
        fseek(fp,0,SEEK_END);
        auto size = ftell(fp);
        data  =   new unsigned char[size];
        fseek(fp,0,SEEK_SET);
        fread(data, 1, size, fp);
        fflush(fp);
        fclose(fp);
    } else {
        LOGE("read_JPEG_file%s failed!", filePath);
        fclose(fp);
        return false;
    }
    // length of bytes to check if it is a valid png file
#define PNGSIGSIZE  8
    bool ret = false;
    png_byte        header[PNGSIGSIZE]   = {0};
    png_structp     png_ptr     =   0;
    png_infop       info_ptr    = 0;

    do
    {
        // png header len is 8 bytes
        //CC_BREAK_IF(dataLen < PNGSIGSIZE);

        // check the data is png or not
        memcpy(header, data, PNGSIGSIZE);
        //CC_BREAK_IF(png_sig_cmp(header, 0, PNGSIGSIZE));

        // init png_struct
        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
        CC_BREAK_IF(! png_ptr);

        // init png_info
        info_ptr = png_create_info_struct(png_ptr);
        CC_BREAK_IF(!info_ptr);

#if (CC_TARGET_PLATFORM != CC_PLATFORM_BADA && CC_TARGET_PLATFORM != CC_PLATFORM_NACL && CC_TARGET_PLATFORM != CC_PLATFORM_TIZEN)
        CC_BREAK_IF(setjmp(png_jmpbuf(png_ptr)));
#endif

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
        if (color_type == PNG_COLOR_TYPE_PALETTE)
        {
            png_set_palette_to_rgb(png_ptr);
        }
        // low-bit-depth grayscale images are to be expanded to 8 bits
        if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        {
            bit_depth = 8;
            png_set_expand_gray_1_2_4_to_8(png_ptr);
        }
        // expand any tRNS chunk data into a full alpha channel
        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        {
            png_set_tRNS_to_alpha(png_ptr);
        }
        // reduce images with 16-bit samples to 8 bits
        if (bit_depth == 16)
        {
            png_set_strip_16(png_ptr);
        }

        // Expanded earlier for grayscale, now take care of palette and rgb
        if (bit_depth < 8)
        {
            png_set_packing(png_ptr);
        }
        // update info
        png_read_update_info(png_ptr, info_ptr);
        bit_depth = png_get_bit_depth(png_ptr, info_ptr);
        color_type = png_get_color_type(png_ptr, info_ptr);
      /*  int _renderFormat =
        switch (color_type)
        {
        case PNG_COLOR_TYPE_GRAY:
            _renderFormat = Texture2D::PixelFormat::I8;
            break;
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            _renderFormat = Texture2D::PixelFormat::AI88;
            break;
        case PNG_COLOR_TYPE_RGB:
            _renderFormat = Texture2D::PixelFormat::RGB888;
            break;
        case PNG_COLOR_TYPE_RGB_ALPHA:
            _renderFormat = Texture2D::PixelFormat::RGBA8888;
            break;
        default:
            break;
        }*/

        // read png data
        png_size_t rowbytes;
        png_bytep* row_pointers = (png_bytep*)malloc( sizeof(png_bytep) * _height );

        rowbytes = png_get_rowbytes(png_ptr, info_ptr);

        _dataLen = rowbytes * _height;
        _data = static_cast<unsigned char*>(malloc(_dataLen * sizeof(unsigned char)));
        if (!_data)
        {
            if (row_pointers != nullptr)
            {
                free(row_pointers);
            }
            break;
        }

        for (unsigned short i = 0; i < _height; ++i)
        {
            row_pointers[i] = _data + i*rowbytes;
        }
        png_read_image(png_ptr, row_pointers);

        png_read_end(png_ptr, nullptr);

        // premultiplied alpha for RGBA8888
        if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
        {
            unsigned int* fourBytes = (unsigned int*)_data;
            for(int i = 0; i < _width * _height; i++)
            {
                unsigned char* p = _data + i * 4;
                fourBytes[i] = CC_RGB_PREMULTIPLY_ALPHA(p[0], p[1], p[2], p[3]);
            }
        }

        if (row_pointers != nullptr)
        {
            free(row_pointers);
        }

        ret = true;
    } while (0);

    if (png_ptr)
    {
        png_destroy_read_struct(&png_ptr, (info_ptr) ? &info_ptr : 0, 0);
    }
    return ret;
}
bool read_PNG_file_data (unsigned char*imageData, ssize_t unpackedLen, unsigned char* &_data, int& _width, int& _height, ssize_t& _dataLen){
    // length of bytes to check if it is a valid png file
#define PNGSIGSIZE  8
    bool ret = false;
    png_byte        header[PNGSIGSIZE]   = {0};
    png_structp     png_ptr     =   0;
    png_infop       info_ptr    = 0;

    do
    {
        // png header len is 8 bytes
        //CC_BREAK_IF(dataLen < PNGSIGSIZE);

        // check the data is png or not
        memcpy(header, imageData, PNGSIGSIZE);
        //CC_BREAK_IF(png_sig_cmp(header, 0, PNGSIGSIZE));

        // init png_struct
        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
        CC_BREAK_IF(! png_ptr);

        // init png_info
        info_ptr = png_create_info_struct(png_ptr);
        CC_BREAK_IF(!info_ptr);

#if (CC_TARGET_PLATFORM != CC_PLATFORM_BADA && CC_TARGET_PLATFORM != CC_PLATFORM_NACL && CC_TARGET_PLATFORM != CC_PLATFORM_TIZEN)
        CC_BREAK_IF(setjmp(png_jmpbuf(png_ptr)));
#endif

        // set the read call back function
        tImageSource imageSource;
        imageSource.data    = (unsigned char*)imageData;
        imageSource.size    = unpackedLen;
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
        if (color_type == PNG_COLOR_TYPE_PALETTE)
        {
            png_set_palette_to_rgb(png_ptr);
        }
        // low-bit-depth grayscale images are to be expanded to 8 bits
        if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        {
            bit_depth = 8;
            png_set_expand_gray_1_2_4_to_8(png_ptr);
        }
        // expand any tRNS chunk data into a full alpha channel
        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        {
            png_set_tRNS_to_alpha(png_ptr);
        }
        // reduce images with 16-bit samples to 8 bits
        if (bit_depth == 16)
        {
            png_set_strip_16(png_ptr);
        }

        // Expanded earlier for grayscale, now take care of palette and rgb
        if (bit_depth < 8)
        {
            png_set_packing(png_ptr);
        }
        // update info
        png_read_update_info(png_ptr, info_ptr);
        bit_depth = png_get_bit_depth(png_ptr, info_ptr);
        color_type = png_get_color_type(png_ptr, info_ptr);
      /*  int _renderFormat =
        switch (color_type)
        {
        case PNG_COLOR_TYPE_GRAY:
            _renderFormat = Texture2D::PixelFormat::I8;
            break;
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            _renderFormat = Texture2D::PixelFormat::AI88;
            break;
        case PNG_COLOR_TYPE_RGB:
            _renderFormat = Texture2D::PixelFormat::RGB888;
            break;
        case PNG_COLOR_TYPE_RGB_ALPHA:
            _renderFormat = Texture2D::PixelFormat::RGBA8888;
            break;
        default:
            break;
        }*/

        // read png data
        png_size_t rowbytes;
        png_bytep* row_pointers = (png_bytep*)malloc( sizeof(png_bytep) * _height );

        rowbytes = png_get_rowbytes(png_ptr, info_ptr);

        _dataLen = rowbytes * _height;
        _data = static_cast<unsigned char*>(malloc(_dataLen * sizeof(unsigned char)));
        if (!_data)
        {
            if (row_pointers != nullptr)
            {
                free(row_pointers);
            }
            break;
        }

        for (unsigned short i = 0; i < _height; ++i)
        {
            row_pointers[i] = _data + i*rowbytes;
        }
        png_read_image(png_ptr, row_pointers);

        png_read_end(png_ptr, nullptr);

        // premultiplied alpha for RGBA8888
        if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
        {
            LOGE("FDSKFDSFKSDF");
            unsigned int* fourBytes = (unsigned int*)_data;
            for(int i = 0; i < _width * _height; i++)
            {
                unsigned char* p = _data + i * 4;
                fourBytes[i] = CC_RGB_PREMULTIPLY_ALPHA(p[0], p[1], p[2], p[3]);
            }
        }
        LOGE("HHGGGG");
        if (row_pointers != nullptr)
        {
            free(row_pointers);
        }

        ret = true;
    } while (0);

    if (png_ptr)
    {
        png_destroy_read_struct(&png_ptr, (info_ptr) ? &info_ptr : 0, 0);
    }
    return ret;
}
bool saveToPNG(unsigned char* _data, bool bAlpha, int _width, int _height, const std::string& filePath){
    bool ret = false;
    do
    {
        FILE *fp;
        png_structp png_ptr;
        png_infop info_ptr;
        png_colorp palette;
        png_bytep *row_pointers;

        fp = fopen(filePath.c_str(), "wb");
        CC_BREAK_IF(nullptr == fp);

        png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

        if (nullptr == png_ptr)
        {
            fclose(fp);
            break;
        }

        info_ptr = png_create_info_struct(png_ptr);
        if (nullptr == info_ptr)
        {
            fclose(fp);
            png_destroy_write_struct(&png_ptr, nullptr);
            break;
        }
        if (setjmp(png_jmpbuf(png_ptr)))
        {
            fclose(fp);
            png_destroy_write_struct(&png_ptr, &info_ptr);
            break;
        }
        png_init_io(png_ptr, fp);

        if (bAlpha)
        {
            png_set_IHDR(png_ptr, info_ptr, _width, _height, 8, PNG_COLOR_TYPE_RGB_ALPHA,
                PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        }
        else
        {
            png_set_IHDR(png_ptr, info_ptr, _width, _height, 8, PNG_COLOR_TYPE_RGB,
                PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        }

        palette = (png_colorp)png_malloc(png_ptr, PNG_MAX_PALETTE_LENGTH * sizeof (png_color));
        png_set_PLTE(png_ptr, info_ptr, palette, PNG_MAX_PALETTE_LENGTH);

        png_write_info(png_ptr, info_ptr);

        png_set_packing(png_ptr);

        row_pointers = (png_bytep *)malloc(_height * sizeof(png_bytep));
        if(row_pointers == nullptr)
        {
            fclose(fp);
            png_destroy_write_struct(&png_ptr, &info_ptr);
            break;
        }

        if (!bAlpha)
        {
            for (int i = 0; i < (int)_height; i++)
            {
                row_pointers[i] = (png_bytep)_data + i * _width * 3;
            }

            png_write_image(png_ptr, row_pointers);

            free(row_pointers);
            row_pointers = nullptr;
        }
        else
        {
            if (!bAlpha)
            {
                unsigned char *tempData = static_cast<unsigned char*>(malloc(_width * _height * 3 * sizeof(unsigned char)));
                if (nullptr == tempData)
                {
                    fclose(fp);
                    png_destroy_write_struct(&png_ptr, &info_ptr);

                    free(row_pointers);
                    row_pointers = nullptr;
                    break;
                }

                for (int i = 0; i < _height; ++i)
                {
                    for (int j = 0; j < _width; ++j)
                    {
                        tempData[(i * _width + j) * 3] = _data[(i * _width + j) * 4];
                        tempData[(i * _width + j) * 3 + 1] = _data[(i * _width + j) * 4 + 1];
                        tempData[(i * _width + j) * 3 + 2] = _data[(i * _width + j) * 4 + 2];
                    }
                }

                for (int i = 0; i < (int)_height; i++)
                {
                    row_pointers[i] = (png_bytep)tempData + i * _width * 3;
                }

                png_write_image(png_ptr, row_pointers);

                free(row_pointers);
                row_pointers = nullptr;

                if (tempData != nullptr)
                {
                    free(tempData);
                }
            }
            else
            {
                for (int i = 0; i < (int)_height; i++)
                {
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