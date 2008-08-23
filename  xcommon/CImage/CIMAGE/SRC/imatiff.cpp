// imatiff.cpp

/*
 * File:	ximatif.cpp
 * Purpose:	Platform Independent TIFF Image Class Loader and Writer
 * 07/Aug/2001 <ing.davide.pizzolato@libero.it>
 */
// Removed fopen/fclose mess - Troels K.

#include "..\include\cimage.h"
#include "imatiff.h"
#pragma hdrstop

#include "..\tiff\tiffio.h"

#define CVT(x)			(((x) * 255L) / ((1L<<16)-1))
#define	SCALE(x)		(((x)*((1L<<16)-1))/255)
#define CalculateLine(width,bitdepth)	(((width * bitdepth) + 7) / 8)
#define CalculatePitch(line)	(line + 3 & ~3)

EXTERN_C TIFF* TIFFOpenEx(FILE* stream, const char* mode);

///////////////////////////////////////////////////////////////////////////
BOOL CImageTIFF::Read(FILE* stream)
{
	TIFF* m_tif = TIFFOpenEx(stream, "rb");

	uint32 height=0;
	uint32 width=0;
	uint16 bitspersample=1;
	uint16 samplesperpixel=1;
	uint32 rowsperstrip=-1;
	uint16 photometric=0;
	uint16 compression=1;
	uint32 x, y;
	BOOL isRGB;
	BYTE *bits;		//pointer to source data
	BYTE *bits2;	//pointer to destination data

  try{
	//check if it's a tiff file
	if (!m_tif)
		throw "Error encountered while opening TIFF file";

	m_info.nNumFrames=0;
	while(TIFFSetDirectory(m_tif,(uint16)m_info.nNumFrames)) m_info.nNumFrames++;


	if (!TIFFSetDirectory(m_tif, (uint16)m_info.nFrame))
		throw "Error: page not present in TIFF file";			

	//get image m_info
	TIFFGetField(m_tif, TIFFTAG_COMPRESSION, &compression);
	if (compression == COMPRESSION_LZW)
		throw "LZW compression is no longer supported due to Unisys patent enforcement";			

	TIFFGetField(m_tif, TIFFTAG_IMAGEWIDTH, &width);
	TIFFGetField(m_tif, TIFFTAG_IMAGELENGTH, &height);
	TIFFGetField(m_tif, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);
	TIFFGetField(m_tif, TIFFTAG_BITSPERSAMPLE, &bitspersample);
	TIFFGetField(m_tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);   
	TIFFGetField(m_tif, TIFFTAG_PHOTOMETRIC, &photometric);

	m_header.biWidth  = width;
	m_header.biHeight = height;
	m_header.biClrUsed=0;
	m_info.nBkgndIndex =-1;

	isRGB = (bitspersample >= 8) &&
		(photometric == PHOTOMETRIC_RGB) ||
		(photometric == PHOTOMETRIC_YCBCR) ||
		(photometric == PHOTOMETRIC_SEPARATED) ||
		(photometric == PHOTOMETRIC_LOGLUV);

	if (isRGB){
		m_header.biBitCount=24;
		m_info.bColorType = COLORTYPE_COLOR;
	}else{
		m_info.bColorType = COLORTYPE_PALETTE;
		if ((photometric==PHOTOMETRIC_MINISBLACK)||(photometric==PHOTOMETRIC_MINISWHITE)){
			if	(bitspersample == 1){
				m_header.biBitCount=1;		//B&W image
				m_header.biClrUsed =2;
			} else {
				m_header.biBitCount=8;		//gray scale
				m_header.biClrUsed =256;
			}
		} else if (bitspersample == 4) {
			m_header.biBitCount=4;			// 16 colors
			m_header.biClrUsed=16;
		} else {
			m_header.biBitCount=8;			//256 colors
			m_header.biClrUsed=256;
		}
	}

	Create(m_header.biWidth,m_header.biHeight,m_header.biBitCount);	//image creation

	if (isRGB) {
		// Read the whole image into one big RGBA buffer using
		// the traditional TIFFReadRGBAImage() API that we trust.
		uint32* raster;		// retrieve RGBA image
		uint32 *row;

		raster = (uint32*)_TIFFmalloc(width * height * sizeof (uint32));
		if (raster == NULL) throw "No space for raster buffer";
			
		// Read the image in one chunk into an RGBA array
		if(!TIFFReadRGBAImage(m_tif, width, height, raster, 1)) {
				_TIFFfree(raster);
				throw "Corrupted TIFF file!";
		}

		// read the raster lines and save them in the DIB
		// with RGB mode, we have to change the order of the 3 samples RGB
		row = &raster[0];
		bits2 = m_info.pImage;
		for (y = 0; y < height; y++) {
			bits = bits2;
			for (x = 0; x < width; x++) {
				*bits++ = (BYTE)TIFFGetB(row[x]);
				*bits++ = (BYTE)TIFFGetG(row[x]);
				*bits++ = (BYTE)TIFFGetR(row[x]);
			}
			row += width;
			bits2 += m_info.dwEffWidth;
		}
		_TIFFfree(raster);
	} else {
		RGBQUAD *pal;
		pal=(RGBQUAD*)calloc(256,sizeof(RGBQUAD));
		if (pal==NULL) throw "Unable to allocate TIFF palette";

		// set up the colormap based on photometric	
		switch(photometric) {
			case PHOTOMETRIC_MINISBLACK:	// bitmap and greyscale image types
			case PHOTOMETRIC_MINISWHITE:
				if (bitspersample == 1) {	// Monochrome image
					if (photometric == PHOTOMETRIC_MINISBLACK) {
						pal[1].rgbRed = pal[1].rgbGreen = pal[1].rgbBlue = 255;
					} else {
						pal[0].rgbRed = pal[0].rgbGreen = pal[0].rgbBlue = 255;
					}
				} else {		// need to build the scale for greyscale images
					if (photometric == PHOTOMETRIC_MINISBLACK) {
						for (int i = 0; i < 256; i++) {
							pal[i].rgbRed = pal[i].rgbGreen = pal[i].rgbBlue = i;
						}
					} else {
						for (int i = 0; i < 256; i++) {
							pal[i].rgbRed =	pal[i].rgbGreen = pal[i].rgbBlue = 255 - i;
						}
					}
				}
				break;
			case PHOTOMETRIC_PALETTE:	// color map indexed
				uint16 *red;
				uint16 *green;
				uint16 *blue;
				TIFFGetField(m_tif, TIFFTAG_COLORMAP, &red, &green, &blue); 

				// Is the palette 16 or 8 bits ?
				BOOL Palette16Bits = FALSE;
				int n=1<<bitspersample;
				while (n-- > 0) {
					if (red[n] >= 256 || green[n] >= 256 || blue[n] >= 256) {
						Palette16Bits=TRUE;
						break;
					}
				}

				// load the palette in the DIB
				for (int i = (1 << bitspersample) - 1; i >= 0; i--) {
					if (Palette16Bits) {
						pal[i].rgbRed =(BYTE) CVT(red[i]);
						pal[i].rgbGreen = (BYTE) CVT(green[i]);
						pal[i].rgbBlue = (BYTE) CVT(blue[i]);           
					} else {
						pal[i].rgbRed = (BYTE) red[i];
						pal[i].rgbGreen = (BYTE) green[i];
						pal[i].rgbBlue = (BYTE) blue[i];        
					}
				}
				break;
		}
		SetPalette(pal,m_header.biClrUsed);	//palette assign
		free(pal);

		// read the tiff lines and save them in the DIB
		uint32 nrow;
		uint32 ys;
		int line = CalculateLine(width, bitspersample * samplesperpixel);
//		int pitch = CalculatePitch(line);
		long bitsize= TIFFStripSize(m_tif);
		bits = (BYTE*)malloc(bitsize);

		for (ys = 0; ys < height; ys += rowsperstrip) {
			nrow = (ys + rowsperstrip > height ? height - ys : rowsperstrip);
			if (TIFFReadEncodedStrip(m_tif, TIFFComputeStrip(m_tif, ys, 0), bits, nrow * line) == -1) {
				free(bits);
				throw "Corrupted TIFF file!";
			}
			for (y = 0; y < nrow; y++) {
				memcpy(m_info.pImage+m_info.dwEffWidth*(height-ys-nrow+y),bits+(nrow-y-1)*line,line);
			}
			/*if (m_header.biClrUsed==2){
				for (y = 0; y < nrow; y++) { for (x = 0; x < width; x++) {
					SetPixelIndex(x,y+ys,(bits[y*line+(x>>3)]>>(7-x%8))&0x01);
			}}}*/
		}
		free(bits);
	}
  } catch (char *message) {
	  strncpy(m_info.szLastError,message,255);
	  if (m_tif)	TIFFClose(m_tif);
	  return FALSE;
  }
	TIFFClose(m_tif);
	return TRUE;
}

