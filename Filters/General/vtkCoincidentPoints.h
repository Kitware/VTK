// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-NVIDIA-USGov
/**
 * @class   vtkCoincidentPoints
 * @brief   contains an octree of labels
 *
 *
 * This class provides a collection of points that is organized such that
 * each coordinate is stored with a set of point id's of points that are
 * all coincident.
 */

#ifndef vtkCoincidentPoints_h
#define vtkCoincidentPoints_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkIdList;
class vtkPoints;

class VTKFILTERSGENERAL_EXPORT vtkCoincidentPoints : public vtkObject
{
public:
  static vtkCoincidentPoints* New();
  vtkTypeMacro(vtkCoincidentPoints, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Accumulates a set of Ids in a map where the point coordinate
   * is the key. All Ids in a given map entry are thus coincident.
   * @param Id - a unique Id for the given \a point that will be stored in an vtkIdList.
   * @param[in] point - the point coordinate that we will store in the map to test if any other
   * points are coincident with it.
   */
  void AddPoint(vtkIdType Id, const double point[3]);

  /**
   * Retrieve the list of point Ids that are coincident with the given \a point.
   * @param[in] point - the coordinate of coincident points we want to retrieve.
   */
  vtkIdList* GetCoincidentPointIds(const double point[3]);

  /**
   * Used to iterate the sets of coincident points within the map.
   * InitTraversal must be called first or nullptr will always be returned.
   */
  vtkIdList* GetNextCoincidentPointIds();

  /**
   * Initialize iteration to the beginning of the coincident point map.
   */
  void InitTraversal();

  /**
   * Iterate through all added points and remove any entries that have
   * no coincident points (only a single point Id).
   */
  void RemoveNonCoincidentPoints();

  /**
   * Clear the maps for reuse. This should be called if the caller
   * might reuse this class (another executive pass for instance).
   */
  void Clear();

  class implementation;
  implementation* GetImplementation() { return this->Implementation; }

  /**
   * Calculate \a num points, at a regular interval, along a parametric
   * spiral. Note this spiral is only in two dimensions having a constant
   * z value.
   */
  static void SpiralPoints(vtkIdType num, vtkPoints* offsets);

protected:
  vtkCoincidentPoints();
  ~vtkCoincidentPoints() override;

private:
  vtkCoincidentPoints(const vtkCoincidentPoints&) = delete;
  void operator=(const vtkCoincidentPoints&) = delete;

  implementation* Implementation;

  friend class implementation;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCoincidentPoints_h
