/*
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//    Copyright (c) 2001-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __DECQTBL_H__
#define __DECQTBL_H__

#ifndef __IPPJ_H__
#include "ippj.h"
#endif
#ifndef __JPEGBASE_H__
#include "jpegbase.h"
#endif




class CJPEGDecoderQuantTable
{
private:
  Ipp8u   m_rbf[DCTSIZE2+(CPU_CACHE_LINE-1)];
  Ipp8u   m_qbf[DCTSIZE2*sizeof(Ipp16u)+(CPU_CACHE_LINE-1)];
  Ipp8u*  m_raw;
  Ipp16u* m_qnt;
  int     m_initialized;

public:
  int     m_id;
  int     m_precision;

  CJPEGDecoderQuantTable(void);
  virtual ~CJPEGDecoderQuantTable(void);

  JERRCODE Init(int id,Ipp8u raw[DCTSIZE2]);

  operator Ipp16u*() { return m_qnt; }
};



#endif // __DECQTBL_H__
