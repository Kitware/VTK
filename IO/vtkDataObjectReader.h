/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectReader.h
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
// .NAME vtkDataObjectReader - read vtk field data file
// .SECTION Description
// vtkDataObjectReader is a source object that reads ASCII or binary field
// data files in vtk format. Fields are general matrix structures used
// represent complex data. (See text for format details).  The output of this
// reader is a single vtkDataObject.  The superclass of this class,
// vtkDataReader, provides many methods for controlling the reading of the
// data file, see vtkDataReader for more information.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.
// .SECTION See Also
// vtkFieldData vtkDataObjectWriter

#ifndef __vtkDataObjectReader_h
#define __vtkDataObjectReader_h

#include "vtkDataReader.h"
#include "vtkDataObject.h"

class VTK_EXPORT vtkDataObjectReader : public vtkDataReader
{
public:
  static vtkDataObjectReader *New();
  vtkTypeMacro(vtkDataObjectReader,vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output field of this reader.
  vtkDataObject *GetOutput();
  vtkDataObject *GetOutput(int idx)
    {return this->vtkSource::GetOutput(idx); };
  void SetOutput(vtkDataObject *);
  
protected:
  vtkDataObjectReader();
  ~vtkDataObjectReader();
  vtkDataObjectReader(const vtkDataObjectReader&);
  void operator=(const vtkDataObjectReader&);

  void Execute();
};

#endif


