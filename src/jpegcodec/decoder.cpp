/*
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//    Copyright (c) 2001-2006 Intel Corporation. All Rights Reserved.
//
*/

#include "precomp.h"

#include <stdio.h>
#include <string.h>

#ifndef __JPEGBASE_H__
#include "jpegbase.h"
#endif
#ifndef __DECODER_H__
#include "decoder.h"
#endif




CJPEGDecoder::CJPEGDecoder(void)
{
  Reset();
  return;
} // ctor


CJPEGDecoder::~CJPEGDecoder(void)
{
  Clean();
  return;
} // dtor


void CJPEGDecoder::Reset(void)
{
  m_src.pData      = 0;
  m_src.DataLen    = 0;

  m_jpeg_width     = 0;
  m_jpeg_height    = 0;
  m_jpeg_ncomp     = 0;
  m_jpeg_precision = 8;
  m_jpeg_sampling  = JS_444;
  m_jpeg_color     = JC_UNKNOWN;
  m_jpeg_quality   = 100;
  m_jpeg_restart_interval = 0;

  m_jpeg_comment_detected = 0;
  m_jpeg_comment          = 0;
  m_jpeg_comment_size     = 0;

  m_precision = 8;
  m_numxMCU = 0;
  m_numyMCU = 0;
  m_mcuWidth  = 0;
  m_mcuHeight = 0;

  m_ccWidth  = 0;
  m_ccHeight = 0;
  m_xPadding = 0;
  m_yPadding = 0;

  m_restarts_to_go   = 0;
  m_next_restart_num = 0;

  m_sos_len  = 0;

  m_curr_comp_no = 0;
  m_scan_ncomps  = 0;

  m_ss = 0;
  m_se = 0;
  m_al = 0;
  m_ah = 0;

  m_jpeg_mode = JPEG_BASELINE;

  m_dc_scan_completed  = 0;
  m_ac_scans_completed = 0;
  m_scan_count         = 0;

  m_marker = JM_NONE;

  m_jfif_app0_detected     = 0;
  m_jfif_app0_major        = 0;
  m_jfif_app0_minor        = 0;
  m_jfif_app0_units        = 0;
  m_jfif_app0_xDensity     = 0;
  m_jfif_app0_yDensity     = 0;
  m_jfif_app0_thumb_width  = 0;
  m_jfif_app0_thumb_height = 0;

  m_avi1_app0_detected     = 0;
  m_avi1_app0_polarity     = 0;
  m_avi1_app0_reserved     = 0;
  m_avi1_app0_field_size   = 0;
  m_avi1_app0_field_size2  = 0;

  m_exif_app1_detected     = 0;
  m_exif_app1_data_size    = 0;
  m_exif_app1_data         = 0;

  m_jfxx_app0_detected   = 0;
  m_jfxx_thumbnails_type = 0;

  m_adobe_app14_detected  = 0;
  m_adobe_app14_version   = 0;
  m_adobe_app14_flags0    = 0;
  m_adobe_app14_flags1    = 0;
  m_adobe_app14_transform = 0;

  m_block_buffer = 0;
  m_nblock       = 1;

#ifdef __TIMING__
  m_clk_dct  = 0;
  m_clk_ss   = 0;
  m_clk_cc   = 0;
  m_clk_diff = 0;
  m_clk_huff = 0;
#endif

  return;
} // CJPEGDecoder::Reset(void)


JERRCODE CJPEGDecoder::Clean(void)
{
  int i;

  m_jpeg_comment_detected = 0;

  if(0 != m_jpeg_comment)
  {
    ippFree(m_jpeg_comment);
    m_jpeg_comment = 0;
    m_jpeg_comment_size = 0;
  }

  m_avi1_app0_detected    = 0;
  m_avi1_app0_polarity    = 0;
  m_avi1_app0_reserved    = 0;
  m_avi1_app0_field_size  = 0;
  m_avi1_app0_field_size2 = 0;

  m_jfif_app0_detected   = 0;
  m_jfxx_app0_detected   = 0;

  m_exif_app1_detected   = 0;

  if(0 != m_exif_app1_data)
  {
    ippFree(m_exif_app1_data);
    m_exif_app1_data = 0;
  }

  m_adobe_app14_detected = 0;

  m_scan_ncomps = 0;

  for(i = 0; i < MAX_COMPS_PER_SCAN; i++)
  {
    m_ccomp[i].m_curr_row = 0;
    m_ccomp[i].m_prev_row = 0;
  }

  for(i = 0; i < MAX_HUFF_TABLES; i++)
  {
    m_dctbl[i].Destroy();
    m_actbl[i].Destroy();
  }

  if(0 != m_block_buffer)
  {
    ippFree(m_block_buffer);
    m_block_buffer = 0;
  }

  m_state.Destroy();

  return JPEG_OK;
} // CJPEGDecoder::Clean()


JERRCODE CJPEGDecoder::SetSource(
  Ipp8u* pSrc,
  int    srcSize)
{
  m_src.pData    = pSrc;
  m_src.DataLen  = srcSize;
  m_src.currPos  = 0;

  return JPEG_OK;
} // CJPEGDecoder::SetSource()


JERRCODE CJPEGDecoder::SetDestination(
  Ipp8u*   pDst,
  int      dstStep,
  IppiSize dstSize,
  int      dstChannels,
  JCOLOR   dstColor,
  JSS      dstSampling,
  int      dstPrecision)
{
  m_dst.p.Data8u   = pDst;
  m_dst.lineStep   = dstStep;
  m_dst.width      = dstSize.width;
  m_dst.height     = dstSize.height;
  m_dst.nChannels  = dstChannels;
  m_dst.color      = dstColor;
  m_dst.sampling   = dstSampling;
  m_dst.precision  = dstPrecision;

  return JPEG_OK;
} // CJPEGDecoder::SetDestination()


 JERRCODE CJPEGDecoder::SetDestination(
  Ipp16s*  pDst,
  int      dstStep,
  IppiSize dstSize,
  int      dstChannels,
  JCOLOR   dstColor,
  JSS      dstSampling,
  int      dstPrecision)
{
  m_dst.p.Data16s  = pDst;
  m_dst.lineStep   = dstStep;
  m_dst.width      = dstSize.width;
  m_dst.height     = dstSize.height;
  m_dst.nChannels  = dstChannels;
  m_dst.color      = dstColor;
  m_dst.sampling   = dstSampling;
  m_dst.precision  = dstPrecision;

  return JPEG_OK;
} // CJPEGDecoder::SetDestination()




JERRCODE CJPEGDecoder::DetectSampling(void)
{
  switch(m_jpeg_ncomp)
  {
  case 1:
    if(m_ccomp[0].m_hsampling == 1 && m_ccomp[0].m_vsampling == 1)
    {
      m_jpeg_sampling = JS_444;
    }
    else
    {
      return JPEG_BAD_SAMPLING;
    }
    break;

  case 3:
    if(m_ccomp[0].m_hsampling == 1 && m_ccomp[0].m_vsampling == 1 &&
       m_ccomp[1].m_hsampling == 1 && m_ccomp[1].m_vsampling == 1 &&
       m_ccomp[2].m_hsampling == 1 && m_ccomp[2].m_vsampling == 1)
    {
      m_jpeg_sampling = JS_444;
    }
    else if(m_ccomp[0].m_hsampling == 2 && m_ccomp[0].m_vsampling == 1 &&
            m_ccomp[1].m_hsampling == 1 && m_ccomp[1].m_vsampling == 1 &&
            m_ccomp[2].m_hsampling == 1 && m_ccomp[2].m_vsampling == 1)
    {
      m_jpeg_sampling = JS_422;
    }
    else if(m_ccomp[0].m_hsampling == 1 && m_ccomp[0].m_vsampling == 2 &&
            m_ccomp[1].m_hsampling == 1 && m_ccomp[1].m_vsampling == 1 &&
            m_ccomp[2].m_hsampling == 1 && m_ccomp[2].m_vsampling == 1)
    {
      m_jpeg_sampling = JS_244;
    }
    else if(m_ccomp[0].m_hsampling == 2 && m_ccomp[0].m_vsampling == 2 &&
            m_ccomp[1].m_hsampling == 1 && m_ccomp[1].m_vsampling == 1 &&
            m_ccomp[2].m_hsampling == 1 && m_ccomp[2].m_vsampling == 1)
    {
      m_jpeg_sampling = JS_411;
    }
    else
    {
      m_jpeg_sampling = JS_OTHER;
    }
    break;

  case 4:
    if(m_ccomp[0].m_hsampling == 1 && m_ccomp[0].m_vsampling == 1 &&
       m_ccomp[1].m_hsampling == 1 && m_ccomp[1].m_vsampling == 1 &&
       m_ccomp[2].m_hsampling == 1 && m_ccomp[2].m_vsampling == 1 &&
       m_ccomp[3].m_hsampling == 1 && m_ccomp[3].m_vsampling == 1)
    {
      m_jpeg_sampling = JS_444;
    }
    else if(m_ccomp[0].m_hsampling == 2 && m_ccomp[0].m_vsampling == 1 &&
            m_ccomp[1].m_hsampling == 1 && m_ccomp[1].m_vsampling == 1 &&
            m_ccomp[2].m_hsampling == 1 && m_ccomp[2].m_vsampling == 1 &&
            m_ccomp[3].m_hsampling == 2 && m_ccomp[3].m_vsampling == 1)
    {
      m_jpeg_sampling = JS_422;
    }
    else if(m_ccomp[0].m_hsampling == 1 && m_ccomp[0].m_vsampling == 2 &&
            m_ccomp[1].m_hsampling == 1 && m_ccomp[1].m_vsampling == 1 &&
            m_ccomp[2].m_hsampling == 1 && m_ccomp[2].m_vsampling == 1 &&
            m_ccomp[3].m_hsampling == 1 && m_ccomp[3].m_vsampling == 2)
    {
      m_jpeg_sampling = JS_244;
    }
    else if(m_ccomp[0].m_hsampling == 2 && m_ccomp[0].m_vsampling == 2 &&
            m_ccomp[1].m_hsampling == 1 && m_ccomp[1].m_vsampling == 1 &&
            m_ccomp[2].m_hsampling == 1 && m_ccomp[2].m_vsampling == 1 &&
            m_ccomp[3].m_hsampling == 2 && m_ccomp[3].m_vsampling == 2)
    {
      m_jpeg_sampling = JS_411;
    }
    else
    {
      m_jpeg_sampling = JS_OTHER;
    }
    break;
  }

  return JPEG_OK;
} // CJPEGDecoder::DetectSampling()


JERRCODE CJPEGDecoder::NextMarker(JMARKER* marker)
{
  int c;
  int n = 0;

  for(;;)
  {
    if(m_src.currPos >= m_src.DataLen)
    {
      LOG0("Error: buffer too small");
      return JPEG_BUFF_TOO_SMALL;
    }

    m_src._READ_BYTE(&c);

    if(c != 0xff)
    {
      do
      {
        if(m_src.currPos >= m_src.DataLen)
        {
          LOG0("Error: buffer too small");
          return JPEG_BUFF_TOO_SMALL;
        }
        n++;
        m_src._READ_BYTE(&c);
      } while(c != 0xff);
    }

    do
    {
      if(m_src.currPos >= m_src.DataLen)
      {
        LOG0("Error: buffer too small");
        return JPEG_BUFF_TOO_SMALL;
      }

      m_src._READ_BYTE(&c);
    } while(c == 0xff);

    if(c != 0)
    {
      *marker = (JMARKER)c;
      break;
    }
  }

  if(n != 0)
  {
    TRC1("  skip enormous bytes - ",n);
  }

  return JPEG_OK;
} // CJPEGDecoder::NextMarker()


