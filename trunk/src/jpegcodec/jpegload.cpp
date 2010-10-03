/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
//     Intel® Integrated Performance Primitives JPEG Viewer Sample
//     for Linux*
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel® Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ipplic.htm located in the root directory of your Intel® IPP
//  product installation for more information.
//
//  JPEG is an international standard promoted by ISO/IEC and other
//  organizations. Implementations of these standards, or the standard enabled
//  platforms may require licenses from various entities, including
//  Intel Corporation.
//
*/

#include "jpegload.h"

//#include "jpegview.h"

// application version info
#define MAJOR 5
#define MINOR 1

#define DEFAULT_WIDTH  640
#define DEFAULT_HEIGHT 480

void RGBA_FPX_to_BGRA(Ipp8u* pSrc,int width,int height)
{
  int    i;
  int    j;
  int    lineStep;
  Ipp8u  r, g, b, a;
  Ipp8u* ptr;

  lineStep = width*4;

  for(i = 0; i < height; i++)
  {
    ptr = pSrc + i*lineStep;
    for(j = 0; j < width; j++)
    {
      r = ptr[0];
      g = ptr[1];
      b = ptr[2];
      a = ptr[3];
      ptr[2] = (Ipp8u)((r*a+1) >> 8);
      ptr[1] = (Ipp8u)((g*a+1) >> 8);
      ptr[0] = (Ipp8u)((b*a+1) >> 8);
      ptr += 4;
    }
  }

  return;
} // RGBA_FPX_to_BGRA()


void BGRA_to_RGBA(Ipp8u* pSrc,int width,int height)
{
  int i, j, line_width;
  Ipp8u r, g, b, a;
  Ipp8u* ptr;

  line_width = width*4;

  for(i = 0; i < height; i++)
  {
    ptr = pSrc + i*line_width;
    for(j = 0; j < width; j++)
    {
      b = ptr[0];
      g = ptr[1];
      r = ptr[2];
      a = ptr[3];
      ptr[0] = r;
      ptr[1] = g;
      ptr[2] = b;
      ptr += 4;
    }
  }

  return;
} // BGRA_to_RGBA()


const char* getModeStr(JMODE mode)
{
  const char* s;
  switch(mode)
  {
    case JPEG_BASELINE:    s = "BAS"; break;
    case JPEG_PROGRESSIVE: s = "PRG"; break;
    case JPEG_LOSSLESS:    s = "LSL"; break;
    default:               s = "UNK"; break;
  }

  return s;
} // getModeStr()


const char* getSamplingStr(JSS sampling)
{
  const char* s;
  switch(sampling)
  {
    case JS_444: s = "444"; break;
    case JS_422: s = "422"; break;
    case JS_244: s = "244"; break;
    case JS_411: s = "411"; break;
    default:     s = "Other"; break;
  }

  return s;
} // getSamplingStr()


const char* getColorStr(JCOLOR color)
{
  const char* s;
  switch(color)
  {
    case JC_GRAY:  s = "Gray";    break;
    case JC_RGB:   s = "RGB";     break;
    case JC_BGR:   s = "BGR";     break;
    case JC_YCBCR: s = "YCbCr";   break;
    case JC_CMYK:  s = "CMYK";    break;
    case JC_YCCK:  s = "YCCK";    break;
    default:       s = "Unknown"; break;
  }
  return s;
} // getColorStr()


void
get_jpeg_info( const char *fileName, int *width, int *height, int *channels)
{
    struct stat st;
    stat( fileName, &st );
    uint size = st.st_size;
    
    FILE *f = fopen( fileName, "r" );
    
    if ( !f ) {
        printf("Error reading JPEG info for file %s\n", fileName);
        return;
    }

    //printf("file %s size: %d\n", fileName, size);
    
    Ipp8u* buf = (Ipp8u*)ippMalloc(size);
    //Ipp64u       c0, c1;
    //clock_t      t0, t1;
    JCOLOR       jpeg_color;
    JSS          jpeg_sampling;
    int          jpeg_nChannels;
    int          jpeg_precision;
    //int          imageSize;
    
    fflush(f);
    
    //int r;
    
    fread( buf, size, 1, f );
    
    fclose(f);
    
    JERRCODE     jerr;
    //IppStatus    status;
    
    CJPEGMetaData metadata;
    CJPEGDecoder decoder;
    int img_width, img_height;
    
    jerr = decoder.SetSource(buf,(int)size);
    if(JPEG_OK != jerr)
        {
    fprintf(stderr,"decoder.SetSource() failed, %s\n",GetErrorStr(jerr));
    return;
        }
    
    jerr = decoder.ReadHeader(
                              &img_width,
                              &img_height,
                              &jpeg_nChannels,
                              &jpeg_color,
                              &jpeg_sampling,
                              &jpeg_precision);
    
    if(JPEG_OK != jerr)
        {
            fprintf(stderr,"decoder.ReadHeader() failed, %s\n",GetErrorStr(jerr));
            return;
        }
        
    *width = img_width;
    *height = img_height;
    *channels = jpeg_nChannels;

    ippFree(buf);
}

