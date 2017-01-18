/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCollectTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
/**
 * @class   vtkCollectTable
 * @brief   Collect distributed table.
 *
 * This filter has code to collect a table from across processes onto node 0.
 * Collection can be turned on or off using the "PassThrough" flag.
*/

#ifndef vtkCollectTable_h
#define vtkCollectTable_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkTableAlgorithm.h"

class vtkMultiProcessController;
class vtkSocketController;

class VTKFILTERSPARALLEL_EXPORT vtkCollectTable : public vtkTableAlgorithm
{
public:
  static vtkCollectTable *New();
  vtkTypeMacro(vtkCollectTable, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * By defualt this filter uses the global controller,
   * but this method can be used to set another instead.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

  //@{
  /**
   * When this filter is being used in client-server mode,
   * this is the controller used to communicate between
   * client and server.  Client should not set the other controller.
   */
  virtual void SetSocketController(vtkSocketController*);
  vtkGetObjectMacro(SocketController, vtkSocketController);
  //@}

  //@{
  /**
   * To collect or just copy input to output. Off (collect) by default.
   */
  vtkSetMacro(PassThrough, int);
  vtkGetMacro(PassThrough, int);
  vtkBooleanMacro(PassThrough, int);
  //@}

protected:
  vtkCollectTable();
  ~vtkCollectTable() VTK_OVERRIDE;

  int PassThrough;

  // Data generation method
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

  vtkMultiProcessController *Controller;
  vtkSocketController *SocketController;

private:
  vtkCollectTable(const vtkCollectTable&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCollectTable&) VTK_DELETE_FUNCTION;
};

#endif