JERRCODE CJPEGDecoder::SkipMarker(void)
{
  int len;

  if(m_src.currPos + 2 >= m_src.DataLen)
  {
    LOG0("Error: buffer too small");
    return JPEG_BUFF_TOO_SMALL;
  }

  m_src._READ_WORD(&len);

  m_src.currPos += len - 2;

  m_marker = JM_NONE;

  return JPEG_OK;
} // CJPEGDecoder::SkipMarker()


JERRCODE CJPEGDecoder::ProcessRestart(void)
{
  JERRCODE  jerr;
  IppStatus status;

  status = ippiDecodeHuffmanStateInit_JPEG_8u(m_state);
  if(ippStsNoErr != status)
  {
    LOG0("Error: ippiDecodeHuffmanStateInit_JPEG_8u() failed");
    return JPEG_INTERNAL_ERROR;
  }

  for(int n = 0; n < m_jpeg_ncomp; n++)
  {
    m_ccomp[n].m_lastDC = 0;
  }

  jerr = ParseRST();
  if(JPEG_OK != jerr)
  {
    LOG0("Error: ParseRST() failed");
//    return jerr;
  }

  m_restarts_to_go = m_jpeg_restart_interval;

  return JPEG_OK;
} // CJPEGDecoder::ProcessRestart()


JERRCODE CJPEGDecoder::ParseSOI(void)
{
  TRC0("-> SOI");
  m_marker = JM_NONE;
  return JPEG_OK;
} // CJPEGDecoder::ParseSOI()


JERRCODE CJPEGDecoder::ParseEOI(void)
{
  TRC0("-> EOI");
  m_marker = JM_NONE;
  return JPEG_OK;
} // CJPEGDecoder::ParseEOI()


const int APP0_JFIF_LENGTH = 14;
const int APP0_JFXX_LENGTH = 6;
const int APP0_AVI1_LENGTH = 14;

JERRCODE CJPEGDecoder::ParseAPP0(void)
{
  int len;

  TRC0("-> APP0");

  if(m_src.currPos + 2 >= m_src.DataLen)
  {
    LOG0("Error: buffer too small");
    return JPEG_BUFF_TOO_SMALL;
  }

  m_src._READ_WORD(&len);
  len -= 2;

  if(len >= APP0_JFIF_LENGTH &&
     m_src.pData[m_src.currPos + 0] == 0x4a && // J
     m_src.pData[m_src.currPos + 1] == 0x46 && // F
     m_src.pData[m_src.currPos + 2] == 0x49 && // I
     m_src.pData[m_src.currPos + 3] == 0x46 && // F
     m_src.pData[m_src.currPos + 4] == 0)
  {
    // we've found JFIF APP0 marker
    len -= 5;
    m_src.currPos += 5;
    m_jfif_app0_detected = 1;

    m_src._READ_BYTE(&m_jfif_app0_major);
    m_src._READ_BYTE(&m_jfif_app0_minor);
    m_src._READ_BYTE(&m_jfif_app0_units);
    m_src._READ_WORD(&m_jfif_app0_xDensity);
    m_src._READ_WORD(&m_jfif_app0_yDensity);
    m_src._READ_BYTE(&m_jfif_app0_thumb_width);
    m_src._READ_BYTE(&m_jfif_app0_thumb_height);
    len -= 9;
  }

  if(len >= APP0_JFXX_LENGTH &&
     m_src.pData[m_src.currPos + 0] == 0x4a && // J
     m_src.pData[m_src.currPos + 1] == 0x46 && // F
     m_src.pData[m_src.currPos + 2] == 0x58 && // X
     m_src.pData[m_src.currPos + 3] == 0x58 && // X
     m_src.pData[m_src.currPos + 4] == 0)
  {
    // we've found JFXX APP0 extension marker
    len -= 5;
    m_src.currPos += 5;
    m_jfxx_app0_detected = 1;

    m_src._READ_BYTE(&m_jfxx_thumbnails_type);

    switch(m_jfxx_thumbnails_type)
    {
    case 0x10: break;
    case 0x11: break;
    case 0x13: break;
    default:   break;
    }
    len -= 1;
  }

  if(len >= APP0_AVI1_LENGTH &&
     m_src.pData[m_src.currPos + 0] == 0x41 && // A
     m_src.pData[m_src.currPos + 1] == 0x56 && // V
     m_src.pData[m_src.currPos + 2] == 0x49 && // I
     m_src.pData[m_src.currPos + 3] == 0x31)   // 1
  {
    // we've found AVI1 APP0 marker
    len -= 4;
    m_src.currPos += 4;
    m_avi1_app0_detected = 1;

    m_src._READ_BYTE(&m_avi1_app0_polarity);

    len -= 1;

    if(len == 7) // old MJPEG AVI
      len -= 7;

    if(len == 9) // ODML MJPEG AVI
    {
      m_src._READ_BYTE(&m_avi1_app0_reserved);

      m_src._READ_DWORD(&m_avi1_app0_field_size);

      m_src._READ_DWORD(&m_avi1_app0_field_size2);

      len -= 9;
    }
  }

  m_src.currPos += len;

  m_marker = JM_NONE;

  return JPEG_OK;
} // CJPEGDecoder::ParseAPP0()


JERRCODE CJPEGDecoder::ParseAPP1(void)
{
  int i, b;
  int len;

  TRC0("-> APP0");

  if(m_src.currPos + 2 >= m_src.DataLen)
  {
    LOG0("Error: buffer too small");
    return JPEG_BUFF_TOO_SMALL;
  }

  m_src._READ_WORD(&len);
  len -= 2;

  if(m_src.pData[m_src.currPos + 0] == 0x45 && // E
     m_src.pData[m_src.currPos + 1] == 0x78 && // x
     m_src.pData[m_src.currPos + 2] == 0x69 && // i
     m_src.pData[m_src.currPos + 3] == 0x66 && // f
     m_src.pData[m_src.currPos + 4] == 0)
  {
    m_exif_app1_detected  = 1;
    m_exif_app1_data_size = len;

    m_src.currPos += 6;
    len -= 6;

    if(m_exif_app1_data != 0)
    {
      ippFree(m_exif_app1_data);
      m_exif_app1_data = 0;
    }

    m_exif_app1_data = (Ipp8u*)ippMalloc(len);
    if(0 == m_exif_app1_data)
      return JPEG_OUT_OF_MEMORY;

    for(i = 0; i < len; i++)
    {
      m_src._READ_BYTE(&b);

      m_exif_app1_data[i] = (Ipp8u)b;
    }
  }
  else
  {
    m_src.currPos += len;
  }

  m_marker = JM_NONE;

  return JPEG_OK;
} // CJPEGDecoder::ParseAPP1()


const int APP14_ADOBE_LENGTH = 12;

JERRCODE CJPEGDecoder::ParseAPP14(void)
{
  int len;

  TRC0("-> APP14");

  if(m_src.currPos + 2 >= m_src.DataLen)
  {
    LOG0("Error: buffer too small");
    return JPEG_BUFF_TOO_SMALL;
  }

  m_src._READ_WORD(&len);
  len -= 2;

  if(len >= APP14_ADOBE_LENGTH &&
     m_src.pData[m_src.currPos + 0] == 0x41 && // A
     m_src.pData[m_src.currPos + 1] == 0x64 && // d
     m_src.pData[m_src.currPos + 2] == 0x6f && // o
     m_src.pData[m_src.currPos + 3] == 0x62 && // b
     m_src.pData[m_src.currPos + 4] == 0x65)   // e
  {
    // we've found Adobe APP14 marker
    len -= 5;
    m_src.currPos += 5;
    m_adobe_app14_detected   = 1;

    m_src._READ_WORD(&m_adobe_app14_version);
    m_src._READ_WORD(&m_adobe_app14_flags0);
    m_src._READ_WORD(&m_adobe_app14_flags1);
    m_src._READ_BYTE(&m_adobe_app14_transform);

    TRC1("  adobe_app14_version   - ",m_adobe_app14_version);
    TRC1("  adobe_app14_flags0    - ",m_adobe_app14_flags0);
    TRC1("  adobe_app14_flags1    - ",m_adobe_app14_flags1);
    TRC1("  adobe_app14_transform - ",m_adobe_app14_transform);

    len -= 7;
  }

  m_src.currPos += len;

  m_marker = JM_NONE;

  return JPEG_OK;
} // CJPEGDecoder::ParseAPP14()


JERRCODE CJPEGDecoder::ParseCOM(void)
{
  int i;
  int c;
  int len;

  TRC0("-> COM");

  if(m_src.currPos + 2 >= m_src.DataLen)
  {
    LOG0("Error: buffer too small");
    return JPEG_BUFF_TOO_SMALL;
  }

  m_src._READ_WORD(&len);
  len -= 2;

  TRC1("  bytes for comment - ",len);

  m_jpeg_comment_detected = 1;
  m_jpeg_comment_size     = len;

  if(m_jpeg_comment != 0)
  {
    ippFree(m_jpeg_comment);
  }

  m_jpeg_comment = (Ipp8u*)ippMalloc(len+1);

  for(i = 0; i < len; i++)
  {
    m_src._READ_BYTE(&c);
    m_jpeg_comment[i] = (Ipp8u)c;
  }

  m_jpeg_comment[len] = 0;

  m_marker = JM_NONE;

  return JPEG_OK;
} // CJPEGDecoder::ParseCOM()


JERRCODE CJPEGDecoder::ParseDQT(void)
{
  int id;
  int len;
  JERRCODE jerr;

  TRC0("-> DQT");

  if(m_src.currPos + 2 >= m_src.DataLen)
  {
    LOG0("Error: buffer too small");
    return JPEG_BUFF_TOO_SMALL;
  }

  m_src._READ_WORD(&len);
  len -= 2;

  while(len > 0)
  {
    m_src._READ_BYTE(&id);

    int precision = (id & 0xf0) >> 4;

    TRC1("  id        - ",(id & 0x0f));
    TRC1("  precision - ",precision);

    if((id & 0x0f) > MAX_QUANT_TABLES)
    {
      return JPEG_BAD_QUANT_SEGMENT;
    }

    int q;
    Ipp8u qnt[DCTSIZE2];

    for(int i = 0; i < DCTSIZE2; i++)
    {
      if(precision)
      {
        m_src._READ_WORD(&q);
      }
      else
      {
        m_src._READ_BYTE(&q);
      }

      qnt[i] = (Ipp8u)q;
    }

    jerr = m_qntbl[id & 0x0f].Init(id,qnt);
    if(JPEG_OK != jerr)
    {
      return jerr;
    }

    len -= DCTSIZE2 + DCTSIZE2*precision + 1;
  }

  if(len != 0)
  {
    return JPEG_BAD_SEGMENT_LENGTH;
  }

  m_marker = JM_NONE;

  return JPEG_OK;
} // CJPEGDecoder::ParseDQT()