/*
void write_jpeg( ImgColor *src, const char *filename, int width, 
                 int height, int nchannels )
{
    write_jpeg (src->IplImage(), filename, width, height, nchannels);
}
*/
void write_jpeg( unsigned char *src, const char *filename, int width, 
                 int height, int nchannels )
{
  unsigned char *tmp = NULL;
  if (nchannels == 1) {
      tmp = (unsigned char*)malloc(3*width*height);
      for (int i=0;i<3*width*height;i++)
          tmp[i]=0;
      for (int r=0;r<height;r++) {
          for (int c=0;c<width;c++) {
              for (int k=0;k<3;k++) {
                  tmp[3*(r*width+c)+k] = src[r*width+c];
              }
          }
      }
  } else {
      tmp = src;
  }

  Ipp8u* buf;
  int imageSize = width * height * 3;

//if(imageSize < 4096)
  //{
  //  imageSize = 4096;
  //}

  buf = (Ipp8u*)ippMalloc(imageSize);
  if(NULL == buf)
  {
    fprintf(stderr,"can't allocate %d bytes of memory\n",imageSize);
    return;
  }

  //Ipp64u  c0, c1;
  //clock_t t0, t1;
  int jpeg_quality  = 100;
  JSS jpeg_sampling = JS_411;
  JCOLOR in_color = JC_BGR;
  JCOLOR jpeg_color = JC_YCBCR;
  JERRCODE jerr;
  CJPEGEncoder encoder;

  IppiSize     m_imageDims;
  m_imageDims.width = width;
  m_imageDims.height = height;
  int m_nChannels = 3;
  int m_lineStep = width * 3;

  encoder.SetSource(tmp,m_lineStep,m_imageDims,m_nChannels,in_color);
  encoder.SetDestination(buf,imageSize,jpeg_quality,jpeg_color,jpeg_sampling,JPEG_BASELINE);

  //if(JC_CMYK == in_color)
  //{
  //  BGRA_to_RGBA(m_imageData,m_imageDims.width,m_imageDims.height);
  //}

  jerr = encoder.WriteHeader();
  if(JPEG_OK != jerr)
  {
    fprintf(stderr,"encoder.WriteHeader() failed, %s\n",GetErrorStr(jerr));
    return;
  }

  jerr = encoder.WriteData();
  if(JPEG_OK != jerr)
  {
    fprintf(stderr,"encoder.WriteData() failed, %s\n",GetErrorStr(jerr));
    return;
  }

  FILE *fp = fopen( filename, "w" );
  
  fwrite( (const char*)buf, imageSize, 1, fp );

  fclose(fp);

  ippFree(buf);

  if (nchannels == 1)
      free (tmp);
} 

