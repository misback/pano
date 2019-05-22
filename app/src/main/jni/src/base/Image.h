#pragma once
#include "Common.h"
#include <vector>
#include <string>
// premultiply alpha, or the effect will wrong when want to use other pixel format in Texture2D,
// such as RGB888, RGB5A1
#define CC_RGB_PREMULTIPLY_ALPHA(vr, vg, vb, va) \
    (unsigned)(((unsigned)((unsigned char)(vr) * ((unsigned char)(va) + 1)) >> 8) | \
    ((unsigned)((unsigned char)(vg) * ((unsigned char)(va) + 1) >> 8) << 8) | \
    ((unsigned)((unsigned char)(vb) * ((unsigned char)(va) + 1) >> 8) << 16) | \
    ((unsigned)(unsigned char)(va) << 24))
typedef struct _MipmapInfo
{
    unsigned char* address;
    int len;
    _MipmapInfo():address(nullptr),len(0){}
}MipmapInfo;
    /** @typedef Texture2D::PixelFormat
     Possible texture pixel formats
     */
    enum class PixelFormat
    {
        //! auto detect the type
        AUTO,
        //! 32-bit texture: BGRA8888
        BGRA8888,
        //! 32-bit texture: RGBA8888
        RGBA8888,
        //! 24-bit texture: RGBA888
        RGB888,
        //! 16-bit texture without Alpha channel
        RGB565,
        //! 8-bit textures used as masks
        A8,
        //! 8-bit intensity texture
        I8,
        //! 16-bit textures used as masks
        AI88,
        //! 16-bit textures: RGBA4444
        RGBA4444,
        //! 16-bit textures: RGB5A1
        RGB5A1,
        //! 4-bit PVRTC-compressed texture: PVRTC4
        PVRTC4,
        //! 4-bit PVRTC-compressed texture: PVRTC4 (has alpha channel)
        PVRTC4A,
        //! 2-bit PVRTC-compressed texture: PVRTC2
        PVRTC2,
        //! 2-bit PVRTC-compressed texture: PVRTC2 (has alpha channel)
        PVRTC2A,
        //! ETC-compressed texture: ETC
        ETC,
        //! S3TC-compressed texture: S3TC_Dxt1
        S3TC_DXT1,
        //! S3TC-compressed texture: S3TC_Dxt3
        S3TC_DXT3,
        //! S3TC-compressed texture: S3TC_Dxt5
        S3TC_DXT5,
        //! ATITC-compressed texture: ATC_RGB
        ATC_RGB,
        //! ATITC-compressed texture: ATC_EXPLICIT_ALPHA
        ATC_EXPLICIT_ALPHA,
        //! ATITC-compressed texture: ATC_INTERPOLATED_ALPHA
        ATC_INTERPOLATED_ALPHA,
        //! Default texture format: AUTO
        DEFAULT = AUTO,

        NONE = -1
    };

class Image
{
public:
    friend class TextureCache;
    /**
     * @js ctor
     */
    Image();
    /**
     * @js NA
     * @lua NA
     */
    virtual ~Image();

    /** Supported formats for Image */
    enum class Format
    {
        //! JPEG
        JPG,
        //! PNG
        PNG,
        //! TIFF
        TIFF,
        //! WebP
        WEBP,
        //! PVR
        PVR,
        //! ETC
        ETC,
        //! S3TC
        S3TC,
        //! ATITC
        ATITC,
        //! TGA
        TGA,
        //! Raw Data
        RAW_DATA,
        //! Unknown format
        UNKNOWN
    };

    /**
     * Enables or disables premultiplied alpha for PNG files.
     *
     *  @param enabled (default: true)
     */
    static void setPNGPremultipliedAlphaEnabled(bool enabled) { PNG_PREMULTIPLIED_ALPHA_ENABLED = enabled; }

    /**
    @brief Load image from stream buffer.
    @param data  stream buffer which holds the image data.
    @param dataLen  data length expressed in (number of) bytes.
    @return true if loaded correctly.
    * @js NA
    * @lua NA
    */
    bool initWithImageData(const unsigned char * data, ssize_t dataLen);

    // @warning kFmtRawData only support RGBA8888
    bool initWithRawData(const unsigned char * data, ssize_t dataLen, int width, int height, int bitsPerComponent, bool preMulti = false);

    // Getters
    unsigned char *   getData()               { return _data; }
    ssize_t           getDataLen()            { return _dataLen; }
    Format            getFileType()           { return _fileType; }
    PixelFormat getRenderFormat()  { return _renderFormat; }
    int               getWidth()              { return _width; }
    int               getHeight()             { return _height; }
    bool              hasPremultipliedAlpha() { return _hasPremultipliedAlpha; }
    std::string getFilePath() const { return _filePath; }

    int                      getBitPerPixel();
    bool                     hasAlpha();
    bool                     isCompressed();


    /**
     @brief    Save Image data to the specified file, with specified format.
     @param    filePath        the file's absolute path, including file suffix.
     @param    isToRGB        whether the image is saved as RGB format.
     */
    bool saveToFile(const std::string &filename, bool isToRGB = true);
    bool saveImageToPNG(unsigned char* _data, bool bAlpha, bool isToRGB, int _width, int _height, const std::string& filePath);
    bool saveImageToJPG(unsigned char* _data, bool bAlpha, int _width, int _height, const std::string& filePath);
    bool saveImageToJPG_FlipY(unsigned char* _data, bool bAlpha, int _width, int _height, const std::string& filePath);
public:
    bool initWithJpgData(const unsigned char * data, ssize_t dataLen);
    bool initWithPngData(const unsigned char * data, ssize_t dataLen);
    bool saveImageToPNG(const std::string& filePath, bool isToRGB = true);
    bool saveImageToJPG(const std::string& filePath);
    
    void premultipliedAlpha();
    void clear();
protected:
    /**
     @brief Determine whether we premultiply alpha for png files.
     */
    static bool PNG_PREMULTIPLIED_ALPHA_ENABLED;
    unsigned char *_data;
    ssize_t _dataLen;
    int _width;
    int _height;
    bool _unpack;
    Format _fileType;
    PixelFormat _renderFormat;
    // false if we can't auto detect the image is premultiplied or not.
    bool _hasPremultipliedAlpha;
    std::string _filePath;


protected:
    // noncopyable
    Image(const Image& rImg);
    Image& operator=(const Image&);
    
    Format detectFormat(const unsigned char * data, ssize_t dataLen);
    bool isPng(const unsigned char * data, ssize_t dataLen);
    bool isJpg(const unsigned char * data, ssize_t dataLen);
};

