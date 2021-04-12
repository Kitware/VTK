/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataPlaneClipper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPolyDataPlaneClipper
 * @brief   clip a vtkPolyData with a plane and optionally cap it
 *
 * vtkPolyDataPlaneClipper clips an input vtkPolyData with a plane to produce
 * an output vtkPolyData. (Here clipping means extracting cells, or portions
 * of cells, that are on one side of a specified plane.) The input
 * vtkPolyData must consist of convex polygons forming one or more manifold
 * shells (use vtkTriangleFilter to triangulate the input if necessary. Note
 * that if the input cells are non-convex, then the clipping operation will
 * likely produce erroneous results.)
 *
 * An optional, second vtkPolyData output may also be generated if either
 * ClippingLoops or Capping is enabled. The clipping loops are a set of lines
 * representing the curve(s) of intersection between the plane and the one or
 * more shells of the input vtkPolyData. If Capping is enabled, then the
 * clipping loops are tessellated to produce a "cap" across the clipped
 * output. The capping option is only available if the input consists of one
 * or more manifold shells. If not, the loop generation will fail and no
 * cap(s) will be generated.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkClipPolyData vtkClipClosedSurface vtkPlaneCutter vtkTriangleFilter vtkCutter
 */

#ifndef vtkPolyDataPlaneClipper_h
#define vtkPolyDataPlaneClipper_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPlane.h"             // For clipping plane
#include "vtkPolyDataAlgorithm.h"
#include "vtkSmartPointer.h" // For SmartPointer

class VTKFILTERSCORE_EXPORT vtkPolyDataPlaneClipper : public vtkPolyDataAlgorithm
{
public:
  ///@{
  /**
   * Standard construction, type, and print methods.
   */
  static vtkPolyDataPlaneClipper* New();
  vtkTypeMacro(vtkPolyDataPlaneClipper, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * The modified time depends on the delegated clipping plane.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Specify the plane (an implicit function) to perform the clipping. The
   * definition of the plane used to perform the clipping (i.e., its origin
   * and normal) is controlled via this instance of vtkPlane.
   */
  void SetPlane(vtkPlane*);
  vtkGetObjectMacro(Plane, vtkPlane);
  ///@}

  ///@{
  /**
   * Specify whether to generate clipping loops, i.e., the intersection of
   * the plane with the input polydata. The generation of clipping loops will
   * function correctly even if the input vtkPolyData consists of non-closed
   * shells; however if the shells are not closed, the loops will not be
   * either. If enabled, a second vtkPolyData output will be produced that
   * contains the clipping loops (in vtkPolyData::Lines)
   */
  vtkSetMacro(ClippingLoops, bool);
  vtkGetMacro(ClippingLoops, bool);
  vtkBooleanMacro(ClippingLoops, bool);
  ///@}

  ///@{
  /**
   * Specify whether to cap the clipped output vtkPolyData. If enabled, a
   * second vtkPolyData output will be produced that contains the capping
   * polygons (in vtkPolyData:Polys). Note that the capping operation assumes
   * that the input to this filter is a manifold shell. If not, no output
   * will be generated. Note that point data or cell data is not produced on
   * this second output (because the results of interpolation across the
   * cap(s) are generally nonsensical).
   */
  vtkSetMacro(Capping, bool);
  vtkGetMacro(Capping, bool);
  vtkBooleanMacro(Capping, bool);
  ///@}

  /**
   * Get the output dataset representing the clipping loops and capping
   * polygons.  This output is empty if both ClippingLoops and Capping is
   * off. Otherwise, if there is an intersection with the clipping plane,
   * then polyline loops are available from the vtkPolyData::Lines, and the
   * capping polygons are available from the vtkPolyData::Polys data arrays.
   */
  vtkPolyData* GetCap();

  ///@{
  /**
   * Set/get the desired precision for the output points type. See the
   * documentation for the vtkAlgorithm::DesiredOutputPrecision enum for an
   * explanation of the available precision settings. OutputPointsPrecision
   * is DEFAULT_PRECISION by default.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

  ///@{
  /**
   * Specify the number of input triangles in a batch, where a batch defines
   * a subset of the input triangles operated on during threaded
   * execution. Generally this is only used for debugging or performance
   * studies (since batch size affects the thread workload).
   */
  vtkSetClampMacro(BatchSize, unsigned int, 1, VTK_INT_MAX);
  vtkGetMacro(BatchSize, unsigned int);
  ///@}

protected:
  vtkPolyDataPlaneClipper();
  ~vtkPolyDataPlaneClipper() override;

  vtkSmartPointer<vtkPlane> Plane;
  bool ClippingLoops;
  bool Capping;
  int OutputPointsPrecision;
  unsigned int BatchSize;

  // Pipeline-related methods
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkPolyDataPlaneClipper(const vtkPolyDataPlaneClipper&) = delete;
  void operator=(const vtkPolyDataPlaneClipper&) = delete;
};

#endif
