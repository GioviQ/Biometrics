//******************************************************************************
//*File Name: ImAccess.cpp
//*Summary of this file: Image Reader & Writer
//*
//* (c)Copyright DevelopGate Corporation,2005  All Rights Reserved.
//* ----------------------------------------------------------------------------
//* History
//* Rev		When			Work		Owner
//*	FV3.0	2007-04-20		New			Kim Jin Su	
//******************************************************************************
//:=*===================*===============*=======================================

#include "StdAfx.h"

#include "BmpFile.h"
#include "ImageCommon.h"
// #include "FpCommon.h"

int	FCGetBmpInfo(const char* p_pszFileName, BITMAP* p_pBmpInfo)
{	
	HBITMAP hBitmap1 = NULL;	
	
	hBitmap1 = (HBITMAP)LoadImageA(0, p_pszFileName, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);  
	
	if (hBitmap1 == NULL)
	{
		return 1;
	}

	GetObject(hBitmap1,sizeof(BITMAP), p_pBmpInfo);
	
	DeleteObject(hBitmap1);

	return 0;
}

int FCLoadImage(const char*	p_pszFileName, unsigned char **p_ppImage, int *p_pnWidth, int *p_pnHeight, int p_nRequest)
{
	int				w_ret = 0; // Fp_TRUE;	
	FCImageType		nImType;
	int				nWidth, nHeight;
	unsigned char	*RGBBuf = 0, *GrayBuf = 0;

	BMPFile			w_stBMP;

	//. Check input parameters
	if ((!p_pszFileName)){
		w_ret = 1; // Fp_Err_PARAM;
		goto L_EXIT;
	}

	w_ret = CheckImageType(p_pszFileName, &nImType);
	if (w_ret != 0){ // Fp_TRUE){
		goto	L_EXIT;
	}

	switch(nImType)
	{
	case FC_UNKNOWN:
		w_ret = 1; // Fp_Err_UnKnown;
		goto	L_EXIT;
		break;
		
	case FC_BMP:
		w_ret = w_stBMP.LoadBMP(p_pszFileName, &nWidth, &nHeight, &RGBBuf);
		if (w_ret != 0){ // Fp_TRUE){
			goto	L_EXIT;
		}		
	}

	if (p_nRequest == 0){

		GrayBuf = new unsigned char[nHeight * nWidth]; // (unsigned char*)alloc_mem(nHeight * nWidth);
		w_ret = RGBToGray(RGBBuf, nHeight, nWidth, GrayBuf);
		delete []RGBBuf; // free_mem(RGBBuf);
		if (w_ret != 0){ // Fp_TRUE){
			goto	L_EXIT;
		}

		*p_pnHeight = nHeight; // p_pImage->height = nHeight;
		*p_pnWidth = nWidth; // p_pImage->width = nWidth;
		*p_ppImage = GrayBuf;

	}
	else{

		*p_pnHeight = nHeight; // p_pImage->height = nHeight;
		*p_pnWidth = nWidth; // p_pImage->width = nWidth;
		*p_ppImage = RGBBuf; // p_pImage->data = RGBBuf;

	}

L_EXIT:
	return w_ret;
}//..FCLoadImage

int FCSaveImage(const char*	p_pszFileName, unsigned char* p_pImage, int p_nWidth, int p_nHeight, int p_nFlag)
{
	int				w_ret = TRUE; // Fp_TRUE;
	FCImageType		w_nType;
	unsigned char	*RGBBuf = 0, *GrayBuf = 0;
	
	BMPFile			w_stBMP;
	
	if ((p_pszFileName == NULL) || (p_pImage == NULL) || p_nWidth < 0 || p_nHeight < 0 || p_nWidth > 1024 || p_nHeight > 1024){
		w_ret = FALSE; // Fp_Err_PARAM;
		goto	L_EXIT;
	}

	w_ret = CheckImageType(p_pszFileName, &w_nType);
	if (w_ret != 0){//   Fp_TRUE){
		goto	L_EXIT;
	}

	switch(w_nType)
	{	
	case FC_BMP:	// BMP
		w_stBMP.SaveBMP(p_pszFileName, p_nWidth, p_nHeight, p_pImage, p_nFlag);
		break;	
	default:
		w_ret = FC_UNKNOWN;
		goto	L_EXIT;
	}	

L_EXIT:
	return	w_ret;
}//..FCSaveImage
