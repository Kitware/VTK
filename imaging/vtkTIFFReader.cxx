/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTIFFReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/

#include "vtkTIFFReader.h"
#include "vtkByteSwap.h"
#include <stdio.h>

void vtkTIFFReader::Swap2(short *stmp)
{
  if (this->SwapBytes)
    {
    vtkByteSwap::SwapVoidRange(stmp, 1, 2);
    }
}
void vtkTIFFReader::Swap4(long *stmp)
{
  if (this->SwapBytes)
    {
    vtkByteSwap::SwapVoidRange(stmp, 1, 4);
    }
}

void vtkTIFFReader::ReadTag(_vtkTifTag *tag, FILE *fp)
{
  short stmp;
  long ltmp;
  
  fread(&stmp,sizeof(short),1,fp);
  this->Swap2(&stmp);
  tag->TagId = stmp;

  fread(&stmp,sizeof(short),1,fp);
  this->Swap2(&stmp);
  tag->DataType = stmp;
  
  fread(&ltmp,sizeof(long),1,fp);
  this->Swap4(&ltmp);
  tag->DataCount = ltmp;

  fread(&ltmp,sizeof(long),1,fp);
  tag->DataOffset = ltmp;
}

long vtkTIFFReader::ReadTagLong(_vtkTifTag *tag, FILE *fp)
{
  long result;
  long curPos;
  
  // if the tag is an offset just return the first value
  if (((tag->DataCount > 1)&&(tag->DataType == 4))||
      ((tag->DataCount > 2)&&(tag->DataType == 3))||
      ((tag->DataCount > 4)&&(tag->DataType == 1)))
    {
    // jump to offset and read the first value
    curPos = ftell(fp);
    // swap offset as 4 byte
    this->Swap4(&tag->DataOffset);
    fseek(fp, tag->DataOffset, SEEK_SET);
    fread(&result,sizeof(long),1,fp);
    switch (tag->DataType) 
      {
      case 1: result = *((unsigned char *)&(result)); break;
      case 3:
	this->Swap2((short *)(&result));
	result = *((short *)&(result)); 
	break;
      case 4:     
	this->Swap4(&result);
        result = (long)(result); 
	break;
      default: vtkGenericWarningMacro("Bad data in tag!");
      }
    fseek(fp, curPos, SEEK_SET);
    }
  else
    {
    result = tag->DataOffset;
    switch (tag->DataType) 
      {
      case 1: result = *((unsigned char *)&(result)); break;
      case 3:
	this->Swap2((short *)(&result));
	result = *((short *)&(result)); 
	break;
      case 4:     
	this->Swap4(&result);
        result = (long)(result); 
	break;
      default: vtkGenericWarningMacro("Bad data in tag!");
      }
    }
  
  return result;
}

