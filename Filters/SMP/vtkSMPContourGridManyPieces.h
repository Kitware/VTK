/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPContourGridManyPiece.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMPContourGridManyPieces
 * @brief   a subclass of vtkContourGrid that works in parallel
 * vtkSMPContourGridManyPieces performs the same functionaliy as vtkContourGrid but does
 * it using multiple threads. This filter generates a multi-block of vtkPolyData. It
 * will generate a relatively large number of pieces - the number is dependent on
 * the input size and number of threads available. See vtkSMPContourGrid is you are
 * interested in a filter that merges the piece. This will probably be merged with
 * vtkContourGrid in the future.
 */

#ifndef vtkSMPContourGridManyPieces_h
#define vtkSMPContourGridManyPieces_h

#include "vtkContourGrid.h"
#include "vtkFiltersSMPModule.h" // For export macro

class vtkPolyData;

#if !defined(VTK_LEGACY_REMOVE)
class VTKFILTERSSMP_EXPORT vtkSMPContourGridManyPieces : public vtkContourGrid
{
public:
  vtkTypeMacro(vtkSMPContourGridManyPieces, vtkContourGrid);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Constructor.
   */
  static vtkSMPContourGridManyPieces* New();

protected:
  vtkSMPContourGridManyPieces();
  ~vtkSMPContourGridManyPieces() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillOutputPortInformation(int port, vtkInformation* info) override;

private:
  vtkSMPContourGridManyPieces(const vtkSMPContourGridManyPieces&) = delete;
  void operator=(const vtkSMPContourGridManyPieces&) = delete;
};
#endif // VTK_LEGACY_REMOVE
#endif
