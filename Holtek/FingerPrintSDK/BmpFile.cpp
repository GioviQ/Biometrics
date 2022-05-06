//	bmpops.cpp : implementation of the BMPFile class
//	
//	This handles the reading and writing of BMP files.
//
//

#include "StdAfx.h"

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "BmpFile.h"
#include "ImageCommon.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BMPFile::BMPFile()
{
	strcpy(m_errorText, "OK");
}

////////////////////////////////////////////////////////////////////////////
//	load a .BMP file - 1,4,8,24 bit
//
//	allocates and returns an RGB buffer containing the image.
//	modifies width and height accordingly - NULL, 0, 0 on error

unsigned char*
BMPFile::LoadBMPCore(
	const char*		fileName, 
	int*			width, 
	int*			height
)
{
	BITMAP inBM;
	unsigned char m1,m2;
    long filesize;
    short res1,res2;
    long pixoff;
    long bmisize;                    
    long compression;
    unsigned long sizeimage;
    long xscale, yscale;
    long colors;
    long impcol;
    
	unsigned char* outBuf=NULL;
	
	// init
	*width=0; *height=0;

	// init
	strcpy(m_errorText, "OK");
	m_bytesRead=0;

	FILE *fp;
	
	fp=fopen(fileName,"rb");
	if (fp==NULL) {
		strcpy(m_errorText,"Can't open file for reading :\n");
		strcat(m_errorText, fileName);
		return NULL;
	} else {
	    long rc;
		rc=fread((unsigned char  *)&(m1),1,1,fp); m_bytesRead+=1;
		if (rc==-1) {strcpy(m_errorText,"Read Error!"); fclose(fp); return NULL;}

		rc=fread((unsigned char  *)&(m2),1,1,fp); m_bytesRead+=1;
		if (rc==-1) strcpy(m_errorText, "Read Error!");
		if ((m1!='B') || (m2!='M')) {
			strcpy(m_errorText, "Not a valid BMP File");
			fclose(fp);
			return NULL;
        }
        
		////////////////////////////////////////////////////////////////////////////
		//
		//	read a ton of header stuff

		rc=fread((long  *)&(filesize),4,1,fp); m_bytesRead+=4;
		if (rc!=1) {strcpy(m_errorText, "Read Error!"); fclose(fp); return NULL;}

		rc=fread((int  *)&(res1),2,1,fp); m_bytesRead+=2;
		if (rc!=1) {strcpy(m_errorText,"Read Error!"); fclose(fp); return NULL;}

		rc=fread((int  *)&(res2),2,1,fp); m_bytesRead+=2;
		if (rc!=1) {strcpy(m_errorText,"Read Error!"); fclose(fp); return NULL;}

		rc=fread((long  *)&(pixoff),4,1,fp); m_bytesRead+=4;
		if (rc!=1) {strcpy(m_errorText, "Read Error!"); fclose(fp); return NULL;}

		rc=fread((long  *)&(bmisize),4,1,fp); m_bytesRead+=4;
		if (rc!=1) {strcpy(m_errorText,"Read Error!"); fclose(fp); return NULL;}

		rc=fread((long  *)&(inBM.bmWidth),4,1,fp);	 m_bytesRead+=4;
		if (rc!=1) {strcpy(m_errorText, "Read Error!"); fclose(fp); return NULL;}

		rc=fread((long  *)&(inBM.bmHeight),4,1,fp); m_bytesRead+=4;
		if (rc!=1) {strcpy(m_errorText,"Read Error!"); fclose(fp); return NULL;}

		rc=fread((int  *)&(inBM.bmPlanes),2,1,fp); m_bytesRead+=2;
		if (rc!=1) {strcpy(m_errorText, "Read Error!"); fclose(fp); return NULL;}

		rc=fread((int  *)&(inBM.bmBitsPixel),2,1,fp); m_bytesRead+=2;
		if (rc!=1) {strcpy(m_errorText, "Read Error!"); fclose(fp); return NULL;}

		rc=fread((long  *)&(compression),4,1,fp); m_bytesRead+=4;
		if (rc!=1) {strcpy(m_errorText, "Read Error!"); fclose(fp); return NULL;}

		rc=fread((long  *)&(sizeimage),4,1,fp); m_bytesRead+=4;
		if (rc!=1) {strcpy(m_errorText, "Read Error!"); fclose(fp); return NULL;}

		rc=fread((long  *)&(xscale),4,1,fp); m_bytesRead+=4;
		if (rc!=1) {strcpy(m_errorText, "Read Error!"); fclose(fp); return NULL;}

		rc=fread((long  *)&(yscale),4,1,fp); m_bytesRead+=4;
		if (rc!=1) {strcpy(m_errorText, "Read Error!"); fclose(fp); return NULL;}

		rc=fread((long  *)&(colors),4,1,fp); m_bytesRead+=4;
		if (rc!=1) {strcpy(m_errorText, "Read Error!"); fclose(fp); return NULL;}

		rc=fread((long  *)&(impcol),4,1,fp); m_bytesRead+=4;
		if (rc!=1) {strcpy(m_errorText, "Read Error!"); fclose(fp); return NULL;}

		////////////////////////////////////////////////////////////////////////////
		//	i don't do RLE files

		if (compression!=BI_RGB) {
	    	strcpy(m_errorText,"This is a compressed file.");
	    	fclose(fp);
	    	return NULL;
	    }

		if (colors == 0) {
			colors = 1 << inBM.bmBitsPixel;
		}


		////////////////////////////////////////////////////////////////////////////
		// read colormap

		RGBQUAD* colormap = NULL;

		switch (inBM.bmBitsPixel) {
		case 24:
			break;
			// read pallete 
		case 1:
		case 4:
		case 8:
			colormap = new RGBQUAD[colors]; // (RGBQUAD*)alloc_mem(sizeof(RGBQUAD) * colors);
			if (colormap==NULL) {
				fclose(fp);
				strcpy(m_errorText,"Out of memory");
				return NULL;
			}

			int i;
			for (i=0;i<colors;i++) {
				unsigned char r,g,b, dummy;

				rc=fread((unsigned char *)&(b),1,1,fp);
				m_bytesRead++;
				if (rc!=1) {
					strcpy(m_errorText, "Read Error!");	
					delete []colormap; //  free_mem(colormap);
					fclose(fp);
					return NULL;
				}

				rc=fread((unsigned char  *)&(g),1,1,fp); 
				m_bytesRead++;
				if (rc!=1) {
					strcpy(m_errorText, "Read Error!");	
					delete []colormap; //  free_mem(colormap);
					fclose(fp);
					return NULL;
				}

				rc=fread((unsigned char  *)&(r),1,1,fp); 
				m_bytesRead++;
				if (rc!=1) {
					strcpy(m_errorText, "Read Error!");	
					delete []colormap; // free_mem(colormap);
					fclose(fp);
					return NULL;
				}


				rc=fread((unsigned char  *)&(dummy),1,1,fp); 
				m_bytesRead++;
				if (rc!=1) {
					strcpy(m_errorText, "Read Error!");	
					delete []colormap; // free_mem(colormap);
					fclose(fp);
					return NULL;
				}

				colormap[i].rgbRed=r;
				colormap[i].rgbGreen=g;
				colormap[i].rgbBlue=b;
			}
			break;
		}


		if ((long)m_bytesRead>pixoff) {
			fclose(fp);
			strcpy(m_errorText,"Corrupt palette");
			delete []colormap; // free_mem(colormap);
			fclose(fp);
			return NULL;
		}

		while ((long)m_bytesRead<pixoff) {
			char dummy;
			fread(&dummy,1,1,fp);
			m_bytesRead++;
		}

		int w=inBM.bmWidth;
		int h=inBM.bmHeight;

		// set the output params
		*width=w;
		*height=h;

		long row_size = w * 3;

		long bufsize = (long)w * 3 * (long)h;

		////////////////////////////////////////////////////////////////////////////
		// alloc our buffer

		outBuf= new unsigned char[bufsize]; // (unsigned char *) alloc_mem(sizeof(unsigned char) * bufsize);
		if (outBuf==NULL) {
			strcpy(m_errorText,"Memory alloc Failed");
		} else {

			////////////////////////////////////////////////////////////////////////////
			//	read it

			long row=0;
			long rowOffset=0;

			// read rows in reverse order
			for (row=inBM.bmHeight-1;row>=0;row--) {

				// which row are we working on?
				rowOffset=(long unsigned)row*row_size;						      

				if (inBM.bmBitsPixel==24) {

					for (int col=0;col<w;col++) {
						long offset = col * 3;
						char pixel[3];

						if (fread((void  *)(pixel),1,3,fp)==3) {
							// we swap red and blue here
							*(outBuf + rowOffset + offset + 0)=pixel[2];		// r
							*(outBuf + rowOffset + offset + 1)=pixel[1];		// g
							*(outBuf + rowOffset + offset + 2)=pixel[0];		// b
						}

					}

					m_bytesRead+=row_size;
					
					// read DWORD padding
					while ((m_bytesRead-pixoff)&3) {
						char dummy;
						if (fread(&dummy,1,1,fp)!=1) {
							strcpy(m_errorText, "Read Error!");
							delete []outBuf; // free_mem(outBuf);
							fclose(fp);
							return NULL;
						}

						m_bytesRead++;
					}
 
					
				} else {	// 1, 4, or 8 bit image

					////////////////////////////////////////////////////////////////
					// pixels are packed as 1 , 4 or 8 bit vals. need to unpack them

					int bit_count = 0;
					unsigned short mask = (1 << inBM.bmBitsPixel) - 1;

					unsigned char inbyte=0;

					for (int col=0;col<w;col++) {
						
						int pix=0;

						// if we need another byte
						if (bit_count <= 0) {
							bit_count = 8;
							if (fread(&inbyte,1,1,fp)!=1) {
								strcpy(m_errorText, "Read Error");
								delete []outBuf; //  free_mem(outBuf);
								delete []colormap; // free_mem(colormap);
								fclose(fp);
								return NULL;
							}
							m_bytesRead++;
						}

						// keep track of where we are in the bytes
						bit_count -= inBM.bmBitsPixel;
						pix = ( inbyte >> bit_count) & mask;

						// lookup the color from the colormap - stuff it in our buffer
						// swap red and blue
						*(outBuf + rowOffset + col * 3 + 2) = colormap[pix].rgbBlue;
						*(outBuf + rowOffset + col * 3 + 1) = colormap[pix].rgbGreen;
						*(outBuf + rowOffset + col * 3 + 0) = colormap[pix].rgbRed;
					}

					// read DWORD padding
					while ((m_bytesRead-pixoff)&3) {
						char dummy;
						if (fread(&dummy,1,1,fp)!=1) {
							strcpy(m_errorText, "Read Error!");
							delete []outBuf; // free_mem(outBuf);
							if (colormap)
								delete []colormap; // free_mem(colormap);
							fclose(fp);
							return NULL;
						}
						m_bytesRead++;
					}
				}
			}
		
		}

		if (colormap) {
			delete []colormap; // free_mem(colormap);
		}

		fclose(fp);

    }

	return outBuf;
}

