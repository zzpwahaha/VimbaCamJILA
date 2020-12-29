#include "ImageWriter.h"

#include <QLibrary>
#include <QString>
#include <QSharedPointer>
#include <QDebug>
#include <stdint.h>

#include "VimbaImageTransform\Include\VmbTransform.h"

#include <QMutex>
#include <QMutexLocker>
typedef struct tiff TIFF;

// constants for configuring Tiff tag data
#define    IMGWRITER_IMAGEWIDTH         256    /* image width in pixels (TIFFTAG_IMAGEWIDTH) */
#define    IMGWRITER_IMAGELENGTH        257    /* image height in pixels (TIFFTAG_IMAGELENGTH) */
#define    IMGWRITER_BITSPERSAMPLE      258    /* bits per channel (sample) (TIFFTAG_BITSPERSAMPLE) */
#define    IMGWRITER_COMPRESSION        259    /* data compression technique (TIFFTAG_COMPRESSION) */
#define    IMGWRITER_PHOTOMETRIC        262    /* photometric interpretation (TIFFTAG_PHOTOMETRIC) */
#define    IMGWRITER_XRESOLUTION        282    /* pixels/resolution in x (TIFFTAG_XRESOLUTION) */
#define    IMGWRITER_YRESOLUTION        283    /* pixels/resolution in y (TIFFTAG_YRESOLUTION) */
#define    IMGWRITER_RESOLUTIONUNIT     296    /* units of resolutions (TIFFTAG_RESOLUTIONUNIT) */
#define    IMGWRITER_PLANARCONFIG       284    /* storage organization (TIFFTAG_PLANARCONFIG) */
#define    IMGWRITER_SAMPLESPERPIXEL    277    /* samples per pixel (TIFFTAG_SAMPLESPERPIXEL) */



class TiffLibrary
{
    bool            m_DidLoad;
    QLibrary        m_LibTiffLibrary;
    static QMutex   m_LibraryLock;

    // function pointer typedefs
    typedef TIFF* (*TIFFOpenPrototype)( const char*, const char* );
    typedef void (*TIFFClosePrototype)( TIFF* );
    typedef int (*TIFFSetFieldProtoype)( TIFF*, uint32_t, ... );
    typedef int (*TIFFWriteScanlinePrototype)( TIFF*, void*, uint32_t, uint16_t );

    TiffLibrary()
    {
        m_LibTiffLibrary.setFileNameAndVersion("libtiff", 5);
        m_DidLoad = m_LibTiffLibrary.load(); 

        if (!m_DidLoad )
        {
            m_LibTiffLibrary.setFileNameAndVersion("libtiff", 4); 
            m_DidLoad = m_LibTiffLibrary.load(); 
        }
        if( m_DidLoad )
        {
            TiffOpen = (TIFFOpenPrototype) m_LibTiffLibrary.resolve("TIFFOpen");
            if( TiffOpen == NULL)
            {
                m_DidLoad = false;
                return;
            }
            TiffClose = (TIFFClosePrototype) m_LibTiffLibrary.resolve("TIFFClose");
            if( TiffClose == NULL)
            {
                m_DidLoad = false;
                return;
            }
            TiffSetField = (TIFFSetFieldProtoype) m_LibTiffLibrary.resolve("TIFFSetField");    
            if( TiffSetField == NULL)
            {
                m_DidLoad = false;
                return;
            }
            TiffWriteScanLine = (TIFFWriteScanlinePrototype) m_LibTiffLibrary.resolve("TIFFWriteScanline");
            if( TiffWriteScanLine == NULL)
            {
                m_DidLoad = false;
                return;
            }
        }
    }
public:
    static const TiffLibrary& Get()
    {
        QMutexLocker local_lock( &m_LibraryLock );
        static const TiffLibrary staticLibrary;
        return staticLibrary;
    }
    bool Valid() const { return m_DidLoad; }
    QString GetLastError( void ) const
    {
        return m_LibTiffLibrary.errorString();
    }

    TIFFOpenPrototype           TiffOpen;
    TIFFClosePrototype          TiffClose;
    TIFFSetFieldProtoype        TiffSetField;
    TIFFWriteScanlinePrototype  TiffWriteScanLine;
};
// static init
QMutex TiffLibrary::m_LibraryLock;

