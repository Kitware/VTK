/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParticleReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

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
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "vtkByteSwap.h"

#include "vtkParticleReader.h"
#include "vtkFloatArray.h"
#include "vtkCellArray.h"
#include "vtkPoints.h"
#include "vtkObjectFactory.h"


// These are copied right from vtkImageReader.
// I do not know what they do.
#ifdef read
#undef read
#endif

#ifdef close
#undef close
#endif



//----------------------------------------------------------------------------
vtkParticleReader* vtkParticleReader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkParticleReader");
  if(ret)
    {
    return (vtkParticleReader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkParticleReader;
}


//----------------------------------------------------------------------------
vtkParticleReader::vtkParticleReader()
{
  this->FileName = NULL;
  this->File = NULL;
#ifndef VTK_WORDS_BIGENDIAN
  this->SwapBytes = 1;
#else
  this->SwapBytes = 0;
#endif


  this->NumberOfPoints = 0;
}

//----------------------------------------------------------------------------
vtkParticleReader::~vtkParticleReader()
{ 
  if (this->File)
    {
    this->File->close();
    delete this->File;
    this->File = NULL;
    }
  
  if (this->FileName)
    {
    delete [] this->FileName;
    this->FileName = NULL;
    }
}


//----------------------------------------------------------------------------
void vtkParticleReader::OpenFile()
{
  if (!this->FileName)
    {
    vtkErrorMacro(<<"FileName must be specified.");
    return;
    }

  // Close file from any previous image
  if (this->File)
    {
    this->File->close();
    delete this->File;
    this->File = NULL;
    }
  
  // Open the new file
  vtkDebugMacro(<< "Initialize: opening file " << this->FileName);
#ifdef _WIN32
  this->File = new ifstream(this->FileName, ios::in | ios::binary);
#else
  this->File = new ifstream(this->FileName, ios::in);
#endif
  if (! this->File || this->File->fail())
    {
    vtkErrorMacro(<< "Initialize: Could not open file " << 
    this->FileName);
    return;
    }
}


//----------------------------------------------------------------------------
// This method returns the largest data that can be generated.
void vtkParticleReader::ExecuteInformation()
{
  vtkPolyData *output = this->GetOutput();

  output->SetMaximumNumberOfPieces(-1);
}


//----------------------------------------------------------------------------
// This method returns the largest data that can be generated.
void vtkParticleReader::Execute()
{
  vtkPolyData *output = this->GetOutput();
  vtkPoints *points;
  vtkFloatArray *array;
  vtkCellArray *verts;
  unsigned long fileLength, start, next, length, ptIdx, cellPtIdx;
  unsigned long cellLength;
  int piece, numPieces;
  float *data, *ptr;

  if (!this->FileName)
    {
    vtkErrorMacro(<<"FileName must be specified.");
    return;
    }
  
  this->OpenFile();
    
  // Get the size of the header from the size of the image
  this->File->seekg(0,ios::end);
  if (this->File->fail())
    {
    vtkErrorMacro("Could not seek to end of file.");
    return;
    }

  
  fileLength = (unsigned long)this->File->tellg();
  this->NumberOfPoints = fileLength / (4 * sizeof(float));

  piece = output->GetUpdatePiece();
  numPieces = output->GetUpdateNumberOfPieces();

  if ((unsigned long)numPieces > this->NumberOfPoints)
    {
    numPieces = (int)(this->NumberOfPoints);
    }
  if (numPieces <= 0 || piece < 0 || piece >= numPieces)
    {
    return;
    }

  start = piece * this->NumberOfPoints / numPieces;
  next = (piece+1) * this->NumberOfPoints / numPieces;

  length = next - start;

  data = new float[length * 4];


  // Seek to the first point in the file.
  this->File->seekg(start*4*sizeof(float), ios::beg);
  if (this->File->fail())
    {
    cerr << "File operation failed: Seeking to " << start*4 << endl;
    delete [] data;
    return;
    }

  // Read the data.
  if ( ! this->File->read((char *)data, length*4*sizeof(float)))
    {
    vtkErrorMacro("Could not read points: " << start 
	   << " to " << next-1);
    delete [] data;
    return;
    }

  // Swap bytes if necessary.
  if (this->GetSwapBytes())
    {
    vtkByteSwap::SwapVoidRange(data, length*4, sizeof(float));
    }

  ptr = data;
  points = vtkPoints::New();
  points->SetNumberOfPoints(length);
  array = vtkFloatArray::New();
  array->SetName("Count");
  verts = vtkCellArray::New();
  // Each cell will have 1000 points.  Leave a little extra space just in case.
  // We break up the cell this way so that the render will check for aborts
  // at a reasonable rate.
  verts->Allocate((int)((float)length * 1.002));
  // Keep adding cells until we run out of points.
  ptIdx = 0;
  while (length > 0)
    {
    cellLength = 1000;
    if (cellLength > length)
      {
      cellLength = length;
      }
    length = length - cellLength;
    verts->InsertNextCell((int)cellLength);
    for (cellPtIdx = 0; cellPtIdx < cellLength; ++cellPtIdx)
      {
      points->SetPoint(ptIdx, ptr[0], ptr[1], ptr[2]);
      array->InsertNextValue(ptr[3]);
      verts->InsertCellPoint(ptIdx);
      ptr += 4;
      ++ptIdx;
      }
    }
  delete [] data;
  data = ptr = NULL;

  output->SetPoints(points);
  points->Delete();
  points = NULL;
  output->SetVerts(verts);
  verts->Delete();
  verts = NULL;
  output->GetPointData()->SetScalars(array);
  array->Delete();
  array = NULL;
}












//----------------------------------------------------------------------------
void vtkParticleReader::SetDataByteOrderToBigEndian()
{
#ifndef VTK_WORDS_BIGENDIAN
  this->SwapBytesOn();
#else
  this->SwapBytesOff();
#endif
}

//----------------------------------------------------------------------------
void vtkParticleReader::SetDataByteOrderToLittleEndian()
{
#ifdef VTK_WORDS_BIGENDIAN
  this->SwapBytesOn();
#else
  this->SwapBytesOff();
#endif
}

void vtkParticleReader::SetDataByteOrder(int byteOrder)
{
  if ( byteOrder == VTK_FILE_BYTE_ORDER_BIG_ENDIAN )
    {
    this->SetDataByteOrderToBigEndian();
    }
  else
    {
    this->SetDataByteOrderToLittleEndian();
    }
}

//----------------------------------------------------------------------------
int vtkParticleReader::GetDataByteOrder()
{
#ifdef VTK_WORDS_BIGENDIAN
  if ( this->SwapBytes )
    {
    return VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN;
    }
  else
    {
    return VTK_FILE_BYTE_ORDER_BIG_ENDIAN;
    }
#else
  if ( this->SwapBytes )
    {
    return VTK_FILE_BYTE_ORDER_BIG_ENDIAN;
    }
  else
    {
    return VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN;
    }
#endif
}

//----------------------------------------------------------------------------
const char *vtkParticleReader::GetDataByteOrderAsString()
{
#ifdef VTK_WORDS_BIGENDIAN
  if ( this->SwapBytes )
    {
    return "LittleEndian";
    }
  else
    {
    return "BigEndian";
    }
#else
  if ( this->SwapBytes )
    {
    return "BigEndian";
    }
  else
    {
    return "LittleEndian";
    }
#endif
}


//----------------------------------------------------------------------------
void vtkParticleReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataSource::PrintSelf(os,indent);

  // this->File, this->Colors need not be printed  
  os << indent << "FileName: " <<
    (this->FileName ? this->FileName : "(none)") << "\n";

  os << indent << "Swap Bytes: " << (this->SwapBytes ? "On\n" : "Off\n");

  os << indent << "NumberOfPoints: " << this->NumberOfPoints << "\n";

}


