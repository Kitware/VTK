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
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * This method is for setting up the processes.
   */
  void Initialize(int*, char***, int) override {}
  void Initialize(int*, char***) override {}
  void Finalize() override {}
  void Finalize(int) override {}

  /**
   * This method always returns 0.
   */
  int GetLocalProcessId() { return 0; }

  /**
   * Directly calls the single method.
   */
  void SingleMethodExecute() override;

  /**
   * Directly calls multiple method 0.
   */
  void MultipleMethodExecute() override;

  /**
   * Does nothing.
   */
  void CreateOutputWindow() override {}

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
  ~vtkDummyController() override;

private:
  vtkDummyController(const vtkDummyController&) = delete;
  void operator=(const vtkDummyController&) = delete;
};

#endif


