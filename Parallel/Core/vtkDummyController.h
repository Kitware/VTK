/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDummyController.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDummyController
 * @brief   Dummy controller for single process applications
 *
 * This is a dummy controller which can be used by applications which always
 * require a controller but are also compile on systems without threads
 * or mpi.
 * @sa
 * vtkMultiProcessController
*/

#ifndef vtkDummyController_h
#define vtkDummyController_h

#include "vtkParallelCoreModule.h" // For export macro
#include "vtkMultiProcessController.h"

class VTKPARALLELCORE_EXPORT vtkDummyController : public vtkMultiProcessController
{
public:
  static vtkDummyController *New();
  vtkTypeMacro(vtkDummyController,vtkMultiProcessController);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * This method is for setting up the processes.
   */
  void Initialize(int*, char***, int) VTK_OVERRIDE {}
  void Initialize(int*, char***) VTK_OVERRIDE {}
  void Finalize() VTK_OVERRIDE {}
  void Finalize(int) VTK_OVERRIDE {}

  /**
   * This method always returns 0.
   */
  int GetLocalProcessId() { return 0; }

  /**
   * Directly calls the single method.
   */
  void SingleMethodExecute() VTK_OVERRIDE;

  /**
   * Directly calls multiple method 0.
   */
  void MultipleMethodExecute() VTK_OVERRIDE;

  /**
   * Does nothing.
   */
  void CreateOutputWindow() VTK_OVERRIDE {}

  //@{
  /**
   * If you don't need any special functionality from the controller, you
   * can swap out the dummy communicator for another one.
   */
  vtkGetObjectMacro(RMICommunicator, vtkCommunicator);
  virtual void SetCommunicator(vtkCommunicator *);
  virtual void SetRMICommunicator(vtkCommunicator *);
  //@}

protected:
  vtkDummyController();
  ~vtkDummyController() VTK_OVERRIDE;

private:
  vtkDummyController(const vtkDummyController&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDummyController&) VTK_DELETE_FUNCTION;
};

#endif