void vtkTIFFReader::UpdateImageInformation()
{
  int xsize, ysize;
  FILE *fp;
  short stmp;
  long IFDOffset;
  _vtkTifTag aTag;
  int i;
  short numTags;
  long ltmp;
  int numComp, bpp;
  int numSlices = 1;
  
  // if the user has not set the extent, but has set the VOI
  // set the zaxis extent to the VOI z axis
  if (this->DataExtent[4]==0 && this->DataExtent[5] == 0 &&
      (this->DataVOI[4] || this->DataVOI[5]))
    {
    this->DataExtent[4] = this->DataVOI[4];
    this->DataExtent[5] = this->DataVOI[5];
    }

  // Allocate the space for the filename
  this->ComputeInternalFileName(this->DataExtent[4]);

  // get the magic number by reading in a file
  fp = fopen(this->InternalFileName,"rb");
  if (!fp)
    {
    vtkErrorMacro("Unable to open file " << this->InternalFileName);
    return;
    }

  // compare magic number to determine file type
  stmp = fgetc(fp);
  if ((fgetc(fp) != stmp)||((stmp != 'I')&&(stmp != 'M')))
    {
    vtkErrorMacro(<<"Unknown file type! Not a TIFF file!");
    fclose(fp);
    return;
    }
  
  // what is the byte order
  if (stmp == 'I')
    {
    this->SetDataByteOrderToLittleEndian();
    }
  else
    {
    this->SetDataByteOrderToBigEndian();
    }
  // default is upper left
  this->FileLowerLeft = 0;

  // check the version word
  fread(&stmp,sizeof(short),1,fp);
  this->Swap2(&stmp);
  if (stmp != 42)
    {
    vtkErrorMacro(<<"Unknown file type! Not a TIFF file!");
    fclose(fp);
    return;
    }
  
  // get the offset to the image file directory
  fread(&IFDOffset,sizeof(long),1,fp);
  this->Swap4(&IFDOffset);

  
  // now we need to read the IFD to get the additional info we need
  // seek to the first IFD
  fseek(fp, IFDOffset, SEEK_SET);
  
  // how many tags are there
  fread(&numTags,sizeof(short),1,fp);
  this->Swap2(&numTags);
  vtkDebugMacro("The IFD contains " << numTags << " tags.");

  // read the tags and act on it  NOTE: we could handle the Xresolution and Yresolution
  // tags and put them into the spacing ivar, but they are almost always 1.0 so
  // for now I'll skip it.
  for (i = 0; i < numTags; i++)
    {
    this->ReadTag(&aTag,fp);
    switch (aTag.TagId) 
      {
      case 256: 
	xsize = this->ReadTagLong(&aTag,fp);
	break;
      case 257:
	ysize = this->ReadTagLong(&aTag,fp);
	break;
      case 258:
	bpp = (int)this->ReadTagLong(&aTag,fp);
	if ((bpp != 8)&&(bpp != 16))
	  {
	  vtkWarningMacro(" vtkTIFFReader only supports 8 and 16 bits per sample!");
	  }
	break;
      case 259:
	ltmp = this->ReadTagLong(&aTag,fp);
	if ((ltmp != 1)&&(ltmp != 32771))
	  {
	  vtkWarningMacro(" vtkTIFFReader does not support compressed TIFF images!");
	  }
	break;
      case 273:
	ltmp = this->ReadTagLong(&aTag,fp);
	this->SetHeaderSize(ltmp);
	break;
      case 274:
	// is corner in upper left or lower left, the default is upper left
	ltmp = this->ReadTagLong(&aTag,fp);
	if (ltmp == 4)
	  {
	  this->FileLowerLeft = 1;
	  }
	break;
      case 277:
	numComp = (int)this->ReadTagLong(&aTag,fp);
	break;
      case 278:
	ltmp = this->ReadTagLong(&aTag,fp);
	if (ltmp != ysize)
	  {
	  vtkWarningMacro(" vtkTIFFReader only supports one strip!");
	  }
	break;
      case 284:
	ltmp = this->ReadTagLong(&aTag,fp);
	if (ltmp != 1)
	  {
	  vtkWarningMacro(" vtkTIFFReader requires planar contiguous images!");
	  }
	break;
      case 297:
	// logic 700 stores volume of data, need to find out how big
	if (aTag.DataCount == 2)
	  {
	  this->Swap2(((short *)(&aTag.DataOffset))+1);
	  numSlices = *(((short *)&aTag.DataOffset)+1); 
	  }
	break;
      }
    }
  
  fclose(fp);
  
  // if the user has set the VOI, just make sure its valid
  if (this->DataVOI[0] || this->DataVOI[1] || 
      this->DataVOI[2] || this->DataVOI[3] ||
      this->DataVOI[4] || this->DataVOI[5])
    { 
    if ((this->DataVOI[0] < 0) ||
	(this->DataVOI[1] >= xsize) ||
	(this->DataVOI[2] < 0) ||
	(this->DataVOI[3] >= ysize))
      {
      vtkWarningMacro("The requested VOI is larger than the file's (" << this->InternalFileName << ") extent ");
      this->DataVOI[0] = 0;
      this->DataVOI[1] = xsize - 1;
      this->DataVOI[2] = 0;
      this->DataVOI[3] = ysize - 1;
      }
    }

  this->DataExtent[0] = 0;
  this->DataExtent[1] = xsize - 1;
  this->DataExtent[2] = 0;
  this->DataExtent[3] = ysize - 1;
  
  // if this is a volumetric TIFF then use numSlices
  if (numSlices > 1)
    {
    this->DataExtent[4] = 0;
    this->DataExtent[5] = numSlices - 1;
    this->SetFileDimensionality(3);
    }
  
  if (bpp == 8)
    {
    this->SetDataScalarTypeToUnsignedChar();
    }
  else
    {
    this->SetDataScalarTypeToUnsignedShort();
    }
  
  this->SetNumberOfScalarComponents(numComp);
  
  vtkImageReader::UpdateImageInformation();
}