////////////////////////////////////////////////////////////////////////////
//	write a 24-bit BMP file
//
//	image MUST be a packed buffer (not DWORD-aligned)
//	image MUST be vertically flipped !
//	image MUST be BGR, not RGB !
//

int 
BMPFile::SaveBMP24(
	const char*		fileName,	// output path	
	int				width,		// pixels
	int				height,
	unsigned char * RGBbuf		// BGR buffer
)
{
	short res1=0;
    short res2=0;
    long pixoff=54;
    long compression=0;
    long cmpsize=0;
    long colors=0;
    long impcol=0;
	char m1='B';
	char m2='M';

	strcpy(m_errorText,"OK");

	DWORD widthDW = WIDTHBYTES(width * 24);

	long bmfsize=sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) +
  							widthDW * height;	
	long byteswritten=0;

	BITMAPINFOHEADER header;
  	header.biSize=40; 						// header size
	header.biWidth=width;
	header.biHeight=height;
	header.biPlanes=1;
	header.biBitCount=24;					// RGB encoded, 24 bit
	header.biCompression=BI_RGB;			// no compression
	header.biSizeImage=0;
	header.biXPelsPerMeter=0;
	header.biYPelsPerMeter=0;
	header.biClrUsed=0;
	header.biClrImportant=0;

	FILE *fp;	
	fp=fopen(fileName,"wb");
	if (fp==0) {
		strcpy(m_errorText,"Can't open file for writing");
		return 0;
	}

	// should probably check for write errors here...
	
	fwrite((BYTE  *)&(m1),1,1,fp); byteswritten+=1;
	fwrite((BYTE  *)&(m2),1,1,fp); byteswritten+=1;
	fwrite((long  *)&(bmfsize),4,1,fp);	byteswritten+=4;
	fwrite((int  *)&(res1),2,1,fp); byteswritten+=2;
	fwrite((int  *)&(res2),2,1,fp); byteswritten+=2;
	fwrite((long  *)&(pixoff),4,1,fp); byteswritten+=4;

	fwrite((BITMAPINFOHEADER *)&header,sizeof(BITMAPINFOHEADER),1,fp);
	byteswritten+=sizeof(BITMAPINFOHEADER);
	
	long row=0;
	long rowidx;
	long row_size;
	row_size=header.biWidth*3;
    long rc;
	for (row=0;row<header.biHeight;row++) {
		rowidx=(long unsigned)row*row_size;						      

		// write a row
		rc=fwrite((void  *)(RGBbuf+rowidx),row_size,1,fp);
		if (rc!=1) {
			strcpy(m_errorText,"fwrite error.");
			break;
		}
		byteswritten+=row_size;	

		// pad to DWORD
		for (DWORD count=row_size;count<widthDW;count++) {
			char dummy=0;
			fwrite(&dummy,1,1,fp);
			byteswritten++;							  
		}

	}
           
	fclose(fp);
	return 1;
}

