// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBoundaryMeshQuality
 * @brief   Computes metrics on the boundary faces of a mesh.
 *
 * vtkBoundaryMeshQuality computes metrics on the boundary faces of a volumetric mesh.
 * The metrics that can be computed on the boundary faces of the mesh and are:
 * - Distance from cell center to face center
 * - Distance from cell center to face's plane
 * - Angle of face's plane normal and cell center to face center vector
 */
#ifndef vtkBoundaryMeshQuality_h
#define vtkBoundaryMeshQuality_h

#include "vtkFiltersVerdictModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSVERDICT_EXPORT vtkBoundaryMeshQuality : public vtkPolyDataAlgorithm
{
public:
  static vtkBoundaryMeshQuality* New();
  vtkTypeMacro(vtkBoundaryMeshQuality, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify whether to compute the distance from the cell center ot the face center.
   */
  vtkSetMacro(DistanceFromCellCenterToFaceCenter, bool);
  vtkGetMacro(DistanceFromCellCenterToFaceCenter, bool);
  vtkBooleanMacro(DistanceFromCellCenterToFaceCenter, bool);
  ///@}

  ///@{
  /**
   * Specify whether to compute the distance from the cell center to the face's plane.
   */
  vtkSetMacro(DistanceFromCellCenterToFacePlane, bool);
  vtkGetMacro(DistanceFromCellCenterToFacePlane, bool);
  vtkBooleanMacro(DistanceFromCellCenterToFacePlane, bool);
  ///@}

  ///@{
  /**
   * Specify whether to compute the angle between the face normal and cell center to face center
   * vector. The angle is in degrees.
   */
  vtkSetMacro(AngleFaceNormalAndCellCenterToFaceCenterVector, bool);
  vtkGetMacro(AngleFaceNormalAndCellCenterToFaceCenterVector, bool);
  vtkBooleanMacro(AngleFaceNormalAndCellCenterToFaceCenterVector, bool);
  ///@}
protected:
  vtkBoundaryMeshQuality();
  ~vtkBoundaryMeshQuality() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkBoundaryMeshQuality(const vtkBoundaryMeshQuality&) = delete;
  void operator=(const vtkBoundaryMeshQuality&) = delete;

  bool DistanceFromCellCenterToFaceCenter = true;
  bool DistanceFromCellCenterToFacePlane = true;
  bool AngleFaceNormalAndCellCenterToFaceCenterVector = true;
};

VTK_ABI_NAMESPACE_END
#endif // vtkBoundaryMeshQuality_h
