/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTIFFWriter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkTIFFWriter.h"
#include "vtkObjectFactory.h"


#if (_MIPS_SZLONG == 64)
typedef int vtkTiffLong;
typedef unsigned int vtkTiffUnsignedLong;
#else
typedef long vtkTiffLong;
typedef unsigned long vtkTiffUnsignedLong;
#endif

//------------------------------------------------------------------------------
vtkTIFFWriter* vtkTIFFWriter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkTIFFWriter");
  if(ret)
    {
    return (vtkTIFFWriter*)ret;
    }
  int longSize = sizeof(vtkTiffLong);
  if (longSize != 4)
    {
    vtkGenericWarningMacro ("vtkTIFFWriter expects sizeof(vtkTiffLong) to be 4,"
		     << " but sizeof (vtkTiffLong) is " << longSize);
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkTIFFWriter;
}




#ifdef write
#undef write
#endif

#ifdef close
#undef close
#endif

/*
 * Tag Image File Format (TIFF)
 *
 * Based on Rev 5.0 from:
 *    Developer's Desk
 *    Aldus Corporation
 *    411 First Ave. South
 *    Suite 200
 *    Seattle, WA  98104
 *    206-622-5500
 */
#define	TIFF_VERSION	42

#define	TIFF_BIGENDIAN		0x4d4d
#define	TIFF_LITTLEENDIAN	0x4949

typedef	struct {
	unsigned short tiff_magic;	/* magic number (defines byte order) */
	unsigned short tiff_version;	/* TIFF version number */
	vtkTiffUnsignedLong  tiff_diroff;	/* byte offset to first directory */
} TIFFHeader;

/*
 * TIFF Image File Directories are comprised of
 * a table of field descriptors of the form shown
 * below.  The table is sorted in ascending order
 * by tag.  The values associated with each entry
 * are disjoint and may appear anywhere in the file
 * (so long as they are placed on a word boundary).
 *
 * If the value is 4 bytes or less, then it is placed
 * in the offset field to save space.  If the value
 * is less than 4 bytes, it is left-justified in the
 * offset field.
 */
typedef	struct {
	unsigned short tdir_tag;	/* see below */
	unsigned short tdir_type;	/* data type; see below */
	vtkTiffUnsignedLong  tdir_count;	/* number of items; length in spec */
	vtkTiffUnsignedLong  tdir_offset;	/* byte offset to field data */
} TIFFDirEntry;

typedef	enum {
	TIFF_NOTYPE = 0,	/* placeholder */
	TIFF_BYTE = 1,		/* 8-bit unsigned integer */
	TIFF_ASCII = 2,		/* 8-bit bytes w/ last byte null */
	TIFF_SHORT = 3,		/* 16-bit unsigned integer */
	TIFF_LONG = 4,		/* 32-bit unsigned integer */
	TIFF_RATIONAL = 5	/* 64-bit fractional (numerator+denominator) */
} TIFFDataType;

/*
 * TIFF Tag Definitions.
 *
 * Those marked with a + are obsoleted by revision 5.0.
 * Those marked with a ! are proposed for revision 6.0.
 */