////////////////////////////////////////////////////////////////////////////
//	write a 8-bit BMP file
int     
BMPFile::SaveBMP8(
	const char*		filename, 
	int				p_nWidth,
	int				p_nHeight, 
	unsigned char*	p_pbImageData
)
{	
	unsigned char head[1078]={
		/***************************/
		//file header
		0x42,0x4d,//file type 
		//0x36,0x6c,0x01,0x00, //file size***
		0x0,0x0,0x0,0x00, //file size***
		0x00,0x00, //reserved
		0x00,0x00,//reserved
		0x36,0x4,0x00,0x00,//head byte***
		/***************************/
		//infoheader
		0x28,0x00,0x00,0x00,//struct size

		//0x00,0x01,0x00,0x00,//map width*** 
		0x00,0x00,0x0,0x00,//map width*** 
		//0x68,0x01,0x00,0x00,//map height***
		0x00,0x00,0x00,0x00,//map height***

		0x01,0x00,//must be 1
		0x08,0x00,//color count***
		0x00,0x00,0x00,0x00, //compression
		//0x00,0x68,0x01,0x00,//data size***
		0x00,0x00,0x00,0x00,//data size***
		0x00,0x00,0x00,0x00, //dpix
		0x00,0x00,0x00,0x00, //dpiy
		0x00,0x00,0x00,0x00,//color used
		0x00,0x00,0x00,0x00,//color important
	};	
	FILE	*fh;
	int		i,j, iImageStep;
	long	num;
	unsigned char	*p1;
	unsigned char	w_bTemp[4];
	
	if (p_nWidth & 0x03){
		iImageStep = p_nWidth + (4 - (p_nWidth & 0x03));
	}
	else{
		iImageStep = p_nWidth;
	}
	
	num=p_nWidth; head[18]= num & 0xFF;
	num=num>>8;  head[19]= num & 0xFF;
	num=num>>8;  head[20]= num & 0xFF;
	num=num>>8;  head[21]= num & 0xFF;
	
	num=p_nHeight; head[22]= num & 0xFF;
	num=num>>8;  head[23]= num & 0xFF;
	num=num>>8;  head[24]= num & 0xFF;
	num=num>>8;  head[25]= num & 0xFF;
	
	j=0;
	for (i=54;i<1078;i=i+4)
	{
		head[i]=head[i+1]=head[i+2]=j; 
		head[i+3]=0;
		j++;
	}

	memset( w_bTemp, 0, sizeof(w_bTemp) );
	
	if( (fh  = fopen( filename, "wb" )) == NULL )
		return 0;		
	
    fwrite(head,sizeof(char),1078,fh);
	
	if (iImageStep == p_nWidth){
		p1 = p_pbImageData + (p_nHeight - 1) * p_nWidth;
		for( i = 0; i < p_nHeight; i ++){	
			fwrite( p1, 1, p_nWidth, fh );
			p1 -= p_nWidth;
		}	
	}
	else{
		iImageStep -= p_nWidth;
		p1 = p_pbImageData + (p_nHeight - 1) * p_nWidth;
		for( i = 0; i < p_nHeight; i ++){	
			fwrite( p1, 1, p_nWidth, fh );
			fwrite( w_bTemp, 1, iImageStep, fh );
			p1 -= p_nWidth;
		}
	}
	fclose(fh);
	
	return 1;
}

