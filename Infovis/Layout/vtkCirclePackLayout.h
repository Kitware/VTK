// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkCirclePackLayout
 * @brief   layout a vtkTree as a circle packing.
 *
 *
 * vtkCirclePackLayout assigns circle shaped regions to each vertex
 * in the tree, creating a circle packing layout.  The data is added
 * as a data array with three components per tuple representing the
 * center and radius of the circle using the format (Xcenter, Ycenter, Radius).
 *
 * This algorithm relies on a helper class to perform the actual layout.
 * This helper class is a subclass of vtkCirclePackLayoutStrategy.
 *
 * An array by default called "size" can be attached to the input tree
 * that specifies the size of each leaf node in the tree.  The filter will
 * calculate the sizes of all interior nodes in the tree based on the sum
 * of the leaf node sizes.  If no "size" array is given in the input vtkTree,
 * a size of 1 is used for all leaf nodes to find the size of the interior nodes.
 *
 * @par Thanks:
 * Thanks to Thomas Otahal from Sandia National Laboratories
 * for help developing this class.
 *
 */

#ifndef vtkCirclePackLayout_h
#define vtkCirclePackLayout_h

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkTreeAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCirclePackLayoutStrategy;
class vtkDoubleArray;
class vtkDataArray;
class vtkTree;

class VTKINFOVISLAYOUT_EXPORT vtkCirclePackLayout : public vtkTreeAlgorithm
{
public:
  static vtkCirclePackLayout* New();

  vtkTypeMacro(vtkCirclePackLayout, vtkTreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * The field name to use for storing the circles for each vertex.
   * The rectangles are stored in a triple float array
   * (Xcenter, Ycenter, Radius).
   * Default name is "circles"
   */
  vtkGetStringMacro(CirclesFieldName);
  vtkSetStringMacro(CirclesFieldName);
  ///@}

  /**
   * The array to use for the size of each vertex.
   * Default name is "size".
   */
  virtual void SetSizeArrayName(const char* name)
  {
    this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, name);
  }

  ///@{
  /**
   * The strategy to use when laying out the tree map.
   */
  vtkGetObjectMacro(LayoutStrategy, vtkCirclePackLayoutStrategy);
  void SetLayoutStrategy(vtkCirclePackLayoutStrategy* strategy);
  ///@}

  /**
   * Returns the vertex id that contains pnt (or -1 if no one contains it)
   * pnt[0] is x, and pnt[1] is y.
   * If cinfo[3] is provided, then (Xcenter, Ycenter, Radius) of the circle
   * containing pnt[2] will be returned.
   */
  vtkIdType FindVertex(double pnt[2], double* cinfo = nullptr);

  /**
   * Return the Xcenter, Ycenter, and Radius of the
   * vertex's bounding circle
   */
  void GetBoundingCircle(vtkIdType id, double* cinfo);

  /**
   * Get the modification time of the layout algorithm.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkCirclePackLayout();
  ~vtkCirclePackLayout() override;

  char* CirclesFieldName;
  vtkCirclePackLayoutStrategy* LayoutStrategy;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkCirclePackLayout(const vtkCirclePackLayout&) = delete;
  void operator=(const vtkCirclePackLayout&) = delete;
  void prepareSizeArray(vtkDoubleArray* mySizeArray, vtkTree* tree);
};

VTK_ABI_NAMESPACE_END
#endif
