/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgrammableFilter.h
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
// .NAME vtkProgrammableFilter - a user-programmable filter
// .SECTION Description
// vtkProgrammableFilter is a filter that can be programmed by the user.  To
// use the filter you define a function that retrieves input of the correct
// type, creates data, and then manipulates the output of the filter.  Using
// this filter avoids the need for subclassing - and the function can be
// defined in an interpreter wrapper language such as Tcl or Java.
//
// The trickiest part of using this filter is that the input and output
// methods are unusual and cannot be compile-time type checked. Instead, as a
// user of this filter it is your responsibility to set and get the correct
// input and output types.

// .SECTION Caveats
// The filter correctly manages modified time and network execution in most
// cases. However, if you change the definition of the filter function,
// you'll want to send a manual Modified() method to the filter to force it
// to reexecute.

// .SECTION See Also
// vtkProgrammablePointDataFilter vtkProgrammableSource

#ifndef __vtkProgrammableFilter_h
#define __vtkProgrammableFilter_h

#include "vtkDataSetToDataSetFilter.h"

class VTK_EXPORT vtkProgrammableFilter : public vtkDataSetToDataSetFilter
{
public:
  static vtkProgrammableFilter *New();
  vtkTypeMacro(vtkProgrammableFilter,vtkDataSetToDataSetFilter);

  // Description:
  // Specify the function to use to operate on the point attribute data. Note
  // that the function takes a single (void *) argument.
  void SetExecuteMethod(void (*f)(void *), void *arg);

  // Description:
  // Set the arg delete method. This is used to free user memory.
  void SetExecuteMethodArgDelete(void (*f)(void *));

  // Description:
  // Get the input as a concrete type. This method is typically used by the
  // writer of the filter function to get the input as a particular type (i.e.,
  // it essentially does type casting). It is the users responsibility to know
  // the correct type of the input data.
  vtkPolyData *GetPolyDataInput();

  // Description:
  // Get the input as a concrete type.
  vtkStructuredPoints *GetStructuredPointsInput();

  // Description:
  // Get the input as a concrete type.
  vtkStructuredGrid *GetStructuredGridInput();

  // Description:
  // Get the input as a concrete type.
  vtkUnstructuredGrid *GetUnstructuredGridInput();

  // Description:
  // Get the input as a concrete type.
  vtkRectilinearGrid *GetRectilinearGridInput();

protected:
  vtkProgrammableFilter();
  ~vtkProgrammableFilter();
  vtkProgrammableFilter(const vtkProgrammableFilter&);
  void operator=(const vtkProgrammableFilter&);

  void Execute();

  void (*ExecuteMethod)(void *); //function to invoke
  void (*ExecuteMethodArgDelete)(void *);
  void *ExecuteMethodArg;
  
};

#endif

