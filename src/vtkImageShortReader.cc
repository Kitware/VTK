/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageShortReader.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include <ctype.h>
#include <string.h>
#include "vtkImageShortReader.hh"
#include "vtkImageTemplatedRegionCache.hh"
#include "vtkImageCache.hh"

//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageShortReader fitler.
vtkImageShortReader::vtkImageShortReader()
{
  this->File = NULL;

  this->Signed = 0;

  this->Size[0] = 256;
  this->Size[1] = 256;
  this->Size[2] = 1;

  this->Inc[0] = 1;
  this->Inc[1] = 256;
  this->Inc[2] = 256 * 256;
}

//----------------------------------------------------------------------------
// Description:
// This sets the dimensions of the image in the file
void vtkImageShortReader::SetSize(int size0, int size1, int size2)
{
  vtkDebugMacro(<< "SetSize: (" 
                << size0 << ", " << size1 << ", " << size2 << ")");

  this->Size[0] = size0;
  this->Size[1] = size1;
  this->Size[2] = size2;

  this->Inc[0] = 1;
  this->Inc[1] = size0;
  this->Inc[2] = size0 * size1;

  this->Modified();
}
void vtkImageShortReader::SetSize(int *size)
{
  this->SetSize(size[0], size[1], size[2]);
}

//----------------------------------------------------------------------------
// Description:
// This function opens a file for reading.
void vtkImageShortReader::SetFileName(char *fileName)
{
  long fileLength;

  // Close file from any previous image
  if (this->File)
    this->File->close();
  
  // Open the new file
  vtkDebugMacro(<< "SetFileName: opening Short file " << fileName);
  this->File = new ifstream(fileName, ios::in);
  if (! this->File)
    {
    vtkErrorMacro(<< "Could not open file " << fileName);
    return;
    }
  
  // Get the size of the header from the size of the image
  this->File->seekg(0,ios::end);
  fileLength = this->File->tellg();

  this->HeaderSize = fileLength
    - sizeof(unsigned short int) * this->Inc[2] * this->Size[2];
  
  vtkDebugMacro(<< "SetFileName: Header " << this->HeaderSize 
                << " bytes, fileLength = " << fileLength << " bytes.");
}

//----------------------------------------------------------------------------
// Description:
// This function is the external write function.  
// It writes the first image set and ignores the rest.
void vtkImageShortReader::GenerateRegion(int *outOffset, int *outSize)
{
  vtkImageTemplatedRegion<float> *region;
  int *offset;
  int size0, size1, size2;
  int inc0, inc1, inc2;
  float *pf0, *pf1;
  long streamStartPos;
  long streamRowSkip;
  long streamRowRead;
  unsigned char *buf, *pbuf;
  int idx0, idx1;


  vtkDebugMacro(<< "GenerateRegion: offset = ("
                << outOffset[0] << ", " << outOffset[1] << ", " << outOffset[2]
                << "), size = ("
                << outSize[0] << ", " << outSize[1] << ", " << outSize[2]
                << ")");

  // Get the region to fill from the cache
  if ( ! this->Output)
    {
    vtkErrorMacro(<< "GenerateRegion: Cache not created yet");
    return;
    }
  region = (vtkImageTemplatedRegion<float>)
    (this->Output->GetRegion(outOffset, outSize));
  
  // get the information needed to find a location in the file
  offset = region->GetOffset();
  region->GetSize(size0, size1, size2);
  region->GetInc(inc0, inc1, inc2);
  streamStartPos = offset[0] * this->Inc[0] 
                 + offset[1] * this->Inc[1] 
                 + offset[2] * this->Inc[2];
  streamStartPos *= sizeof(unsigned short int);
  streamStartPos += this->HeaderSize;

  streamRowRead = size0 * sizeof(unsigned short int);
  streamRowSkip = (this->Inc[1] - size0 * this->Inc[0]) 
    * sizeof(unsigned short int);

  // move to the correct location in the file (offset of region)
  this->File->seekg(streamStartPos, ios::beg);
  if (this->File->fail())
    {
    vtkErrorMacro(<< "File operation failed.");
    return;
    }
  
  // create a buffer to hold a row of the region
  buf = new unsigned char[size0 * 2 + 2]; 
  
  // read the data row by row
  pf1 = region->GetPointer(offset);
  for (idx1 = 0; idx1 < size1; ++idx1)
    {
    if ( ! this->File->read(buf, streamRowRead))
      {
      vtkErrorMacro(<< "File operation failed. row = " << idx1
		    << ", StartPos = " << streamStartPos
		    << ", RowSkip = " << streamRowSkip
		    << ", RowRead = " << streamRowRead
		    << ", FilePos = " << this->File->tellg());
      return;
      }
    // copy the bytes into the float region
    pf0 = pf1;
    pbuf = buf;
    for (idx0 = 0; idx0 < size0; ++idx0)
      {
      if (this->Signed)
	*pf0 = (float)(*((short int *)pbuf));
      else
	*pf0 = (float)(*((unsigned short int *)pbuf));

      pbuf += 2;
      pf0 += inc0;
      }
    // move to the next row in the file and region
    this->File->seekg(streamRowSkip, ios::cur);
    pf1 += inc1;
    }
  
  // delete the temporary buffer
  delete [] buf;
}


//----------------------------------------------------------------------------
// Description:
// This method returns in "offset" and "size" the boundary of data
// in the image. Request for regions of the image out side of these
// bounds will have unpridictable effects and will give a file read error.
// i.e. no error checking is performed.
void vtkImageShortReader::GetBoundary(int *offset, int *size)
{
  offset[0] = offset[1] = offset[2] = 0;
  size[0] = this->Size[0];
  size[1] = this->Size[1];
  size[2] = this->Size[2];
  
  vtkDebugMacro(<< "GetBoundary: returning offset = ("
          << offset[0] << ", " << offset[1] << ", " << offset[2]
          << "), size = (" << size[0] << ", " << size[1] << ", " << size[2]
          << ")");  
}

  
  
  
  
  








