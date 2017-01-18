/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractPointCloudPiece.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkExtractPointCloudPiece
 * @brief   Return a piece of a point cloud
 *
 * This filter takes the output of a vtkHierarchicalBinningFilter and allows
 * the pipeline to stream it. Pieces are detemined from an offset integral
 * array is associated with the field data of the input.
*/

#ifndef vtkExtractPointCloudPiece_h
#define vtkExtractPointCloudPiece_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkIdList;
class vtkIntArray;

class VTKFILTERSPOINTS_EXPORT vtkExtractPointCloudPiece : public vtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiation, printing, and type information.
   */
  static vtkExtractPointCloudPiece *New();
  vtkTypeMacro(vtkExtractPointCloudPiece, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Turn on or off modulo sampling of the points. By default this is on and the
   * points in a given piece will be reordered in an attempt to reduce spatial
   * coherency.
   */
  vtkSetMacro(ModuloOrdering,bool);
  vtkGetMacro(ModuloOrdering,bool);
  vtkBooleanMacro(ModuloOrdering,bool);
  //@}

protected:
  vtkExtractPointCloudPiece();
  ~vtkExtractPointCloudPiece() VTK_OVERRIDE {}

  // Usual data generation method
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  bool ModuloOrdering;

private:
  vtkExtractPointCloudPiece(const vtkExtractPointCloudPiece&) VTK_DELETE_FUNCTION;
  void operator=(const vtkExtractPointCloudPiece&) VTK_DELETE_FUNCTION;
};

#endif