JERRCODE CJPEGDecoder::ParseDHT(void)
{
  int i;
  int len;
  int index;
  int count;
  JERRCODE jerr;

  TRC0("-> DHT");

  if(m_src.currPos + 2 >= m_src.DataLen)
  {
    LOG0("Error: buffer too small");
    return JPEG_BUFF_TOO_SMALL;
  }

  m_src._READ_WORD(&len);
  len -= 2;

  int v;
  Ipp8u bits[MAX_HUFF_BITS];
  Ipp8u vals[MAX_HUFF_VALS];

  while(len > 16)
  {
    m_src._READ_BYTE(&index);
    count = 0;
    for(i = 0; i < MAX_HUFF_BITS; i++)
    {
      m_src._READ_BYTE(&v);
      bits[i] = (Ipp8u)v;
      count += bits[i];
    }

    len -= 16 + 1;

    if(count > MAX_HUFF_VALS || count > len)
    {
      return JPEG_BAD_HUFF_TBL;
    }

    for(i = 0; i < count; i++)
    {
      m_src._READ_BYTE(&v);
      vals[i] = (Ipp8u)v;
    }

    len -= count;

    if(index >> 4)
    {
      // make AC Huffman table
      if(m_actbl[index & 0x0f].IsEmpty())
      {
        jerr = m_actbl[index & 0x0f].Create();
        if(JPEG_OK != jerr)
        {
          LOG0("    Can't create AC huffman table");
          return JPEG_INTERNAL_ERROR;
        }
      }

      TRC1("    AC Huffman Table - ",index & 0x0f);
      jerr = m_actbl[index & 0x0f].Init(index & 0x0f,index >> 4,bits,vals);
      if(JPEG_OK != jerr)
      {
        LOG0("    Can't build AC huffman table");
        return JPEG_INTERNAL_ERROR;
      }
    }
    else
    {
      // make DC Huffman table
      if(m_dctbl[index & 0x0f].IsEmpty())
      {
        jerr = m_dctbl[index & 0x0f].Create();
        if(JPEG_OK != jerr)
        {
          LOG0("    Can't create DC huffman table");
          return JPEG_INTERNAL_ERROR;
        }
      }

      TRC1("    DC Huffman Table - ",index & 0x0f);
      jerr = m_dctbl[index & 0x0f].Init(index & 0x0f,index >> 4,bits,vals);
      if(JPEG_OK != jerr)
      {
        LOG0("    Can't build DC huffman table");
        return JPEG_INTERNAL_ERROR;
      }
    }
  }

  if(len != 0)
  {
    return JPEG_BAD_SEGMENT_LENGTH;
  }

  m_marker = JM_NONE;

  return JPEG_OK;
} // CJPEGDecoder::ParseDHT()


JERRCODE CJPEGDecoder::ParseSOF0(void)
{
  int i;
  int len;
  JERRCODE jerr;

  TRC0("-> SOF0");

  if(m_src.currPos + 2 >= m_src.DataLen)
  {
    LOG0("Error: buffer too small");
    return JPEG_BUFF_TOO_SMALL;
  }

  m_src._READ_WORD(&len);
  len -= 2;

   m_src._READ_BYTE(&m_jpeg_precision);

  if(m_jpeg_precision != 8)
  {
    return JPEG_NOT_IMPLEMENTED;
  }

  m_src._READ_WORD(&m_jpeg_height);
  m_src._READ_WORD(&m_jpeg_width);
  m_src._READ_BYTE(&m_jpeg_ncomp);

  TRC1("  height    - ",m_jpeg_height);
  TRC1("  width     - ",m_jpeg_width);
  TRC1("  nchannels - ",m_jpeg_ncomp);

  if(m_jpeg_ncomp < 0 || m_jpeg_ncomp > MAX_COMPS_PER_SCAN)
  {
    return JPEG_BAD_FRAME_SEGMENT;
  }

  len -= 6;

  if(len != m_jpeg_ncomp * 3)
  {
    return JPEG_BAD_SEGMENT_LENGTH;
  }

  for(i = 0; i < m_jpeg_ncomp; i++)
  {
    m_src._READ_BYTE(&m_ccomp[i].m_id);

    int ss;

    m_src._READ_BYTE(&ss);

    m_ccomp[i].m_hsampling  = (ss >> 4) & 0x0f;
    m_ccomp[i].m_vsampling  = (ss     ) & 0x0f;

    m_src._READ_BYTE(&m_ccomp[i].m_q_selector);

    if(m_ccomp[i].m_hsampling <= 0 || m_ccomp[i].m_vsampling <= 0)
    {
      return JPEG_BAD_FRAME_SEGMENT;
    }

    m_ccomp[i].m_nblocks = m_ccomp[i].m_hsampling * m_ccomp[i].m_vsampling;

    TRC1("    id ",m_ccomp[i].m_id);
    TRC1("      hsampling - ",m_ccomp[i].m_hsampling);
    TRC1("      vsampling - ",m_ccomp[i].m_vsampling);
    TRC1("      qselector - ",m_ccomp[i].m_q_selector);
  }

  jerr = DetectSampling();
  if(JPEG_OK != jerr)
  {
    return jerr;
  }

  m_max_hsampling = m_ccomp[0].m_hsampling;
  m_max_vsampling = m_ccomp[0].m_vsampling;

  for(i = 0; i < m_jpeg_ncomp; i++)
  {
    if(m_max_hsampling < m_ccomp[i].m_hsampling)
      m_max_hsampling = m_ccomp[i].m_hsampling;

    if(m_max_vsampling < m_ccomp[i].m_vsampling)
      m_max_vsampling = m_ccomp[i].m_vsampling;
  }

  for(i = 0; i < m_jpeg_ncomp; i++)
  {
    m_ccomp[i].m_h_factor = m_max_hsampling / m_ccomp[i].m_hsampling;
    m_ccomp[i].m_v_factor = m_max_vsampling / m_ccomp[i].m_vsampling;
  }

  m_jpeg_mode = JPEG_BASELINE;

  m_marker = JM_NONE;

  return JPEG_OK;
} // CJPEGDecoder::ParseSOF0()


JERRCODE CJPEGDecoder::ParseSOF2(void)
{
  int i;
  int len;
  JERRCODE jerr;

  TRC0("-> SOF2");

  if(m_src.currPos + 2 >= m_src.DataLen)
  {
    LOG0("Error: buffer too small");
    return JPEG_BUFF_TOO_SMALL;
  }

  m_src._READ_WORD(&len);
  len -= 2;

  m_src._READ_BYTE(&m_jpeg_precision);

  if(m_jpeg_precision != 8)
  {
    return JPEG_NOT_IMPLEMENTED;
  }

  m_src._READ_WORD(&m_jpeg_height);
  m_src._READ_WORD(&m_jpeg_width);
  m_src._READ_BYTE(&m_jpeg_ncomp);

  TRC1("  height    - ",m_jpeg_height);
  TRC1("  width     - ",m_jpeg_width);
  TRC1("  nchannels - ",m_jpeg_ncomp);

  if(m_jpeg_ncomp < 0 || m_jpeg_ncomp > MAX_COMPS_PER_SCAN)
  {
    return JPEG_BAD_FRAME_SEGMENT;
  }

  len -= 6;

  if(len != m_jpeg_ncomp * 3)
  {
    return JPEG_BAD_SEGMENT_LENGTH;
  }

  for(i = 0; i < m_jpeg_ncomp; i++)
  {
    m_src._READ_BYTE(&m_ccomp[i].m_id);
    m_ccomp[i].m_comp_no = i;

    int ss;

    m_src._READ_BYTE(&ss);

    m_ccomp[i].m_hsampling  = (ss >> 4) & 0x0f;
    m_ccomp[i].m_vsampling  = (ss     ) & 0x0f;

    m_src._READ_BYTE(&m_ccomp[i].m_q_selector);

    if(m_ccomp[i].m_hsampling <= 0 || m_ccomp[i].m_vsampling <= 0)
    {
      return JPEG_BAD_FRAME_SEGMENT;
    }

    m_ccomp[i].m_nblocks = m_ccomp[i].m_hsampling * m_ccomp[i].m_vsampling;

    TRC1("    id ",m_ccomp[i].m_id);
    TRC1("      hsampling - ",m_ccomp[i].m_hsampling);
    TRC1("      vsampling - ",m_ccomp[i].m_vsampling);
    TRC1("      qselector - ",m_ccomp[i].m_q_selector);
  }

  jerr = DetectSampling();
  if(JPEG_OK != jerr)
  {
    return jerr;
  }

  m_max_hsampling = m_ccomp[0].m_hsampling;
  m_max_vsampling = m_ccomp[0].m_vsampling;

  for(i = 0; i < m_jpeg_ncomp; i++)
  {
    if(m_max_hsampling < m_ccomp[i].m_hsampling)
      m_max_hsampling = m_ccomp[i].m_hsampling;

    if(m_max_vsampling < m_ccomp[i].m_vsampling)
      m_max_vsampling = m_ccomp[i].m_vsampling;
  }

  for(i = 0; i < m_jpeg_ncomp; i++)
  {
    m_ccomp[i].m_h_factor = m_max_hsampling / m_ccomp[i].m_hsampling;
    m_ccomp[i].m_v_factor = m_max_vsampling / m_ccomp[i].m_vsampling;
  }

  m_jpeg_mode = JPEG_PROGRESSIVE;

  m_marker = JM_NONE;

  return JPEG_OK;
} // CJPEGDecoder::ParseSOF2()


JERRCODE CJPEGDecoder::ParseSOF3(void)
{
  int i;
  int len;
  JERRCODE jerr;

  TRC0("-> SOF3");

  if(m_src.currPos + 2 >= m_src.DataLen)
  {
    LOG0("Error: buffer too small");
    return JPEG_BUFF_TOO_SMALL;
  }

  m_src._READ_WORD(&len);
  len -= 2;

  m_src._READ_BYTE(&m_jpeg_precision);

  if(m_jpeg_precision < 2 || m_jpeg_precision > 16)
  {
    return JPEG_BAD_FRAME_SEGMENT;
  }

  m_src._READ_WORD(&m_jpeg_height);
  m_src._READ_WORD(&m_jpeg_width);
  m_src._READ_BYTE(&m_jpeg_ncomp);

  TRC1("  height    - ",m_jpeg_height);
  TRC1("  width     - ",m_jpeg_width);
  TRC1("  nchannels - ",m_jpeg_ncomp);

  if(m_jpeg_ncomp != 1)
  {
    return JPEG_NOT_IMPLEMENTED;
  }

  len -= 6;

  if(len != m_jpeg_ncomp * 3)
  {
    return JPEG_BAD_SEGMENT_LENGTH;
  }

  for(i = 0; i < m_jpeg_ncomp; i++)
  {
    m_src._READ_BYTE(&m_ccomp[i].m_id);

    int ss;

    m_src._READ_BYTE(&ss);

    m_ccomp[i].m_hsampling  = (ss >> 4) & 0x0f;
    m_ccomp[i].m_vsampling  = (ss     ) & 0x0f;

    m_src._READ_BYTE(&m_ccomp[i].m_q_selector);

    if(m_ccomp[i].m_hsampling <= 0 || m_ccomp[i].m_vsampling <= 0)
    {
      return JPEG_BAD_FRAME_SEGMENT;
    }

    m_ccomp[i].m_nblocks = m_ccomp[i].m_hsampling * m_ccomp[i].m_vsampling;

    TRC1("    id ",m_ccomp[i].m_id);
    TRC1("      hsampling - ",m_ccomp[i].m_hsampling);
    TRC1("      vsampling - ",m_ccomp[i].m_vsampling);
    TRC1("      qselector - ",m_ccomp[i].m_q_selector);
  }

  jerr = DetectSampling();
  if(JPEG_OK != jerr)
  {
    return jerr;
  }

  m_max_hsampling = m_ccomp[0].m_hsampling;
  m_max_vsampling = m_ccomp[0].m_vsampling;

  for(i = 0; i < m_jpeg_ncomp; i++)
  {
    if(m_max_hsampling < m_ccomp[i].m_hsampling)
      m_max_hsampling = m_ccomp[i].m_hsampling;

    if(m_max_vsampling < m_ccomp[i].m_vsampling)
      m_max_vsampling = m_ccomp[i].m_vsampling;
  }

  for(i = 0; i < m_jpeg_ncomp; i++)
  {
    m_ccomp[i].m_h_factor = m_max_hsampling / m_ccomp[i].m_hsampling;
    m_ccomp[i].m_v_factor = m_max_vsampling / m_ccomp[i].m_vsampling;
  }

  m_jpeg_mode = JPEG_LOSSLESS;

  m_marker = JM_NONE;

  return JPEG_OK;
} // CJPEGDecoder::ParseSOF3()


