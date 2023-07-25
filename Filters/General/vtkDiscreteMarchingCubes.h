// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .SECTION Thanks
// Jim Miller at GE Research implemented the original version of this
// filter.
// This work was supported by PHS Research Grant No. 1 P41 RR13218-01
// from the National Center for Research Resources and supported by a
// grant from the DARPA, executed by the U.S. Army Medical Research
// and Materiel Command/TATRC Cooperative Agreement,
// Contract # W81XWH-04-2-0012.
/**
 * @class   vtkDiscreteMarchingCubes
 * @brief   generate object boundaries from
 * labelled volumes
 *
 * This filter extracts object boundaries from label maps (label maps are
 * volumes in which each voxel is labeled according to the region in which it
 * is contained).  The filter takes as input a volume (e.g., 3D structured
 * point set) of segmentation labels and generates on output one or more
 * models representing the boundaries between the specified label and the
 * adjacent structures.  One or more label values must be specified to
 * generate the models.  The boundary positions are always defined to be
 * half-way between adjacent voxels. This filter works best with integral
 * scalar values.
 *
 * If ComputeScalars is on (the default), each output cell will have cell
 * data that corresponding to the scalar value (segmentation label) of the
 * corresponding cube. Note that this differs from vtkMarchingCubes, which
 * stores the scalar value as point data. The rationale for this difference
 * is that cell vertices may be shared between multiple cells. This also
 * means that the resultant polydata may be non-manifold (cell faces may be
 * coincident). To further process the polydata, users should either: 1)
 * extract cells that have a common scalar value using vtkThreshold, or 2)
 * process the data with filters that can handle non-manifold polydata
 * (e.g. vtkWindowedSincPolyDataFilter). Also note, Normals and Gradients are
 * not computed.
 *
 * If ComputeAdjacentScalars is on (default is off), each output point will
 * have point data that contains the label value of the neighbouring voxel.
 * This allows to remove regions of the resulting vtkPolyData that are
 * adjacent to specific label meshes. For example, if the input is a label
 * image that was created by running a watershed transformation on a distance
 * map followed by masking with the original binary segmentation. For further
 * details and images see the VTK Journal paper:
 * "Providing values of adjacent voxel with vtkDiscreteMarchingCubes"
 * by Roman Grothausmann:
 * http://hdl.handle.net/10380/3559
 * http://www.vtkjournal.org/browse/publication/975
 *
 * @warning
 * This filter is specialized to volumes. If you are interested in contouring
 * other types of data, use the general vtkContourFilter. If you want to
 * contour an image (i.e., a volume slice), use vtFlyingEdges2D or
 * vtkMarchingSquares.
 *
 * @warning
 * See also vtkPackLabels which is a utility class for renumbering the labels
 * found in the input segmentation mask to contiguous forms of smaller type.
 *
 * @sa
 * vtkSurfaceNets3D vtkDiscreteFlyingEdges3D vtkSurfaceNets2D
 * vtkContourFilter vtkSliceCubes vtkMarchingSquares vtkDividingCubes
 * vtkPackLabels
 */

#ifndef vtkDiscreteMarchingCubes_h
#define vtkDiscreteMarchingCubes_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkMarchingCubes.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkDiscreteMarchingCubes : public vtkMarchingCubes
{
public:
  ///@{
  /**
   * Standard methods for instantiation, obtaining type information, and
   * printing the state of an object.
   */
  static vtkDiscreteMarchingCubes* New();
  vtkTypeMacro(vtkDiscreteMarchingCubes, vtkMarchingCubes);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Set/Get the computation of neighbouring voxel values.
   */
  vtkSetMacro(ComputeAdjacentScalars, vtkTypeBool);
  vtkGetMacro(ComputeAdjacentScalars, vtkTypeBool);
  vtkBooleanMacro(ComputeAdjacentScalars, vtkTypeBool);
  ///@}

protected:
  vtkDiscreteMarchingCubes();
  ~vtkDiscreteMarchingCubes() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  vtkTypeBool ComputeAdjacentScalars;

private:
  vtkDiscreteMarchingCubes(const vtkDiscreteMarchingCubes&) = delete;
  void operator=(const vtkDiscreteMarchingCubes&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
