/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPDataSetWriter.h
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
// .NAME vtkPDataSetWriter - Manages writing pieces of a data set.
// .SECTION Description
// vtkPDataSetWriter will write a piece of a file, and will also create
// a metadata file that lists all of the files in a data set.


#ifndef __vtkPDataSetWriter_h
#define __vtkPDataSetWriter_h

#include "vtkDataSetWriter.h"


class VTK_EXPORT vtkPDataSetWriter : public vtkDataSetWriter
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeMacro(vtkPDataSetWriter,vtkDataSetWriter);
  static vtkPDataSetWriter *New();

  // Description:
  // Write the pvtk file and cooresponding vtk files.
  virtual void Write();

  // Description:
  // This is how many pieces the whole data set will be divided into.
  vtkSetMacro(NumberOfPieces, int);
  vtkGetMacro(NumberOfPieces, int);

  // Description:
  // Extra ghost cells will be written out to each piece file
  // if this value is larger than 0.
  vtkSetMacro(GhostLevel, int);
  vtkGetMacro(GhostLevel, int);
  
  // Description:
  // This is the range of pieces that that this writer is 
  // responsible for writing.  All pieces must be written
  // by some process.  The process that writes piece 0 also
  // writes the pvtk file that lists all the piece file names.
  vtkSetMacro(StartPiece, int);
  vtkGetMacro(StartPiece, int);
  vtkSetMacro(EndPiece, int);
  vtkGetMacro(EndPiece, int);
  
  // Description:
  // This file pattern uses the file name and piece number
  // to contruct a file name for the piece file.
  vtkSetStringMacro(FilePattern);
  vtkGetStringMacro(FilePattern);

protected:
  vtkPDataSetWriter();
  ~vtkPDataSetWriter();
  vtkPDataSetWriter(const vtkPDataSetWriter&);
  void operator=(const vtkPDataSetWriter&);

//BTX
  ostream *vtkPDataSetWriter::OpenFile();
//ETX

  int StartPiece;
  int EndPiece;
  int NumberOfPieces;
  int GhostLevel;

  char *FilePattern;
};

#endif
