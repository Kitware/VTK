/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBMPReader.cxx
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

#include "vtkBMPReader.h"
#include "vtkByteSwap.h"
#include <stdio.h>

vtkBMPReader::vtkBMPReader()
{
  this->Colors = NULL;
  this->SetDataByteOrderToLittleEndian();
}

//----------------------------------------------------------------------------
vtkBMPReader::~vtkBMPReader()
{ 
  // free any old memory
  if (this->Colors) 
    {
    delete this->Colors;
    this->Colors = NULL;
    }
}

void vtkBMPReader::UpdateImageInformation()
{
  int xsize, ysize;
  FILE *fp;
  long tmp;
  short stmp;
  long infoSize;
  
  // free any old memory
  if (this->Colors) 
    {
    delete this->Colors;
    this->Colors = NULL;
    }
      
  // if the user has not set the extent, but has set the VOI
  // set the zaxis extent to the VOI z axis
  if (this->DataExtent[4]==0 && this->DataExtent[5] == 0 &&
      (this->DataVOI[4] || this->DataVOI[5]))
    {
    this->DataExtent[4] = this->DataVOI[4];
    this->DataExtent[5] = this->DataVOI[5];
    }

  this->ComputeInternalFileName(this->DataExtent[4]);
  
  // get the magic number by reading in a file
  fp = fopen(this->InternalFileName,"rb");
  if (!fp)
    {
    vtkErrorMacro("Unable to open file " << this->InternalFileName);
    return;
    }

  // compare magic number to determine file type
  if ((fgetc(fp) != 'B')||(fgetc(fp) != 'M'))
    {
    vtkErrorMacro(<<"Unknown file type! Not a Windows BMP file!");
    fclose(fp);
    return;
    }

  // get the size of the file
  fread(&tmp,sizeof(long),1,fp);
  // skip 4 bytes
  fread(&tmp,sizeof(long),1,fp);
  // read the offset
  fread(&tmp,sizeof(long),1,fp);

  // get size of header
  fread(&infoSize,sizeof(long),1,fp);
  vtkByteSwap::Swap4LE(&infoSize);
		       
  // error checking
  if ((infoSize != 40)&&(infoSize != 12))
    {
    vtkErrorMacro(<<"Unknown file type! Not a Windows BMP file!");
    fclose(fp);
    return;
    }
  
  // there are two different types of BMP files
  if (infoSize == 40)
    {
    // now get the dimensions
    fread(&xsize,sizeof(long),1,fp);
    vtkByteSwap::Swap4LE(&xsize);
    fread(&ysize,sizeof(long),1,fp);
    vtkByteSwap::Swap4LE(&ysize);
    }
  else
    {
    fread(&stmp,sizeof(short),1,fp);
    vtkByteSwap::Swap2LE(&stmp);
    xsize = stmp;
    fread(&stmp,sizeof(short),1,fp);
    vtkByteSwap::Swap2LE(&stmp);
    ysize = stmp;
    }
  
  // is corner in upper left or lower left
  if (ysize < 0)
    {
    ysize = ysize*-1;
    this->FileLowerLeft = 0;
    }
  else
    {
    this->FileLowerLeft = 1;
    }
    
  // ignore planes
  fread(&stmp,sizeof(short),1,fp);
  // read depth
  fread(&this->Depth,sizeof(short),1,fp);
  vtkByteSwap::Swap2LE(&this->Depth);
  if ((this->Depth != 8)&&(this->Depth != 24))
    {
    vtkErrorMacro(<<"Only BMP depths of (8,24) are supported. Not " << this->Depth);
    fclose(fp);
    return;
    }
  
  // skip over rest of info for long format
  if (infoSize == 40)
    {
    fread(&tmp,4,1,fp);
    fread(&tmp,4,1,fp);
    fread(&tmp,4,1,fp);
    fread(&tmp,4,1,fp);
    fread(&tmp,4,1,fp);
    fread(&tmp,4,1,fp);
    }
  
  // read in color table if required
  if (this->Depth < 24)
    {
    int numColors = 256;
    this->Colors = new unsigned char [numColors*3];
    for (tmp = 0; tmp < numColors; tmp++)
      {
      this->Colors[tmp*3+2] = fgetc(fp);
      this->Colors[tmp*3+1] = fgetc(fp);
      this->Colors[tmp*3] = fgetc(fp);
      if (infoSize == 40) fgetc(fp);
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
  
  this->SetDataScalarTypeToUnsignedChar();
  this->SetNumberOfScalarComponents(3);
  
  vtkImageReader::UpdateImageInformation();
}

//----------------------------------------------------------------------------
// Description:
// This function opens a file to determine the file size, and to
// automatically determine the header size.
void vtkBMPReader::ComputeDataIncrements()
{
  int idx;
  int fileDataLength;
  
  // Determine the expected length of the data ...
  switch (this->DataScalarType)
    {
    case VTK_FLOAT:
      fileDataLength = sizeof(float);
      break;
    case VTK_INT:
      fileDataLength = sizeof(int);
      break;
    case VTK_SHORT:
      fileDataLength = sizeof(short);
      break;
    case VTK_UNSIGNED_SHORT:
      fileDataLength = sizeof(unsigned short);
      break;
    case VTK_UNSIGNED_CHAR:
      fileDataLength = sizeof(unsigned char);
      break;
    default:
      vtkErrorMacro(<< "Unknown DataScalarType");
      return;
    }
  
  fileDataLength *= (this->Depth/8);
  
  // a row must end on a 4 byte boundary
  // so update the Increments[1]
  this->DataIncrements[0] = fileDataLength;
  fileDataLength = fileDataLength *
    (this->DataExtent[1] - this->DataExtent[0] + 1);
  // move to 4 byte boundary
  fileDataLength = fileDataLength + (4 - fileDataLength%4)%4;
  
  // compute the fileDataLength (in units of bytes)
  for (idx = 1; idx < 3; ++idx)
    {
    this->DataIncrements[idx] = fileDataLength;
    fileDataLength = fileDataLength *
      (this->DataExtent[idx*2+1] - this->DataExtent[idx*2] + 1);
    }
}

//----------------------------------------------------------------------------
// Description:
// This function reads in one data of data.
// templated to handle different data types.
template <class OT>
static void vtkBMPReaderUpdate2(vtkBMPReader *self, vtkImageData *data, 
				OT *outPtr)
{
  int inIncr[3], outIncr[3];
  OT *outPtr0, *outPtr1, *outPtr2;
  long streamSkip0, streamSkip1;
  long streamRead;
  int idx0, idx1, idx2, pixelRead;
  unsigned char *buf;
  int inExtent[6];
  int dataExtent[6];
  int pixelSkip;
  unsigned char *inPtr;
  
  // Get the requested extents.
  data->GetExtent(inExtent);
  // Convert them into to the extent needed from the file. 
  self->ComputeInverseTransformedExtent(inExtent,dataExtent);
  
  // get and transform the increments
  data->GetIncrements(inIncr);
  self->ComputeInverseTransformedIncrements(inIncr,outIncr);

  // compute outPtr2 
  outPtr2 = outPtr;
  if (outIncr[0] < 0) 
    outPtr2 = outPtr2 - outIncr[0]*(dataExtent[1] - dataExtent[0]);
  if (outIncr[1] < 0) 
    outPtr2 = outPtr2 - outIncr[1]*(dataExtent[3] - dataExtent[2]);
  if (outIncr[2] < 0) 
    outPtr2 = outPtr2 - outIncr[2]*(dataExtent[5] - dataExtent[4]);

  // length of a row, num pixels read at a time
  pixelRead = dataExtent[1] - dataExtent[0] + 1; 
  streamRead = pixelRead * self->DataIncrements[0];  
  streamSkip0 = self->DataIncrements[1] - streamRead;
  streamSkip1 = self->DataIncrements[2] - 
    (dataExtent[3] - dataExtent[2] + 1)* self->DataIncrements[1];
  pixelSkip = self->Depth/8;
    
  // read from the bottom up
  if (!self->GetFileLowerLeft()) 
    streamSkip0 = -streamRead - self->DataIncrements[1];
  
  // create a buffer to hold a row of the data
  buf = new unsigned char[streamRead];
  
  // read the data row by row
  if (self->GetFileDimensionality() == 3)
    {
    self->OpenAndSeekFile(dataExtent,0);
    }
  for (idx2 = dataExtent[4]; idx2 <= dataExtent[5]; ++idx2)
    {
    if (self->GetFileDimensionality() == 2)
      {
      self->OpenAndSeekFile(dataExtent,idx2);
      }
    outPtr1 = outPtr2;
    for (idx1 = dataExtent[2]; idx1 <= dataExtent[3]; ++idx1)
      {
      outPtr0 = outPtr1;
      
      // read the row.
      if ( ! self->File->read((char *)buf, streamRead))
	{
	vtkGenericWarningMacro("File operation failed. row = " << idx1
			       << ", Read = " << streamRead
			       << ", Skip0 = " << streamSkip0
			       << ", Skip1 = " << streamSkip1
			       << ", FilePos = " << self->File->tellg());
	return;
	}
      
      // copy the bytes into the typed data
      inPtr = buf;
      for (idx0 = dataExtent[0]; idx0 <= dataExtent[1]; ++idx0)
	{
	// Copy pixel into the output.
	if (self->Depth == 8)
	  {
	  outPtr0[0] = (OT)(self->Colors[inPtr[0]*3]);
	  outPtr0[1] = (OT)(self->Colors[inPtr[0]*3+1]);
	  outPtr0[2] = (OT)(self->Colors[inPtr[0]*3+2]);
	  }
	else
	  {
	  outPtr0[0] = (OT)(inPtr[2]);
	  outPtr0[1] = (OT)(inPtr[1]);
	  outPtr0[2] = (OT)(inPtr[0]);
	  }
	// move to next pixel
	inPtr += pixelSkip;
	outPtr0 += outIncr[0];
	}
      // move to the next row in the file and data
      self->File->seekg(self->File->tellg() + streamSkip0, ios::beg);
      outPtr1 += outIncr[1];
      }
    // move to the next image in the file and data
    self->File->seekg(self->File->tellg() + streamSkip1, ios::beg);
    outPtr2 += outIncr[2];
    }

  // delete the temporary buffer
  delete [] buf;
}


//----------------------------------------------------------------------------
// Description:
// This function reads a data from a file.  The datas extent/axes
// are assumed to be the same as the file extent/order.
void vtkBMPReader::Execute(vtkImageData *data)
{
  this->ComputeDataIncrements();
  
  // Call the correct templated function for the output
  void *outPtr;

  // Call the correct templated function for the input
  outPtr = data->GetScalarPointer();
  switch (data->GetScalarType())
    {
    case VTK_FLOAT:
      vtkBMPReaderUpdate2(this, data, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkBMPReaderUpdate2(this, data, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkBMPReaderUpdate2(this, data, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkBMPReaderUpdate2(this, data, (unsigned short *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkBMPReaderUpdate2(this, data, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown data type");
    }  
}
