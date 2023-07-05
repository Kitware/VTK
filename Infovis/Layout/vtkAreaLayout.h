// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkAreaLayout
 * @brief   layout a vtkTree into a tree map
 *
 *
 * vtkAreaLayout assigns sector regions to each vertex in the tree,
 * creating a tree ring.  The data is added as a data array with four
 * components per tuple representing the location and size of the
 * sector using the format (StartAngle, EndAngle, innerRadius, outerRadius).
 *
 * This algorithm relies on a helper class to perform the actual layout.
 * This helper class is a subclass of vtkAreaLayoutStrategy.
 *
 * @par Thanks:
 * Thanks to Jason Shepherd from Sandia National Laboratories
 * for help developing this class.
 */

#ifndef vtkAreaLayout_h
#define vtkAreaLayout_h

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkTreeAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkAreaLayoutStrategy;

class VTKINFOVISLAYOUT_EXPORT vtkAreaLayout : public vtkTreeAlgorithm
{
public:
  static vtkAreaLayout* New();
  vtkTypeMacro(vtkAreaLayout, vtkTreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * The array name to use for retrieving the relative size of each vertex.
   * If this array is not found, use constant size for each vertex.
   */
  virtual void SetSizeArrayName(const char* name)
  {
    this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, name);
  }

  ///@{
  /**
   * The name for the array created for the area for each vertex.
   * The rectangles are stored in a quadruple float array
   * (startAngle, endAngle, innerRadius, outerRadius).
   * For rectangular layouts, this is (minx, maxx, miny, maxy).
   */
  vtkGetStringMacro(AreaArrayName);
  vtkSetStringMacro(AreaArrayName);
  ///@}

  ///@{
  /**
   * Whether to output a second output tree with vertex locations
   * appropriate for routing bundled edges. Default is on.
   */
  vtkGetMacro(EdgeRoutingPoints, bool);
  vtkSetMacro(EdgeRoutingPoints, bool);
  vtkBooleanMacro(EdgeRoutingPoints, bool);
  ///@}

  ///@{
  /**
   * The strategy to use when laying out the tree map.
   */
  vtkGetObjectMacro(LayoutStrategy, vtkAreaLayoutStrategy);
  void SetLayoutStrategy(vtkAreaLayoutStrategy* strategy);
  ///@}

  /**
   * Get the modification time of the layout algorithm.
   */
  vtkMTimeType GetMTime() override;

  /**
   * Get the vertex whose area contains the point, or return -1
   * if no vertex area covers the point.
   */
  vtkIdType FindVertex(float pnt[2]);

  /**
   * The bounding area information for a certain vertex id.
   */
  void GetBoundingArea(vtkIdType id, float* sinfo);

protected:
  vtkAreaLayout();
  ~vtkAreaLayout() override;

  char* AreaArrayName;
  bool EdgeRoutingPoints;
  char* EdgeRoutingPointsArrayName;
  vtkAreaLayoutStrategy* LayoutStrategy;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkAreaLayout(const vtkAreaLayout&) = delete;
  void operator=(const vtkAreaLayout&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
