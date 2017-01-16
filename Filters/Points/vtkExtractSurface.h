/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSurface.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkExtractSurface
 * @brief   generate zero-crossing isosurface from
 * truncated signed distance volume
 *
 *
 * This filter extracts the zero-crossing isosurface from a truncated signed
 * distance function TSDF. The TSDF is sampled across a volume, and is
 * extracted using a modified version of the Flying Edges algorithm for
 * increased speed, and to support multithreading. To use the filter, an
 * input volume should be assigned, which may have special values indicating
 * empty and/or unseen portions of the volume. These values are equal to +/-
 * radius value of the signed distance function, and should be consistent
 * with any filters used to generate the input volume (e.g.,
 * vtkSignedDistance).
 *
 * The Flying Edges algorithm is modified to deal with the nature of the
 * truncated, signed distance function. Being truncated, the distance
 * function typically is not computed throughout the volume, rather the
 * special data values "unseen" and/or "empty" maybe assigned to distant or
 * bordering voxels. The implications of this are that this implementation
 * may produce non-closed, non-manifold surfaces, which is what is needed
 * to extract surfaces.
 *
 * More specifically, voxels may exist in one of three states: 1) within the
 * TSDF, which extends +/-Radius from a generating geometry (typically a
 * point cloud); 2) in the empty state, in which it is known that the surface
 * does not exist; and 3) the unseen state, where a surface may exist but not
 * enough information is known to be certain. Such situations arise, for
 * example, when laser scanners generate point clouds, and the propagation of
 * the laser beam "carves" out regions where no geometry exists (thereby
 * defining empty space). Furthermore, areas in which the beam are occluded
 * by geometry are known as "unseen" and the boundary between empty and
 * unseen can be processed to produced a portion of the output isosurface
 * (this is called hole filling).
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @warning
 * <pre>
 * Notes on the implementation:
 * 1. This is a lightly modified version of vtkFlyingEdges3D. Some design goals
 *    included minimizing the impact on the FE algorithm, and not adding extra
 *    memory requirements.
 * 2. It presumes an isocontour value=0.0  (the zero crossing of a signed
 *    distance function).
 * 3. The major modifications are to the edge cases. In Flying Edges, a single
 *    byte represents the case of an edge, and within that byte only 2 bits
 *    are needed (the extra six bytes are not used). Here, these unused bytes
 *    are repurposed to represent the "state" of the edge, whether it is
 *    1) near to the TSDF; 2) in an empty state; or 3) unseen state.
 * 4. Since these now-used bits encode extra state information, masking and
 *    related methods are used to tease apart the edge cases from the edge
 *    state.
 * 5. Voxels with edges marked "empty" are not processed, i.e., no output
 *    triangle primitives are generated. Depending on whether hole filling is
 *    enabled, voxels with edges marked "unseen" may not be processed either.
 * 6. As a result of #1 and #5, and the desire to keep the implementation simple,
 *    it is possible to produce output points which are not attached to any output
 *    triangle.
 *</pre>
 *
 * @warning
 * This algorithm loosely follows the most excellent paper by Curless and
 * Levoy: "A Volumetric Method for Building Complex Models from Range
 * Images."
 *
 * @sa
 * vtkSignedDistance vtkFlyingEdges3D
*/

#ifndef vtkExtractSurface_h
#define vtkExtractSurface_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"
#include "vtkContourValues.h" // Passes calls through

class vtkImageData;

class VTKFILTERSPOINTS_EXPORT vtkExtractSurface : public vtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiating the class, providing type information,
   * and printing.
   */
  static vtkExtractSurface *New();
  vtkTypeMacro(vtkExtractSurface,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Specify the radius of influence of the signed distance function. Data
   * values (which are distances) that are greater than or equal to the
   * radius (i.e., d >= Radius) are considered unseen voxels; those voxel
   * data values d <= -Radius are considered empty voxels.
   */
  vtkSetClampMacro(Radius,double,0.0,VTK_FLOAT_MAX);
  vtkGetMacro(Radius,double);
  //@}

  //@{
  /**
   * Enable hole filling. This generates separating surfaces between the
   * empty and unseen portions of the volume.
   */
  vtkSetMacro(HoleFilling,bool);
  vtkGetMacro(HoleFilling,bool);
  vtkBooleanMacro(HoleFilling,bool);
  //@}

  //@{
  /**
   * Set/Get the computation of normals. Normal computation is fairly
   * expensive in both time and storage. If the output data will be processed
   * by filters that modify topology or geometry, it may be wise to turn
   * Normals and Gradients off.
   */
  vtkSetMacro(ComputeNormals,int);
  vtkGetMacro(ComputeNormals,int);
  vtkBooleanMacro(ComputeNormals,int);
  //@}

  //@{
  /**
   * Set/Get the computation of gradients. Gradient computation is fairly
   * expensive in both time and storage. Note that if ComputeNormals is on,
   * gradients will have to be calculated, but will not be stored in the
   * output dataset. If the output data will be processed by filters that
   * modify topology or geometry, it may be wise to turn Normals and
   * Gradients off.
   */
  vtkSetMacro(ComputeGradients,int);
  vtkGetMacro(ComputeGradients,int);
  vtkBooleanMacro(ComputeGradients,int);
  //@}

protected:
  vtkExtractSurface();
  ~vtkExtractSurface() VTK_OVERRIDE;

  double Radius;
  bool HoleFilling;
  int ComputeNormals;
  int ComputeGradients;

  int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *) VTK_OVERRIDE;
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                                  vtkInformationVector *) VTK_OVERRIDE;
  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

private:
  vtkExtractSurface(const vtkExtractSurface&) VTK_DELETE_FUNCTION;
  void operator=(const vtkExtractSurface&) VTK_DELETE_FUNCTION;
};

#endif