#define	TIFFTAG_SUBFILETYPE		254	/* subfile data descriptor */
#define	    FILETYPE_REDUCEDIMAGE	0x1	/* reduced resolution version */
#define	    FILETYPE_PAGE		0x2	/* one page of many */
#define	    FILETYPE_MASK		0x4	/* transparency mask */
#define	TIFFTAG_OSUBFILETYPE		255	/* +kind of data in subfile */
#define	    OFILETYPE_IMAGE		1	/* full resolution image data */
#define	    OFILETYPE_REDUCEDIMAGE	2	/* reduced size image data */
#define	    OFILETYPE_PAGE		3	/* one page of many */
#define	TIFFTAG_IMAGEWIDTH		256	/* image width in pixels */
#define	TIFFTAG_IMAGELENGTH		257	/* image height in pixels */
#define	TIFFTAG_BITSPERSAMPLE		258	/* bits per channel (sample) */
#define	TIFFTAG_COMPRESSION		259	/* data compression technique */
#define	    COMPRESSION_NONE		1	/* dump mode */
#define	    COMPRESSION_CCITTRLE	2	/* CCITT modified Huffman RLE */
#define	    COMPRESSION_CCITTFAX3	3	/* CCITT Group 3 fax encoding */
#define	    COMPRESSION_CCITTFAX4	4	/* CCITT Group 4 fax encoding */
#define	    COMPRESSION_LZW		5	/* Lempel-Ziv  & Welch */
#define	    COMPRESSION_NEXT		32766	/* NeXT 2-bit RLE */
#define	    COMPRESSION_CCITTRLEW	32771	/* #1 w/ word alignment */
#define	    COMPRESSION_PACKBITS	32773	/* Macintosh RLE */
#define	    COMPRESSION_THUNDERSCAN	32809	/* ThunderScan RLE */
#define	    COMPRESSION_JPEG		32865	/* JPEG compression */
#define	    COMPRESSION_PICIO		32900	/* old Pixar picio RLE */
#define	    COMPRESSION_SGIRLE		32901	/* Silicon Graphics RLE */
#define	TIFFTAG_PHOTOMETRIC		262	/* photometric interpretation */
#define	    PHOTOMETRIC_MINISWHITE	0	/* min value is white */
#define	    PHOTOMETRIC_MINISBLACK	1	/* min value is black */
#define	    PHOTOMETRIC_RGB		2	/* RGB color model */
#define	    PHOTOMETRIC_PALETTE		3	/* color map indexed */
#define	    PHOTOMETRIC_MASK		4	/* holdout mask */
#define	    PHOTOMETRIC_SEPARATED	5	/* !color separations */
#define	    PHOTOMETRIC_YCBCR		6	/* CCIR 601 */
#define	    PHOTOMETRIC_DEPTH		32768	/* z-depth data */
#define	TIFFTAG_THRESHHOLDING		263	/* +thresholding used on data */
#define	    THRESHHOLD_BILEVEL		1	/* b&w art scan */
#define	    THRESHHOLD_HALFTONE		2	/* or dithered scan */
#define	    THRESHHOLD_ERRORDIFFUSE	3	/* usually floyd-steinberg */
#define	TIFFTAG_CELLWIDTH		264	/* +dithering matrix width */
#define	TIFFTAG_CELLLENGTH		265	/* +dithering matrix height */
#define	TIFFTAG_FILLORDER		266	/* +data order within a byte */
#define	    FILLORDER_MSB2LSB		1	/* most significant -> least */
#define	    FILLORDER_LSB2MSB		2	/* least significant -> most */
#define	TIFFTAG_DOCUMENTNAME		269	/* name of doc. image is from */
#define	TIFFTAG_IMAGEDESCRIPTION	270	/* info about image */
#define	TIFFTAG_MAKE			271	/* scanner manufacturer name */
#define	TIFFTAG_MODEL			272	/* scanner model name/number */
#define	TIFFTAG_STRIPOFFSETS		273	/* offsets to data strips */
#define	TIFFTAG_ORIENTATION		274	/* +image orientation */
#define	    ORIENTATION_TOPLEFT		1	/* row 0 top, col 0 lhs */
#define	    ORIENTATION_TOPRIGHT	2	/* row 0 top, col 0 rhs */
#define	    ORIENTATION_BOTRIGHT	3	/* row 0 bottom, col 0 rhs */
#define	    ORIENTATION_BOTLEFT		4	/* row 0 bottom, col 0 lhs */
#define	    ORIENTATION_LEFTTOP		5	/* row 0 lhs, col 0 top */
#define	    ORIENTATION_RIGHTTOP	6	/* row 0 rhs, col 0 top */
#define	    ORIENTATION_RIGHTBOT	7	/* row 0 rhs, col 0 bottom */
#define	    ORIENTATION_LEFTBOT		8	/* row 0 lhs, col 0 bottom */
#define	TIFFTAG_SAMPLESPERPIXEL		277	/* samples per pixel */
#define	TIFFTAG_ROWSPERSTRIP		278	/* rows per strip of data */
#define	TIFFTAG_STRIPBYTECOUNTS		279	/* bytes counts for strips */
#define	TIFFTAG_MINSAMPLEVALUE		280	/* +minimum sample value */
#define	TIFFTAG_MAXSAMPLEVALUE		281	/* maximum sample value */
#define	TIFFTAG_XRESOLUTION		282	/* pixels/resolution in x */
#define	TIFFTAG_YRESOLUTION		283	/* pixels/resolution in y */
#define	TIFFTAG_PLANARCONFIG		284	/* storage organization */
#define	    PLANARCONFIG_CONTIG		1	/* single image plane */
#define	    PLANARCONFIG_SEPARATE	2	/* separate planes of data */
#define	TIFFTAG_PAGENAME		285	/* page name image is from */
#define	TIFFTAG_XPOSITION		286	/* x page offset of image lhs */
#define	TIFFTAG_YPOSITION		287	/* y page offset of image lhs */
#define	TIFFTAG_FREEOFFSETS		288	/* +byte offset to free block */
#define	TIFFTAG_FREEBYTECOUNTS		289	/* +sizes of free blocks */
#define	TIFFTAG_GRAYRESPONSEUNIT	290	/* gray scale curve accuracy */
#define	    GRAYRESPONSEUNIT_10S	1	/* tenths of a unit */
#define	    GRAYRESPONSEUNIT_100S	2	/* hundredths of a unit */
#define	    GRAYRESPONSEUNIT_1000S	3	/* thousandths of a unit */
#define	    GRAYRESPONSEUNIT_10000S	4	/* ten-thousandths of a unit */
#define	    GRAYRESPONSEUNIT_100000S	5	/* hundred-thousandths */
#define	TIFFTAG_GRAYRESPONSECURVE	291	/* gray scale response curve */
#define	TIFFTAG_GROUP3OPTIONS		292	/* 32 flag bits */
#define	    GROUP3OPT_2DENCODING	0x1	/* 2-dimensional coding */
#define	    GROUP3OPT_UNCOMPRESSED	0x2	/* data not compressed */
#define	    GROUP3OPT_FILLBITS		0x4	/* fill to byte boundary */
#define	TIFFTAG_GROUP4OPTIONS		293	/* 32 flag bits */
#define	    GROUP4OPT_UNCOMPRESSED	0x2	/* data not compressed */
#define	TIFFTAG_RESOLUTIONUNIT		296	/* units of resolutions */
#define	    RESUNIT_NONE		1	/* no meaningful units */
#define	    RESUNIT_INCH		2	/* english */
#define	    RESUNIT_CENTIMETER		3	/* metric */
#define	TIFFTAG_PAGENUMBER		297	/* page numbers of multi-page */
#define	TIFFTAG_COLORRESPONSEUNIT	300	/* color scale curve accuracy */
#define	    COLORRESPONSEUNIT_10S	1	/* tenths of a unit */
#define	    COLORRESPONSEUNIT_100S	2	/* hundredths of a unit */
#define	    COLORRESPONSEUNIT_1000S	3	/* thousandths of a unit */
#define	    COLORRESPONSEUNIT_10000S	4	/* ten-thousandths of a unit */
#define	    COLORRESPONSEUNIT_100000S	5	/* hundred-thousandths */
#define	TIFFTAG_COLORRESPONSECURVE	301	/* RGB response curve */
#define	TIFFTAG_SOFTWARE		305	/* name & release */
#define	TIFFTAG_DATETIME		306	/* creation date and time */
#define	TIFFTAG_ARTIST			315	/* creator of image */
#define	TIFFTAG_HOSTCOMPUTER		316	/* machine where created */
#define	TIFFTAG_PREDICTOR		317	/* prediction scheme w/ LZW */
#define	TIFFTAG_WHITEPOINT		318	/* image white point */
#define	TIFFTAG_PRIMARYCHROMATICITIES	319	/* primary chromaticities */
#define	TIFFTAG_COLORMAP		320	/* RGB map for pallette image */
#define	TIFFTAG_TILEWIDTH		322	/* !rows/data tile */
#define	TIFFTAG_TILELENGTH		323	/* !cols/data tile */
#define TIFFTAG_TILEOFFSETS		324	/* !offsets to data tiles */
#define TIFFTAG_TILEBYTECOUNTS		325	/* !byte counts for tiles */
#define	TIFFTAG_BADFAXLINES		326	/* lines w/ wrong pixel count */
#define	TIFFTAG_CLEANFAXDATA		327	/* regenerated line info */
#define	    CLEANFAXDATA_CLEAN		0	/* no errors detected */
#define	    CLEANFAXDATA_REGENERATED	1	/* receiver regenerated lines */
#define	    CLEANFAXDATA_UNCLEAN	2	/* uncorrected errors exist */
#define	TIFFTAG_CONSECUTIVEBADFAXLINES	328	/* max consecutive bad lines */
#define	TIFFTAG_INKSET			332	/* !inks in separated image */
#define	    INKSET_CMYK			1	/* cyan-magenta-yellow-black */
/* tags 32995-32999 are private tags registered to SGI */
#define	TIFFTAG_MATTEING		32995	/* alpha channel is present */
#define	TIFFTAG_DATATYPE		32996	/* how to interpret data */
#define	    DATATYPE_VOID		0	/* untyped data */
#define	    DATATYPE_INT		1	/* signed integer data */
#define	    DATATYPE_UINT		2	/* unsigned integer data */
#define	    DATATYPE_IEEEFP		3	/* IEEE floating point data */
#define	TIFFTAG_IMAGEDEPTH		32997	/* z depth of image */
#define	TIFFTAG_TILEDEPTH		32998	/* z depth/data tile */
/* tags 33603-33612 are private tags registered to C-Cube */
#define	TIFFTAG_JPEGPROC		33603	/* JPEG processing algorithm */
#define	    JPEGPROC_BASELINE		0	/* baseline algorithm */
#define	TIFFTAG_JPEGQTABLEPREC		33605	/* quant. table precision */
#define	    JPEGQTABLEPREC_8BIT		0	/* 8-bit precision */
#define	    JPEGQTABLEPREC_16BIT	1	/* 16-bit precision */
#define	TIFFTAG_JPEGQTABLES		33606	/* offsets to Q matrices */
#define	TIFFTAG_JPEGDCTABLES		33607	/* offsets to DCT tables */
#define	TIFFTAG_JPEGACTABLES		33608	/* offsets to AC coefficients */
#define	TIFFTAG_LUMACOEFS		33611	/* RGB -> YCbCr transform */
#define	    LUMACOEFS_CCIR601		0	/* CCIR recommendation 601-1 */
#define	    LUMACOEFS_SMPTE		1	/* SMPTE standard 240M-1988 */
#define	    LUMACOEFS_CCIR709		2	/* CCIR recommendation 709 */
#define	TIFFTAG_YCBCRSAMPLING		33612	/* YCbCr encoding parameters */
#define TIFFTAG_EXTRASAMPLES            338     /* Extra Samples */
#define     EXTRASAMPLES_UNSPECIFIEDDATA 0
#define     EXTRASAMPLES_ASSOCIATEDALPHA 1
#define     EXTRASAMPLES_UNASSOCIATEDALPHA 2