VmbError_t GetBitsPerPixelUsed( VmbUint32_t * BitsPerPixelUsed, VmbPixelFormat_t PixelFormat)
{
    if( BitsPerPixelUsed == 0 ) return VmbErrorBadParameter;
    switch(PixelFormat)
    {
    case VmbPixelFormatBayerGR8:
    case VmbPixelFormatBayerRG8:
    case VmbPixelFormatBayerGB8:
    case VmbPixelFormatBayerBG8:
    case VmbPixelFormatMono8:
        *BitsPerPixelUsed = 8;
        return VmbErrorSuccess;
    case VmbPixelFormatBayerGR10:
    case VmbPixelFormatBayerRG10:
    case VmbPixelFormatBayerGB10:
    case VmbPixelFormatBayerBG10:
    case VmbPixelFormatBayerGR10p:
    case VmbPixelFormatBayerRG10p:
    case VmbPixelFormatBayerGB10p:
    case VmbPixelFormatBayerBG10p:
    case VmbPixelFormatMono10:
    case VmbPixelFormatMono10p:
        *BitsPerPixelUsed = 10;
        return VmbErrorSuccess;
    case VmbPixelFormatBayerGR12:
    case VmbPixelFormatBayerRG12:
    case VmbPixelFormatBayerGB12:
    case VmbPixelFormatBayerBG12:
    case VmbPixelFormatMono12:
    case VmbPixelFormatMono12Packed:
    case VmbPixelFormatMono12p:
    case VmbPixelFormatBayerGR12Packed:
    case VmbPixelFormatBayerRG12Packed:
    case VmbPixelFormatBayerGB12Packed:
    case VmbPixelFormatBayerBG12Packed:
    case VmbPixelFormatBayerGR12p:
    case VmbPixelFormatBayerRG12p:
    case VmbPixelFormatBayerGB12p:
    case VmbPixelFormatBayerBG12p:
        *BitsPerPixelUsed = 12;
        return VmbErrorSuccess;
    case VmbPixelFormatMono14:
        *BitsPerPixelUsed = 14;
        return VmbErrorSuccess;
    case VmbPixelFormatBayerGR16:
    case VmbPixelFormatBayerRG16:
    case VmbPixelFormatBayerGB16:
    case VmbPixelFormatBayerBG16:
    case VmbPixelFormatMono16:
        *BitsPerPixelUsed = 16;
        return VmbErrorSuccess;
    case VmbPixelFormatRgb8:
    case VmbPixelFormatBgr8:
        *BitsPerPixelUsed = 24;
        return VmbErrorSuccess;
    case VmbPixelFormatArgb8:
    case VmbPixelFormatBgra8:
        *BitsPerPixelUsed = 32;
        return VmbErrorSuccess;
    case VmbPixelFormatRgb10:
    case VmbPixelFormatBgr10:
        *BitsPerPixelUsed = 30;
        return VmbErrorSuccess;
    case VmbPixelFormatRgb12:
    case VmbPixelFormatBgr12:
        *BitsPerPixelUsed = 36;
        return VmbErrorSuccess;
    case VmbPixelFormatRgb14:
    case VmbPixelFormatBgr14:
        *BitsPerPixelUsed = 42;
        return VmbErrorSuccess;
    case VmbPixelFormatRgb16:
    case VmbPixelFormatBgr16:
        *BitsPerPixelUsed = 48;
        return VmbErrorSuccess;
    case VmbPixelFormatRgba10:
    case VmbPixelFormatBgra10:
        *BitsPerPixelUsed = 40;
        return VmbErrorSuccess;
    case VmbPixelFormatRgba12:
    case VmbPixelFormatBgra12:
        *BitsPerPixelUsed = 48;
        return VmbErrorSuccess;
    case VmbPixelFormatRgba14:
    case VmbPixelFormatBgra14:
        *BitsPerPixelUsed = 56;
        return VmbErrorSuccess;
    case VmbPixelFormatRgba16:
    case VmbPixelFormatBgra16:
        *BitsPerPixelUsed = 64;
        return VmbErrorSuccess;
    case VmbPixelFormatYuv411:
    case VmbPixelFormatYCbCr411_8_CbYYCrYY:
        *BitsPerPixelUsed = 12;
        return VmbErrorSuccess;
    case VmbPixelFormatYuv422:
    case VmbPixelFormatYCbCr422_8_CbYCrY:
        *BitsPerPixelUsed = 16;
        return VmbErrorSuccess;
    case VmbPixelFormatYuv444:
    case VmbPixelFormatYCbCr8_CbYCr:
        *BitsPerPixelUsed = 24;
        return VmbErrorSuccess;
    default:
        return VmbErrorBadParameter;
    }
}

