/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGESignaReader.cxx
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
#include "vtkGESignaReader.h"
#include "vtkByteSwap.h"
#include "vtkObjectFactory.h"

//-------------------------------------------------------------------------
vtkGESignaReader* vtkGESignaReader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkGESignaReader");
  if(ret)
    {
    return (vtkGESignaReader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkGESignaReader;
}


void vtkGESignaReader::ExecuteInformation()
{
  this->ComputeInternalFileName(this->DataExtent[4]);
  if (this->InternalFileName == NULL)
    {
    return;
    }

  FILE *fp = fopen(this->InternalFileName, "rb");
  if (!fp)
    {
    vtkErrorMacro("Unable to open file " << this->InternalFileName);
    return;
    }

  int magic;
  fread(&magic, 4, 1, fp);
  vtkByteSwap::Swap4BE(&magic);
  
  if (magic != 0x494d4746)
    {
    vtkErrorMacro(<<"Unknown file type! Not a GE ximg file!");
    fclose(fp);
    return;
    }

  // read in the pixel offset from the header
  int offset;
  fread(&offset, 4, 1, fp);
  vtkByteSwap::Swap4BE(&offset);
  this->SetHeaderSize(offset);

  int width, height, depth;
  fread(&width, 4, 1, fp);
  vtkByteSwap::Swap4BE(&width);
  fread(&height, 4, 1, fp);
  vtkByteSwap::Swap4BE(&height);
  // depth in bits
  fread(&depth, 4, 1, fp);
  vtkByteSwap::Swap4BE(&depth);

  int compression;
  fread(&compression, 4, 1, fp);
  vtkByteSwap::Swap4BE(&compression);

  // seek to the image header offsets
  fseek(fp, 148, SEEK_SET);
  int imgHdrOffset;
  fread(&imgHdrOffset, 4, 1, fp);
  vtkByteSwap::Swap4BE(&imgHdrOffset);

  // now seek to the image header and read some values
  fseek(fp, imgHdrOffset + 26, SEEK_SET);
  
  float spacingX, spacingY, spacingZ;
  fread(&spacingZ, 4, 1, fp);
  vtkByteSwap::Swap4BE(&spacingZ);
  fseek(fp, imgHdrOffset + 50, SEEK_SET);
  fread(&spacingX, 4, 1, fp);
  vtkByteSwap::Swap4BE(&spacingX);
  fread(&spacingY, 4, 1, fp);
  vtkByteSwap::Swap4BE(&spacingY);

  this->DataExtent[0] = 0;
  this->DataExtent[1] = width - 1;
  this->DataExtent[2] = 0;
  this->DataExtent[3] = height - 1;

  this->SetDataScalarTypeToUnsignedShort();

  this->SetNumberOfScalarComponents(1);
  this->SetDataSpacing(spacingX, spacingY, spacingZ);
  this->vtkImageReader2::ExecuteInformation();

  // close the file
  fclose(fp);
}

namespace {
void copygenesisimage(FILE *infp, int width, int height, int compress,
		      short *map_left, short *map_wide,
		      unsigned short *output)
{
  unsigned short row;
  unsigned short last_pixel=0;
  for (row=0; row<height; ++row) 
    {
      unsigned short j;
      unsigned short start;
      unsigned short end;
      
      if (compress == 2 || compress == 4) 
	{ // packed/compacked
	  start=map_left[row];
	  end=start+map_wide[row];
	}
      else 
	{
	  start=0;
	  end=width;
	}
      // Pad the first "empty" part of the line ...
      for (j=0; j<start; j++) 
	{
	  (*output) = 0;
	  ++output;
	}

      if (compress == 3 || compress == 4) 
	{ // compressed/compacked
	  while (start<end) 
	    {
	      unsigned char byte;
	      if (!fread(&byte,1,1,infp))
		{
		  return;
		}
	      if (byte & 0x80) 
		{
		  unsigned char byte2;
		  if (!fread(&byte2,1,1,infp))
		    {
		      return;
		    }
		  if (byte & 0x40) 
		    {      // next word
		      if (!fread(&byte,1,1,infp))
			{
			  return;
			}
		      last_pixel=
			(((unsigned short)byte2<<8)+byte);
		    }
		  else 
		    {                  // 14 bit delta
		      if (byte & 0x20) 
			{
			  byte|=0xe0;
			}
		      else 
			{
			  byte&=0x1f;
			}
		      last_pixel+=
			(((short)byte<<8)+byte2);
		    }
		}
	      else 
		{                          // 7 bit delta
		  if (byte & 0x40) 
		    {
		      byte|=0xc0;
		    }
		  last_pixel+=(signed char)byte;
		}
	      (*output) = last_pixel;
	      ++output;
	      ++start;
	    }
	}
      else 
	{
	  while (start<end) 
	    {
	      unsigned short u;
	      if (!fread(&u,2,1,infp))
		{
		  return;
		}
	      vtkByteSwap::Swap2BE(&u);
	      (*output) = u;
	      ++output;
	      ++start;
	    }
	}
      
      // Pad the last "empty" part of the line ...
      for (j=end; j<width; j++) 
	{
	  (*output) = 0;
	  ++output;
	}
    }
}
} // end anonymous namespace


static void vtkGESignaReaderUpdate2(vtkGESignaReader *self, 
				    unsigned short *outPtr, int *outExt, 
				    int *outInc)
{
  FILE *fp = fopen(self->GetInternalFileName(), "rb");
  if (!fp)
    {
    return;
    }

  int magic;
  fread(&magic, 4, 1, fp);
  vtkByteSwap::Swap4BE(&magic);
  
  if (magic != 0x494d4746)
    {
    vtkGenericWarningMacro(<<"Unknown file type! Not a GE ximg file!");
    fclose(fp);
    return;
    }

  // read in the pixel offset from the header
  int offset;
  fread(&offset, 4, 1, fp);
  vtkByteSwap::Swap4BE(&offset);

  int width, height, depth;
  fread(&width, 4, 1, fp);
  vtkByteSwap::Swap4BE(&width);
  fread(&height, 4, 1, fp);
  vtkByteSwap::Swap4BE(&height);
  // depth in bits
  fread(&depth, 4, 1, fp);
  vtkByteSwap::Swap4BE(&depth);

  int compression;
  fread(&compression, 4, 1, fp);
  vtkByteSwap::Swap4BE(&compression);

  short *leftMap = 0;
  short *widthMap = 0;

  if (compression == 2 || compression == 4) 
    { // packed/compacked
      leftMap = new short [height];
      widthMap = new short [height];

      fseek(fp, 64, SEEK_SET);
      int packHdrOffset;
      fread(&packHdrOffset, 4, 1, fp);
      vtkByteSwap::Swap4BE(&packHdrOffset);
      
      // now seek to the pack header and read some values
      fseek(fp, packHdrOffset, SEEK_SET);
      // read in the maps
      int i;
      for (i = 0; i < height; i++)
	{
	  fread(leftMap+i, 2, 1, fp);
	  vtkByteSwap::Swap2BE(leftMap+i);
	  fread(widthMap+i, 2, 1, fp);
	  vtkByteSwap::Swap2BE(widthMap+i);
	}
    }

  // seek to pixel data
  fseek(fp, offset, SEEK_SET);

  // read in the pixels
  unsigned short *tmp = new unsigned short [width*height];
  int *dext = self->GetDataExtent();
  copygenesisimage(fp, dext[1] + 1, dext[3] + 1, 
		   compression, leftMap, widthMap, tmp);

  // now copy into desired extent
  int yp;
  for (yp = outExt[2]; yp <= outExt[3]; ++yp)
    {
      int ymod = height - yp - 1;
      memcpy(outPtr,tmp+ymod*width+outExt[0],2*width);
      outPtr = outPtr + width;
    }

  delete [] tmp;
  if (leftMap)
    {
      delete [] leftMap;
    }
  if (widthMap)
    {
      delete [] widthMap;
    }
  fclose(fp);
}

//----------------------------------------------------------------------------
// This function reads in one data of data.
// templated to handle different data types.
static void vtkGESignaReaderUpdate(vtkGESignaReader *self, vtkImageData *data, 
				   unsigned short *outPtr)
{
  int outIncr[3];
  int outExtent[6];
  unsigned short *outPtr2;

  data->GetExtent(outExtent);
  data->GetIncrements(outIncr);

  outPtr2 = outPtr;
  int idx2;
  for (idx2 = outExtent[4]; idx2 <= outExtent[5]; ++idx2)
    {
    self->ComputeInternalFileName(idx2);
    // read in a PNG file
    vtkGESignaReaderUpdate2(self, outPtr2, outExtent, outIncr);
    self->UpdateProgress((idx2 - outExtent[4])/
                         (outExtent[5] - outExtent[4] + 1.0));
    outPtr2 += outIncr[2];
    }
}


//----------------------------------------------------------------------------
// This function reads a data from a file.  The datas extent/axes
// are assumed to be the same as the file extent/order.
void vtkGESignaReader::ExecuteData(vtkDataObject *output)
{
  vtkImageData *data = this->AllocateOutputData(output);

  if (this->InternalFileName == NULL)
    {
    vtkErrorMacro(<< "Either a FileName or FilePrefix must be specified.");
    return;
    }

  this->ComputeDataIncrements();
  
  // Call the correct templated function for the output
  void *outPtr;

  // Call the correct templated function for the input
  outPtr = data->GetScalarPointer();
  vtkGESignaReaderUpdate(this, data, (unsigned short *)(outPtr));
}