JERRCODE CJPEGDecoder::ParseDRI(void)
{
  int len;

  TRC0("-> DRI");

  if(m_src.currPos + 2 >= m_src.DataLen)
  {
    LOG0("Error: buffer too small");
    return JPEG_BUFF_TOO_SMALL;
  }

  m_src._READ_WORD(&len);
  len -= 2;

  if(len != 2)
  {
    return JPEG_BAD_SEGMENT_LENGTH;
  }

  m_src._READ_WORD(&m_jpeg_restart_interval);

  TRC1("  restart interval - ",m_jpeg_restart_interval);

  m_restarts_to_go = m_jpeg_restart_interval;

  m_marker = JM_NONE;

  return JPEG_OK;
} // CJPEGDecoder::ParseDRI()


JERRCODE CJPEGDecoder::ParseRST(void)
{
  JERRCODE jerr;

  TRC0("-> RST");

  if(m_marker == 0xff)
  {
    m_src.currPos--;
    m_marker = JM_NONE;
  }

  if(m_marker == JM_NONE)
  {
    jerr = NextMarker(&m_marker);
    if(JPEG_OK != jerr)
    {
      LOG0("Error: NextMarker() failed");
      return JPEG_INTERNAL_ERROR;
    }
  }

  TRC1("restart interval ",m_next_restart_num);
  if(m_marker == ((int)JM_RST0 + m_next_restart_num))
  {
    m_marker = JM_NONE;
  }
  else
  {
    LOG1("  - got marker   - ",m_marker);
    LOG1("  - but expected - ",(int)JM_RST0 + m_next_restart_num);
    m_marker = JM_NONE;
//    return JPEG_BAD_RESTART;
  }

  // Update next-restart state
  m_next_restart_num = (m_next_restart_num + 1) & 7;

  return JPEG_OK;
} // CJPEGDecoder::ParseRST()


JERRCODE CJPEGDecoder::ParseSOS(void)
{
  int i;
  int ci;
  int len;

  TRC0("-> SOS");

  if(m_src.currPos + 2 >= m_src.DataLen)
  {
    LOG0("Error: buffer too small");
    return JPEG_BUFF_TOO_SMALL;
  }

  m_src._READ_WORD(&len);

  // store position to return to in subsequent ReadData call
  m_sos_len = len;

  len -= 2;

  m_src._READ_BYTE(&m_scan_ncomps);

  if(m_scan_ncomps < 1 || m_scan_ncomps > MAX_COMPS_PER_SCAN)
  {
    return JPEG_BAD_SCAN_SEGMENT;
  }

  if(JPEG_PROGRESSIVE != m_jpeg_mode && m_scan_ncomps < m_jpeg_ncomp && m_scan_ncomps != 1)
  {
    // TODO:
    // does not support baseline multi-scan images with more than 1 component per scan for now..
    return JPEG_NOT_IMPLEMENTED;
  }

  if(len != ((m_scan_ncomps * 2) + 4))
  {
    return JPEG_BAD_SEGMENT_LENGTH;
  }

  TRC1("  ncomps - ",m_scan_ncomps);

  for(i = 0; i < m_scan_ncomps; i++)
  {
    int id;
    int huff_sel;

    m_src._READ_BYTE(&id);
    m_src._READ_BYTE(&huff_sel);

    TRC1("    id - ",id);
    TRC1("      dc_selector - ",(huff_sel >> 4) & 0x0f);
    TRC1("      ac_selector - ",(huff_sel     ) & 0x0f);

    m_ccomp[i].m_lastDC = 0;

    for(ci = 0; ci < m_jpeg_ncomp; ci++)
    {
      if(id == m_ccomp[ci].m_id)
      {
        m_curr_comp_no        = ci;
        m_ccomp[ci].m_comp_no = ci;
        goto comp_id_match;
      }
    }

    return JPEG_BAD_COMPONENT_ID;

comp_id_match:

    m_ccomp[ci].m_dc_selector = (huff_sel >> 4) & 0x0f;
    m_ccomp[ci].m_ac_selector = (huff_sel     ) & 0x0f;
  }

  m_src._READ_BYTE(&m_ss);
  m_src._READ_BYTE(&m_se);

  int t;

  m_src._READ_BYTE(&t);

  m_ah = (t >> 4) & 0x0f;
  m_al = (t     ) & 0x0f;

  TRC1("  Ss - ",m_ss);
  TRC1("  Se - ",m_se);
  TRC1("  Ah - ",m_ah);
  TRC1("  Al - ",m_al);

  // detect JPEG color space
  if(m_jfif_app0_detected)
  {
    switch(m_jpeg_ncomp)
    {
    case 1:  m_jpeg_color = JC_GRAY;    break;
    case 3:  m_jpeg_color = JC_YCBCR;   break;
    default: m_jpeg_color = JC_UNKNOWN; break;
    }
  }

  if(m_adobe_app14_detected)
  {
    switch(m_adobe_app14_transform)
    {
    case 0:
      switch(m_jpeg_ncomp)
      {
      case 1:  m_jpeg_color = JC_GRAY;    break;
      case 3:  m_jpeg_color = JC_RGB;     break;
      case 4:  m_jpeg_color = JC_CMYK;    break;
      default: m_jpeg_color = JC_UNKNOWN; break;
      }
      break;

    case 1:  m_jpeg_color = JC_YCBCR;   break;
    case 2:  m_jpeg_color = JC_YCCK;    break;
    default: m_jpeg_color = JC_UNKNOWN; break;
    }
  }

  // try to guess what color space is used...
  if(!m_jfif_app0_detected && !m_adobe_app14_detected)
  {
    switch(m_jpeg_ncomp)
    {
    case 1:  m_jpeg_color = JC_GRAY;    break;
    case 3:  m_jpeg_color = JC_YCBCR;   break;
    default: m_jpeg_color = JC_UNKNOWN; break;
    }
  }

  m_restarts_to_go   = m_jpeg_restart_interval;
  m_next_restart_num = 0;

  m_marker = JM_NONE;

  return JPEG_OK;
} // CJPEGDecoder::ParseSOS()


JERRCODE CJPEGDecoder::ParseJPEGBitStream(JOPERATION op)
{
  int      i;
#ifdef __TIMING__
  Ipp64u   c0;
  Ipp64u   c1;
#endif
  JERRCODE jerr = JPEG_OK;

  m_marker = JM_NONE;

  for(;;)
  {
    if(JM_NONE == m_marker)
    {
      jerr = NextMarker(&m_marker);
      if(JPEG_OK != jerr)
      {
        return jerr;
      }
    }

    switch(m_marker)
    {
    case JM_SOI:
      jerr = ParseSOI();
      if(JPEG_OK != jerr)
      {
        return jerr;
      }
      break;

    case JM_APP0:
      jerr = ParseAPP0();
      if(JPEG_OK != jerr)
      {
        return jerr;
      }
      break;

    case JM_APP1:
      jerr = ParseAPP1();
      if(JPEG_OK != jerr)
      {
        return jerr;
      }
      break;

    case JM_APP14:
      jerr = ParseAPP14();
      if(JPEG_OK != jerr)
      {
        return jerr;
      }
      break;

    case JM_COM:
      jerr = ParseCOM();
      if(JPEG_OK != jerr)
      {
        return jerr;
      }
      break;

    case JM_DQT:
      jerr = ParseDQT();
      if(JPEG_OK != jerr)
      {
        return jerr;
      }
      break;

    case JM_SOF0:
    case JM_SOF1:
      jerr = ParseSOF0();
      if(JPEG_OK != jerr)
      {
        return jerr;
      }
      break;

    case JM_SOF2:
      jerr = ParseSOF2();
      if(JPEG_OK != jerr)
      {
        return jerr;
      }
      break;

    case JM_SOF3:
      jerr = ParseSOF3();
      if(JPEG_OK != jerr)
      {
        return jerr;
      }
      break;

    case JM_SOF5:
    case JM_SOF6:
    case JM_SOF7:
    case JM_SOF9:
    case JM_SOFA:
    case JM_SOFB:
    case JM_SOFD:
    case JM_SOFE:
    case JM_SOFF:
      return JPEG_NOT_IMPLEMENTED;

    case JM_DHT:
      jerr = ParseDHT();
      if(JPEG_OK != jerr)
      {
        return jerr;
      }
      break;

    case JM_DRI:
      jerr = ParseDRI();
      if(JPEG_OK != jerr)
      {
        return jerr;
      }
      break;

    case JM_SOS:
      jerr = ParseSOS();
      if(JPEG_OK != jerr)
      {
        return jerr;
      }

      if(JO_READ_HEADER == op)
      {
        m_src.currPos -= m_sos_len + 2;
        // stop here, when we are reading header
        return JPEG_OK;
      }

      if(JO_READ_DATA == op)
      {
        jerr = Init();
        if(JPEG_OK != jerr)
        {
          return jerr;
        }

        switch(m_jpeg_mode)
        {
        case JPEG_BASELINE:
          if(m_scan_ncomps == m_jpeg_ncomp)
            jerr = DecodeScanBaseline();
          else
          {
            jerr = DecodeScanMultiple();

            if(m_ac_scans_completed == m_jpeg_ncomp)
            {
              Ipp16s* pMCUBuf;

              for(i = 0; i < m_numyMCU; i++)
              {
                pMCUBuf = m_block_buffer + (i* m_numxMCU * DCTSIZE2* m_nblock);
#ifdef __TIMING__
              c0 = ippGetCpuClocks();
#endif
                ReconstructMCURowBL(pMCUBuf,0);
#ifdef __TIMING__
              c1 = ippGetCpuClocks();
              m_clk_dct += c1 - c0;
#endif

#ifdef __TIMING__
              c0 = ippGetCpuClocks();
#endif
                UpSampling(i,0);
#ifdef __TIMING__
              c1 = ippGetCpuClocks();
              m_clk_ss += c1 - c0;
#endif

#ifdef __TIMING__
              c0 = ippGetCpuClocks();
#endif
                ColorConvert(i,0);
#ifdef __TIMING__
              c1 = ippGetCpuClocks();
              m_clk_cc += c1 - c0;
#endif
              }
            }
          }
          break;

        case JPEG_PROGRESSIVE:
          {
            jerr = DecodeScanProgressive();

            m_ac_scans_completed = 0;
            for(i = 0; i < m_jpeg_ncomp; i++)
            {
              m_ac_scans_completed += m_ccomp[i].m_ac_scan_completed;
            }

            if(JPEG_OK != jerr ||
              (m_dc_scan_completed != 0 && m_ac_scans_completed == m_jpeg_ncomp))
            {
              Ipp16s* pMCUBuf;
              for(i = 0; i < m_numyMCU; i++)
              {
                pMCUBuf = m_block_buffer + (i* m_numxMCU * DCTSIZE2* m_nblock);
                ReconstructMCURowBL(pMCUBuf,0);
                UpSampling(i,0);
                ColorConvert(i,0);
              }
            }
          }
          break;

        case JPEG_LOSSLESS:
          jerr = DecodeScanLossless();
          break;
        } // m_jpeg_mode

        if(JPEG_OK != jerr)
          return jerr;
      }

      break;

    case JM_RST0:
    case JM_RST1:
    case JM_RST2:
    case JM_RST3:
    case JM_RST4:
    case JM_RST5:
    case JM_RST6:
    case JM_RST7:
      jerr = ParseRST();
      if(JPEG_OK != jerr)
      {
        return jerr;
      }
      break;

    case JM_EOI:
      jerr = ParseEOI();
      goto Exit;

    default:
      TRC1("-> Unknown marker ",m_marker);
      TRC0("..Skipping");
      jerr = SkipMarker();
      if(JPEG_OK != jerr)
        return jerr;

      break;
    }
  }

Exit:

  return jerr;
} // CJPEGDecoder::ParseJPEGBitStream()


