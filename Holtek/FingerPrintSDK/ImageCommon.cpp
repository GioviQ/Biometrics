#include "StdAfx.h"

#include <string.h>
#include <ctype.h>
#include "ImageCommon.h"
#include "BmpFile.h"
// #include "FpCommon.h"

////////////////////////////////////////////////////////////////////////////////
//.convert to Gray form RGB
//:=*===================*===============*=======================================
int
RGBToGray(
	  unsigned char*	RGBData
,	  int				iHeight
,	  int				iWidth
,	  unsigned char*	GrayData
)
{
	int		w_ret = 0; // Fp_TRUE;
	int		i, nPixel;
	unsigned char *bptr1, *bptr2;

	if ((!RGBData) || (!GrayData) || (iHeight < 1) || (iWidth < 1)){
		w_ret = 1; // Fp_Err_PARAM;
		goto	L_EXIT;
	}

	nPixel = iWidth * iHeight;
	bptr1 = &RGBData[0];
	bptr2 = &GrayData[0];

	for ( i = 0 ; i < nPixel ; i ++){
		
		//.2006-03-20	Fixed by Kim Jin Su. 
		*bptr2 = (unsigned char)(0.29893602129378 * bptr1[2] +
			0.58704307445112 * bptr1[1] + 
			0.11402090425510 * bptr1[0] + 0.5);
		
		bptr1 += 3;
		bptr2 ++;
	}

L_EXIT:
	return  w_ret;
}//..RGBToGray

int
CheckImageType(
	const char*		p_pszFileName
,	FCImageType*	p_pnType
)
{
	int		w_ret = 0; // Fp_TRUE;
	char	ExtName[5];
	int		ch;
	int		i;

	//. Check input parameters
	if ((!p_pszFileName) || (!p_pnType)){
		w_ret = 1; // Fp_Err_PARAM;
		goto	L_EXIT;
	}

	//. Init
	*p_pnType = FC_UNKNOWN;

	//. Compare Extension
	strcpy(ExtName, p_pszFileName + strlen(p_pszFileName) - 4);

	for(i = 1; i < 4; i++){
		ch = ExtName[i];
		if(islower(ch) != 0)
			ExtName[i] = (char)towupper(ch);
	}
	
	if(strcmp(ExtName,".BMP") == 0){
		*p_pnType = FC_BMP;
	}
	else if (strcmp(ExtName,".JPG") == 0){
		*p_pnType = FC_JPEG;
	}	

L_EXIT:
	return	w_ret;
}//.. CheckImageType

bool
BGRFromRGB(
	unsigned char*		buf, 
	int					width, 
	int					height
)
{
	int				i;
	unsigned char*	bptr;
	unsigned char	tmp;

	if (buf == NULL)
		return false;
	
	bptr = &buf[0];
	for( i = 0; i < height * width; i ++ ) {
			
		// swap red and blue
		tmp = bptr[0];
		bptr[0] = bptr[2];
		bptr[2] = tmp;
		
		bptr += 3;
	}

	return true;
}

//
//	vertically flip a buffer 
//	note, this operates on a buffer of widthBytes bytes, not pixels!!!
//

bool
VertFlipBuf(
	unsigned char*		inbuf, 
	int					widthBytes, 
	int					height
)
{   
	unsigned char  *tb1;
	unsigned char  *tb2;
	int				bufsize;
	int				row_cnt;     
	unsigned long	off1=0;
	unsigned long	off2=0;

	if (inbuf == NULL)
		return false;

	bufsize = widthBytes;

	tb1= new unsigned char[bufsize] ;// (unsigned char *)alloc_mem(bufsize);
	if (tb1 == NULL) {
		return false;
	}

	tb2= new unsigned char[bufsize]; // (unsigned char *)alloc_mem(bufsize);
	if (tb2 == NULL) {
		delete []tb1; //  free_mem(tb1);
		return false;
	}
	
	for (row_cnt=0;row_cnt<(height+1)/2;row_cnt++) {
		off1=row_cnt*bufsize;
		off2=((height-1)-row_cnt)*bufsize;   
		
		memcpy(tb1,inbuf+off1,bufsize);
		memcpy(tb2,inbuf+off2,bufsize);	
		memcpy(inbuf+off1,tb2,bufsize);
		memcpy(inbuf+off2,tb1,bufsize);
	}	

	delete []tb1; //  free_mem(tb1);
	delete []tb2; //  free_mem(tb2);

	return true;
};        
