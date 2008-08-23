// imagif.cpp

/*
 * File:	imagif.cc
 * Purpose:	Platform Independent GIF Image Class
 * Author:	Alejandro Aguilar Sierra
 * Created:	1995
 * Copyright: (c) 1995, Alejandro Aguilar Sierra <asierra@servidor.unam.mx>
 *
 * 6/15/97 Randy Spann: Added GIF87a writing support
 *         R.Spann@ConnRiver.net
 */

/*
 * File:	ximagif.cpp
 * Purpose:	Platform Independent GIF Image Class Loader and Writer
 * 07/Aug/2001 <ing.davide.pizzolato@libero.it>
 */

// Removed fopen/fclose mess - Troels K.
// Fixed struct alignment mess - Troels K.

#include "..\include\cimage.h"
#pragma hdrstop
#include "imagif.h"
#include "iterator.h"

#pragma pack(1)
////////////////////////// AD - for transparency ///////////////////////////////

// TRK: added bitfields, added handling of multiple images

struct DSCGIF 
{          /* Logic Screen Descriptor  */
   char header[6];        /* Firma and version */
   USHORT cx;
   USHORT cy;
   union
   {
      struct
      {
         BYTE sogct:3;
         BYTE b0:1;
         BYTE colres:3;
         BYTE colormap:1;
      } bitfield;
      BYTE byte;
   } pflds;
   char bcindx;
   char pxasrat;
};

struct GIFGCE 
{
   BYTE transpcolflag:1;
   BYTE userinputflag:1;
   BYTE dispmeth:3;
   BYTE res:3;
   WORD delaytime;
   BYTE transpcolindex;
}; // Graphic Control Extension

struct IMAGE
{           /* Image Descriptor */
  USHORT left;
  USHORT top;
  USHORT cx;
  USHORT cy;
  struct
  {
     BYTE sogct:3;
     BYTE b3:1;
     BYTE b4:1;
     BYTE b5:1;
     BYTE interlaced:1;
     BYTE colormap:1;
  } flags;
};

struct TABCOL {			/*   Tabla de colores  */
  SHORT colres;			/* color resolution */
  SHORT sogct;			/* size of global color table */
  rgb_color_struct paleta[256]; /*  paleta  */
};

#pragma pack()

