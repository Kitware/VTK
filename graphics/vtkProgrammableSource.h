/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgrammableSource.h
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
// .NAME vtkProgrammableSource - generate source data via a user-specified function
// .SECTION Description
// vtkProgrammableSource is a source object that is programmable by the
// user. To use this object, you must specify a function that creates the
// output.  It is possible to generate output of any type; it is up to the
// function to properly initialize and define the output. Typically, you use
// one of the methods to get a concrete output type (e.g.,
// GetPolyDataOutput() or GetStructuredPointsOutput()), and then manipulate
// the output in the user-specified function.
//
// Example use of this include writing a function to read a data file or
// interface to another system. (You might want to do this in favor of
// deriving a new class.) Another important use of this class is that it
// allows users of interpreters (e.g., Tcl or Java) the ability to write
// source objects without having to recompile C++ code or generate new
// libraries.
// .SECTION See Also
// vtkProgrammableFilter vtkProgrammablePointDataFilter

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

// Description:
// Construct programmable filter with empty execute method.
  vtkProgrammableSource();

  ~vtkProgrammableSource();
  static vtkProgrammableSource *New() {return new vtkProgrammableSource;};
  const char *GetClassName() {return "vtkProgrammableSource";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // methods used to define user function

// Description:
// Specify the function to use to generate the source data. Note
// that the function takes a single (void *) argument.
  void SetExecuteMethod(void (*f)(void *), void *arg);


// Description:
// Set the arg delete method. This is used to free user memory.
  void SetExecuteMethodArgDelete(void (*f)(void *));


  // methods used to get the output data - user of this source can get
  // different output types - the execute method must manipulate the correct
  // type.

// Description:
// Get the output as a concrete type. This method is typically used by the
// writer of the source function to get the output as a particular type (i.e.,
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
  vtkPolyData *PolyData;
  vtkStructuredPoints *StructuredPoints;
  vtkStructuredGrid *StructuredGrid;
  vtkUnstructuredGrid *UnstructuredGrid;
  vtkRectilinearGrid *RectilinearGrid;
};

#endif