// helper functions to test if pixel format is of type Mono
bool IsMonochrome( VmbPixelFormat_t format )
{
    bool isMono;

    switch(format)
    {
    case VmbPixelFormatMono8:            
    case VmbPixelFormatMono10:
    case VmbPixelFormatMono10p:
    case VmbPixelFormatMono12:
    case VmbPixelFormatMono12Packed:
    case VmbPixelFormatMono12p:
    case VmbPixelFormatMono14:
    case VmbPixelFormatMono16:
        isMono = true;
        break;
    default:
        isMono = false;
    }

    return isMono;
}


// helper functions to test if pixel format is packed
bool IsPackedFormat( VmbPixelFormat_t format )
{
    bool isPacked;

    switch(format)
    {
    case VmbPixelFormatMono10p:
    case VmbPixelFormatMono12Packed:
    case VmbPixelFormatMono12p:
    case VmbPixelFormatBayerGR12Packed:
    case VmbPixelFormatBayerRG12Packed:
    case VmbPixelFormatBayerGB12Packed:
    case VmbPixelFormatBayerBG12Packed:
    case VmbPixelFormatBayerGR10p:
    case VmbPixelFormatBayerRG10p:
    case VmbPixelFormatBayerGB10p:
    case VmbPixelFormatBayerBG10p:
    case VmbPixelFormatBayerGR12p:
    case VmbPixelFormatBayerRG12p:
    case VmbPixelFormatBayerGB12p:
    case VmbPixelFormatBayerBG12p:
        isPacked = true;
        break;
    default:
        isPacked = false;
    }

    return isPacked;
}


// Helper function to fill in the TIFF file parameters 
bool WriteStandardTags( TIFF* TiffFile, const tFrameInfo &frame_info)
{
    int         result = 0;
    VmbUint32_t BitsPerPixelUsed;
    // calculate bit depth
    GetBitsPerPixelUsed(&BitsPerPixelUsed, frame_info.PixelFormat() );
    VmbUint16_t BitsPerSample = static_cast<VmbUint16_t>( ( BitsPerPixelUsed > 8 ) ? 16 : 8 );
    
    // check if function symbol could be resolved (using QLibrary) 
    result = (*TiffLibrary::Get().TiffSetField)( TiffFile, IMGWRITER_IMAGEWIDTH, static_cast<unsigned long>(frame_info.Width() ));
    if (result == 0)
        return false;
    result = (*TiffLibrary::Get().TiffSetField)( TiffFile, IMGWRITER_IMAGELENGTH, static_cast<unsigned long>(frame_info.Height() ));
    if (result == 0)
        return false;
    result = (*TiffLibrary::Get().TiffSetField)( TiffFile, IMGWRITER_COMPRESSION, static_cast<unsigned short>(1) ); // No compression
    if (result == 0)
        return false;
    result = (*TiffLibrary::Get().TiffSetField)( TiffFile, IMGWRITER_XRESOLUTION, static_cast<float>(1.0F));
    if (result == 0)
        return false;
    result = (*TiffLibrary::Get().TiffSetField)( TiffFile, IMGWRITER_YRESOLUTION, static_cast<float>(1.0F));
    if (result == 0)
        return false;
    result = (*TiffLibrary::Get().TiffSetField)( TiffFile, IMGWRITER_RESOLUTIONUNIT, static_cast<unsigned short>(1)); // No units
    if (result == 0)
        return false;
    result = (*TiffLibrary::Get().TiffSetField)( TiffFile, IMGWRITER_PLANARCONFIG, static_cast<unsigned short>(1));
    if (result == 0)
        return false;

        // raw image data, bayer or mono
    if ( IsMonochrome( frame_info.PixelFormat() ) || ! frame_info.UseColorInterpolation() ) 
    {
        // Monochrome
        result = (*TiffLibrary::Get().TiffSetField)( TiffFile, IMGWRITER_BITSPERSAMPLE, BitsPerSample );
        if (result == 0)
            return false;
        result = (*TiffLibrary::Get().TiffSetField)( TiffFile, IMGWRITER_PHOTOMETRIC, static_cast<unsigned short>(1)); // Monochrome, black=0
        if (result == 0)
            return false;
    }
    else // bayer format && color interpolation on
    {
        // Color
        result = (*TiffLibrary::Get().TiffSetField)( TiffFile, IMGWRITER_BITSPERSAMPLE, BitsPerSample, BitsPerSample, BitsPerSample );
        if (result == 0)
            return false;
        result = (*TiffLibrary::Get().TiffSetField)( TiffFile, IMGWRITER_SAMPLESPERPIXEL, static_cast<unsigned short>(3));
        if (result == 0)
            return false;
        result = (*TiffLibrary::Get().TiffSetField)( TiffFile, IMGWRITER_PHOTOMETRIC, static_cast<unsigned short>(2)); // RGB color
        if (result == 0)
            return false;
    }
    return true;
}

