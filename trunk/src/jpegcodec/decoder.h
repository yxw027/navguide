/*
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//    Copyright (c) 2001-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __DECODER_H__
#define __DECODER_H__

#ifdef _OPENMP
#include "omp.h"
#endif
#ifndef __IPPDEFS_H__
#include "ippdefs.h"
#endif
#ifndef __IPPCORE_H__
#include "ippcore.h"
#endif
#ifndef __IPPS_H__
#include "ipps.h"
#endif
#ifndef __IPPI_H__
#include "ippi.h"
#endif
#ifndef __IPPCC_H__
#include "ippcc.h"
#endif
#ifndef __IPPJ_H__
#include "ippj.h"
#endif
#ifndef __JPEGBASE_H__
#include "jpegbase.h"
#endif
#ifndef __DECQTBL_H__
#include "decqtbl.h"
#endif
#ifndef __DECHTBL_H__
#include "dechtbl.h"
#endif
#ifndef __COLORCOMP_H__
#include "colorcomp.h"
#endif




class CJPEGDecoder
{
public:

  CJPEGDecoder(void);
  virtual ~CJPEGDecoder(void);

  void Reset(void);

  JERRCODE SetSource(
    Ipp8u*   pSrc,
    int      srcSize);

  JERRCODE SetDestination(
    Ipp8u*   pDst,
    int      dstStep,
    IppiSize dstSize,
    int      dstChannels,
    JCOLOR   dstColor,
    JSS      dstSampling = JS_444,
    int      dstPrecision = 8);

  JERRCODE SetDestination(
    Ipp16s*  pDst,
    int      dstStep,
    IppiSize dstSize,
    int      dstChannels,
    JCOLOR   dstColor,
    JSS      dstSampling = JS_444,
    int      dstPrecision = 16);

  JERRCODE ReadHeader(
    int*     width,
    int*     height,
    int*     nchannels,
    JCOLOR*  color,
    JSS*     sampling,
    int*     precision);

  JERRCODE ReadData(void);

  BITSTREAM m_src;
  IMAGE     m_dst;
  int       m_jpeg_width;
  int       m_jpeg_height;
  int       m_jpeg_ncomp;
  int       m_jpeg_precision;
  JSS       m_jpeg_sampling;
  JCOLOR    m_jpeg_color;
  int       m_jpeg_quality;
  int       m_jpeg_restart_interval;
  JMODE     m_jpeg_mode;

  // JPEG embedded comments variables
  int      m_jpeg_comment_detected;
  int      m_jpeg_comment_size;
  Ipp8u*   m_jpeg_comment;

  // JFIF APP0 related varibales
  int      m_jfif_app0_detected;
  int      m_jfif_app0_major;
  int      m_jfif_app0_minor;
  int      m_jfif_app0_units;
  int      m_jfif_app0_xDensity;
  int      m_jfif_app0_yDensity;
  int      m_jfif_app0_thumb_width;
  int      m_jfif_app0_thumb_height;

  // JFXX APP0 related variables
  int      m_jfxx_app0_detected;
  int      m_jfxx_thumbnails_type;

  // AVI1 APP0 related variables
  int      m_avi1_app0_detected;
  int      m_avi1_app0_polarity;
  int      m_avi1_app0_reserved;
  int      m_avi1_app0_field_size;
  int      m_avi1_app0_field_size2;

  // Exif APP1 related variables
  int      m_exif_app1_detected;
  int      m_exif_app1_data_size;
  Ipp8u*   m_exif_app1_data;

  // Adobe APP14 related variables
  int      m_adobe_app14_detected;
  int      m_adobe_app14_version;
  int      m_adobe_app14_flags0;
  int      m_adobe_app14_flags1;
  int      m_adobe_app14_transform;

  int      m_precision;
  int      m_max_hsampling;
  int      m_max_vsampling;
  int      m_numxMCU;
  int      m_numyMCU;
  int      m_mcuWidth;
  int      m_mcuHeight;
  int      m_ccWidth;
  int      m_ccHeight;
  int      m_xPadding;
  int      m_yPadding;
  int      m_restarts_to_go;
  int      m_next_restart_num;
  int      m_sos_len;
  int      m_curr_comp_no;
  int      m_scan_ncomps;
  int      m_ss;
  int      m_se;
  int      m_al;
  int      m_ah;
  int      m_dc_scan_completed;
  int      m_ac_scans_completed;
  JMARKER  m_marker;

  int      m_scan_count;

  Ipp16s*  m_block_buffer;
  int      m_num_threads;
  int      m_nblock;

#ifdef __TIMING__
  Ipp64u   m_clk_dct;
  Ipp64u   m_clk_ss;
  Ipp64u   m_clk_cc;
  Ipp64u   m_clk_diff;
  Ipp64u   m_clk_huff;
#endif

  CJPEGColorComponent         m_ccomp[MAX_COMPS_PER_SCAN];
  CJPEGDecoderQuantTable      m_qntbl[MAX_QUANT_TABLES];
  CJPEGDecoderHuffmanTable    m_dctbl[MAX_HUFF_TABLES];
  CJPEGDecoderHuffmanTable    m_actbl[MAX_HUFF_TABLES];
  CJPEGDecoderHuffmanState    m_state;

  JERRCODE Init(void);
  JERRCODE Clean(void);
  JERRCODE ColorConvert(int nMCURow,int thread_id);
  JERRCODE UpSampling(int nMCURow,int thread_id);

  JERRCODE ParseJPEGBitStream(JOPERATION op);
  JERRCODE ParseSOI(void);
  JERRCODE ParseEOI(void);
  JERRCODE ParseAPP0(void);
  JERRCODE ParseAPP1(void);
  JERRCODE ParseAPP14(void);
  JERRCODE ParseSOF0(void);
  JERRCODE ParseSOF2(void);
  JERRCODE ParseSOF3(void);
  JERRCODE ParseDRI(void);
  JERRCODE ParseRST(void);
  JERRCODE ParseSOS(void);
  JERRCODE ParseDQT(void);
  JERRCODE ParseDHT(void);
  JERRCODE ParseCOM(void);
  JERRCODE DecodeScanBaseline(void);
  JERRCODE DecodeScanMultiple(void);
  JERRCODE DecodeScanLossless(void);
  JERRCODE DecodeScanProgressive(void);

  JERRCODE ProcessRestart(void);

  JERRCODE NextMarker(JMARKER* marker);
  JERRCODE SkipMarker(void);

  // huffman decode mcu row lossless process
  JERRCODE DecodeHuffmanMCURowLS(Ipp16s* pMCUBuf);

  // huffman decode mcu row baseline process
  JERRCODE DecodeHuffmanMCURowBL(Ipp16s* pMCUBuf);

  // inverse DCT, de-quantization, level-shift for mcu row
  JERRCODE ReconstructMCURowBL(Ipp16s* pMCUBuf, int thread_id);

  // reconstruct mcu row lossless process
  JERRCODE ReconstructMCURowLS(Ipp16s* pMCUBuf, int nMCURow,int thread_id);

  JERRCODE DetectSampling(void);
};

#endif // __DECODER_H__