BOOL CImageTIFF::Write(FILE* stream)
{
	//prepare the palette struct
	RGBQUAD pal[256];
	if (GetPalette()){
		BYTE b;
		memcpy(pal,GetPalette(),GetPaletteSize());
		for(WORD a=0;a<m_header.biClrUsed;a++){	//swap blue and red components
			b=pal[a].rgbBlue; pal[a].rgbBlue=pal[a].rgbRed; pal[a].rgbRed=b;
		}
	}
	
	TIFF *m_tif=NULL; // = (TIFF *)new(byte[512]);

	uint32 height=m_header.biHeight;
	uint32 width=m_header.biWidth;
//	uint32 rowsperstrip = (uint32) -1;
	uint16 bitspersample=m_header.biBitCount;
	uint16 samplesperpixel;
	uint16 photometric;
	uint16 compression;
	uint16 pitch;
	int line;
	uint32 x, y;

	samplesperpixel = ((bitspersample == 24) || (bitspersample == 32)) ? 3 : 1;
	photometric = (samplesperpixel==3) ? PHOTOMETRIC_RGB : PHOTOMETRIC_PALETTE;
	line = CalculateLine(width, bitspersample * samplesperpixel);
	pitch = CalculatePitch(line);

	m_tif=TIFFOpenEx(stream, "wb");
	if (m_tif==NULL) return FALSE;

	// handle standard width/height/bpp stuff
	TIFFSetField(m_tif, TIFFTAG_IMAGEWIDTH, width);
	TIFFSetField(m_tif, TIFFTAG_IMAGELENGTH, height);
	TIFFSetField(m_tif, TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);
	TIFFSetField(m_tif, TIFFTAG_BITSPERSAMPLE, ((bitspersample == 32) ? 24 : bitspersample) / samplesperpixel);
	TIFFSetField(m_tif, TIFFTAG_PHOTOMETRIC, photometric);
	TIFFSetField(m_tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);	// single image plane 
	TIFFSetField(m_tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(m_tif, TIFFTAG_ROWSPERSTRIP, 1);
//	TIFFSetField(m_tif, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(m_tif, rowsperstrip));

	// handle metrics
	TIFFSetField(m_tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
	TIFFSetField(m_tif, TIFFTAG_XRESOLUTION, 72.0);
	TIFFSetField(m_tif, TIFFTAG_YRESOLUTION, 72.0);

	// multi-paging
//	if (page >= 0) {
//		char page_number[20];
//		sprintf(page_number, "Page %d", page);
//
//		TIFFSetField(m_tif, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
//		TIFFSetField(m_tif, TIFFTAG_PAGENUMBER, page);
//		TIFFSetField(m_tif, TIFFTAG_PAGENAME, page_number);
//	} else {
		TIFFSetField(m_tif, TIFFTAG_SUBFILETYPE, 0);
//	}

	// palettes (image colormaps are automatically scaled to 16-bits)
	if (photometric == PHOTOMETRIC_PALETTE) {
		uint16 *r, *g, *b;
		r = (uint16 *) _TIFFmalloc(sizeof(uint16) * 3 * 256);
		g = r + 256;
		b = g + 256;

		for (int i = 255; i >= 0; i--) {
			b[i] = SCALE((uint16)pal[i].rgbRed);
			g[i] = SCALE((uint16)pal[i].rgbGreen);
			r[i] = SCALE((uint16)pal[i].rgbBlue);
		}

		TIFFSetField(m_tif, TIFFTAG_COLORMAP, r, g, b);
		_TIFFfree(r);
	}

	// compression
	switch(bitspersample) {
		case 1 :
			compression = COMPRESSION_CCITTFAX4;
			break;
		case 8 :
		case 24 :
		case 32 :
			compression = COMPRESSION_PACKBITS;
			break;
		default :
			compression = COMPRESSION_NONE;
			break;
	}
	TIFFSetField(m_tif, TIFFTAG_COMPRESSION, compression);

	// read the DIB lines from bottom to top
	// and save them in the TIF
	// -------------------------------------	
	BYTE *bits;
	switch(bitspersample) {				
		case 1 :
		case 4 :
		case 8 :
		{
			for (y = 0; y < height; y++) {
				bits= m_info.pImage + (height - y - 1)*m_info.dwEffWidth;
				TIFFWriteScanline(m_tif,bits, y, 0);
			}
			break;
		}				
		case 24:
		case 32 :
		{
			BYTE *buffer = (BYTE *)malloc(m_info.dwEffWidth);
			for (y = 0; y < height; y++) {
				// get a pointer to the scanline
				memcpy(buffer, m_info.pImage + (height - y - 1)*m_info.dwEffWidth, m_info.dwEffWidth);
				// TIFFs store color data RGB instead of BGR
				BYTE *pBuf = buffer;
				for (x = 0; x < width; x++) {
					BYTE tmp = pBuf[0];
					pBuf[0] = pBuf[2];
					pBuf[2] = tmp;
					pBuf += 3;
				}
				// write the scanline to disc
				TIFFWriteScanline(m_tif, buffer, y, 0);
			}
			free(buffer);
			break;
		}				
	}
	TIFFClose(m_tif);
	return true;
}
