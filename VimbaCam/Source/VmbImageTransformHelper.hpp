#ifndef VMB_IMAGE_TRANSFORM_HELPER_H_
#define VMB_IMAGE_TRANSFORM_HELPER_H_

#include "VimbaImageTransform\Include\VmbTransform.h"
#include <exception>
#include <string>

namespace AVT
{
    class vimba_transform_exception : public std::exception
    {
    private:
        std::string     m_Function;
        std::string     m_Message;
        VmbError_t      m_Result;
    public:
        vimba_transform_exception( const char* const&fun, const char* const & msg, VmbError_t result)
            : m_Result( result)
        {
            try
            {   if( NULL != fun )
                    m_Function = std::string( fun);
            }
            catch (...) {}
            try
            {   if( NULL != msg )
                    m_Message = std::string( msg);
            }
            catch (...) {}
        }
        ~vimba_transform_exception() throw() {}
        const std::string&  Function()  const   { return m_Function; }
        const std::string&  Message()   const   { return m_Message; }
        VmbError_t          Result()    const   { return m_Result; }
    };
    inline VmbUint32_t GetUsedBits( VmbPixelFormatType pf)
    {
        switch( pf )
        {
        default:
            throw vimba_transform_exception( __FUNCTION__,"unknown pixel format", VmbErrorBadParameter);
        case VmbPixelFormatMono8:
        case VmbPixelFormatBayerGR8:
        case VmbPixelFormatBayerRG8:
        case VmbPixelFormatBayerGB8:
        case VmbPixelFormatBayerBG8:
        case VmbPixelFormatRgb8:
        case VmbPixelFormatBgr8:
        case VmbPixelFormatArgb8:
        case VmbPixelFormatBgra8:
        case VmbPixelFormatYuv411:
        case VmbPixelFormatYuv422:
        case VmbPixelFormatYuv444:
            return 8;
        case VmbPixelFormatMono10:
        case VmbPixelFormatMono10p:
        case VmbPixelFormatBayerGR10:
        case VmbPixelFormatBayerRG10:
        case VmbPixelFormatBayerGB10:
        case VmbPixelFormatBayerBG10:
        case VmbPixelFormatBayerGR10p:
        case VmbPixelFormatBayerRG10p:
        case VmbPixelFormatBayerGB10p:
        case VmbPixelFormatBayerBG10p:
            return 10;
        case VmbPixelFormatMono12:
        case VmbPixelFormatMono12Packed:
        case VmbPixelFormatMono12p:
        case VmbPixelFormatBayerGR12:
        case VmbPixelFormatBayerRG12:
        case VmbPixelFormatBayerGB12:
        case VmbPixelFormatBayerBG12:
        case VmbPixelFormatBayerGR12Packed:
        case VmbPixelFormatBayerRG12Packed:
        case VmbPixelFormatBayerGB12Packed:
        case VmbPixelFormatBayerBG12Packed:
        case VmbPixelFormatBayerGR12p:
        case VmbPixelFormatBayerRG12p:
        case VmbPixelFormatBayerGB12p:
        case VmbPixelFormatBayerBG12p:
        case VmbPixelFormatRgb12:
            return 12;
        case VmbPixelFormatMono14:
            return 14;
        case VmbPixelFormatMono16:
        case VmbPixelFormatBayerGR16:
        case VmbPixelFormatBayerRG16:
        case VmbPixelFormatBayerGB16:
        case VmbPixelFormatBayerBG16:
        case VmbPixelFormatRgb16:
            return 16;
        }
    }
    inline bool IsRawPixelFormat(VmbPixelFormatType pf)
    {
        switch( pf)
        {
        default:
            return false;
        case VmbPixelFormatBayerGR8:
        case VmbPixelFormatBayerRG8:
        case VmbPixelFormatBayerGB8:
        case VmbPixelFormatBayerBG8:
        case VmbPixelFormatBayerGR10:
        case VmbPixelFormatBayerRG10:
        case VmbPixelFormatBayerGB10:
        case VmbPixelFormatBayerBG10:
        case VmbPixelFormatBayerGR10p:
        case VmbPixelFormatBayerRG10p:
        case VmbPixelFormatBayerGB10p:
        case VmbPixelFormatBayerBG10p:
        case VmbPixelFormatBayerGR12:
        case VmbPixelFormatBayerRG12:
        case VmbPixelFormatBayerGB12:
        case VmbPixelFormatBayerBG12:
        case VmbPixelFormatBayerGR12Packed:
        case VmbPixelFormatBayerRG12Packed:
        case VmbPixelFormatBayerGB12Packed:
        case VmbPixelFormatBayerBG12Packed:
        case VmbPixelFormatBayerGR12p:
        case VmbPixelFormatBayerRG12p:
        case VmbPixelFormatBayerGB12p:
        case VmbPixelFormatBayerBG12p:
        case VmbPixelFormatBayerGR16:
        case VmbPixelFormatBayerRG16:
        case VmbPixelFormatBayerGB16:
        case VmbPixelFormatBayerBG16:
            return true;
        }
    }
    inline VmbPixelFormatType GetCompatibleMonoPixelFormatForRaw( VmbPixelFormatType pf)
    {
        if(! IsRawPixelFormat( pf) )
            throw vimba_transform_exception(__FUNCTION__,"call for non raw format", VmbErrorBadParameter);
        switch( 0xff&(pf>>16) )
        {
        default:
            vimba_transform_exception( __FUNCTION__," not supported mono type", VmbErrorBadParameter);
        case 8:
            return VmbPixelFormatMono8;
        case 12:
            return VmbPixelFormatMono12Packed;
        case 16:
            switch( GetUsedBits( pf) )
            {
            default:
                return VmbPixelFormatMono16;
            case 10:
                return VmbPixelFormatMono10;
            case 12:
                return VmbPixelFormatMono12;
            case 14: 
                return VmbPixelFormatMono14;
            }
        }
    }
    //
    // default empty image traits
    //
    template<typename IMAGE_TYPE>  struct image_traits{};
    //
    // CImage traits only available if ATL Image is available
    //
#ifdef _ATL_VER
    template<> struct image_traits<CImage>
    {
        static bool IsNull(const CImage &img)
        {
            return img.IsNull();
        }
        static int Width(const CImage &img)
        {
            return img.GetWidth();
        }
        static int Hight(const CImage &img)
        {
            return img.GetHeight();
        }
        static int BitsPerPixel( const CImage &img)
        {
            return img.GetBPP();
        }
        static void* DataBegin( CImage &img)
        {
            return img.GetBits();
        }
        static int Pitch(const CImage &img)
        {
            return img.GetPitch();
        }
        static size_t ByteCount( const CImage &img)
        {
            int pitch,h;
            pitch   = img.GetPitch();
            if( pitch < 0)
            {
                pitch =-pitch;
            }
            h       = img.GetHeight();
            return  pitch*h;
        }
    };
#endif
    // 
    // QImage traits only available if QT is available
    //
#ifdef QT_VERSION
    template<> struct image_traits<QImage>
    {
        static bool IsNull(const QImage &img)
        {
            return img.isNull();
        }
        static int Width(const QImage &img)
        {
            return img.width();
        }
        static int Hight(const QImage &img)
        {
            return img.height();
        }
        static int BitsPerPixel( const QImage &img)
        {
            switch( img.format() )
            {
            case QImage::Format_Indexed8:   return 8;
            case QImage::Format_RGB888:     return 24;
            case QImage::Format_ARGB32:     return 32;
            case QImage::Format_RGB32:      return 32;
            default: return 0;
            }
        }
        static void* DataBegin( QImage &img)
        {
            return img.bits();
        }
        static int Pitch(const QImage &img)
        {
            return img.bytesPerLine();
        }
        static size_t ByteCount( const QImage &img)
        {
            return  img.byteCount();
        }
    };
#endif