unsigned char *jpeg_decompress (unsigned char *src, int size, int *width, int *height, int *channels)
{
    JCOLOR       jpeg_color;
    JSS          jpeg_sampling;
    int          jpeg_nChannels;
    int          jpeg_precision;

    JERRCODE     jerr;

    CJPEGMetaData metadata;
    CJPEGDecoder decoder;
    int img_width, img_height;
    
    jerr = decoder.SetSource(src,(int)size);
    if(JPEG_OK != jerr)
        {
            ippFree(src);
            fprintf(stderr,"decoder.SetSource() failed, %s\n",GetErrorStr(jerr));
            return NULL;
        }
    
    jerr = decoder.ReadHeader(
                              &img_width,
                              &img_height,
                              &jpeg_nChannels,
                              &jpeg_color,
                              &jpeg_sampling,
                              &jpeg_precision);
    
    if(JPEG_OK != jerr)
        {
            fprintf(stderr,"decoder.ReadHeader() failed, %s\n",GetErrorStr(jerr));
            return NULL;
        }
    
    if (width)    *width = img_width;
    if (height)   *height = img_height;
    if (channels) *channels = jpeg_nChannels;
    
    if(decoder.m_exif_app1_detected)
        {
            int i, j, n;
            
            metadata.ProcessAPP1_Exif(decoder.m_exif_app1_data,decoder.m_exif_app1_data_size);

            for(i = TIFF_IFD_0; i < TIFF_IFD_MAX; i++)
                {
                    n = metadata.GetIfdNumEntries(i);
                    for(j = 0; j < n; j++)
      {
          printf("%s\n",metadata.ShowIfdData(i,j));
      }
                }
        }
    
    
    JCOLOR m_color;
    int m_nChannels;
    
    switch(jpeg_nChannels)
        {
        case 1:
            m_nChannels = 1;
            m_color     = JC_GRAY;
            break;
            
        case 3:
            m_nChannels = 3;
            m_color     = JC_BGR;
            break;
            
        case 4:
            m_nChannels = 4;
            m_color     = JC_CMYK;
            break;
            
        default:
            jpeg_color  = JC_UNKNOWN;
            m_color     = JC_UNKNOWN;
            m_nChannels = jpeg_nChannels;
            break;
        }
    
    IppiSize     m_imageDims;
    m_imageDims.width = img_width;
    m_imageDims.height = img_height;
    
    int m_lineStep  = m_imageDims.width * m_nChannels;
    int imageSize   = m_lineStep * m_imageDims.height;
    
    Ipp8u *temp = (unsigned char*)malloc(imageSize);
    
    //dbg( DBG_INFO, "Setting destination...");
    
    jerr = decoder.SetDestination(
                                  temp,
                                  img_width * m_nChannels,
                                  m_imageDims,
                                  jpeg_nChannels,
                                  m_color);
    
    if(JPEG_OK != jerr)
        {
            fprintf(stderr,"decoder.SetDestination() failed, %s\n",GetErrorStr(jerr));
            return NULL;
        }
    
    jerr = decoder.ReadData();
    if(JPEG_OK != jerr)
        {
            fprintf(stderr,"decoder.ReadData() failed, %s\n",GetErrorStr(jerr));
            return NULL;
        }

    
    return temp;
}

