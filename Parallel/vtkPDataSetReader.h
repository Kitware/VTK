/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPDataSetReader.h
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
// .NAME vtkPDataSetReader - Manages writing pieces of a data set.
// .SECTION Description
// vtkPDataSetReader will write a piece of a file, and will also create
// a metadata file that lists all of the files in a data set.


#ifndef __vtkPDataSetReader_h
#define __vtkPDataSetReader_h

#include "vtkSource.h"
#include "vtkDataSet.h"


class VTK_EXPORT vtkPDataSetReader : public vtkSource
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeMacro(vtkPDataSetReader,vtkSource);
  static vtkPDataSetReader *New();
  
  // Description:
  // This file to open and read.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // The output of this reader depends on the file choosen.
  // You cannot get the output until the filename is set.
  void SetOutput(vtkDataSet *output);
  virtual vtkDataSet *GetOutput();

  // Description:
  // We need to define this so that the output gets created.
  virtual void Update();

  // Description:
  // This is set when UpdateInformation is called. 
  // It shows the type of the output.
  vtkGetMacro(DataType, int);
  
protected:
  vtkPDataSetReader();
  ~vtkPDataSetReader();
  vtkPDataSetReader(const vtkPDataSetReader&);
  void operator=(const vtkPDataSetReader&);

  virtual void ExecuteInformation();
  virtual void Execute();
  void PolyDataExecute();
  void UnstructuredGridExecute();

  vtkDataSet *CheckOutput();
  void SetNumberOfPieces(int num);

//BTX
  ifstream *vtkPDataSetReader::OpenFile();
//ETX

  int VTKFileFlag;
  char *FileName;
  int DataType;
  int NumberOfPieces;
  char **PieceFileNames;
};

#endif