void vtkTIFFWriter::WriteFileHeader(ofstream *file, vtkImageData *cache)
{
  int min0, max0, min1, max1, min2, max2;
  int bpp, i;
  TIFFHeader myHeader;
  TIFFDirEntry myDir;
  int longShift;
  int width, height;
  vtkTiffLong nextIFDOffset;
  short numDirEntries;
  vtkTiffUnsignedLong offset;
  
  // Find the length of the rows to write.
  cache->GetWholeExtent(min0, max0, min1, max1, min2, max2);
  bpp = cache->GetNumberOfScalarComponents();
  width = (max0 - min0 + 1);
  height = (max1 - min1 + 1);
  
  // fill the header and write it
#ifdef VTK_WORDS_BIGENDIAN
  myHeader.tiff_magic = TIFF_BIGENDIAN;
  longShift = 16;
#else
  myHeader.tiff_magic = TIFF_LITTLEENDIAN;
  longShift = 0;
#endif
  
  myHeader.tiff_version = TIFF_VERSION;
  myHeader.tiff_diroff = sizeof (TIFFHeader);
  file->write((char *)&myHeader, sizeof (myHeader));

  // fill the image file directory
  if (bpp == 1 || bpp == 3) 
    { 
    numDirEntries = 15; 
    }
  else
    {
    numDirEntries = 16;
    }
  
  file->write((char *)&numDirEntries, sizeof (numDirEntries));
  offset = (vtkTiffUnsignedLong)
    (sizeof(myHeader) + sizeof(numDirEntries) + 
     numDirEntries*sizeof(myDir) + sizeof(nextIFDOffset));

  vtkTiffLong newSubfileType = 0;
  myDir.tdir_tag = TIFFTAG_SUBFILETYPE;
  myDir.tdir_type = TIFF_LONG;
  myDir.tdir_count = 1;
  myDir.tdir_offset = newSubfileType;
  file->write((char *)&myDir, sizeof (myDir));
  
  myDir.tdir_tag = TIFFTAG_IMAGEWIDTH;
  myDir.tdir_type = TIFF_LONG;
  myDir.tdir_count = 1;
  myDir.tdir_offset = width;
  file->write((char *)&myDir, sizeof (myDir));
  
  myDir.tdir_tag = TIFFTAG_IMAGELENGTH;
  myDir.tdir_type = TIFF_LONG;
  myDir.tdir_count = 1;
  myDir.tdir_offset = height;
  file->write((char *)&myDir, sizeof (myDir));
  
  if (bpp < 3)
    {
    myDir.tdir_tag = TIFFTAG_BITSPERSAMPLE;
    myDir.tdir_type = TIFF_SHORT;
    myDir.tdir_count = bpp;
    if (bpp == 1)
      {
      short depth = 8;
      myDir.tdir_offset = (vtkTiffLong) depth << longShift;
      }
    else
      {
      vtkTiffUnsignedLong depth = 8 + (8 << 16);
      myDir.tdir_offset = depth;
      }
    file->write((char *)&myDir, sizeof (myDir));
    }
  else
    {
    myDir.tdir_tag = TIFFTAG_BITSPERSAMPLE;
    myDir.tdir_type = TIFF_SHORT;
    myDir.tdir_count = bpp;
    myDir.tdir_offset = offset;
    file->write((char *)&myDir, sizeof (myDir));
    }
  
  short compression = COMPRESSION_NONE;
  myDir.tdir_tag = TIFFTAG_COMPRESSION;
  myDir.tdir_type = TIFF_SHORT;
  myDir.tdir_count = 1;
  myDir.tdir_offset = (vtkTiffLong) compression << longShift;
  file->write((char *)&myDir, sizeof (myDir));
  
  short photometricInterpretation = PHOTOMETRIC_RGB;
  if (bpp == 1 || bpp == 2)
    {
    photometricInterpretation = PHOTOMETRIC_MINISBLACK;
    }
  myDir.tdir_tag = TIFFTAG_PHOTOMETRIC;
  myDir.tdir_type = TIFF_SHORT;
  myDir.tdir_count = 1;
  myDir.tdir_offset = (vtkTiffLong) photometricInterpretation << longShift;
  file->write((char *)&myDir, sizeof (myDir));
  
  myDir.tdir_tag = TIFFTAG_STRIPOFFSETS;
  myDir.tdir_type = TIFF_LONG;
  myDir.tdir_count = 1;
  myDir.tdir_offset = (vtkTiffUnsignedLong) (offset + bpp*sizeof(short) + 4*4); // TIFF_LONG is 4 bytes
  file->write((char *)&myDir, sizeof (myDir));
  
  vtkTiffLong orientation = 1;
  myDir.tdir_tag = TIFFTAG_ORIENTATION;
  myDir.tdir_type = TIFF_SHORT;
  myDir.tdir_count = 1;
  myDir.tdir_offset = (vtkTiffLong) orientation << longShift;
  file->write((char *)&myDir, sizeof (myDir));
  
  short samplesPerPixel = bpp;
  myDir.tdir_tag = TIFFTAG_SAMPLESPERPIXEL;
  myDir.tdir_type = TIFF_SHORT;
  myDir.tdir_count = 1;
  myDir.tdir_offset = (vtkTiffLong) samplesPerPixel << longShift;
  file->write((char *)&myDir, sizeof (myDir));

  vtkTiffLong rowsPerStrip = height;
  myDir.tdir_tag = TIFFTAG_ROWSPERSTRIP;
  myDir.tdir_type = TIFF_LONG;
  myDir.tdir_count = 1;
  myDir.tdir_offset = rowsPerStrip;
  file->write((char *)&myDir, sizeof (myDir));
  
  vtkTiffLong stripByteCounts = width*height*bpp;
  myDir.tdir_tag = TIFFTAG_STRIPBYTECOUNTS;
  myDir.tdir_type = TIFF_LONG;
  myDir.tdir_count = 1;
  myDir.tdir_offset = stripByteCounts;
  file->write((char *)&myDir, sizeof (myDir));
  
  myDir.tdir_tag = TIFFTAG_XRESOLUTION;
  myDir.tdir_type = TIFF_RATIONAL;
  myDir.tdir_count = 1;
  myDir.tdir_offset = (vtkTiffUnsignedLong) (offset + bpp*sizeof(short));
  file->write((char *)&myDir, sizeof (myDir));
  
  myDir.tdir_tag = TIFFTAG_YRESOLUTION;
  myDir.tdir_type = TIFF_RATIONAL;
  myDir.tdir_count = 1;
  myDir.tdir_offset = (vtkTiffUnsignedLong) (offset + bpp*sizeof(short) + 2*4); // 32bits 
  file->write((char *)&myDir, sizeof (myDir));
  
  short planarConfiguration = PLANARCONFIG_CONTIG;
  myDir.tdir_tag = TIFFTAG_PLANARCONFIG;
  myDir.tdir_type = TIFF_SHORT;
  myDir.tdir_count = 1;
  myDir.tdir_offset = (vtkTiffLong) planarConfiguration << longShift;
  file->write((char *)&myDir, sizeof (myDir));
  
  short resolutionUnit = RESUNIT_NONE;
  myDir.tdir_tag = TIFFTAG_RESOLUTIONUNIT;
  myDir.tdir_type = TIFF_SHORT;
  myDir.tdir_count = 1;
  myDir.tdir_offset = (vtkTiffLong) resolutionUnit << longShift;
  file->write((char *)&myDir, sizeof (myDir));
  
  if (bpp == 2 || bpp == 4)
    {
    short extraSamples = EXTRASAMPLES_ASSOCIATEDALPHA;
    myDir.tdir_tag = TIFFTAG_EXTRASAMPLES;
    myDir.tdir_type = TIFF_SHORT;
    myDir.tdir_count = 1;
    myDir.tdir_offset = (vtkTiffLong) extraSamples << longShift;
    file->write((char *)&myDir, sizeof (myDir));
    }
  
  // end of image file directory
  nextIFDOffset = 0;
  file->write((char *)&nextIFDOffset, sizeof (nextIFDOffset));
  
  // store data for tags here
  short depth = 8;
  for (i = 0; i < bpp; i++)
    {
    file->write((char *)&depth, sizeof (depth));
    }
  
  vtkTiffLong xResolution[2] = {1, 1};
  file->write((char *)&xResolution, sizeof (xResolution));
  
  vtkTiffLong yResolution[2] = {1, 1};
  file->write((char *)&yResolution, sizeof (yResolution));
}


void vtkTIFFWriter::WriteFile(ofstream *file, vtkImageData *data,
			      int extent[6])
{
  int idx1, idx2;
  int rowLength; // in bytes
  void *ptr;
  
  // Make sure we actually have data.
  if ( !data->GetPointData()->GetScalars())
    {
    vtkErrorMacro(<< "Could not get data from input.");
    return;
    }

  // take into consideration the scalar type
  switch (data->GetScalarType())
    {
    case VTK_UNSIGNED_CHAR:
      rowLength = sizeof(unsigned char); 
      break;
    default:
      vtkErrorMacro("TIFFWriter only accepts unsigned char scalars!");
      return; 
    }
  rowLength *= data->GetNumberOfScalarComponents();
  rowLength *= (extent[1] - extent[0] + 1);
    
  for (idx2 = extent[4]; idx2 <= extent[5]; ++idx2)
    {
    for (idx1 = extent[3]; idx1 >= extent[2]; idx1--)
      {
      ptr = data->GetScalarPointer(extent[0], idx1, idx2);
      if ( ! file->write((char *)ptr, rowLength))
	{
	vtkErrorMacro("WriteFile: write failed");
	file->close();
	delete file;
	}
      }
    }
}
