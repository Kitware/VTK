/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgrammableAttributeDataFilter.h
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
// .NAME vtkProgrammableAttributeDataFilter - manipulate attribute (cell and point) data via a user-specified function
// .SECTION Description
// vtkProgrammableAttributeDataFilter is a filter that allows you to write a
// custom procedure to manipulate attribute data - either point or cell
// data. For example, you could generate scalars based on a complex formula;
// convert vectors to normals; compute scalar values as a function of
// vectors, texture coords, and/or any other point data attribute; and so
// on. The filter takes multiple inputs (input plus an auxiliary input list),
// so you can write procedures that combine several dataset point
// attributes. Note that the output of the filter is the same type
// (topology/geometry) as the input.
//
// The filter works as follows. It operates like any other filter (i.e.,
// checking and managing modified and execution times, processing Update()
// and Execute() methods, managing release of data, etc.), but the difference
// is that the Execute() method simply invokes a user-specified function with
// an optional (void *) argument (typically the "this" pointer in C++). It is
// also possible to specify a function to delete the argument via
// ExecuteMethodArgDelete().
//
// To use the filter, you write a procedure to process the input datasets,
// process the data, and generate output data. Typically, this means grabbing
// the input point or cell data (using GetInput() and maybe GetInputList()),
// operating on it (creating new point and cell attributes such as scalars,
// vectors, etc.), and then setting the point and/or cell attributes in the
// output dataset (you'll need to use GetOutput() to access the output).
// (Note: besides C++, it is possible to do the same thing in Tcl, Java, or
// other languages that wrap the C++ core.) Remember, proper filter protocol
// requires that you don't modify the input data - you create new output data
// from the input.
//
// .SECTION Caveats
// This filter operates on any combination of the filter input plus a list of
// additional inputs (at a minimum you must set the filter input via
// SetInput()).  It is up to you check whether the input is valid, and to
// insure that the output is valid. Also, you have to write the control
// structure for the traversal and operation on the point and cell attribute
// data.
//
// By default the output point and cell data will be copied through from the
// input point data (using reference counting).  You can control this using
// the output's CopyAllOff() flag, or by using individual flags for each
// point data field (i.e., scalars, vectors, etc.)
//
// The output of this filter is the abstract type vtkDataSet, even if your
// input is a concrete type like vtkPolyData. Thus you may need to use
// vtkCastToConcrete to obtain the output as a particular concrete type, or
// one of the special methods of the superclass (e.g.,
// vtkDataSetToDataSetFilter::GetPolyDataOutput) to retrieve output of the
// correct type.
//
// The filter correctly manages modified time and network execution in most
// cases. However, if you change the definition of the filter function,
// you'll want to send a manual Modified() method to the filter to force it
// to reexecute.

#ifndef __vtkProgrammableAttributeDataFilter_h
#define __vtkProgrammableAttributeDataFilter_h

#include "vtkDataSetToDataSetFilter.h"
#include "vtkDataSetCollection.h"

class VTK_GRAPHICS_EXPORT vtkProgrammableAttributeDataFilter : public vtkDataSetToDataSetFilter 
{
public:
  static vtkProgrammableAttributeDataFilter *New();
  vtkTypeMacro(vtkProgrammableAttributeDataFilter,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a dataset to the list of data to process.
  void AddInput(vtkDataSet *in);

  // Description:
  // Remove a dataset from the list of data to process.
  void RemoveInput(vtkDataSet *in);

  // Description:
  // Return the list of inputs.
  vtkDataSetCollection *GetInputList() {return this->InputList;};

  // Description:
  // Specify the function to use to operate on the point attribute data. Note
  // that the function takes a single (void *) argument.
  void SetExecuteMethod(void (*f)(void *), void *arg);

  // Description:
  // Set the arg delete method. This is used to free user memory.
  void SetExecuteMethodArgDelete(void (*f)(void *));

protected:
  vtkProgrammableAttributeDataFilter();
  ~vtkProgrammableAttributeDataFilter();
  vtkProgrammableAttributeDataFilter(const vtkProgrammableAttributeDataFilter&);
  void operator=(const vtkProgrammableAttributeDataFilter&);

  void Execute();
  vtkDataSetCollection *InputList; //list of datasets to process
  void (*ExecuteMethod)(void *); //function to invoke
  void (*ExecuteMethodArgDelete)(void *);
  void *ExecuteMethodArg;

private:
  // hide the superclass' AddInput() from the user and the compiler
  void AddInput(vtkDataObject *)
    { vtkErrorMacro( << "AddInput() must be called with a vtkDataSet not a vtkDataObject."); };
  void RemoveInput(vtkDataObject *input)
    { this->vtkProcessObject::RemoveInput(input); };
};

#endif


