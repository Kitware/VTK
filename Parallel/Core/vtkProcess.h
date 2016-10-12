/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProcess.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkProcess
 * @brief   a process that can be launched by a vtkMultiProcessController
 *
 * vtkProcess is an abstract class representing a process that can be launched
 * by a vtkMultiProcessController. Concrete classes just have to implement
 * Execute() method and make sure it set the proper value in ReturnValue.
 *
 * @par Example:
 *  class MyProcess: public vtkProcess
 *  ...
 *  vtkMultiProcessController *c;
 *  MyProcess *p=new MyProcess::New();
 *  p->SetArgs(argc,argv); // some parameters specific to the process
 *  p->SetX(10.0); // ...
 *  c->SetSingleProcess(p);
 *  c->SingleMethodExecute();
 *  int returnValue=p->GetReturnValue();
 *
 * @sa
 * vtkMultiProcessController
*/

#ifndef vtkProcess_h
#define vtkProcess_h

#include "vtkParallelCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkMultiProcessController;

class VTKPARALLELCORE_EXPORT vtkProcess : public vtkObject
{
public:
  vtkTypeMacro(vtkProcess,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Entry point of the process.
   * This method is expected to update ReturnValue.
   */
  virtual void Execute()=0;

  /**
   * Give access to the controller that launched the process.
   * Initial value is NULL.
   */
  vtkMultiProcessController *GetController();

  /**
   * This method should not be called directly but set by the controller
   * itself.
   */
  void SetController(vtkMultiProcessController *aController);

  /**
   * Value set at the end of a call to Execute.
   */
  int GetReturnValue();

protected:
  vtkProcess();

  vtkMultiProcessController *Controller;
  int ReturnValue;

private:
  vtkProcess(const vtkProcess&) VTK_DELETE_FUNCTION;
  void operator=(const vtkProcess&) VTK_DELETE_FUNCTION;
};

#endif