JERRCODE CJPEGDecoder::Init(void)
{
  int i;
  int tr_buf_size = 0;
  CJPEGColorComponent* curr_comp;
  JERRCODE  jerr;

  m_num_threads = get_num_threads();

  // not implemented yet
  if(m_jpeg_sampling == JS_OTHER)
  {
    return JPEG_NOT_IMPLEMENTED;
  }

  // TODO:
  //   need to add support for images with more then 4 components per frame

  for(m_nblock = 0, i = 0; i < m_jpeg_ncomp; i++)
  {
    curr_comp = &m_ccomp[i];

    m_nblock += (curr_comp->m_hsampling * curr_comp->m_vsampling);

    switch(m_jpeg_mode)
    {
    case JPEG_BASELINE:
      {
        curr_comp->m_cc_height = m_mcuHeight;
        curr_comp->m_cc_step   = m_numxMCU * m_mcuWidth;

        curr_comp->m_ss_height = curr_comp->m_cc_height / curr_comp->m_v_factor;
        curr_comp->m_ss_step   = curr_comp->m_cc_step   / curr_comp->m_h_factor;

        if(curr_comp->m_h_factor == 2 && curr_comp->m_v_factor == 2)
        {
          curr_comp->m_ss_height += 2; // add border lines (top and bottom)
        }

        if(m_scan_ncomps == m_jpeg_ncomp)
          tr_buf_size = m_numxMCU * m_nblock * DCTSIZE2 * sizeof(Ipp16s) * m_num_threads;
        else
          tr_buf_size = m_numxMCU * m_numyMCU * m_nblock * DCTSIZE2 * sizeof(Ipp16s);

        break;
      } // JPEG_BASELINE

      case JPEG_PROGRESSIVE:
      {

        curr_comp->m_cc_height = m_mcuHeight;
        curr_comp->m_cc_step   = m_numxMCU * m_mcuWidth;

        curr_comp->m_ss_height = curr_comp->m_cc_height / curr_comp->m_v_factor;
        curr_comp->m_ss_step   = curr_comp->m_cc_step   / curr_comp->m_h_factor;

        if(curr_comp->m_h_factor == 2 && curr_comp->m_v_factor == 2)
        {
          curr_comp->m_ss_height += 2; // add border lines (top and bottom)
        }

        tr_buf_size = m_numxMCU * m_numyMCU * m_nblock * DCTSIZE2 * sizeof(Ipp16s);

        break;
      } // JPEG_PROGRESSIVE

      case JPEG_LOSSLESS:
      {
        curr_comp->m_cc_height = m_mcuHeight;
        curr_comp->m_cc_step   = m_numxMCU * m_mcuWidth * sizeof(Ipp16s);

        curr_comp->m_ss_height = curr_comp->m_cc_height / curr_comp->m_v_factor;
        curr_comp->m_ss_step   = curr_comp->m_cc_step   / curr_comp->m_h_factor;

        tr_buf_size = m_numxMCU*sizeof(Ipp16s);

        break;
      } // JPEG_LOSSLESS
    } // m_jpeg_mode

    // color convert buffer
    jerr = curr_comp->CreateBufferCC(m_num_threads);
    if(JPEG_OK != jerr)
      return jerr;

    jerr = curr_comp->CreateBufferSS(m_num_threads);
    if(JPEG_OK != jerr)
      return jerr;

    curr_comp->m_curr_row = (Ipp16s*)curr_comp->GetCCBufferPtr();
    curr_comp->m_prev_row = (Ipp16s*)curr_comp->GetSSBufferPtr();
  } // m_jpeg_ncomp

  if(0 == m_block_buffer)
  {
    m_block_buffer = (Ipp16s*)ippMalloc(tr_buf_size);
    if(0 == m_block_buffer)
    {
      return JPEG_OUT_OF_MEMORY;
    }

    ippsZero_8u((Ipp8u*)m_block_buffer,tr_buf_size);
  }

  m_state.Create();

  return JPEG_OK;
} // CJPEGDecoder::Init()


JERRCODE CJPEGDecoder::ColorConvert(int nMCURow,int thread_id)
{
  int       dstStep;
  Ipp8u*    dst;
  IppiSize  roi;
  IppStatus status;

  if(nMCURow == m_numyMCU - 1)
  {
    m_ccHeight = m_mcuHeight - m_yPadding;
  }

  roi.width  = m_dst.width;
  roi.height = m_ccHeight;

  dst     = m_dst.p.Data8u + nMCURow * m_mcuHeight * m_dst.lineStep;
  dstStep = m_dst.lineStep;

  if(m_jpeg_color == JC_UNKNOWN && m_dst.color == JC_UNKNOWN)
  {
    switch(m_jpeg_ncomp)
    {
    case 1:
      {
        int    srcStep;
        Ipp8u* src;

        src = m_ccomp[0].GetCCBufferPtr(thread_id);

        srcStep = m_ccomp[0].m_cc_step;

        status = ippiCopy_8u_C1R(src,srcStep,dst,dstStep,roi);

        if(ippStsNoErr != status)
        {
          LOG1("IPP Error: ippiCopy_8u_C1R() failed - ",status);
          return JPEG_INTERNAL_ERROR;
        }
      }
      break;

    case 3:
      {
        int    srcStep;
        Ipp8u* src[3];

        src[0] = m_ccomp[0].GetCCBufferPtr(thread_id);
        src[1] = m_ccomp[1].GetCCBufferPtr(thread_id);
        src[2] = m_ccomp[2].GetCCBufferPtr(thread_id);

        srcStep = m_ccomp[0].m_cc_step;

        status = ippiCopy_8u_P3C3R(src,srcStep,dst,dstStep,roi);

        if(ippStsNoErr != status)
        {
          LOG1("IPP Error: ippiCopy_8u_P3C3R() failed - ",status);
          return JPEG_INTERNAL_ERROR;
        }
      }
      break;

    case 4:
      {
        int    srcStep;
        Ipp8u* src[4];

        src[0] = m_ccomp[0].GetCCBufferPtr(thread_id);
        src[1] = m_ccomp[1].GetCCBufferPtr(thread_id);
        src[2] = m_ccomp[2].GetCCBufferPtr(thread_id);
        src[3] = m_ccomp[3].GetCCBufferPtr(thread_id);

        srcStep = m_ccomp[0].m_cc_step;

        status = ippiCopy_8u_P4C4R(src,srcStep,dst,dstStep,roi);

        if(ippStsNoErr != status)
        {
          LOG1("IPP Error: ippiCopy_8u_P4C4R() failed - ",status);
          return JPEG_INTERNAL_ERROR;
        }
      }
      break;

    default:
      return JPEG_NOT_IMPLEMENTED;
    }
  }

  // Gray to Gray
  if(m_jpeg_color == JC_GRAY && m_dst.color == JC_GRAY)
  {
    int    srcStep;
    Ipp8u* src;

    src = m_ccomp[0].GetCCBufferPtr(thread_id);

    srcStep = m_ccomp[0].m_cc_step;

    status = ippiCopy_8u_C1R(src,srcStep,dst,dstStep,roi);

    if(ippStsNoErr != status)
    {
      LOG1("IPP Error: ippiCopy_8u_C1R() failed - ",status);
      return JPEG_INTERNAL_ERROR;
    }
  }

  // Gray to RGB
  if(m_jpeg_color == JC_GRAY && m_dst.color == JC_RGB)
  {
    int    srcStep;
    Ipp8u* src[3];

    src[0] = m_ccomp[0].GetCCBufferPtr(thread_id);
    src[1] = m_ccomp[1].GetCCBufferPtr(thread_id);
    src[2] = m_ccomp[2].GetCCBufferPtr(thread_id);

    srcStep = m_ccomp[0].m_cc_step;

    status = ippiCopy_8u_P3C3R(src,srcStep,dst,dstStep,roi);

    if(ippStsNoErr != status)
    {
      LOG1("IPP Error: ippiCopy_8u_P3C3R() failed - ",status);
      return JPEG_INTERNAL_ERROR;
    }
  }

  // Gray to BGR
  if(m_jpeg_color == JC_GRAY && m_dst.color == JC_BGR)
  {
    int    srcStep;
    Ipp8u* src[3];

    src[0] = m_ccomp[0].GetCCBufferPtr(thread_id);
    src[1] = m_ccomp[1].GetCCBufferPtr(thread_id);
    src[2] = m_ccomp[2].GetCCBufferPtr(thread_id);

    srcStep = m_ccomp[0].m_cc_step;

    status = ippiCopy_8u_P3C3R(src,srcStep,dst,dstStep,roi);

    if(ippStsNoErr != status)
    {
      LOG1("IPP Error: ippiCopy_8u_P3C3R() failed - ",status);
      return JPEG_INTERNAL_ERROR;
    }
  }

  // RGB to RGB
  if(m_jpeg_color == JC_RGB && m_dst.color == JC_RGB)
  {
    int    srcStep;
    Ipp8u* src[3];

    src[0] = m_ccomp[0].GetCCBufferPtr(thread_id);
    src[1] = m_ccomp[1].GetCCBufferPtr(thread_id);
    src[2] = m_ccomp[2].GetCCBufferPtr(thread_id);

    srcStep = m_ccomp[0].m_cc_step;

    status = ippiCopy_8u_P3C3R(src,srcStep,dst,dstStep,roi);

    if(ippStsNoErr != status)
    {
      LOG1("IPP Error: ippiCopy_8u_P3C3R() failed - ",status);
      return JPEG_INTERNAL_ERROR;
    }
  }

  // RGB to BGR
  if(m_jpeg_color == JC_RGB && m_dst.color == JC_BGR)
  {
    int    srcStep;
    Ipp8u* src[3];

    src[0] = m_ccomp[0].GetCCBufferPtr(thread_id);
    src[1] = m_ccomp[1].GetCCBufferPtr(thread_id);
    src[2] = m_ccomp[2].GetCCBufferPtr(thread_id);

    srcStep = m_ccomp[0].m_cc_step;

    status = ippiCopy_8u_P3C3R(src,srcStep,dst,dstStep,roi);

    if(ippStsNoErr != status)
    {
      LOG1("IPP Error: ippiCopy_8u_P3C3R() failed - ",status);
      return JPEG_INTERNAL_ERROR;
    }
  }

  // YCbCr to Gray
  // TODO: need to add YCbCr to Gray branch

  // YCbCr to RGB
  if(m_jpeg_color == JC_YCBCR && m_dst.color == JC_RGB)
  {
    int srcStep;
    const Ipp8u* src[3];
    src[0] = m_ccomp[0].GetCCBufferPtr(thread_id);
    src[1] = m_ccomp[1].GetCCBufferPtr(thread_id);
    src[2] = m_ccomp[2].GetCCBufferPtr(thread_id);

    srcStep = m_ccomp[0].m_cc_step;

    status = ippiYCbCrToRGB_JPEG_8u_P3C3R(src,srcStep,dst,dstStep,roi);

    if(ippStsNoErr != status)
    {
      LOG1("IPP Error: ippiYCbCrToRGB_JPEG_8u_P3C3R() failed - ",status);
      return JPEG_INTERNAL_ERROR;
    }
  }

  // YCbCr to BGR
  if(m_jpeg_color == JC_YCBCR && m_dst.color == JC_BGR)
  {
    int srcStep;
    const Ipp8u* src[3];
    src[0] = m_ccomp[0].GetCCBufferPtr(thread_id);
    src[1] = m_ccomp[1].GetCCBufferPtr(thread_id);
    src[2] = m_ccomp[2].GetCCBufferPtr(thread_id);

    srcStep = m_ccomp[0].m_cc_step;

    status = ippiYCbCrToBGR_JPEG_8u_P3C3R(src,srcStep,dst,dstStep,roi);

    if(ippStsNoErr != status)
    {
      LOG1("IPP Error: ippiYCbCrToBGR_JPEG_8u_P3C3R() failed - ",status);
      return JPEG_INTERNAL_ERROR;
    }
  }

  // YCbCr to YCbCr (422 sampling)
  if(m_jpeg_color == JC_YCBCR && m_dst.color == JC_YCBCR &&
     m_jpeg_sampling == JS_422 && m_dst.sampling == JS_422)
  {
    int srcStep[3];
    const Ipp8u* src[3];
    src[0] = m_ccomp[0].GetCCBufferPtr(thread_id);
    src[1] = m_ccomp[1].GetCCBufferPtr(thread_id);
    src[2] = m_ccomp[2].GetCCBufferPtr(thread_id);

    srcStep[0] = m_ccomp[0].m_cc_step;
    srcStep[1] = m_ccomp[1].m_cc_step;
    srcStep[2] = m_ccomp[2].m_cc_step;

    status = ippiYCbCr422_8u_P3C2R(src,srcStep,dst,dstStep,roi);

    if(ippStsNoErr != status)
    {
      LOG1("IPP Error: ippiYCbCr422_8u_P3C2R() failed - ",status);
      return JPEG_INTERNAL_ERROR;
    }
  }

  // CMYK to CMYK
  if(m_jpeg_color == JC_CMYK && m_dst.color == JC_CMYK)
  {
    int    srcStep;
    Ipp8u* src[4];

    src[0] = m_ccomp[0].GetCCBufferPtr(thread_id);
    src[1] = m_ccomp[1].GetCCBufferPtr(thread_id);
    src[2] = m_ccomp[2].GetCCBufferPtr(thread_id);
    src[3] = m_ccomp[3].GetCCBufferPtr(thread_id);

    srcStep = m_ccomp[0].m_cc_step;

    status = ippiCopy_8u_P4C4R(src,srcStep,dst,dstStep,roi);

    if(ippStsNoErr != status)
    {
      LOG1("IPP Error: ippiCopy_8u_P4C4R() failed - ",status);
      return JPEG_INTERNAL_ERROR;
    }
  }

  // YCCK to CMYK
  if(m_jpeg_color == JC_YCCK && m_dst.color == JC_CMYK)
  {
    int    srcStep;
    const Ipp8u* src[4];

    src[0] = m_ccomp[0].GetCCBufferPtr(thread_id);
    src[1] = m_ccomp[1].GetCCBufferPtr(thread_id);
    src[2] = m_ccomp[2].GetCCBufferPtr(thread_id);
    src[3] = m_ccomp[3].GetCCBufferPtr(thread_id);

    srcStep = m_ccomp[0].m_cc_step;

    status = ippiYCCKToCMYK_JPEG_8u_P4C4R(src,srcStep,dst,dstStep,roi);

    if(ippStsNoErr != status)
    {
      LOG1("IPP Error: ippiYCCKToCMYK_JPEG_8u_P4C4R() failed - ",status);
      return JPEG_INTERNAL_ERROR;
    }
  }

  return JPEG_OK;
} // CJPEGDecoder::ColorConvert()


