/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProcessObject.h
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
// .NAME vtkProcessObject - abstract class specifies interface for visualization filters
// .SECTION Description
// vtkProcessObject is an abstract object that specifies behavior and interface
// of visualization network process objects (sources, filters, mappers). Source
// objects are creators of visualization data; filters input, process, and output 
// visualization data; and mappers transform data into another form (like rendering 
// primitives or write data to a file).
//
// vtkProcessObject provides a mechanism for invoking the methods StartMethod() and
// EndMethod() before and after object execution (via Execute()). These are
// convenience methods you can use for any purpose (e.g., debugging info,
// highlighting/notifying user interface, etc.) These methods accept a single
// void* pointer that can be used to send data to the methods. It is also
// possible to specify a function to delete the argument via 
// StartMethodArgDelete and EndMethodArgDelete.
//
// Another method, ProgressMethod() can be specified. Some filters invoke this 
// method periodically during their execution. The use is similar to that of 
// StartMethod() and EndMethod(). Filters may also check their AbortExecute
// flag to determine whether to prematurally end their execution.
//
// An important feature of subclasses of vtkProcessObject is that it is possible 
// to control the memory-management model (i.e., retain output versus delete
// output data). If enabled the ReleaseDataFlag enables the deletion of the
// output data once the downstream process object finishes processing the
// data (please see text).
// .SECTION See Also
// vtkDataObject vtkSource vtkFilter vtkMapper vtkWriter

#ifndef __vtkProcessObject_h
#define __vtkProcessObject_h

#include "vtkObject.h"

class VTK_EXPORT vtkProcessObject : public vtkObject
{
public:
  vtkProcessObject();
  ~vtkProcessObject();
  static vtkProcessObject *New() {return new vtkProcessObject;};
  const char *GetClassName() {return "vtkProcessObject";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetStartMethod(void (*f)(void *), void *arg);
  void SetProgressMethod(void (*f)(void *), void *arg);
  void SetEndMethod(void (*f)(void *), void *arg);
  void SetStartMethodArgDelete(void (*f)(void *));
  void SetProgressMethodArgDelete(void (*f)(void *));
  void SetEndMethodArgDelete(void (*f)(void *));

  // Description:
  // Set/Get the AbortExecute flag for the process object. Process objects
  // may handle premature termination of execution in different ways.
  vtkSetMacro(AbortExecute,int);
  vtkGetMacro(AbortExecute,int);
  vtkBooleanMacro(AbortExecute,int);

  // Description:
  // Set/Get the execution progress of a process object.
  vtkSetClampMacro(Progress,float,0.0,1.0);
  vtkGetMacro(Progress,float);

  // Description:
  // Update the progress of the process object. If a ProgressMethod exists, executes it. 
  // Then set the Progress ivar to amount. The parameter amount should range between (0,1).
  void UpdateProgress(float amount);

protected:
  void (*StartMethod)(void *);
  void (*StartMethodArgDelete)(void *);
  void *StartMethodArg;
  void (*ProgressMethod)(void *);
  void *ProgressMethodArg;
  void (*ProgressMethodArgDelete)(void *);
  void (*EndMethod)(void *);
  void (*EndMethodArgDelete)(void *);
  void *EndMethodArg;
  float Progress;
  int AbortExecute;
};

#endif

