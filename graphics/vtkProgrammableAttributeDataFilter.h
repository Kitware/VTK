/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgrammableAttributeDataFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkProgrammableAttributeDataFilter - manipulate attribute (cell and point) data via a user-specified function
// .SECTION Description
// vtkProgrammableAttributeDataFilter is a filter that allows you to write a
// custom procedure to manipulate attribute data - either point or cell
// data. For example, you could generate scalars based on a complex formula;
// convert vectors to normals; compute scalar values as a function of
// vectors, texture coords, and/or any other point data attribute; and so
// on. The filter takes multiple inputs (input plus an auxiliary input list),
// so you can write procedures that combine several datset point
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
// By default the output point and cell data will be copied through from the input
// point data (using reference counting).  You can control this using the
// output's CopyAllOff() flag, or by using individual flags for each point
// data field (i.e., scalars, vectors, etc.)
//
// The output of this filter is the abstract type vtkDataSet, even if your input 
// is a concrete type like vtkPolyData. Thus you may need to use vtkCastToConcrete
// to obtain the output as a particular concrete type, or one of the special
// methods of the superclass (e.g., vtkDataSetToDataSetFilter::GetPolyDataOutput)
// to retrieve output of the correct type.
//
// The filter correctly manages modified time and network execution in most
// cases. However, if you change the definition of the filter function,
// you'll want to send a manual Modified() method to the filter to force it
// to reexecute.

#ifndef __vtkProgrammableAttributeDataFilter_h
#define __vtkProgrammableAttributeDataFilter_h

#include "vtkDataSetToDataSetFilter.h"
#include "vtkDataSetCollection.h"

class VTK_EXPORT vtkProgrammableAttributeDataFilter : public vtkDataSetToDataSetFilter 
{
public:
  vtkProgrammableAttributeDataFilter();
  ~vtkProgrammableAttributeDataFilter();
  static vtkProgrammableAttributeDataFilter *New() {return new vtkProgrammableAttributeDataFilter;};
  const char *GetClassName() {return "vtkProgrammableAttributeDataFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void AddInput(vtkDataSet *in);
  void AddInput(vtkDataSet& in) {this->AddInput(&in);};
  void RemoveInput(vtkDataSet *in);
  void RemoveInput(vtkDataSet& in) {this->RemoveInput(&in);};
  vtkDataSetCollection *GetInputList() {return &(this->InputList);};

  void SetExecuteMethod(void (*f)(void *), void *arg);
  void SetExecuteMethodArgDelete(void (*f)(void *));

  // filter interface - is different because of multiple input
  void Update();

protected:
  void Execute();
  vtkDataSetCollection InputList; //list of datasets to process
  void (*ExecuteMethod)(void *); //function to invoke
  void (*ExecuteMethodArgDelete)(void *);
  void *ExecuteMethodArg;

};

#endif


