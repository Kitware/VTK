/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgrammableSource.h
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
// .NAME vtkProgrammableSource - generate source dataset via a user-specified function
// .SECTION Description
// vtkProgrammableSource is a source object that is programmable by the
// user. To use this object, you must specify a function that creates the
// output.  It is possible to generate an output dataset of any (concrete) 
// type; it is up to the function to properly initialize and define the 
// output. Typically, you use one of the methods to get a concrete output 
// type (e.g., GetPolyDataOutput() or GetStructuredPointsOutput()), and 
// then manipulate the output in the user-specified function.
//
// Example use of this include writing a function to read a data file or
// interface to another system. (You might want to do this in favor of
// deriving a new class.) Another important use of this class is that it
// allows users of interpreters (e.g., Tcl or Java) the ability to write
// source objects without having to recompile C++ code or generate new
// libraries.
// .SECTION See Also
// vtkProgrammableFilter vtkProgrammableAttributeDataFilter
// vtkProgrammableDataObjectSource

#ifndef __vtkProgrammableSource_h
#define __vtkProgrammableSource_h

#include "vtkSource.h"

class vtkPolyData;
class vtkStructuredPoints;
class vtkStructuredGrid;
class vtkUnstructuredGrid;
class vtkRectilinearGrid;

class VTK_EXPORT vtkProgrammableSource : public vtkSource
{
public:
  static vtkProgrammableSource *New();
  vtkTypeMacro(vtkProgrammableSource,vtkSource);

  // Description:
  // Specify the function to use to generate the source data. Note
  // that the function takes a single (void *) argument.
  void SetExecuteMethod(void (*f)(void *), void *arg);

  // Description:
  // Set the arg delete method. This is used to free user memory.
  void SetExecuteMethodArgDelete(void (*f)(void *));

  // Description:
  // Get the output as a concrete type. This method is typically used by the
  // writer of the source function to get the output as a particular type
  // (i.e., it essentially does type casting). It is the users responsibility
  // to know the correct type of the output data.
  vtkPolyData *GetPolyDataOutput();

  // Description:
  // Get the output as a concrete type.
  vtkStructuredPoints *GetStructuredPointsOutput();

  // Description:
  // Get the output as a concrete type.
  vtkStructuredGrid *GetStructuredGridOutput();

  // Description:
  // Get the output as a concrete type.
  vtkUnstructuredGrid *GetUnstructuredGridOutput();

  // Description:
  // Get the output as a concrete type.
  vtkRectilinearGrid *GetRectilinearGridOutput();

  void UpdateInformation();
  void UpdateData(vtkDataObject *output);

protected:
  vtkProgrammableSource();
  ~vtkProgrammableSource();
  vtkProgrammableSource(const vtkProgrammableSource&);
  void operator=(const vtkProgrammableSource&);

  void Execute();

  void (*ExecuteMethod)(void *); //function to invoke
  void (*ExecuteMethodArgDelete)(void *);
  void *ExecuteMethodArg;  

  vtkTimeStamp ExecuteTime;
};

#endif

