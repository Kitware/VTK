/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFlyingEdgesPlaneCutter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFlyingEdgesPlaneCutter
 * @brief   cut a volume with a plane and generate a
 * polygonal cut surface
 *
 * vtkFlyingEdgesPlaneCutter is a specialization of the FlyingEdges algorithm
 * to cut a volume with a single plane. It is designed for performance and
 * an exploratory, fast workflow.
 *
 * This algorithm is not only fast because it uses flying edges, but also
 * because it plays some "tricks" during processing. For example, rather
 * than evaluate the cut (plane) function on all volume points like vtkCutter
 * and its ilk do, this algorithm intersects the volume x-edges against the
 * plane to (potentially) generate the single intersection point. It then
 * quickly classifies the voxel edges as above, below, or straddling the cut
 * plane. Thus the number of plane evaluations is greatly reduced.
 *
 * For more information see vtkFlyingEdges3D and/or the paper "Flying Edges:
 * A High-Performance Scalable Isocontouring Algorithm" by Schroeder,
 * Maynard, Geveci. Proc. of LDAV 2015. Chicago, IL.
 *
 * @warning
 * This filter is specialized to 3D volumes. This implementation can produce
 * degenerate triangles (i.e., zero-area triangles).
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkFlyingEdges2D vtkFlyingEdges3D
*/

#ifndef vtkFlyingEdgesPlaneCutter_h
#define vtkFlyingEdgesPlaneCutter_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkImageData;
class vtkPlane;

class VTKFILTERSCORE_EXPORT vtkFlyingEdgesPlaneCutter : public vtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard construction and print methods.
   */
  static vtkFlyingEdgesPlaneCutter *New();
  vtkTypeMacro(vtkFlyingEdgesPlaneCutter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * The modified time depends on the delegated cut plane.
   */
  vtkMTimeType GetMTime() override;

  //@{
  /**
   * Specify the plane (an implicit function) to perform the cutting. The
   * definition of the plane (its origin and normal) is controlled via this
   * instance of vtkPlane.
   */
  virtual void SetPlane(vtkPlane*);
  vtkGetObjectMacro(Plane,vtkPlane);
  //@}

  //@{
  /**
   * Set/Get the computation of normals. The normal generated is simply the
   * cut plane normal. By default this is disabled.
   */
  vtkSetMacro(ComputeNormals,vtkTypeBool);
  vtkGetMacro(ComputeNormals,vtkTypeBool);
  vtkBooleanMacro(ComputeNormals,vtkTypeBool);
  //@}

  //@{
  /**
   * Indicate whether to interpolate other attribute data besides the input
   * scalars (which are required). That is, as the isosurface is generated,
   * interpolate all other point attribute data across intersected edges.
   */
  vtkSetMacro(InterpolateAttributes,vtkTypeBool);
  vtkGetMacro(InterpolateAttributes,vtkTypeBool);
  vtkBooleanMacro(InterpolateAttributes,vtkTypeBool);
  //@}

  //@{
  /**
   * Set/get which component of the scalar array to contour on; defaults to 0.
   */
  vtkSetMacro(ArrayComponent, int);
  vtkGetMacro(ArrayComponent, int);
  //@}

protected:
  vtkFlyingEdgesPlaneCutter();
  ~vtkFlyingEdgesPlaneCutter() override;

  vtkPlane *Plane;
  vtkTypeBool ComputeNormals;
  vtkTypeBool InterpolateAttributes;
  int ArrayComponent;

  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) override;
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *) override;
  int FillInputPortInformation(int port, vtkInformation *info) override;

private:
  vtkFlyingEdgesPlaneCutter(const vtkFlyingEdgesPlaneCutter&) = delete;
  void operator=(const vtkFlyingEdgesPlaneCutter&) = delete;
};

#endif