BOOL CImageGIF::Read(FILE* file)
{
   GIFGCE gifgce;
   DSCGIF dscgif;
   IMAGE  image;
   TABCOL TabCol;

   C_ASSERT(13 == sizeof(dscgif));
   BOOL bOK = (1 == fread(&dscgif,sizeof(dscgif),1,file));
   if (bOK)
   {
	   //if (strncmp(dscgif.header,"GIF8",3)!=0) {
	   static const char szMagic[] = "GIF8";
      bOK = (0 == strncmp(dscgif.header,szMagic,_countof(szMagic) - 1));
   }
   if (!bOK) return FALSE;

////////////////////////// AD - for interlace ///////////////////////////////
   TabCol.sogct  = 1 << ((dscgif.pflds.bitfield.sogct)+1);
	TabCol.colres = dscgif.pflds.bitfield.colres + 1; //((dscgif.pflds.bitfield.colres-0)<<1) + 1;

   //TabCol.sogct = 1 << (dscgif.pflds.byte & 0x07)+1;
   //TabCol.colres = ((int)(dscgif.pflds.byte & 0x70) >> 3) + 1;


	// Global colour map?
   if (dscgif.pflds.bitfield.colormap)
   {
      C_ASSERT(sizeof(struct rgb_color_struct) == 3);
		fread(TabCol.paleta,sizeof(struct rgb_color_struct)*TabCol.sogct,1,file);
   }

   char ch;

	int iImage = 0;
   for (BOOL bContinue = TRUE; bContinue; )
   {
	   if (fread(&ch, sizeof(ch), 1, file) != 1)
      {
         break;
      }
      switch (ch)
      {
         case '!':                     // extension
         {
		      unsigned char count;
		      unsigned char fc;

		      bContinue = (1 == fread(&fc, sizeof(fc), 1, file));
            if (bContinue)
            {
		         //////////////// AD - for transparency //////////////////////////
		         if (fc == TRANSPARENCY_CODE)		
               {
			         bContinue = (1 == fread(&count, sizeof(count), 1, file));
                  if (bContinue)
                  {
			            C_ASSERT(sizeof(gifgce) == 4);
                     bContinue = (count == fread(&gifgce, 1, sizeof(gifgce), file));
                     if (bContinue)
                     {
			               if (gifgce.transpcolflag) m_info.nBkgndIndex  = gifgce.transpcolindex;
                        m_info.dwFrameDelay = gifgce.delaytime;
                     }
                  }
		         }
		         ///////////////////////////////////////////////////////////////////
		         while (bContinue && fread(&count, sizeof(count), 1, file) && count)
               {
			         //log << "Skipping " << count << " bytes" << endl;
			         fseek(file, count, SEEK_CUR);
		         }
            }
		      break;
	      } 
         case ',': // image
         {
		      C_ASSERT(sizeof(image) == 9);
            fread(&image,sizeof(image),1,file);
		      //log << "Image header" << endl;

		      // Local colour map?
		      if (image.flags.colormap)
            {
               TabCol.sogct  = 1 << ((image.flags.sogct)+1);
			      C_ASSERT(3 == sizeof(struct rgb_color_struct));
               fread(TabCol.paleta,sizeof(struct rgb_color_struct)*TabCol.sogct,1,file);
		      }
		      Create(image.cx, image.cy, /*TabCol.colres*/8); // TRK - can this be fixed?

		      if (image.flags.colormap || dscgif.pflds.bitfield.colormap)
            {
			      unsigned char r[256], g[256], b[256];
			      int i, has_white = 0;

			      for (i=0; i < TabCol.sogct; i++)
               {
				      r[i] = TabCol.paleta[i].r;
				      g[i] = TabCol.paleta[i].g;
				      b[i] = TabCol.paleta[i].b;

				      if (RGB(r[i],g[i],b[i]) == 0xFFFFFF) has_white = 1;
			      }

			      // Make transparency colour black...
			      if(0)if (m_info.nBkgndIndex != -1)
				      r[m_info.nBkgndIndex] = g[m_info.nBkgndIndex] = b[m_info.nBkgndIndex] = 0;
			      // Fill in with white // AD
			      if (m_info.nBkgndIndex != -1)	
               {
				      while (i < 256)	
                  {
					      has_white = 1;
					      r[i] = g[i] = b[i] = 255;
					      i++;
				      }
			      }

			      // Force last colour to white...   // AD
			      if ((m_info.nBkgndIndex != -1) && !has_white)
               {
				      r[255] = g[255] = b[255] = 255;
               }

			      SetPalette((m_info.nBkgndIndex != -1 ? 256 : TabCol.sogct), r, g, b);
		      }

		      CImageIterator* iter = new CImageIterator(this);
		      iter->Upset();
		      int badcode;
		      m_ibf = GIFBUFTAM+1;
      //		GIFDecoder gifdec;

		      interlaced = image.flags.interlaced;
		      iheight = image.cy;
		      istep = 8;
		      iypos = 0;
		      ipass = 0;

		      //if (interlaced) log << "Interlaced" << endl;
		      decoder(file, iter, image.cx, badcode);
		      delete iter;
		      break;
         }
         case ';':
            _asm NOP
         default:
            if (m_info.nFrame != iImage)
            {
			      fseek(file, -(GIFBUFTAM - m_ibf), SEEK_CUR);
               iImage++;
            }
            else
            {
               bContinue = FALSE;
            }
		      break;
      }
   }

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//   - This external (machine specific) function is expected to return
// either the next BYTE from the GIF file, or a negative error number.
int CImageGIF::get_byte(FILE* file)
{
	if (m_ibf>=GIFBUFTAM)
   {
		// FW 06/02/98 >>>
		// fread(buf,GIFBUFTAM,1,file);
		int n = fread( m_buf , 1 , GIFBUFTAM , file ) ;
		if( n < GIFBUFTAM ) m_buf[ n ] = -1 ;
		// FW 06/02/98 <<<
		m_ibf = 0;
	}
	return m_buf[m_ibf++];
}
/////////////////////////////////////////////////////////////////////////////
/*   - This function takes a full line of pixels (one BYTE per pixel) and
 * displays them (or does whatever your program wants with them...).  It
 * should return zero, or negative if an error or some other event occurs
 * which would require aborting the decode process...  Note that the length
 * passed will almost always be equal to the line length passed to the
 * decoder function, with the sole exception occurring when an ending code
 * occurs in an odd place in the GIF file...  In any case, linelen will be
 * equal to the number of pixels passed...
*/
int CImageGIF::out_line(CImageIterator* iter, unsigned char *pixels, int linelen)
{
////////////////////////// AD - for interlace ///////////////////////////////
	if (interlaced) {
		iter->SetY(iheight-iypos-1);
		iter->SetRow(pixels, linelen);

		if ((iypos += istep) >= iheight) {
			do {
				if (ipass++ > 0)
					istep /= 2;

				iypos = istep / 2;
			}
			while (iypos > iheight);
		}
		return 0;
	}
	else
/////////////////////////////////////////////////////////////////////////////
	if (iter->ItOK()) {
		iter->SetRow(pixels, linelen);
		(void)iter->PrevRow();
		return 0;
	} else {
		//	 puts("chafeo");
		return -1;
	}
}
/////////////////////////////////////////////////////////////////////
// SaveFile - writes GIF87a gif file
// Randy Spann 6/15/97
// R.Spann@ConnRiver.net
// Bug fix: ignored transparency (Troels K.)
BOOL CImageGIF::Write(FILE* fp)
{
	int InitCodeSize;	

	Width = GetWidth();
	Height = GetHeight();
	BitsPerPixel = m_header.biBitCount;
	
	if(m_header.biBitCount > 8)	
   {
		strcpy(m_info.szLastError,"GIF Images must be 8 bit or less");
		return FALSE;
	}

	if (fp==NULL) {
		strcpy(m_info.szLastError,"Can't open GIF for writing");
		return FALSE;
	}
	
	//int ColorMapSize = 1 << BitsPerPixel;
	int LeftOfs = 0, TopOfs = 0;
	cur_accum = 0;
	cur_bits = 0;
//	int Resolution = BitsPerPixel;
 	CountDown = (long)Width * (long)Height;

	if (BitsPerPixel <=1)	InitCodeSize=2;
	else	InitCodeSize = BitsPerPixel;

	curx = 0;
	cury = Height - 1;//because we read the image bottom to top

   static const char szMagic[] = "GIF87a";

   struct DSCGIF dscgif;
   strncpy(dscgif.header, szMagic,_countof(dscgif.header));	   //GIF Header
   dscgif.cx = Width;
   dscgif.cy = Height;
   
   dscgif.pflds.bitfield.colres = BitsPerPixel - 1;
   dscgif.pflds.bitfield.sogct  = (m_header.biClrUsed-1) >> 1;   
   
   dscgif.pflds.bitfield.b0 = 0;
   dscgif.pflds.bitfield.colormap = 1;

   dscgif.bcindx = 0;
   dscgif.pxasrat = 0;
   fwrite(&dscgif, sizeof(dscgif), 1, fp);
/*
   fwrite(szMagic,_countof(szMagic) - 1, 1, fp);	   //GIF Header

	Putword(Width,fp);			   //Logical screen descriptor
	Putword(Height,fp);

	int B=0x80;						   
	B |=(Resolution -1) << 5;
	B |=(BitsPerPixel - 1);
	fputc(B,fp);				   //GIF "packed fields"
	fputc(0,fp);
	fputc(0,fp);				   //GIF "pixel aspect ratio"
*/

/*
	long bufsize = CountDown;
	bufsize *= 3;
*/
	m_buffer = m_info.pImage;

	const RGBQUAD* pPal = GetPalette();
	for(DWORD i=0; i<m_header.biClrUsed; ++i) 
	{
		fputc(pPal[i].rgbRed,fp);
		fputc(pPal[i].rgbGreen,fp);
		fputc(pPal[i].rgbBlue,fp);
	}

   // TRK BEGIN
	fputc('!',fp);
	fputc(TRANSPARENCY_CODE,fp);
   struct GIFGCE temp;
   temp.transpcolflag = (m_info.nBkgndIndex != -1) ? 1 : 0;
   temp.userinputflag = 0;
   temp.dispmeth = 0;
   temp.res = 0;
   temp.delaytime = m_info.dwFrameDelay;
   temp.transpcolindex = m_info.nBkgndIndex;	   
   fputc(sizeof(temp),fp);
   fwrite(&temp, sizeof(temp), 1, fp);
   fputc(0,fp);
   // TRK END

	fputc(',',fp);

	Putword(LeftOfs, fp);
	Putword(TopOfs , fp);
	Putword(Width  , fp);
	Putword(Height , fp);

	fputc(0x00,fp);

	 // Write out the initial code size
	fputc(InitCodeSize, fp );
	 // Go and actually compress the data
	compress(InitCodeSize+1, fp);
	 // Write out a Zero-length packet (to end the series)
	fputc( 0, fp );
	 // Write the GIF file terminator
	fputc( ';', fp );
	 // And close the file

	return TRUE;
}
/////////////////////////////////////////////////////////////////////////////
//  Bump the 'curx' and 'cury' to point to the next pixel
void CImageGIF::BumpPixel()
{
	// Bump the current X position
	++curx;
	if( curx == Width ) {
		curx = 0;
		cury--;	             //bottom to top
	}
}
/////////////////////////////////////////////////////////////////////////////
// Return the next pixel from the image
int CImageGIF::GIFNextPixel( )
{
	unsigned long index;
	int r;
	
	if( CountDown == 0 )
		return EOF;
	--CountDown;
	index= (unsigned long)curx + (unsigned long)cury * 
		(((unsigned long)Width * BitsPerPixel + 31) / 32) * 4;//effective width
	r = *(m_buffer+index);
	BumpPixel();
	return r;
}
/////////////////////////////////////////////////////////////////////////////
void CImageGIF::Putword(int w, FILE *fp )
{
	fputc( w & 0xff, fp );
	fputc( (w / 256) & 0xff, fp );
}
/***************************************************************************
 *
 *  GIFCOMPR.C       -     LZW GIF Image compression routines
 *
 ***************************************************************************/
#define BITS    12
#define HSIZE  5003            /* 80% occupancy */
typedef        unsigned char   char_type;

static int n_bits;                        /* number of bits/code */
static int maxbits = BITS;                /* user settable max # bits/code */
static code_int maxcode;                  /* maximum code, given n_bits */
static code_int maxmaxcode = (code_int)1 << BITS; /* should NEVER generate this code */

#define MAXCODE(n_bits)        (((code_int) 1 << (n_bits)) - 1)

static count_int htab [HSIZE];
static unsigned short codetab [HSIZE];
#define HashTabOf(i)       htab[i]
#define CodeTabOf(i)    codetab[i]

static code_int free_ent = 0;                  /* first unused entry */

/*
 * block compression parameters -- after all codes are used up,
 * and compression rate changes, start over.
 */
static int clear_flg = 0;

/*
 * compress pixels to GIF packets
 */

static int g_init_bits;
static FILE* g_outfile;

static int ClearCode;
static int EOFCode;

void CImageGIF::compress( int init_bits, FILE* outfile)
{
	register long fcode;
	register code_int i /* = 0 */;
	register int c;
	register code_int ent;
	register code_int disp;
	register int hshift;

	/*
	 * Set up the globals:  g_init_bits - initial number of bits
	 *                      g_outfile   - pointer to output file
	 */
	g_init_bits = init_bits;
	g_outfile = outfile;

	 // Set up the necessary values
	clear_flg = 0;
	maxcode = MAXCODE(n_bits = g_init_bits);

	ClearCode = (1 << (init_bits - 1));
	EOFCode = ClearCode + 1;
	free_ent = ClearCode + 2;

	char_init();

	ent = GIFNextPixel( );

	hshift = 0;
	for ( fcode = (long) HSIZE;  fcode < 65536L; fcode *= 2L )
		++hshift;
	hshift = 8 - hshift;                /* set hash code range bound */

	cl_hash( (count_int) HSIZE);            /* clear hash table */

	output( (code_int)ClearCode );

	while ( (c = GIFNextPixel( )) != EOF ) {    

		fcode = (long) (((long) c << maxbits) + ent);
		i = (((code_int)c << hshift) ^ ent);    /* xor hashing */

		if ( HashTabOf (i) == fcode ) {
			ent = CodeTabOf (i);
			continue;
		} else if ( (long)HashTabOf (i) < 0 )      /* empty slot */
			goto nomatch;
		disp = HSIZE - i;           /* secondary hash (after G. Knott) */
		if ( i == 0 )
			disp = 1;
probe:
		if ( (i -= disp) < 0 )
			i += HSIZE;

		if ( HashTabOf (i) == fcode ) {
			ent = CodeTabOf (i);
			continue;
		}
		if ( (long)HashTabOf (i) > 0 )
			goto probe;
nomatch:
		output ( (code_int) ent );
		ent = c;
		if ( free_ent < maxmaxcode ) {  
			CodeTabOf (i) = free_ent++; /* code -> hashtable */
			HashTabOf (i) = fcode;
		} else
				cl_block();
	}
	 // Put out the final code.
	output( (code_int)ent );
	output( (code_int) EOFCode );
}

static const unsigned long masks[] = { 0x0000, 0x0001, 0x0003, 0x0007, 0x000F,
								  0x001F, 0x003F, 0x007F, 0x00FF,
								  0x01FF, 0x03FF, 0x07FF, 0x0FFF,
								  0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF };

void CImageGIF::output( code_int  code)
{
	cur_accum &= masks[ cur_bits ];

	if( cur_bits > 0 )
		cur_accum |= ((long)code << cur_bits);
	else
		cur_accum = code;

	cur_bits += n_bits;

	while( cur_bits >= 8 ) {
		char_out( (unsigned int)(cur_accum & 0xff) );
		cur_accum >>= 8;
		cur_bits -= 8;
	}

	/*
	 * If the next entry is going to be too big for the code size,
	 * then increase it, if possible.
	 */
   if ( free_ent > maxcode || clear_flg ) {
		if( clear_flg ) {
			maxcode = MAXCODE (n_bits = g_init_bits);
			clear_flg = 0;
		} else {
			++n_bits;
			if ( n_bits == maxbits )
				maxcode = maxmaxcode;
			else
				maxcode = MAXCODE(n_bits);
		}
	}
	
	if( code == EOFCode ) {
		 // At EOF, write the rest of the buffer.
		while( cur_bits > 0 ) {
			char_out( (unsigned int)(cur_accum & 0xff) );
			cur_accum >>= 8;
			cur_bits -= 8;
		}
	
		flush_char();
	
		fflush( g_outfile );
	
		if( ferror( g_outfile ) ) {
			strcpy(m_info.szLastError,"Write Error in GIF file");
		}
	}
}

void CImageGIF::cl_block()
{
	cl_hash((count_int)HSIZE);
	free_ent=ClearCode+2;
	clear_flg=1;

	output((code_int)ClearCode);
}

void CImageGIF::cl_hash(register count_int hsize)

{
	register count_int *htab_p = htab+hsize;

	register long i;
	register long m1 = -1L;

	i = hsize - 16;

	do {
		*(htab_p-16)=m1;
		*(htab_p-15)=m1;
		*(htab_p-14)=m1;
		*(htab_p-13)=m1;
		*(htab_p-12)=m1;
		*(htab_p-11)=m1;
		*(htab_p-10)=m1;
		*(htab_p-9)=m1;
		*(htab_p-8)=m1;
		*(htab_p-7)=m1;
		*(htab_p-6)=m1;
		*(htab_p-5)=m1;
		*(htab_p-4)=m1;
		*(htab_p-3)=m1;
		*(htab_p-2)=m1;
		*(htab_p-1)=m1;
		
		htab_p-=16;
	} while ((i-=16) >=0);

	for (i+=16;i>0;--i)
		*--htab_p=m1;
}

/*******************************************************************************
*   GIF specific
*******************************************************************************/

static int a_count;

void CImageGIF::char_init()
{
	a_count=0;
}

static char accum[256];

void CImageGIF::char_out(int c)
{
	accum[a_count++]=c;
	if (a_count >=254)
		flush_char();
}

void CImageGIF::flush_char()
{
	if (a_count > 0) {
		fputc(a_count,g_outfile);
		fwrite(accum,1,a_count,g_outfile);
		a_count=0;
	}
}

/*******************************************************************************
*   GIF decoder
*******************************************************************************/
/* DECODE.C - An LZW decoder for GIF
 * Copyright (C) 1987, by Steven A. Bennett
 * Copyright (C) 1994, C++ version by Alejandro Aguilar Sierra
*
 * Permission is given by the author to freely redistribute and include
 * this code in any program as long as this credit is given where due.
 *
 * In accordance with the above, I want to credit Steve Wilhite who wrote
 * the code which this is heavily inspired by...
 *
 * GIF and 'Graphics Interchange Format' are trademarks (tm) of
 * Compuserve, Incorporated, an H&R Block Company.
 *
 * Release Notes: This file contains a decoder routine for GIF images
 * which is similar, structurally, to the original routine by Steve Wilhite.
 * It is, however, somewhat noticably faster in most cases.
 *
 */

static const LONG code_mask[13]=
{
	  0,
	  0x0001, 0x0003,
	  0x0007, 0x000F,
	  0x001F, 0x003F,
	  0x007F, 0x00FF,
	  0x01FF, 0x03FF,
	  0x07FF, 0x0FFF
	  };

SHORT CImageGIF::init_exp(SHORT size)
{
	curr_size = size + 1;
	top_slot = 1 << curr_size;
	clear = 1 << size;
	ending = clear + 1;
	slot = newcodes = ending + 1;
	navail_bytes = nbits_left = 0;

	memset(stack,0,MAX_CODES + 1);
	memset(prefix,0,MAX_CODES + 1);
	memset(suffix,0,MAX_CODES + 1);
	return(0);
}

/* get_next_code()
 * - gets the next code from the GIF file.  Returns the code, or else
 * a negative number in case of file errors...
 */
SHORT CImageGIF::get_next_code(FILE* file)
	{
	SHORT i, x;
	ULONG ret;

	if (nbits_left == 0)
		{
		if (navail_bytes <= 0)
			{

			/* Out of bytes in current block, so read next block
			 */
			pbytes = byte_buff;
			if ((navail_bytes = get_byte(file)) < 0)
				return(navail_bytes);
			else if (navail_bytes)
		 {
				for (i = 0; i < navail_bytes; ++i)
					{
					if ((x = get_byte(file)) < 0)
						return(x);
					byte_buff[i] = (BYTE)x;
					}
				}
			}
		b1 = *pbytes++;
		nbits_left = 8;
		--navail_bytes;
		}

	ret = b1 >> (8 - nbits_left);
	while (curr_size > nbits_left)
		{
		if (navail_bytes <= 0)
			{

			/* Out of bytes in current block, so read next block
			 */
			pbytes = byte_buff;
			if ((navail_bytes = get_byte(file)) < 0)
				return(navail_bytes);
			else if (navail_bytes)
				{
				for (i = 0; i < navail_bytes; ++i)
					{
					if ((x = get_byte(file)) < 0)
						return(x);
					byte_buff[i] = (BYTE)x;
					}
				}
			}
		b1 = *pbytes++;
		ret |= b1 << nbits_left;
		nbits_left += 8;
		--navail_bytes;
		}
	nbits_left -= curr_size;
	ret &= code_mask[curr_size];
	return((SHORT)(ret));
}



/* SHORT decoder(linewidth)
 *    SHORT linewidth;               * Pixels per line of image *
 *
 * - This function decodes an LZW image, according to the method used
 * in the GIF spec.  Every *linewidth* "characters" (ie. pixels) decoded
 * will generate a call to out_line(), which is a user specific function
 * to display a line of pixels.  The function gets it's codes from
 * get_next_code() which is responsible for reading blocks of data and
 * seperating them into the proper size codes.  Finally, get_byte() is
 * the global routine to read the next BYTE from the GIF file.
 *
 * It is generally a good idea to have linewidth correspond to the actual
 * width of a line (as specified in the Image header) to make your own
 * code a bit simpler, but it isn't absolutely necessary.
 *
 * Returns: 0 if successful, else negative.  (See ERRS.H)
 *
 */
/* bad_code_count is incremented each time an out of range code is read.
 * When this value is non-zero after a decode, your GIF file is probably
 * corrupt in some way...
 */
SHORT CImageGIF::decoder(FILE* file, CImageIterator* iter, SHORT linewidth, INT&  bad_code_count)
	{
	register BYTE *sp, *bufptr;
	BYTE *buffer;
	register SHORT code, fc, oc, bufcnt;
	SHORT c, size, ret;

	/* Initialize for decoding a new image...
	 */
	bad_code_count = 0;
	if ((size = get_byte(file)) < 0)
		return(size);
	if (size < 2 || 9 < size)
		return(BAD_CODE_SIZE);
/*   out_line = outline;*/
	init_exp(size);
//  printf("L %d %x\n",linewidth,size);

	/* Initialize in case they forgot to put in a clear code.
	 * (This shouldn't happen, but we'll try and decode it anyway...)
	 */
	oc = fc = 0;

   /* Allocate space for the decode buffer
    */
	if ((buffer = new BYTE[linewidth + 1]) == NULL)
      return(OUT_OF_MEMORY);

   /* Set up the stack pointer and decode buffer pointer
    */
   sp = stack;
   bufptr = buffer;
   bufcnt = linewidth;

   /* This is the main loop.  For each code we get we pass through the
    * linked list of prefix codes, pushing the corresponding "character" for
	 * each code onto the stack.  When the list reaches a single "character"
	 * we push that on the stack too, and then start unstacking each
    * character for output in the correct order.  Special handling is
	 * included for the clear code, and the whole thing ends when we get
    * an ending code.
    */
   while ((c = get_next_code(file)) != ending)
      {

      /* If we had a file error, return without completing the decode
       */
      if (c < 0)
         {
	 delete[] buffer;
			return(0);
         }

      /* If the code is a clear code, reinitialize all necessary items.
		 */
      if (c == clear)
         {
			curr_size = size + 1;
         slot = newcodes;
         top_slot = 1 << curr_size;

         /* Continue reading codes until we get a non-clear code
          * (Another unlikely, but possible case...)
          */
         while ((c = get_next_code(file)) == clear)
            ;

			/* If we get an ending code immediately after a clear code
          * (Yet another unlikely case), then break out of the loop.
          */
         if (c == ending)
				break;

         /* Finally, if the code is beyond the range of already set codes,
          * (This one had better NOT happen...  I have no idea what will
			 * result from this, but I doubt it will look good...) then set it
          * to color zero.
          */
         if (c >= slot)
            c = 0;

         oc = fc = c;

         /* And let us not forget to put the char into the buffer... And
			 * if, on the off chance, we were exactly one pixel from the end
          * of the line, we have to send the buffer to the out_line()
          * routine...
          */
			*bufptr++ = (BYTE)c;
         if (--bufcnt == 0)
				{
            if ((ret = out_line(iter, buffer, linewidth)) < 0)
               {
			 delete[] buffer;
               return(ret);
               }
            bufptr = buffer;
            bufcnt = linewidth;
            }
         }
      else
			{

         /* In this case, it's not a clear code or an ending code, so
          * it must be a code code...  So we can now decode the code into
			 * a stack of character codes. (Clear as mud, right?)
          */
         code = c;

         /* Here we go again with one of those off chances...  If, on the
          * off chance, the code we got is beyond the range of those already
			 * set up (Another thing which had better NOT happen...) we trick
          * the decoder into thinking it actually got the last code read.
          * (Hmmn... I'm not sure why this works...  But it does...)
          */
         if (code >= slot)
            {
            if (code > slot)
					++bad_code_count;
            code = oc;
            *sp++ = (BYTE)fc;
            }

			/* Here we scan back along the linked list of prefixes, pushing
          * helpless characters (ie. suffixes) onto the stack as we do so.
          */
			while (code >= newcodes)
            {
            *sp++ = suffix[code];
				code = prefix[code];
            }

         /* Push the last character on the stack, and set up the new
          * prefix and suffix, and if the required slot number is greater
          * than that allowed by the current bit size, increase the bit
          * size.  (NOTE - If we are all full, we *don't* save the new
          * suffix and prefix...  I'm not certain if this is correct...
          * it might be more proper to overwrite the last code...
          */
         *sp++ = (BYTE)code;
         if (slot < top_slot)
            {
            suffix[slot] = (BYTE)(fc = (BYTE)code);
            prefix[slot++] = oc;
				oc = c;
            }
         if (slot >= top_slot)
				if (curr_size < 12)
               {
               top_slot <<= 1;
               ++curr_size;
               } 

         /* Now that we've pushed the decoded string (in reverse order)
          * onto the stack, lets pop it off and put it into our decode
          * buffer...  And when the decode buffer is full, write another
          * line...
          */
         while (sp > stack)
            {
            *bufptr++ = *(--sp);
            if (--bufcnt == 0)
					{
               if ((ret = out_line(iter, buffer, linewidth)) < 0)
                  {
		  delete[] buffer;
                  return(ret);
                  }
               bufptr = buffer;
               bufcnt = linewidth;
               }
            }
         }
      }
   ret = 0;
   if (bufcnt != linewidth)
      ret = out_line(iter, buffer, (linewidth - bufcnt));
   delete[] buffer;
   return(ret);
   }
/////////////////////////////////////////////////////////////////////////////
