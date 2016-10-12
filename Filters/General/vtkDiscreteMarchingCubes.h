/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDiscreteMarchingCubes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

// .SECTION Thanks
// Jim Miller at GE Research implemented the original version of this
// filter.
// This work was supported by PHS Research Grant No. 1 P41 RR13218-01
// from the National Center for Research Resources and supported by a
// grant from the DARPA, executed by the U.S. Army Medical Research
// and Materiel Command/TATRC Cooperative Agreement,
// Contract # W81XWH-04-2-0012.

=========================================================================*/
/**
 * @class   vtkDiscreteMarchingCubes
 * @brief   generate object boundaries from
 * labelled volumes
 *
 * takes as input a volume (e.g., 3D structured point set) of
 * segmentation labels and generates on output one or more
 * models representing the boundaries between the specified label and
 * the adjacent structures.  One or more label values must be specified to
 * generate the models.  The boundary positions are always defined to
 * be half-way between adjacent voxels. This filter works best with
 * integral scalar values.
 * If ComputeScalars is on (the default), each output cell will have
 * cell data that corresponds to the scalar value (segmentation label)
 * of the corresponding cube. Note that this differs from vtkMarchingCubes,
 * which stores the scalar value as point data. The rationale for this
 * difference is that cell vertices may be shared between multiple
 * cells. This also means that the resultant polydata may be
 * non-manifold (cell faces may be coincident). To further process the
 * polydata, users should either: 1) extract cells that have a common
 * scalar value using vtkThreshold, or 2) process the data with
 * filters that can handle non-manifold polydata
 * (e.g. vtkWindowedSincPolyDataFilter).
 * Also note, Normals and Gradients are not computed.
 * If ComputeAdjacentScalars is on (default is off), each output point will have
 * point data that contains the label value of the neighbouring voxel.
 * This allows to remove regions of the resulting vtkPolyData that are
 * adjacent to specific label meshes. For example, if the input is a label
 * image that was created by running a watershed transformation on a distance
 * map followed by masking with the original binary segmentation. For further
 * details and images see the VTK Journal paper
 * "Providing values of adjacent voxel with vtkDiscreteMarchingCubes"
 * by Roman Grothausmann:
 * http://hdl.handle.net/10380/3559
 * http://www.vtkjournal.org/browse/publication/975
 * @warning
 * This filter is specialized to volumes. If you are interested in
 * contouring other types of data, use the general vtkContourFilter. If you
 * want to contour an image (i.e., a volume slice), use vtkMarchingSquares.
 * @sa
 * vtkContourFilter vtkSliceCubes vtkMarchingSquares vtkDividingCubes
*/

#ifndef vtkDiscreteMarchingCubes_h
#define vtkDiscreteMarchingCubes_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkMarchingCubes.h"

class VTKFILTERSGENERAL_EXPORT vtkDiscreteMarchingCubes : public vtkMarchingCubes
{
public:
  static vtkDiscreteMarchingCubes *New();
  vtkTypeMacro(vtkDiscreteMarchingCubes,vtkMarchingCubes);

  //@{
  /**
   * Set/Get the computation of neighbouring voxel values.
   */
  vtkSetMacro(ComputeAdjacentScalars,int);
  vtkGetMacro(ComputeAdjacentScalars,int);
  vtkBooleanMacro(ComputeAdjacentScalars,int);
  //@}

protected:
  vtkDiscreteMarchingCubes();
  ~vtkDiscreteMarchingCubes() VTK_OVERRIDE;

  int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *) VTK_OVERRIDE;
  int ComputeAdjacentScalars;

private:
  vtkDiscreteMarchingCubes(const vtkDiscreteMarchingCubes&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDiscreteMarchingCubes&) VTK_DELETE_FUNCTION;

};

#endif