#if 0
ImgColor *
load_jpeg(const char* fileName, int *width, int *height, int *channels)
{

    struct stat st;
    stat( fileName, &st );
    uint size = st.st_size;

    FILE *f = fopen( fileName, "r" );

    if ( !f ) {
        printf("Error opening file %s\n", fileName);
        return NULL;
    }

    
    /*
  if(!f.open(IO_ReadOnly))
    return;

  uint size = f.size();

  Ipp8u* buf = (Ipp8u*)ippMalloc(size);
  if(NULL == buf)
  {
    fprintf(stderr,"can't allocate %d bytes of memory\n",size);
    return;
  }

  f.flush();

  int r;

  r = f.readBlock((char*)buf,size);

  f.close();
 */

    //printf("file %s size: %d\n", fileName, size);

  Ipp8u* buf = (Ipp8u*)ippMalloc(size);
  //Ipp64u       c0, c1;
  //clock_t      t0, t1;
  JCOLOR       jpeg_color;
  JSS          jpeg_sampling;
  int          jpeg_nChannels;
  int          jpeg_precision;
  //int          imageSize;

  fflush(f);

  //int r;

  fread( buf, size, 1, f );

  fclose(f);

  JERRCODE     jerr;
  //IppStatus    status;

  CJPEGMetaData metadata;
  CJPEGDecoder decoder;
  int img_width, img_height;

  jerr = decoder.SetSource(buf,(int)size);
  if(JPEG_OK != jerr)
  {
    ippFree(buf);
    fprintf(stderr,"decoder.SetSource() failed, %s\n",GetErrorStr(jerr));
    return NULL;
  }

  jerr = decoder.ReadHeader(
    &img_width,
    &img_height,
    &jpeg_nChannels,
    &jpeg_color,
    &jpeg_sampling,
    &jpeg_precision);

  if(JPEG_OK != jerr)
  {
    ippFree(buf);
    fprintf(stderr,"decoder.ReadHeader() failed, %s\n",GetErrorStr(jerr));
    return NULL;
  }

  *width = img_width;
  *height = img_height;
  *channels = jpeg_nChannels;

  if(decoder.m_exif_app1_detected)
  {
    int i, j, n;

    metadata.ProcessAPP1_Exif(decoder.m_exif_app1_data,decoder.m_exif_app1_data_size);

    for(i = TIFF_IFD_0; i < TIFF_IFD_MAX; i++)
    {
      n = metadata.GetIfdNumEntries(i);
      for(j = 0; j < n; j++)
      {
        printf("%s\n",metadata.ShowIfdData(i,j));
      }
    }
  }


  JCOLOR m_color;
  int m_nChannels;

  switch(jpeg_nChannels)
  {
    case 1:
      m_nChannels = 1;
      m_color     = JC_GRAY;
      break;

    case 3:
      m_nChannels = 3;
      m_color     = JC_BGR;
      break;

    case 4:
      m_nChannels = 4;
      m_color     = JC_CMYK;
      break;

    default:
      jpeg_color  = JC_UNKNOWN;
      m_color     = JC_UNKNOWN;
      m_nChannels = jpeg_nChannels;
      break;
  }

  IppiSize     m_imageDims;
  m_imageDims.width = img_width;
  m_imageDims.height = img_height;

  int m_lineStep  = m_imageDims.width * m_nChannels;
  int imageSize   = m_lineStep * m_imageDims.height;

  ImgColor *outbuf = new ImgColor( img_width, img_height ); //(Ipp8u*)ippMalloc(imageSize);

  Ipp8u *temp = (Ipp8u*)ippMalloc(imageSize);

  //dbg( DBG_INFO, "Setting destination...");

  jerr = decoder.SetDestination(
    temp,
    img_width * m_nChannels,
    m_imageDims,
    jpeg_nChannels,
    m_color);

  if(JPEG_OK != jerr)
  {
    ippFree(buf);
    fprintf(stderr,"decoder.SetDestination() failed, %s\n",GetErrorStr(jerr));
    return NULL;
  }

  // read data
  //dbg(DBG_INFO, "Reading the data...");

  jerr = decoder.ReadData();
  if(JPEG_OK != jerr)
  {
    ippFree(buf);
    fprintf(stderr,"decoder.ReadData() failed, %s\n",GetErrorStr(jerr));
    return NULL;
  }

  //dbg(DBG_INFO, "Reading done.");

  outbuf->Copy( temp );

  
  //memcpy( outbuf->IplImagePtr(), temp, imageSize );

  //dbg(DBG_INFO, "Copying done.");

  int c,r;
  int rcnt = 0;
  for (r=0;r<img_height;r++) {
      for (c=0;c<img_width;c++) {
          if ( outbuf->GetPixel(c,r,0) != 0 )
              rcnt++;
      }
  }

  // free memory
  ippFree(buf);
  ippFree(temp);

  // return data
  if ( !outbuf ) {
      printf("Warning: returning NULL image data!\n");
  }
  
  //dbg(DBG_INFO, "reading done.");

  return outbuf;
  
  /*
  if(m_imageDims.width < DEFAULT_WIDTH || m_imageDims.height < DEFAULT_HEIGHT)
  {
    resize(m_imageDims.width,m_imageDims.height);
  }
  else
  {
    resize(DEFAULT_WIDTH,DEFAULT_HEIGHT);
  }

  m_view->resize(m_imageDims.width,m_imageDims.height);
  

  switch(jpeg_nChannels)
  {
    case 1:
      m_nChannels = 1;
      m_color     = JC_GRAY;
      break;

    case 3:
      m_nChannels = 3;
      m_color     = JC_BGR;
      break;

    case 4:
      m_nChannels = 4;
      m_color     = JC_CMYK;
      break;

    default:
      jpeg_color  = JC_UNKNOWN;
      m_color     = JC_UNKNOWN;
      m_nChannels = jpeg_nChannels;
      break;
  }

  if(NULL != m_imageData)
  {
    ippFree(m_imageData);
    m_imageData = NULL;
  }

  m_lineStep  = m_imageDims.width * m_nChannels;
  imageSize   = m_lineStep * m_imageDims.height;

  Ipp8u* m_imageData = (Ipp8u*)ippMalloc(imageSize);
  if(NULL == m_imageData)
  {
    fprintf(stderr,"can't allocate %d bytes of memory\n",imageSize);
    return;
  }

  jerr = decoder.SetDestination(
    m_imageData,
    m_lineStep,
    m_imageDims,
    m_nChannels,
    m_color);

  if(JPEG_OK != jerr)
  {
    fprintf(stderr,"decoder.SetDestination() failed, %s\n",GetErrorStr(jerr));
    return;
  }

  t0 = clock();
  c0 = ippGetCpuClocks();

  jerr = decoder.ReadData();
  if(JPEG_OK != jerr)
  {
    fprintf(stderr,"decoder.ReadData() failed, %s\n",GetErrorStr(jerr));
    return;
  }

  c1 = ippGetCpuClocks();
  t1 = clock();
  */


  /*
  QImage image(m_imageDims.width,m_imageDims.height,32);

  switch(m_nChannels)
  {
    case 1:
      status = ippiCopy_8u_C1C4R(
                 m_imageData,
                 m_lineStep,
                 image.bits() + 0,
                 image.bytesPerLine(),
                 m_imageDims);

      if(ippStsNoErr != status)
      {
        fprintf(stderr,"ippiCopy_8u_C1C4R() failed, %s\n",ippGetStatusString(status));
        return;
      }

      status = ippiCopy_8u_C1C4R(
                 m_imageData,
                 m_lineStep,
                 image.bits() + 1,
                 image.bytesPerLine(),
                 m_imageDims);

      if(ippStsNoErr != status)
      {
        fprintf(stderr,"ippiCopy_8u_C1C4R() failed, %s\n",ippGetStatusString(status));
        return;
      }

      status = ippiCopy_8u_C1C4R(
                 m_imageData,
                 m_lineStep,
                 image.bits() + 2,
                 image.bytesPerLine(),
                 m_imageDims);

      if(ippStsNoErr != status)
      {
        fprintf(stderr,"ippiCopy_8u_C1C4R() failed, %s\n",ippGetStatusString(status));
        return;
      }
    break;

    case 3:
      status = ippiCopy_8u_C3AC4R(
                 m_imageData,
                 m_lineStep,
                 image.bits(),
                 image.bytesPerLine(),
                 m_imageDims);

      if(ippStsNoErr != status)
      {
        fprintf(stderr,"ippiCopy_8u_C3AC4R() failed, %s\n",ippGetStatusString(status));
        return;
      }
      break;

    case 4:
      RGBA_FPX_to_BGRA(m_imageData,m_imageDims.width,m_imageDims.height);

      status = ippiCopy_8u_C4R(
                 m_imageData,
                 m_lineStep,
                 image.bits(),
                 image.bytesPerLine(),
                 m_imageDims);

      if(ippStsNoErr != status)
      {
        fprintf(stderr,"ippiCopy_8u_C4R() failed, %s\n",ippGetStatusString(status));
        return;
      }
      break;

    default:
    break;

  }

  QPixmap p;
  p.convertFromImage(image,0);
  m_view->setBackgroundPixmap(p);

  setCaption(fileName);

  QString s;

  s.sprintf("%s",getModeStr(decoder.m_jpeg_mode));
  m_lMode->setText(s);

  s.sprintf("%dx%dx%d",m_imageDims.width,m_imageDims.height,m_nChannels);
  m_lSize->setText(s);

  s.sprintf("%s",getSamplingStr(jpeg_sampling));
  m_lSampling->setText(s);

  s.sprintf("%s",getColorStr(jpeg_color));
  m_lColor->setText(s);

  s.sprintf("CPE:%4.1f",(float)(Ipp64s)(c1 - c0)/(m_imageDims.width*m_imageDims.height));
  m_lCPE->setText(s);

  double usec = (double)(t1 - t0)*1000000 / CLOCKS_PER_SEC;
  s.sprintf("USEC:%6.1f",(double)usec);
  m_lUSEC->setText(s);

  s.sprintf("%s",fileName);
  statusBar()->message(s);
  */
} // ApplicationWindow::load_jpeg()

