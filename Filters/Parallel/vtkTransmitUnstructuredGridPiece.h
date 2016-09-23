/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransmitUnstructuredGridPiece.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTransmitRectilinearGridPiece
 * @brief   Redistributes data produced
 * by serial readers
 *
 *
 * This filter can be used to redistribute data from producers that can't
 * produce data in parallel. All data is produced on first process and
 * the distributed to others using the multiprocess controller.
*/

#ifndef vtkTransmitUnstructuredGridPiece_h
#define vtkTransmitUnstructuredGridPiece_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkTransmitUnstructuredGridPiece : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkTransmitUnstructuredGridPiece *New();
  vtkTypeMacro(vtkTransmitUnstructuredGridPiece, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

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
   * Turn on/off creating ghost cells (on by default).
   */
  vtkSetMacro(CreateGhostCells, int);
  vtkGetMacro(CreateGhostCells, int);
  vtkBooleanMacro(CreateGhostCells, int);
  //@}

protected:
  vtkTransmitUnstructuredGridPiece();
  ~vtkTransmitUnstructuredGridPiece();

  // Data generation method
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  void RootExecute(vtkUnstructuredGrid *input, vtkUnstructuredGrid *output,
                   vtkInformation *outInfo);
  void SatelliteExecute(int procId, vtkUnstructuredGrid *output,
                        vtkInformation *outInfo);

  int CreateGhostCells;
  vtkMultiProcessController *Controller;

private:
  vtkTransmitUnstructuredGridPiece(const vtkTransmitUnstructuredGridPiece&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTransmitUnstructuredGridPiece&) VTK_DELETE_FUNCTION;
};

#endif
