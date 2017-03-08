/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDuplicatePolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDuplicatePolyData
 * @brief   For distributed tiled displays.
 *
 * This filter collects poly data and duplicates it on every node.
 * Converts data parallel so every node has a complete copy of the data.
 * The filter is used at the end of a pipeline for driving a tiled
 * display.
*/

#ifndef vtkDuplicatePolyData_h
#define vtkDuplicatePolyData_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"
class vtkSocketController;
class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkDuplicatePolyData : public vtkPolyDataAlgorithm
{
public:
  static vtkDuplicatePolyData *New();
  vtkTypeMacro(vtkDuplicatePolyData, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * By defualt this filter uses the global controller,
   * but this method can be used to set another instead.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

  void InitializeSchedule(int numProcs);

  //@{
  /**
   * This flag causes sends and receives to be matched.
   * When this flag is off, two sends occur then two receives.
   * I want to see if it makes a difference in performance.
   * The flag is on by default.
   */
  vtkSetMacro(Synchronous, int);
  vtkGetMacro(Synchronous, int);
  vtkBooleanMacro(Synchronous, int);
  //@}

  //@{
  /**
   * This duplicate filter works in client server mode when this
   * controller is set.  We have a client flag to differentiate the
   * client and server because the socket controller is odd:
   * Proth processes think their id is 0.
   */
  vtkSocketController *GetSocketController() {return this->SocketController;}
  void SetSocketController (vtkSocketController *controller);
  vtkSetMacro(ClientFlag,int);
  vtkGetMacro(ClientFlag,int);
  //@}

  //@{
  /**
   * This returns to size of the output (on this process).
   * This method is not really used.  It is needed to have
   * the same API as vtkCollectPolyData.
   */
  vtkGetMacro(MemorySize, unsigned long);
  //@}

protected:
  vtkDuplicatePolyData();
  ~vtkDuplicatePolyData() VTK_OVERRIDE;

  // Data generation method
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  void ClientExecute(vtkPolyData *output);

  vtkMultiProcessController *Controller;
  int Synchronous;

  int NumberOfProcesses;
  int ScheduleLength;
  int **Schedule;

  // For client server mode.
  vtkSocketController *SocketController;
  int ClientFlag;

  unsigned long MemorySize;

private:
  vtkDuplicatePolyData(const vtkDuplicatePolyData&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDuplicatePolyData&) VTK_DELETE_FUNCTION;
};

#endif