/*
void ApplicationWindow::load_bmp(const char* fileName)
{
  QFile f(fileName);

  if(!f.open(IO_ReadOnly))
    return;

  f.flush();

  int readCount;
  BITMAPFILEHEADER bmfh;
  BITMAPINFOHEADER bmih;

  readCount = f.readBlock((char*)&bmfh,sizeof(BITMAPFILEHEADER));
  if(readCount != sizeof(BITMAPFILEHEADER))
  {
    fprintf(stderr,"can't read %d bytes from file %s\n",
      (int)sizeof(BITMAPFILEHEADER),fileName);
    return;
  }

  if(0x4D42 != bmfh.bfType)
  {
    fprintf(stderr,"not valid BMP file\n");
    return;
  }

  readCount = f.readBlock((char*)&bmih,sizeof(BITMAPINFOHEADER));
  if(readCount != sizeof(BITMAPINFOHEADER))
  {
    fprintf(stderr,"can't read %d bytes from file %s\n",
      (int)sizeof(BITMAPINFOHEADER),fileName);
    return;
  }

  if(24 != bmih.biBitCount)
  {
    fprintf(stderr,"unsupported BMP file\n");
    return;
  }

  if(BI_RGB != bmih.biCompression)
  {
    fprintf(stderr,"unsupported BMP file\n");
    return;
  }

  m_imageDims.width  = bmih.biWidth;
  m_imageDims.height = bmih.biHeight;
  m_nChannels        = bmih.biBitCount >> 3;
  m_color            = JC_BGR;


  Ipp64u       c0, c1;
  clock_t      t0, t1;
  int          imageSize;
  IppStatus    status;


  if(m_imageDims.width < DEFAULT_WIDTH || m_imageDims.height < DEFAULT_HEIGHT)
  {
    resize(m_imageDims.width,m_imageDims.height);
  }
  else
  {
    resize(DEFAULT_WIDTH,DEFAULT_HEIGHT);
  }

  m_view->resize(m_imageDims.width,m_imageDims.height);

  if(NULL != m_imageData)
  {
    ippFree(m_imageData);
    m_imageData = NULL;
  }

  int pad = DIB_PAD_BYTES(m_imageDims.width,m_nChannels);
  m_lineStep  = m_imageDims.width*m_nChannels + pad;;
  imageSize   = m_lineStep * m_imageDims.height;

  m_imageData = (Ipp8u*)ippMalloc(imageSize);
  if(NULL == m_imageData)
  {
    fprintf(stderr,"can't allocate %d bytes of memory\n",imageSize);
    return;
  }

  t0 = clock();
  c0 = ippGetCpuClocks();

  readCount = f.readBlock((char*)m_imageData,imageSize);
  if(readCount != imageSize)
  {
    fprintf(stderr,"can't read %d bytes from file %s\n",imageSize,fileName);
    return;
  }

  c1 = ippGetCpuClocks();
  t1 = clock();


  f.close();

  QImage image(m_imageDims.width,m_imageDims.height,32);

  if(m_nChannels == 3)
  {
    status = ippiMirror_8u_C3IR(
      m_imageData,
      m_lineStep,
      m_imageDims,
      ippAxsHorizontal);

    if(ippStsNoErr != status)
    {
      fprintf(stderr,"ippiMirror_8u_C3IR() failed, %s\n",ippGetStatusString(status));
      return;
    }

    status = ippiCopy_8u_C3AC4R(
      m_imageData,
      m_lineStep,
      image.bits(),
      image.bytesPerLine(),
      m_imageDims);

    if(ippStsNoErr != status)
    {
      fprintf(stderr,"ippiCopy_8u_C3AC4R() failed, %s\n",ippGetStatusString(status));
      return;
    }
  }
  else
  {
    RGBA_FPX_to_BGRA(m_imageData,m_imageDims.width,m_imageDims.height);

    status = ippiMirror_8u_C4IR(
      m_imageData,
      m_lineStep,
      m_imageDims,
      ippAxsHorizontal);

    if(ippStsNoErr != status)
    {
      fprintf(stderr,"ippiMirror_8u_C4IR() failed, %s\n",ippGetStatusString(status));
      return;
    }

    status = ippiCopy_8u_C4R(
      m_imageData,
      m_lineStep,
      image.bits(),
      image.bytesPerLine(),
      m_imageDims);

    if(ippStsNoErr != status)
    {
      fprintf(stderr,"ippiCopy_8u_C4R() failed, %s\n",ippGetStatusString(status));
      return;
    }
  }


  QPixmap p;
  p.convertFromImage(image,0);
  m_view->setBackgroundPixmap(p);

  setCaption(fileName);

  QString s;

  s.sprintf("BMP");
  m_lMode->setText(s);

  s.sprintf("%dx%dx%d",m_imageDims.width,m_imageDims.height,m_nChannels);
  m_lSize->setText(s);

  s.sprintf("%s",getSamplingStr(JS_444));
  m_lSampling->setText(s);

  s.sprintf("%s",getColorStr(m_color));
  m_lColor->setText(s);

  s.sprintf("CPE:%4.1f",(float)(Ipp64s)(c1 - c0)/(m_imageDims.width*m_imageDims.height));
  m_lCPE->setText(s);

  double usec = (double)(t1 - t0)*1000000 / CLOCKS_PER_SEC;
  s.sprintf("USEC:%6.1f",(double)usec);
  m_lUSEC->setText(s);

  s.sprintf("%s",fileName);
  statusBar()->message(s);

  return;
} // ApplicationWindow::load_bmp()


void ApplicationWindow::save_jpeg(void)
{
  Ipp8u* buf;
  int imageSize = m_imageDims.width * m_nChannels * m_imageDims.height;

  if(imageSize < 4096)
  {
    imageSize = 4096;
  }

  buf = (Ipp8u*)ippMalloc(imageSize);
  if(NULL == buf)
  {
    fprintf(stderr,"can't allocate %d bytes of memory\n",imageSize);
    return;
  }

  Ipp64u  c0, c1;
  clock_t t0, t1;
  int jpeg_quality  = 75;
  JSS jpeg_sampling = JS_411;
  JCOLOR in_color;
  JCOLOR jpeg_color;
  JERRCODE jerr;
  CJPEGEncoder encoder;

  switch(m_nChannels)
  {
    case 3:
      in_color   = m_color;
      jpeg_color = JC_YCBCR;
      break;

    case 4:
      in_color   = JC_CMYK;
      jpeg_color = JC_YCCK;
      break;

    default:
      in_color   = JC_UNKNOWN;
      jpeg_color = JC_UNKNOWN;
      break;
  }

  encoder.SetSource(m_imageData,m_lineStep,m_imageDims,m_nChannels,in_color);
  encoder.SetDestination(buf,imageSize,jpeg_quality,jpeg_color,jpeg_sampling,JPEG_BASELINE);

  if(JC_CMYK == in_color)
  {
    BGRA_to_RGBA(m_imageData,m_imageDims.width,m_imageDims.height);
  }

  jerr = encoder.WriteHeader();
  if(JPEG_OK != jerr)
  {
    fprintf(stderr,"encoder.WriteHeader() failed, %s\n",GetErrorStr(jerr));
    return;
  }

  t0 = clock();
  c0 = ippGetCpuClocks();

  jerr = encoder.WriteData();
  if(JPEG_OK != jerr)
  {
    fprintf(stderr,"encoder.WriteData() failed, %s\n",GetErrorStr(jerr));
    return;
  }

  c1 = ippGetCpuClocks();
  t1 = clock();

  QFile f(m_filename);
  if(!f.open(IO_WriteOnly))
  {
    QString s;
    s.sprintf("Could not write to %s",(const char*)m_filename);
    statusBar()->message(s);
    return;
  }

  int r;
  r = f.writeBlock((const char*)buf,imageSize);
  if(-1 == r)
  {
    fprintf(stderr,"write to file failed\n");
    return;
  }

  f.close();

  ippFree(buf);


  QString s;

  s.sprintf("%s",getModeStr(encoder.m_jpeg_mode));
  m_lMode->setText(s);

  s.sprintf("%dx%dx%d",m_imageDims.width,m_imageDims.height,m_nChannels);
  m_lSize->setText(s);

  s.sprintf("%s",getSamplingStr(jpeg_sampling));
  m_lSampling->setText(s);

  s.sprintf("%s",getColorStr(jpeg_color));
  m_lColor->setText(s);

  s.sprintf("CPE:%4.1f",(float)(Ipp64s)(c1 - c0)/(m_imageDims.width*m_imageDims.height));
  m_lCPE->setText(s);

  double usec = (double)(t1 - t0)*1000000 / CLOCKS_PER_SEC;
  s.sprintf("USEC:%6.1f",(double)usec);
  m_lUSEC->setText(s);

  s.sprintf("File %s saved",(const char*)m_filename);
  statusBar()->message(s,2000);

  return;
} // ApplicationWindow::save_jpeg()
*/
#endif
