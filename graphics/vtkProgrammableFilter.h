/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgrammableFilter.h
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
// The filter correctlymanages modified time and network execution in most
// cases. However, if you change the definition of the filter function,
// you'll want to send a manual Modified() method to the filter to force it
// to reexecute.

// .SECTION See Also
// vtkProgrammablePointDataFilter vtkProgrammableSource

#ifndef __vtkProgrammableFilter_h
#define __vtkProgrammableFilter_h

#include "vtkFilter.h"
#include "vtkDataSet.h"

class vtkPolyData;
class vtkStructuredPoints;
class vtkStructuredGrid;
class vtkUnstructuredGrid;
class vtkRectilinearGrid;
class vtkImageCache;

class VTK_EXPORT vtkProgrammableFilter : public vtkFilter
{
public:

// Description:
// Construct programmable filter with empty execute method.
  vtkProgrammableFilter();

  ~vtkProgrammableFilter();
  static vtkProgrammableFilter *New() {return new vtkProgrammableFilter;};
  const char *GetClassName() {return "vtkProgrammableFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // methods used to define user function

// Description:
// Specify the function to use to operate on the point attribute data. Note
// that the function takes a single (void *) argument.
  void SetExecuteMethod(void (*f)(void *), void *arg);


// Description:
// Set the arg delete method. This is used to free user memory.
  void SetExecuteMethodArgDelete(void (*f)(void *));


  // methods to set input data

// Description:
// Specify the input data or filter.
  void SetInput(vtkDataSet *input);


  // methods used to get the input data - user of this filter can get
  // filter input types - the execute method must manipulate the correct
  // type.

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


  // methods used to get the output data - user of this filter can get
  // different output types - the execute method must manipulate the correct
  // type.

// Description:
// Get the output as a concrete type. This method is typically used by the
// writer of the filter function to get the output as a particular type (i.e.,
// it essentially does type casting). It is the users responsibility to know
// the correct type of the output data.
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


protected:
  void Execute();

  void (*ExecuteMethod)(void *); //function to invoke
  void (*ExecuteMethodArgDelete)(void *);
  void *ExecuteMethodArg;
  
  // objects used to support the retrieval of output
  vtkPolyData *OutputPolyData;
  vtkStructuredPoints *OutputStructuredPoints;
  vtkStructuredGrid *OutputStructuredGrid;
  vtkUnstructuredGrid *OutputUnstructuredGrid;
  vtkRectilinearGrid *OutputRectilinearGrid;
  
};

#endif

