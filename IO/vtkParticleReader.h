/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParticleReader.h
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
// .NAME vtkParticleReader - Read raw particle data and one array.
// .SECTION Description
// vtkParticleReader reads a raw file with particles.  It supports
// random access into the file to read pieces.  The format is:
// x, y, z, value (all floats).  This class was developed with a 
// specific file in mind, but may be made more general in the future.
// Here are a couple of features I am considering:  
// any data types could be used,
// arbitrary list of arrays to read (extended to vectors as well),
// and time serries based on file names.

#ifndef __vtkParticleReader_h
#define __vtkParticleReader_h

#include "vtkPolyDataSource.h"

#define VTK_FILE_BYTE_ORDER_BIG_ENDIAN 0
#define VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN 1

class VTK_EXPORT vtkParticleReader : public vtkPolyDataSource
{
public:
  static vtkParticleReader *New();
  vtkTypeMacro(vtkParticleReader,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);   

  // Description:
  // Specify file name.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // These methods should be used instead of the SwapBytes methods.
  // They indicate the byte ordering of the file you are trying
  // to read in. These methods will then either swap or not swap
  // the bytes depending on the byte ordering of the machine it is
  // being run on. For example, reading in a BigEndian file on a
  // BigEndian machine will result in no swapping. Trying to read
  // the same file on a LittleEndian machine will result in swapping.
  // As a quick note most UNIX machines are BigEndian while PC's
  // and VAX tend to be LittleEndian. So if the file you are reading
  // in was generated on a VAX or PC, SetDataByteOrderToLittleEndian 
  // otherwise SetDataByteOrderToBigEndian. 
  void SetDataByteOrderToBigEndian();
  void SetDataByteOrderToLittleEndian();
  int GetDataByteOrder();
  void SetDataByteOrder(int);
  const char *GetDataByteOrderAsString();

  // Description:
  // Set/Get the byte swapping to explicitly swap the bytes of a file.
  vtkSetMacro(SwapBytes,int);
  int GetSwapBytes() {return this->SwapBytes;}
  vtkBooleanMacro(SwapBytes,int);

protected:
  vtkParticleReader();
  ~vtkParticleReader();
  vtkParticleReader(const vtkParticleReader&);
  void operator=(const vtkParticleReader&);

  void OpenFile();

  char *FileName;
  ifstream *File;
  int SwapBytes;

  unsigned long NumberOfPoints;
  
  void ExecuteInformation();
  void Execute();
};

#endif