int 
BMPFile::LoadBMP( 
	const char*		fileName, 
	int*			width, 
	int*			height, 
	unsigned char**	p_ppRGBbuf 
)
{
	int		w_ret = 0; // Fp_TRUE;
	unsigned char*	RGBbuf = NULL;

	if ((!fileName) || (!p_ppRGBbuf) || (!width) || (!height)){
		w_ret = 1; // Fp_Err_PARAM;
		goto	L_EXIT;
	}

	RGBbuf = LoadBMPCore(fileName, width, height);
	if (RGBbuf == NULL || strcmp(m_errorText,"OK") != 0){
		w_ret = 1; // Fp_Err_ImRead;
		goto	L_EXIT;
	}

	BGRFromRGB(RGBbuf, *width, *height);

	*p_ppRGBbuf = RGBbuf;

L_EXIT:
	return	w_ret;
}

int BMPFile::SaveBMP( 
	const char*		filename, 
	int				width, 
	int				height, 
	unsigned char*	buf, 
	int				color
)
{
	int				w_ret = TRUE; // Fp_TRUE;
	unsigned char*	RGBData;	

	//. Check
	if ((!buf) || (width < 1) || (height < 1) || (!filename)){
		w_ret = FALSE; // Fp_Err_PARAM;	
		goto	L_EXIT;
	}

	RGBData = new BYTE[width * height * 3]; //  (BYTE*)alloc_mem( width * height * 3 );

	if (color == 0){		
		memcpy( RGBData, buf, width * height );
		w_ret = SaveBMP8( filename, width, height, RGBData );				
	}
	else{
		memcpy(RGBData, buf, width * height * 3);
		VertFlipBuf(RGBData, width * 3, height);
		w_ret = SaveBMP24(filename, width, height, RGBData);		
	}

	delete []RGBData; //  free_mem(RGBData);
L_EXIT:
	return	w_ret;
}