// Helper function to write the data buffer to the TIFF file
bool WriteFrameBuffer( TIFF* TiffFile, const tFrameInfo &frame_info )
{
    const VmbUint32_t   ImageSizePixels = frame_info.Width() * frame_info.Height();
    VmbUint32_t         BitsPerPixelUsed;
    GetBitsPerPixelUsed(&BitsPerPixelUsed, frame_info.PixelFormat() );
    VmbUint32_t         BytesPerPixel   = ( BitsPerPixelUsed > 8 ) ? 2 : 1;
    
    // allocate the temporary buffer used to write to the file
    try
    {
        QSharedPointer<unsigned char> tmpBuffer;

        // copy or manipulate data into temporary buffer
        if  (       IsMonochrome(frame_info.PixelFormat() )
                || !frame_info.UseColorInterpolation()
            ) // raw mono or bayer (no color interpolation)
        {       
            if( 8 == BitsPerPixelUsed ) // mono or raw 8 bit transfer
            {
                 tmpBuffer = frame_info.DataPtr();
            }
            else // 16 bit tranfer formats
            {   
                if( IsPackedFormat(frame_info.PixelFormat() ) ) // unpack mono_p raw_p
                {
                    tmpBuffer = QSharedPointer<unsigned char> (new unsigned char [ImageSizePixels * 2], DeleteArray<unsigned char> ); // space for mono 16
                    if( tmpBuffer == NULL)
                    {
                        return false;
                    }
                    VmbImage            sourceImage;
                    VmbImage            destinationImage;

                    // set size member for verification inside API
                    sourceImage.Size = sizeof ( sourceImage );
                    destinationImage.Size = sizeof ( destinationImage );

                    // attach the data buffers
                    sourceImage.Data = (void*) frame_info.Data(); // hard cast from const
                    destinationImage.Data = tmpBuffer.data();

                    // fill image info from pixel format string
                    VmbSetImageInfoFromPixelFormat ( frame_info.PixelFormat(), frame_info.Width(), frame_info.Height(), &sourceImage );

                    // fill image info from pixel format
                    VmbSetImageInfoFromInputImage(&sourceImage, VmbPixelLayoutMono, 16, &destinationImage);

                    // perform the transformation
                    if ( VmbErrorSuccess != VmbImageTransform ( &sourceImage , &destinationImage , 0 , 0 ) )
                    {
                        return false;
                    }
                }
                else
                {
                    tmpBuffer = frame_info.DataPtr();
                }
                // change to msb
                unsigned short*                 pData       = (unsigned short*)tmpBuffer.data();
                const unsigned short*           pDataEnd    = pData + ImageSizePixels;
                const unsigned short            bitshift    = 16 - BitsPerPixelUsed;

                while (pData < pDataEnd)
                {
                    *(pData) <<= bitshift;
                    ++pData;
                }
            }
        }
        else // bayer && color interpolation on
        {
            if( BytesPerPixel == 1 ) // bayer8
            {
                tmpBuffer = QSharedPointer<unsigned char>( new unsigned char[ ImageSizePixels * 3], DeleteArray<unsigned  char> ); // storage for RGB pixel

                VmbImage            sourceImage, destinationImage;

                sourceImage.Size = sizeof( destinationImage );
                sourceImage.Data = (void*)frame_info.Data();
                VmbSetImageInfoFromPixelFormat( frame_info.PixelFormat(), frame_info.Width(), frame_info.Height(), &sourceImage );

                destinationImage.Size = sizeof( destinationImage );
                destinationImage.Data = tmpBuffer.data();

                VmbSetImageInfoFromInputImage(&sourceImage, VmbPixelLayoutRGB, 8, &destinationImage);

                if( VmbErrorSuccess != VmbImageTransform( &sourceImage, &destinationImage, NULL, 0) )
                {
                    return false;
                }
            }
            else
            {
                tmpBuffer = QSharedPointer<unsigned char>( new unsigned char[ ImageSizePixels * 3 * BytesPerPixel ], DeleteArray<unsigned char> ); // storage for RGB 16 bit
            

                VmbImage sourceImage;
                sourceImage.Size = sizeof( sourceImage );
                sourceImage.Data = (void*)frame_info.Data();
                VmbSetImageInfoFromPixelFormat( frame_info.PixelFormat(), frame_info.Width(), frame_info.Height(), &sourceImage );

                VmbImage destinationImage;
                destinationImage.Size = sizeof( destinationImage );
                destinationImage.Data = tmpBuffer.data();
                VmbSetImageInfoFromInputImage(&sourceImage, VmbPixelLayoutRGB, 16, &destinationImage);
                        
                if( VmbErrorSuccess != VmbImageTransform( &sourceImage, &destinationImage, NULL, 0) )
                {
                    return false;
                }

                unsigned short*                 pDest = (unsigned short*)tmpBuffer.data();
                const unsigned short* const     pDestEnd = pDest + ImageSizePixels * 3;
                const unsigned short            bitshift = 16 - BitsPerPixelUsed;
                while (pDest < pDestEnd)
                {
                    *(pDest++) <<= bitshift;
                }
            }
        }

        // Write data into TIFF file.
        const VmbUint32_t   ColorsPerPixel  = ( IsMonochrome( frame_info.PixelFormat() ) || !frame_info.UseColorInterpolation() ) ? 1 : 3;
        const VmbUint32_t   LineSize        = BytesPerPixel * frame_info.Width() * ColorsPerPixel; // in bytes
        unsigned char* pScanLine = tmpBuffer.data();
        for (unsigned long i = 0; i < frame_info.Height(); i++)
        {
            int result = (*TiffLibrary::Get().TiffWriteScanLine)( TiffFile, pScanLine, i, 0);
            if (result == 0)
                return false;

            pScanLine += LineSize; 
        }
    }
    catch (std::bad_alloc& badAlloc)
    {
        qDebug() << "bad_alloc caught: " << badAlloc.what() << '\n';
        return false;
    }
    catch(...)
    {
        return false;
    }


    return true;
}