JERRCODE CJPEGDecoder::UpSampling(int nMCURow,int thread_id)
{
  int i, k, n;
  CJPEGColorComponent* curr_comp;
  IppStatus status;

  for(k = 0; k < m_jpeg_ncomp; k++)
  {
    curr_comp = &m_ccomp[k];

    // sampling 444
    // nothing to do for 444

    // sampling 422
    if(curr_comp->m_h_factor == 2 && curr_comp->m_v_factor == 1)
    {
      int srcStep;
      int dstStep;
      Ipp8u* src = curr_comp->GetSSBufferPtr(thread_id);
      Ipp8u* dst = curr_comp->GetCCBufferPtr(thread_id);
      srcStep = curr_comp->m_ss_step;
      dstStep = curr_comp->m_cc_step;

      if(m_dst.sampling == JS_422)
      {
        IppiSize roi;
        roi.width  = curr_comp->m_ss_step;
        roi.height = curr_comp->m_ss_height;

        status = ippiCopy_8u_C1R(src,srcStep,dst,dstStep,roi);
        if(ippStsNoErr != status)
        {
          LOG0("Error: ippiCopy_8u_C1R() failed!");
          return JPEG_INTERNAL_ERROR;
        }
      }
      else
      {
        for(i = 0; i < m_mcuHeight; i++)
        {
          status = ippiSampleUpRowH2V1_Triangle_JPEG_8u_C1(src,srcStep,dst);
          if(ippStsNoErr != status)
          {
            LOG0("Error: ippiSampleUpRowH2V1_Triangle_JPEG_8u_C1() failed!");
            return JPEG_INTERNAL_ERROR;
          }
          src += srcStep;
          dst += dstStep;
        }
      }
    }

    // sampling 244
    if(curr_comp->m_h_factor == 1 && curr_comp->m_v_factor == 2)
    {
      int srcStep;
      Ipp8u* src = curr_comp->GetSSBufferPtr(thread_id);
      Ipp8u* dst = curr_comp->GetCCBufferPtr(thread_id);

      srcStep = curr_comp->m_ss_step;

      for(i = 0; i < m_mcuHeight >> 1; i++)
      {
        for(n = 0; n < 2; n++)
        {
          status = ippsCopy_8u(src,dst,srcStep);
          if(ippStsNoErr != status)
          {
            LOG0("Error: ippsCopy_8u() failed!");
            return JPEG_INTERNAL_ERROR;
          }
          dst += srcStep;
        }
        src += srcStep;
      }
    }

    // sampling 411
    if(curr_comp->m_h_factor == 2 && curr_comp->m_v_factor == 2)
    {
      int L    = nMCURow == 0 ? 1 : 9;
      int srcStep;
      int dstStep;
      Ipp8u* p1;
      Ipp8u* p2;
      Ipp8u* src = curr_comp->GetSSBufferPtr(thread_id);
      Ipp8u* dst = curr_comp->GetCCBufferPtr(thread_id);
      srcStep = curr_comp->m_ss_step;
      dstStep = curr_comp->m_cc_step;
      p1 = src + srcStep;
      p2 = src;

      ippsCopy_8u(src + L*srcStep,src,         srcStep);
      ippsCopy_8u(src + 8*srcStep,src + 9*srcStep,srcStep);

      for(i = 0; i < m_mcuHeight >> 1; i++)
      {
        for(n = 0; n < 2; n++)
        {
          p2 = (n == 0) ? p1 - srcStep : p1 + srcStep;

          status = ippiSampleUpRowH2V2_Triangle_JPEG_8u_C1(p1,p2,srcStep,dst);
          if(ippStsNoErr != status)
          {
            LOG0("Error: ippiSampleUpRowH2V2_Triangle_JPEG_8u_C1() failed!");
            return JPEG_INTERNAL_ERROR;
          }
          dst += dstStep;
        }
        p1 += srcStep;
      }
    } // 411
  } // for m_jpeg_ncomp

  return JPEG_OK;
} // CJPEGDecoder::UpSampling()


JERRCODE CJPEGDecoder::DecodeHuffmanMCURowBL(Ipp16s* pMCUBuf)
{
  int       j, n, k, l;
  int       srcLen;
  Ipp8u*    src;
  JERRCODE  jerr;
  IppStatus status;

  src    = m_src.pData;
  srcLen = m_src.DataLen;

  for(j = 0; j < m_numxMCU; j++)
  {
    if(m_jpeg_restart_interval)
    {
      if(m_restarts_to_go == 0)
      {
        jerr = ProcessRestart();
        if(JPEG_OK != jerr)
        {
          LOG0("Error: ProcessRestart() failed!");
          return jerr;
        }
      }
    }

    for(n = 0; n < m_jpeg_ncomp; n++)
    {
      Ipp16s*                lastDC = &m_ccomp[n].m_lastDC;
      IppiDecodeHuffmanSpec* dctbl  = m_dctbl[m_ccomp[n].m_dc_selector];
      IppiDecodeHuffmanSpec* actbl  = m_actbl[m_ccomp[n].m_ac_selector];

      for(k = 0; k < m_ccomp[n].m_vsampling; k++)
      {
        for(l = 0; l < m_ccomp[n].m_hsampling; l++)
        {
          status = ippiDecodeHuffman8x8_JPEG_1u16s_C1(
                     src,srcLen,&m_src.currPos,pMCUBuf,lastDC,(int*)&m_marker,
                     dctbl,actbl,m_state);

          if(ippStsNoErr > status)
          {
            LOG0("Error: ippiDecodeHuffman8x8_JPEG_1u16s_C1() failed!");
            m_marker = JM_NONE;
            return JPEG_INTERNAL_ERROR;
          }

          pMCUBuf += DCTSIZE2;
        } // for m_hsampling
      } // for m_vsampling
    } // for m_jpeg_ncomp

    m_restarts_to_go--;
  } // for m_numxMCU

  return JPEG_OK;
} // CJPEGDecoder::DecodeHuffmanMCURowBL()


JERRCODE CJPEGDecoder::DecodeHuffmanMCURowLS(Ipp16s* pMCUBuf)
{
  int       j, n, k, l;
  Ipp8u*    src;
  int       srcLen;
  JERRCODE  jerr;
  IppStatus status;

  src    = m_src.pData;
  srcLen = m_src.DataLen;

  for(j = 0; j < m_numxMCU; j++)
  {
    if(m_jpeg_restart_interval)
    {
      if(m_restarts_to_go == 0)
      {
        jerr = ProcessRestart();
        if(JPEG_OK != jerr)
        {
          LOG0("Error: ProcessRestart() failed!");
          return jerr;
        }
      }
    }

    for(n = 0; n < m_jpeg_ncomp; n++)
    {
      IppiDecodeHuffmanSpec* dctbl = m_dctbl[m_ccomp[n].m_dc_selector];

      for(k = 0; k < m_ccomp[n].m_vsampling; k++)
      {
        for(l = 0; l < m_ccomp[n].m_hsampling; l++)
        {
          status = ippiDecodeHuffmanOne_JPEG_1u16s_C1(
                     src,srcLen,&m_src.currPos,pMCUBuf,(int*)&m_marker,
                     dctbl,m_state);

          if(ippStsNoErr > status)
          {
            LOG0("Error: ippiDecodeHuffmanOne_JPEG_1u16s_C1() failed!");
            return JPEG_INTERNAL_ERROR;
          }

          pMCUBuf++;
        } // for m_hsampling
      } // for m_vsampling
    } // for m_jpeg_ncomp

    m_restarts_to_go --;
  } // for m_numxMCU

  return JPEG_OK;
} // CJPEGDecoder::DecodeHuffmanMCURowLS()


