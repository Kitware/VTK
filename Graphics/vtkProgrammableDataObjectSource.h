/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgrammableDataObjectSource.h
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
// .NAME vtkProgrammableDataObjectSource - generate source data object via a user-specified function
// .SECTION Description
// vtkProgrammableDataObjectSource is a source object that is programmable by
// the user. The output of the filter is a data object (vtkDataObject) which
// represents data via an instance of field data. To use this object, you
// must specify a function that creates the output.  
//
// Example use of this filter includes reading tabular data and encoding it
// as vtkFieldData. You can then use filters like vtkDataObjectToDataSetFilter
// to convert the data object to a dataset and then visualize it.  Another
// important use of this class is that it allows users of interpreters (e.g.,
// Tcl or Java) the ability to write source objects without having to
// recompile C++ code or generate new libraries.
// 
// .SECTION See Also
// vtkProgrammableFilter vtkProgrammableAttributeDataFilter
// vtkProgrammableSource vtkDataObjectToDataSetFilter

#ifndef __vtkProgrammableDataObjectSource_h
#define __vtkProgrammableDataObjectSource_h

#include "vtkSource.h"

class VTK_GRAPHICS_EXPORT vtkProgrammableDataObjectSource : public vtkSource
{
public:
  static vtkProgrammableDataObjectSource *New();
  vtkTypeMacro(vtkProgrammableDataObjectSource,vtkSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the function to use to generate the output data object. Note
  // that the function takes a single (void *) argument.
  void SetExecuteMethod(void (*f)(void *), void *arg);

  // Description:
  // Set the arg delete method. This is used to free user memory.
  void SetExecuteMethodArgDelete(void (*f)(void *));

  // Description:
  // Get the output data object.
  vtkDataObject *GetOutput();
  vtkDataObject *GetOutput(int idx)
    {return (vtkDataObject *) this->vtkSource::GetOutput(idx); };

protected:
  vtkProgrammableDataObjectSource();
  ~vtkProgrammableDataObjectSource();
  vtkProgrammableDataObjectSource(const vtkProgrammableDataObjectSource&);
  void operator=(const vtkProgrammableDataObjectSource&);

  void Execute();

  void (*ExecuteMethod)(void *); //function to invoke
  void (*ExecuteMethodArgDelete)(void *);
  void *ExecuteMethodArg;
};

#endif