// Attempts to load the LibTiff library dynamically using Qt QLibrary 
bool ImageWriter::IsAvailable( void )
{
    return TiffLibrary::Get().Valid();
}

// Get the error message from the last QLibrary call
QString ImageWriter::GetLastError( void )
{
    return TiffLibrary::Get().GetLastError();
}

// Write the frame to the specified file 
bool ImageWriter::WriteTiff(  const tFrameInfo & frame_info, const char* filename, ILogTarget *log)
{
    bool result = true;
    // Create a new TIFF file by constructing a Tiff object
    TIFF *TiffFile = (*TiffLibrary::Get().TiffOpen)(filename, "w");

    if( NULL != TiffFile )
    {
        // Set the TIFF tags
        result = WriteStandardTags(TiffFile, frame_info );

        if( true == result)
        {
            // Write the image data 
            result = WriteFrameBuffer(TiffFile, frame_info);
            if( !result)
            {
                if( NULL != log)
                {
                    log->Log("error could not write frame buffer");
                }
            }
        }
        else
        {
            if( NULL != log)
            {
                log->Log("error could not write TIFF standard flags");
            }
        }
        (*TiffLibrary::Get().TiffClose)(TiffFile);
    }
    else
    {
        if( NULL != log)
        {
            log->Log("error could not open tiff file for writing");
        }
        result = false;
    }
    return result;
}