JERRCODE CJPEGDecoder::ReconstructMCURowBL(
  Ipp16s* pMCUBuf,
  int     thread_id)
{
  int       mcu_col, c, k, l;
  Ipp8u*    dst     = 0;
  int       dstStep = m_ccWidth;
  Ipp16u*   qtbl;
  IppStatus status;
  CJPEGColorComponent* curr_comp;
#ifdef __TIMING__
  Ipp64u   c0;
  Ipp64u   c1;
#endif

  for(mcu_col = 0; mcu_col < m_numxMCU; mcu_col++)
  {
    for(c = 0; c < m_jpeg_ncomp; c++)
    {
      curr_comp = &m_ccomp[c];

      qtbl = m_qntbl[curr_comp->m_q_selector];

      for(k = 0; k < curr_comp->m_vsampling; k++)
      {
        if(curr_comp->m_hsampling == m_max_hsampling &&
           curr_comp->m_vsampling == m_max_vsampling)
        {
          dstStep = curr_comp->m_cc_step;
          dst     = curr_comp->GetCCBufferPtr(thread_id) + mcu_col*8*curr_comp->m_hsampling + k*8*dstStep;
        }
        else
        {
          dstStep = curr_comp->m_ss_step;
          dst     = curr_comp->GetSSBufferPtr(thread_id) + mcu_col*8*curr_comp->m_hsampling + k*8*dstStep;
        }

        // skip border row (when 244 or 411 sampling)
        if(curr_comp->m_h_factor == 2 && curr_comp->m_v_factor == 2)
        {
          dst += dstStep;
        }

        for(l = 0; l < curr_comp->m_hsampling; l++)
        {
          dst += l*8;

#ifdef __TIMING__
          c0 = ippGetCpuClocks();
#endif
          status = ippiDCTQuantInv8x8LS_JPEG_16s8u_C1R(
                     pMCUBuf,dst,dstStep,qtbl);

          if(ippStsNoErr > status)
          {
            LOG0("Error: ippiDCTQuantInv8x8LS_JPEG_16s8u_C1R() failed!");
            return JPEG_INTERNAL_ERROR;
          }
#ifdef __TIMING__
          c1 = ippGetCpuClocks();
          m_clk_dct += c1 - c0;
#endif

          pMCUBuf += DCTSIZE2;
        } // for m_hsampling
      } // for m_vsampling
    } // for m_jpeg_ncomp
  } // for m_numxMCU

  return JPEG_OK;
} // CJPEGDecoder::ReconstructMCURowBL()


JERRCODE CJPEGDecoder::ReconstructMCURowLS(
  Ipp16s* pMCUBuf,
  int     nMCURow,
  int     idThread)
{
  Ipp16s*   pCurrRow;
  Ipp16s*   pPrevRow;
  Ipp8u*    pDst8u  = 0;
  Ipp16s*   pDst16s = 0;
  IppiSize  roi;
  IppStatus status;

  idThread = idThread; // remove warning

  pCurrRow = m_ccomp[0].m_curr_row;
  pPrevRow = m_ccomp[0].m_prev_row;

  roi.width  = m_dst.width;
  roi.height = 1;

  if(m_dst.precision <= 8)
    pDst8u  =                   m_dst.p.Data8u  + nMCURow*m_dst.lineStep;
  else
    pDst16s = (Ipp16s*)((Ipp8u*)m_dst.p.Data16s + nMCURow*m_dst.lineStep);

  if(0 != nMCURow)
  {
    status = ippiReconstructPredRow_JPEG_16s_C1(
               pMCUBuf,pPrevRow,pCurrRow,m_dst.width,m_ss);
  }
  else
  {
    status = ippiReconstructPredFirstRow_JPEG_16s_C1(
               pMCUBuf,pCurrRow,m_dst.width,m_jpeg_precision,m_al);
  }

  if(ippStsNoErr != status)
  {
    return JPEG_INTERNAL_ERROR;
  }

  status = ippsLShiftC_16s(pCurrRow,m_al,pPrevRow,m_dst.width);
  if(ippStsNoErr != status)
  {
    return JPEG_INTERNAL_ERROR;
  }

  if(m_dst.precision <= 8)
    status = ippiConvert_16s8u_C1R(pPrevRow,m_dst.width*sizeof(Ipp16s),pDst8u,m_dst.lineStep,roi);
  else
    status = ippsCopy_16s(pPrevRow,pDst16s,m_dst.width);

  if(ippStsNoErr != status)
  {
    return JPEG_INTERNAL_ERROR;
  }

  m_ccomp[0].m_curr_row = pPrevRow;
  m_ccomp[0].m_prev_row = pCurrRow;

  return JPEG_OK;
} // CJPEGDecoder::ReconstructMCURowLS()


JERRCODE CJPEGDecoder::DecodeScanBaseline(void)
{
  int scount = 0;
  IppStatus status;
#ifdef __TIMING__
  Ipp64u   c0;
  Ipp64u   c1;
#endif

  status = ippiDecodeHuffmanStateInit_JPEG_8u(m_state);
  if(ippStsNoErr != status)
  {
    return JPEG_INTERNAL_ERROR;
  }

  m_marker = JM_NONE;

  if(m_dctbl[0].IsEmpty())
  {
    m_dctbl[0].Create();
    m_dctbl[0].Init(0,0,(Ipp8u*)&DefaultLuminanceDCBits[0],(Ipp8u*)&DefaultLuminanceDCValues[0]);
  }

  if(m_dctbl[1].IsEmpty())
  {
    m_dctbl[1].Create();
    m_dctbl[1].Init(1,0,(Ipp8u*)&DefaultChrominanceDCBits[0],(Ipp8u*)&DefaultChrominanceDCValues[0]);
  }

  if(m_actbl[0].IsEmpty())
  {
    m_actbl[0].Create();
    m_actbl[0].Init(0,1,(Ipp8u*)&DefaultLuminanceACBits[0],(Ipp8u*)&DefaultLuminanceACValues[0]);
  }

  if(m_actbl[1].IsEmpty())
  {
    m_actbl[1].Create();
    m_actbl[1].Init(1,1,(Ipp8u*)&DefaultChrominanceACBits[0],(Ipp8u*)&DefaultChrominanceACValues[0]);
  }

#ifdef _OPENMP
#pragma omp parallel default(shared)
#endif
  {
    int     i;
    int     idThread = 0;
    Ipp16s* pMCUBuf;  // the pointer to Buffer for a current thread.

#ifdef _OPENMP
    idThread = omp_get_thread_num(); // the thread id of the calling thread.
#endif

    pMCUBuf = m_block_buffer + idThread * m_numxMCU * m_nblock * DCTSIZE2;

    i = 0;

    while(i < m_numyMCU)
    {
#ifdef _OPENMP
#pragma omp critical (IPP_JPEG_OMP)
#endif
      {
        i = scount;
        scount++;
        if(i < m_numyMCU)
        {
#ifdef __TIMING__
          c0 = ippGetCpuClocks();
#endif
          ippsZero_16s(pMCUBuf,m_numxMCU * m_nblock * DCTSIZE2);
          DecodeHuffmanMCURowBL(pMCUBuf);
#ifdef __TIMING__
          c1 = ippGetCpuClocks();
          m_clk_huff += (c1 - c0);
#endif
        }
      }

      if(i < m_numyMCU)
      {
#ifdef __TIMING__
        c0 = ippGetCpuClocks();
#endif
        ReconstructMCURowBL(pMCUBuf, idThread);
#ifdef __TIMING__
        c1 = ippGetCpuClocks();
        m_clk_dct += c1 - c0;
#endif

#ifdef __TIMING__
        c0 = ippGetCpuClocks();
#endif
        UpSampling(i,idThread);
#ifdef __TIMING__
        c1 = ippGetCpuClocks();
        m_clk_ss += c1 - c0;
#endif

#ifdef __TIMING__
        c0 = ippGetCpuClocks();
#endif
        ColorConvert(i,idThread);
#ifdef __TIMING__
        c1 = ippGetCpuClocks();
        m_clk_cc += c1 - c0;
#endif
      }

      i++;
    } // for m_numyMCU
  } // OMP

  return JPEG_OK;
} // CJPEGDecoder::DecodeScanBaseline()


JERRCODE CJPEGDecoder::DecodeScanMultiple(void)
{
  int       i, j, k, l, c;
  int       srcLen;
  Ipp8u*    src;
  Ipp16s*   block;
  JERRCODE  jerr;
  IppStatus status;
#ifdef __TIMING__
  Ipp64u    c0;
  Ipp64u    c1;
#endif

  m_scan_count++;
  m_ac_scans_completed += m_scan_ncomps;

  status = ippiDecodeHuffmanStateInit_JPEG_8u(m_state);
  if(ippStsNoErr != status)
  {
    return JPEG_INTERNAL_ERROR;
  }

  m_marker = JM_NONE;

  src    = m_src.pData;
  srcLen = m_src.DataLen;

  if(m_dctbl[0].IsEmpty())
  {
    m_dctbl[0].Create();
    m_dctbl[0].Init(0,0,(Ipp8u*)&DefaultLuminanceDCBits[0],(Ipp8u*)&DefaultLuminanceDCValues[0]);
  }

  if(m_dctbl[1].IsEmpty())
  {
    m_dctbl[1].Create();
    m_dctbl[1].Init(1,0,(Ipp8u*)&DefaultChrominanceDCBits[0],(Ipp8u*)&DefaultChrominanceDCValues[0]);
  }

  if(m_actbl[0].IsEmpty())
  {
    m_actbl[0].Create();
    m_actbl[0].Init(0,1,(Ipp8u*)&DefaultLuminanceACBits[0],(Ipp8u*)&DefaultLuminanceACValues[0]);
  }

  if(m_actbl[1].IsEmpty())
  {
    m_actbl[1].Create();
    m_actbl[1].Init(1,1,(Ipp8u*)&DefaultChrominanceACBits[0],(Ipp8u*)&DefaultChrominanceACValues[0]);
  }

  for(i = 0; i < m_numyMCU; i++)
  {
    for(k = 0; k < m_ccomp[m_curr_comp_no].m_vsampling; k++)
    {
      if(i*m_ccomp[m_curr_comp_no].m_vsampling*8 + k*8 >= m_jpeg_height)
        break;

      for(j = 0; j < m_numxMCU; j++)
      {
        for(c = 0; c < m_scan_ncomps; c++)
        {
          block = m_block_buffer + (DCTSIZE2*m_nblock*(j+(i*m_numxMCU)));

          // skip any relevant components
          for(c = 0; c < m_ccomp[m_curr_comp_no].m_comp_no; c++)
          {
            block += (DCTSIZE2*m_ccomp[c].m_nblocks);
          }

          // Skip over relevant 8x8 blocks from this component.
          block += (k * DCTSIZE2 * m_ccomp[m_curr_comp_no].m_hsampling);

          for(l = 0; l < m_ccomp[m_curr_comp_no].m_hsampling; l++)
          {
            // Ignore the last column(s) of the image.
            if(((j*m_ccomp[m_curr_comp_no].m_hsampling*8) + (l*8)) >= m_jpeg_width)
              break;

            if(m_jpeg_restart_interval)
            {
              if(m_restarts_to_go == 0)
              {
                jerr = ProcessRestart();
                if(JPEG_OK != jerr)
                {
                  LOG0("Error: ProcessRestart() failed!");
                  return jerr;
                }
              }
            }

            Ipp16s*                lastDC = &m_ccomp[m_curr_comp_no].m_lastDC;
            IppiDecodeHuffmanSpec* dctbl = m_dctbl[m_ccomp[m_curr_comp_no].m_dc_selector];
            IppiDecodeHuffmanSpec* actbl = m_actbl[m_ccomp[m_curr_comp_no].m_ac_selector];

#ifdef __TIMING__
            c0 = ippGetCpuClocks();
#endif
            status = ippiDecodeHuffman8x8_JPEG_1u16s_C1(
                       src,
                       srcLen,
                       &m_src.currPos,
                       block,
                       lastDC,
                       (int*)&m_marker,
                       dctbl,
                       actbl,
                       m_state);
#ifdef __TIMING__
            c1 = ippGetCpuClocks();
            m_clk_huff += (c1 - c0);
#endif

            if(ippStsNoErr > status)
            {
              LOG0("Error: ippiDecodeHuffman8x8_JPEG_1u16s_C1() failed!");
              return JPEG_INTERNAL_ERROR;
            }

            block += DCTSIZE2;

            m_restarts_to_go --;
          } // for m_hsampling
        } // for m_scan_ncomps
      } // for m_numxMCU
    } // for m_vsampling
  } // for m_numyMCU

  return JPEG_OK;
} // CJPEGDecoder::DecodeScanMultiple()


