//******************************************************************************
//*File Name: ImReader.h
//*
//* (c)Copyright DevelopGate Corporation,2005  All Rights Reserved.
//* ----------------------------------------------------------------------------
//* History
//* Rev				When			Work		Owner
//*
//******************************************************************************
//:=*===================*===============*=======================================

#ifndef __IM_READER_H__
#define __IM_READER_H__

#include "BMPFile.h"

int	FCGetBmpInfo(const char* p_pszFileName, BITMAP* p_pBmpInfo);

int FCLoadImage(const char*	p_pszFileName, unsigned char **p_ppImage, int *p_pnWidth, int *p_pnHeight, int p_nRequest);

int FCSaveImage(const char*	p_pszFileName, unsigned char* p_pImage, int p_nWidth, int p_nHeight, int p_nFlag);

#endif//..#ifndef __IM_READER_H__