    template <typename IMAGE_TYPE>
    VmbError_t VmbImageTransform( IMAGE_TYPE & Dst, VmbVoidPtr_t Src, VmbUint32_t w, VmbUint32_t h,VmbPixelFormat_t pf)
    {
        VmbError_t Result;
        typedef image_traits<IMAGE_TYPE> image_traits_;
        if( image_traits_::IsNull( Dst) )
        {
            return VmbErrorBadParameter;
        }
        VmbImage                    source_image;
        VmbImage                    destination_image;
        source_image.Size       =   sizeof(source_image);
        destination_image.Size  =   sizeof(destination_image);
        Result = VmbSetImageInfoFromPixelFormat( pf,w ,h , &source_image );
        if( VmbErrorSuccess != Result)
        {
            return Result;
        }
        
        VmbUint32_t     dst_bits_per_pixel = image_traits_::BitsPerPixel( Dst);
        std::string name;
        switch( dst_bits_per_pixel )
        {
        default:
            return VmbErrorBadParameter;
        case 8:
            name = "Mono8_REC601";
            break;
        case 24:
            name = "BGR24";
            break;
        case 32:
            name = "BGRA32";
            break;
        }
        Result = VmbSetImageInfoFromString(     name.c_str(), 
                                                static_cast<VmbUint32_t>( name.size() ), 
                                                static_cast<VmbUint32_t> ( image_traits_::Width(Dst) ),
                                                static_cast<VmbUint32_t> ( image_traits_::Hight(Dst) ),
                                                &destination_image);
        source_image.Data       = Src;
        destination_image.Data  = image_traits_::DataBegin(Dst);
        return VmbImageTransform( &source_image,&destination_image,NULL,0 );
    }

    //
    // Method:    VmbImageTransform()
    //
    // Purpose:   convert FRAME_TYPE to IMAGE_TYPE
    //
    // Parameters:
    //  [in,out]    Dst    destination image with matching dimensions and set pixel format type with concept image_traits
    //  [in]        Src    source frame that exposes width,height,buffer,pixelFormat members
    // Returns:
    //  - VmbErrorSuccess:       If no error
    //  - VmbErrorBadParameter:  if any parameter is invalid or not supported
    //
    // Details:   On successful return, the API is initialized; this is a necessary call.
    //
    //
    template <typename IMAGE_TYPE,typename FRAME_TYPE>
    VmbError_t VmbImageTransform( IMAGE_TYPE & Dst,const FRAME_TYPE * frame)
    {
        if( NULL == frame)
        {
            return VmbErrorBadParameter;
        }
        return VmbImageTransform( Dst, frame->buffer,frame->width, frame->height,frame->pixelFormat);
    }
}

#endif