JERRCODE CJPEGDecoder::DecodeScanProgressive(void)
{
  int       i, j, k, n, l, c;
  int       srcLen;
  Ipp8u*    src;
  Ipp16s*   block;
  JERRCODE  jerr;
  IppStatus status;

  m_scan_count++;

  status = ippiDecodeHuffmanStateInit_JPEG_8u(m_state);
  if(ippStsNoErr != status)
  {
    return JPEG_INTERNAL_ERROR;
  }

  m_marker = JM_NONE;

  src    = m_src.pData;
  srcLen = m_src.DataLen;

  if(m_ss != 0 && m_se != 0)
  {
    // AC scan
    for(i = 0; i < m_numyMCU; i++)
    {
      for(k = 0; k < m_ccomp[m_curr_comp_no].m_vsampling; k++)
      {
        if(i*m_ccomp[m_curr_comp_no].m_vsampling*8 + k*8 >= m_jpeg_height)
          break;

        for(j = 0; j < m_numxMCU; j++)
        {
          block = m_block_buffer + (DCTSIZE2*m_nblock*(j+(i*m_numxMCU)));

          // skip any relevant components
          for(c = 0; c < m_ccomp[m_curr_comp_no].m_comp_no; c++)
          {
            block += (DCTSIZE2*m_ccomp[c].m_nblocks);
          }

          // Skip over relevant 8x8 blocks from this component.
          block += (k * DCTSIZE2 * m_ccomp[m_curr_comp_no].m_hsampling);

          for(l = 0; l < m_ccomp[m_curr_comp_no].m_hsampling; l++)
          {
            // Ignore the last column(s) of the image.
            if(((j*m_ccomp[m_curr_comp_no].m_hsampling*8) + (l*8)) >= m_jpeg_width)
              break;

            if(m_jpeg_restart_interval)
            {
              if(m_restarts_to_go == 0)
              {
                jerr = ProcessRestart();
                if(JPEG_OK != jerr)
                {
                  LOG0("Error: ProcessRestart() failed!");
                  return jerr;
                }
              }
            }

            IppiDecodeHuffmanSpec* actbl = m_actbl[m_ccomp[m_curr_comp_no].m_ac_selector];

            if(m_ah == 0)
            {
              status = ippiDecodeHuffman8x8_ACFirst_JPEG_1u16s_C1(
                         src,
                         srcLen,
                         &m_src.currPos,
                         block,
                         (int*)&m_marker,
                         m_ss,
                         m_se,
                         m_al,
                         actbl,
                         m_state);

              if(ippStsNoErr > status)
              {
                LOG0("Error: ippiDecodeHuffman8x8_ACFirst_JPEG_1u16s_C1() failed!");
                return JPEG_INTERNAL_ERROR;
              }
            }
            else
            {
              status = ippiDecodeHuffman8x8_ACRefine_JPEG_1u16s_C1(
                         src,
                         srcLen,
                         &m_src.currPos,
                         block,
                         (int*)&m_marker,
                         m_ss,
                         m_se,
                         m_al,
                         actbl,
                         m_state);

              if(ippStsNoErr > status)
              {
                LOG0("Error: ippiDecodeHuffman8x8_ACRefine_JPEG_1u16s_C1() failed!");
                return JPEG_INTERNAL_ERROR;
              }
            }

            block += DCTSIZE2;

            m_restarts_to_go --;
          } // for m_hsampling
        } // for m_numxMCU
      } // for m_vsampling
    } // for m_numyMCU

    if(m_al == 0)
    {
      m_ccomp[m_curr_comp_no].m_ac_scan_completed = 1;
    }
  }
  else
  {
    // DC scan
    for(i = 0; i < m_numyMCU; i++)
    {
      for(j = 0; j < m_numxMCU; j++)
      {
        if(m_jpeg_restart_interval)
        {
          if(m_restarts_to_go == 0)
          {
            jerr = ProcessRestart();
            if(JPEG_OK != jerr)
            {
              LOG0("Error: ProcessRestart() failed!");
              return jerr;
            }
          }
        }

        block = m_block_buffer + (DCTSIZE2*m_nblock*(j+(i*m_numxMCU)));

        if(m_ah == 0)
        {
          // first DC scan
          for(n = 0; n < m_jpeg_ncomp; n++)
          {
            Ipp16s*                lastDC = &m_ccomp[n].m_lastDC;
            IppiDecodeHuffmanSpec* dctbl  = m_dctbl[m_ccomp[n].m_dc_selector];

            for(k = 0; k < m_ccomp[n].m_vsampling; k++)
            {
              for(l = 0; l < m_ccomp[n].m_hsampling; l++)
              {
                status = ippiDecodeHuffman8x8_DCFirst_JPEG_1u16s_C1(
                           src,
                           srcLen,
                           &m_src.currPos,
                           block,
                           lastDC,
                           (int*)&m_marker,
                           m_al,
                           dctbl,
                           m_state);

                if(ippStsNoErr > status)
                {
                  LOG0("Error: ippiDecodeHuffman8x8_DCFirst_JPEG_1u16s_C1() failed!");
                  return JPEG_INTERNAL_ERROR;
                }

                block += DCTSIZE2;
              } // for m_hsampling
            } // for m_vsampling
          } // for m_jpeg_ncomp
        }
        else
        {
          // refine DC scan
          for(n = 0; n < m_jpeg_ncomp; n++)
          {
            for(k = 0; k < m_ccomp[n].m_vsampling; k++)
            {
              for(l = 0; l < m_ccomp[n].m_hsampling; l++)
              {
                status = ippiDecodeHuffman8x8_DCRefine_JPEG_1u16s_C1(
                           src,
                           srcLen,
                           &m_src.currPos,
                           block,
                           (int*)&m_marker,
                           m_al,
                           m_state);

                if(ippStsNoErr > status)
                {
                  LOG0("Error: ippiDecodeHuffman8x8_DCRefine_JPEG_1u16s_C1() failed!");
                  return JPEG_INTERNAL_ERROR;
                }

                block += DCTSIZE2;
              } // for m_hsampling
            } // for m_vsampling
          } // for m_jpeg_ncomp
        }
        m_restarts_to_go --;
      } // for m_numxMCU
    } // for m_numyMCU

    if(m_al == 0)
    {
      m_dc_scan_completed = 1;
    }
  }

  return JPEG_OK;
} // CJPEGDecoder::DecodeScanProgressive()


JERRCODE CJPEGDecoder::DecodeScanLossless(void)
{
  int       i;
  Ipp16s*   pMCUBuf;
  JERRCODE  jerr;
  IppStatus status;
#ifdef __TIMING__
  Ipp64u    c0;
  Ipp64u    c1;
#endif

  status = ippiDecodeHuffmanStateInit_JPEG_8u(m_state);
  if(ippStsNoErr != status)
  {
    return JPEG_INTERNAL_ERROR;
  }

  m_marker = JM_NONE;

  pMCUBuf = m_block_buffer;

  for(i = 0; i < m_numyMCU; i++)
  {
#ifdef __TIMING__
    c0 = ippGetCpuClocks();
#endif
    jerr = DecodeHuffmanMCURowLS(pMCUBuf);
    if(JPEG_OK != jerr)
    {
      return jerr;
    }
#ifdef __TIMING__
    c1 = ippGetCpuClocks();
    m_clk_huff += c1 - c0;
#endif

#ifdef __TIMING__
    c0 = ippGetCpuClocks();
#endif
    jerr = ReconstructMCURowLS(pMCUBuf, i, 0);
    if(JPEG_OK != jerr)
    {
      return jerr;
    }
#ifdef __TIMING__
    c1 = ippGetCpuClocks();
    m_clk_diff += c1 - c0;
#endif
  } // for m_numyMCU

  return JPEG_OK;
} // CJPEGDecoder::DecodeScanLossless()


JERRCODE CJPEGDecoder::ReadHeader(
  int*    width,
  int*    height,
  int*    nchannels,
  JCOLOR* color,
  JSS*    sampling,
  int*    precision)
{
  JERRCODE jerr;

  jerr = ParseJPEGBitStream(JO_READ_HEADER);

  if(JPEG_OK != jerr)
  {
    LOG0("Error: ParseJPEGBitStream() failed");
    return jerr;
  }

  int du_width  = (JPEG_LOSSLESS == m_jpeg_mode) ? 1 : 8;
  int du_height = (JPEG_LOSSLESS == m_jpeg_mode) ? 1 : 8;

  m_mcuWidth  = du_width  * m_max_hsampling;
  m_mcuHeight = du_height * m_max_vsampling;

  m_numxMCU = (m_jpeg_width  + (m_mcuWidth  - 1)) / m_mcuWidth;
  m_numyMCU = (m_jpeg_height + (m_mcuHeight - 1)) / m_mcuHeight;

  m_xPadding = m_numxMCU * m_mcuWidth  - m_jpeg_width;
  m_yPadding = m_numyMCU * m_mcuHeight - m_jpeg_height;

  m_ccWidth  = m_mcuWidth * m_numxMCU;
  m_ccHeight = m_mcuHeight;

  *width     = m_jpeg_width;
  *height    = m_jpeg_height;
  *nchannels = m_jpeg_ncomp;
  *precision = m_jpeg_precision;
  *color     = m_jpeg_color;
  *sampling  = m_jpeg_sampling;

  return JPEG_OK;
} // CJPEGDecoder::ReadHeader()


JERRCODE CJPEGDecoder::ReadData(void)
{
  return ParseJPEGBitStream(JO_READ_DATA);
} // CJPEGDecoder::ReadData()

