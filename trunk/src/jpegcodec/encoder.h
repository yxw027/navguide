/*
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//    Copyright (c) 2001-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __ENCODER_H__
#define __ENCODER_H__

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
#ifndef __IPPJ_H__
#include "ippj.h"
#endif
#ifndef __JPEGBASE_H__
#include "jpegbase.h"
#endif
#ifndef __ENCQTBL_H__
#include "encqtbl.h"
#endif
#ifndef __ENCHTBL_H__
#include "enchtbl.h"
#endif
#ifndef __COLORCOMP_H__
#include "colorcomp.h"
#endif




typedef struct _JPEG_SCAN
{
  int ncomp;
  int id[MAX_COMPS_PER_SCAN];
  int Ss;
  int Se;
  int Ah;
  int Al;
} JPEG_SCAN;


class CJPEGEncoder
{
public:

  CJPEGEncoder(void);
  virtual ~CJPEGEncoder(void);

  JERRCODE SetSource(
    Ipp8u*   pSrc,
    int      srcStep,
    IppiSize srcSize,
    int      srcChannels,
    JCOLOR   srcColor,
    JSS      dstSampling = JS_444,
    int      precision = 8);

  JERRCODE SetSource(
    Ipp16s*  pSrc,
    int      srcStep,
    IppiSize srcSize,
    int      srcChannels,
    JCOLOR   srcColor,
    JSS      dstSampling = JS_444,
    int      precision = 16);

  JERRCODE SetDestination(
    Ipp8u*   pDst,
    int      dstSize,
    int      dstQuality,
    JCOLOR   dstColor,
    JSS      dstSampling,
    JMODE    dstMode = JPEG_BASELINE,
    int      dstRestartInt = 0);

  JERRCODE WriteHeader(void);

  JERRCODE WriteData(void);

  IMAGE      m_src;
  BITSTREAM  m_dst;

  int        m_jpeg_ncomp;
  int        m_jpeg_precision;
  JSS        m_jpeg_sampling;
  JCOLOR     m_jpeg_color;
  int        m_jpeg_quality;
  int        m_jpeg_restart_interval;
  JMODE      m_jpeg_mode;

  int        m_numxMCU;
  int        m_numyMCU;
  int        m_mcuWidth;
  int        m_mcuHeight;
  int        m_ccWidth;
  int        m_ccHeight;
  int        m_xPadding;
  int        m_yPadding;
  int        m_restarts_to_go;
  int        m_next_restart_num;
  int        m_scan_count;
  int        m_ss;
  int        m_se;
  int        m_al;
  int        m_ah;
  int        m_predictor;
  int        m_pt;
  JPEG_SCAN* m_scan_script;

  Ipp16s*    m_block_buffer;
  int        m_num_threads;
  int        m_nblock;

#ifdef __TIMING__
  Ipp64u   m_clk_dct;
  Ipp64u   m_clk_ss;
  Ipp64u   m_clk_cc;
  Ipp64u   m_clk_diff;
  Ipp64u   m_clk_huff;
#endif

  CJPEGColorComponent        m_ccomp[MAX_COMPS_PER_SCAN];
  CJPEGEncoderQuantTable     m_qntbl[MAX_QUANT_TABLES];
  CJPEGEncoderHuffmanTable   m_dctbl[MAX_HUFF_TABLES];
  CJPEGEncoderHuffmanTable   m_actbl[MAX_HUFF_TABLES];
  CJPEGEncoderHuffmanState   m_state;

  JERRCODE Init(void);
  JERRCODE Clean(void);
  JERRCODE ColorConvert(int nMCURow, int thread_id = 0);
  JERRCODE DownSampling(int nMCURow, int thread_id = 0);
  JERRCODE TransformMCURowBL(Ipp16s* pMCUBuf, int thread_id = 0);
  JERRCODE EncodeScan(int ncomp,int id[MAX_COMPS_PER_SCAN],int Ss,int Se,int Ah,int Al);
  JERRCODE EncodeScan(void);
  JERRCODE SelectScanScripts(void);
  JERRCODE GenerateHuffmanTables(int ncomp,int id[MAX_COMPS_PER_SCAN],int Ss,int Se,int Ah,int Al);
  JERRCODE GenerateHuffmanTables(void);

  JERRCODE WriteSOI(void);
  JERRCODE WriteEOI(void);
  JERRCODE WriteAPP0(void);
  JERRCODE WriteAPP14(void);
  JERRCODE WriteSOF0(void);
  JERRCODE WriteSOF1(void);
  JERRCODE WriteSOF2(void);
  JERRCODE WriteSOF3(void);
  JERRCODE WriteDRI(int restart_interval);
  JERRCODE WriteRST(int next_restart_num);
  JERRCODE WriteSOS(void);
  JERRCODE WriteSOS(int ncomp,int id[MAX_COMPS_PER_SCAN],int Ss,int Se,int Ah,int Al);
  JERRCODE WriteDQT(CJPEGEncoderQuantTable* tbl);
  JERRCODE WriteDHT(CJPEGEncoderHuffmanTable* tbl);
  JERRCODE WriteCOM(char* comment = 0);
  JERRCODE EncodeScanBaseline(void);
  JERRCODE EncodeScanProgressive(void);
  JERRCODE EncodeScanLossless(void);

  JERRCODE ProcessRestart(int id[MAX_COMPS_PER_SCAN],int Ss,int Se,int Ah,int Al);
  JERRCODE ProcessRestart(int stat[2][256],int id[MAX_COMPS_PER_SCAN],int Ss,int Se,int Ah,int Al);

  JERRCODE EncodeHuffmanMCURowBL(Ipp16s* pMCUBuf);

};


#endif // __ENCODER_H__

