#ifndef __IMAGE_COMMON_H__
#define __IMAGE_COMMON_H__	

// #include "MemMgr.h"
//. Possible Image Type in FCReader
enum FCImageType
{
	FC_UNKNOWN,	//. Impossible
	FC_BMP,		//.	Bitmap
	FC_JPEG		//. Jpeg
};

int
RGBToGray(
	  unsigned char*	RGBData
,	  int				iHeight
,	  int				iWidth
,	  unsigned char*	GrayData
);

int
CheckImageType(
	const char*		p_pszFileName
,	FCImageType*	p_pnType
);

bool
BGRFromRGB(
	unsigned char*		buf, 
	int					width, 
	int					height
);

bool
VertFlipBuf(
	unsigned char*		inbuf, 
	int					widthBytes, 
	int					height
);

#endif//.. #ifndef __IMAGE_COMMON_H__