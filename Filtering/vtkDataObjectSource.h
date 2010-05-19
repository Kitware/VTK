/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataObjectSource - abstract class specifies interface for
//  field source (or objects that generate field output)

// .SECTION Description
// vtkDataObjectSource is an abstract object that specifies behavior and
// interface of field source objects. Field source objects are source objects
// that create vtkFieldData (field data) on output.
//
// Concrete subclasses of vtkDataObjectSource must define Update() and
// Execute() methods. The public method Update() invokes network execution
// and will bring the network up-to-date. The protected Execute() method
// actually does the work of data creation/generation. The difference between
// the two methods is that Update() implements input consistency checks and
// modified time comparisons and then invokes the Execute() which is an
// implementation of a particular algorithm.
//
// vtkDataObjectSource provides a mechanism for invoking the methods
// StartMethod() and EndMethod() before and after object execution (via
// Execute()). These are convenience methods you can use for any purpose
// (e.g., debugging info, highlighting/notifying user interface, etc.) These
// methods accept a single void* pointer that can be used to send data to the
// methods. It is also possible to specify a function to delete the argument
// via StartMethodArgDelete and EndMethodArgDelete.
//
// Another method, ProgressMethod() can be specified. Some filters invoke this 
// method periodically during their execution. The use is similar to that of 
// StartMethod() and EndMethod().
//
// An important feature of subclasses of vtkDataObjectSource is that it is
// possible to control the memory-management model (i.e., retain output
// versus delete output data). If enabled the ReleaseDataFlag enables the
// deletion of the output data once the downstream process object finishes
// processing the data (please see text).

// .SECTION See Also
// vtkSource vtkFilter vtkFieldDataFilter

#ifndef __vtkDataObjectSource_h
#define __vtkDataObjectSource_h

#include "vtkSource.h"

class vtkDataObject;

class VTK_FILTERING_EXPORT vtkDataObjectSource : public vtkSource
{
public:
  vtkTypeMacro(vtkDataObjectSource,vtkSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output field of this source.
  vtkDataObject *GetOutput();
  vtkDataObject *GetOutput(int idx)
    {
      return this->vtkSource::GetOutput(idx);
    }
  
  void SetOutput(vtkDataObject *);
  
protected:
  vtkDataObjectSource();
  ~vtkDataObjectSource() {};

  virtual int FillOutputPortInformation(int, vtkInformation*);
private:
  vtkDataObjectSource(const vtkDataObjectSource&);  // Not implemented.
  void operator=(const vtkDataObjectSource&);  // Not implemented.
};

#endif

