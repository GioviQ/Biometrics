#ifndef __BMPFILE_H__
#define __BMPFILE_H__

#define WIDTHBYTES(bits)    (((bits) + 31) / 32 * 4)
#define BMP_HEADERSIZE (3 * 2 + 4 * 12)
#define BMP_BYTESPERLINE (width, bits) ((((width) * (bits) + 31) / 32) * 4)
#define BMP_PIXELSIZE(width, height, bits) (((((width) * (bits) + 31) / 32) * 4) * height)

class BMPFile
{
protected:

	unsigned char*
	LoadBMPCore(
		const char*		fileName, 
		int*			width, 
		int*			height
	);

	int
	SaveBMP24(
		const char*		fileName, 		
		int				width, 
		int				height,
		unsigned char*	RGBbuf 
	);

	int
	SaveBMP8(
		const char*		filename, 
		int				p_nWidth,
		int				p_nHeight, 
		unsigned char*	p_pbImageData
	);

public:
	// parameters
	char m_errorText[255];
	long m_bytesRead;

public:

	// operations
	BMPFile();

	int
	LoadBMP(
		const char*		fileName, 
		int*			width, 
		int*			height,
		unsigned char**	RGBbuf
	);

	int 
	SaveBMP(
		const char*		fileName, 		
		int				width, 
		int				height,
		unsigned char*	buf,
		int				color
	);

};

#endif//..__BMPFILE_H__