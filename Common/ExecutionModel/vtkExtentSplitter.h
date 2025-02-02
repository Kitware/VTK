// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkExtentSplitter
 * @brief   Split an extent across other extents.
 *
 * vtkExtentSplitter splits each input extent into non-overlapping
 * sub-extents that are completely contained within other "source
 * extents".  A source extent corresponds to some resource providing
 * an extent.  Each source extent has an integer identifier, integer
 * priority, and an extent.  The input extents are split into
 * sub-extents according to priority, availability, and amount of
 * overlap of the source extents.  This can be used by parallel data
 * readers to read as few piece files as possible.
 */

#ifndef vtkExtentSplitter_h
#define vtkExtentSplitter_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkDeprecation.h"                // For VTK_DEPRECATED_IN_9_5_0
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkExtentSplitterInternals;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkExtentSplitter : public vtkObject
{
public:
  vtkTypeMacro(vtkExtentSplitter, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkExtentSplitter* New();

  ///@{
  /**
   * Add/Remove a source providing the given extent.  Sources with
   * higher priority numbers are favored.  Source id numbers and
   * priorities must be non-negative.
   */
  void AddExtentSource(int id, int priority, int x0, int x1, int y0, int y1, int z0, int z1);
  void AddExtentSource(int id, int priority, int* extent);
  void RemoveExtentSource(int id);
  void RemoveAllExtentSources();
  ///@}

  ///@{
  /**
   * Add an extent to the queue of extents to be split among the
   * available sources.
   */
  void AddExtent(int x0, int x1, int y0, int y1, int z0, int z1);
  void AddExtent(int* extent);
  ///@}

  /**
   * Split the extents currently in the queue among the available
   * sources.  The queue is empty when this returns.  Returns 1 if all
   * extents could be read.  Returns 0 if any portion of any extent
   * was not available through any source.
   */
  int ComputeSubExtents();

  /**
   * Get the number of sub-extents into which the original set of
   * extents have been split across the available sources.  Valid
   * after a call to ComputeSubExtents.
   */
  int GetNumberOfSubExtents();

  ///@{
  /**
   * Get the sub-extent associated with the given index.  Use
   * GetSubExtentSource to get the id of the source from which this
   * sub-extent should be read.  Valid after a call to
   * ComputeSubExtents.
   */
  int* GetSubExtent(int index) VTK_SIZEHINT(6);
  void GetSubExtent(int index, int* extent);
  ///@}

  /**
   * Get the id of the source from which the sub-extent associated
   * with the given index should be read.  Returns -1 if no source
   * provides the sub-extent.
   */
  int GetSubExtentSource(int index);

  ///@{
  /**
   * Get/Set whether "point mode" is on.  In point mode, sub-extents
   * are generated to ensure every point in the update request is
   * read, but not necessarily every cell.  This can be used when
   * point data are stored in a planar slice per piece with no cell
   * data.  The default is OFF.
   */
  vtkGetMacro(PointMode, vtkTypeBool);
  vtkSetMacro(PointMode, vtkTypeBool);
  vtkBooleanMacro(PointMode, vtkTypeBool);
  ///@}

protected:
  vtkExtentSplitter();
  ~vtkExtentSplitter() override;

  // Internal utility methods.
  void SplitExtent(int* extent, int* subextent);
  int IntersectExtents(const int* extent1, const int* extent2, int* result);
  VTK_DEPRECATED_IN_9_5_0("Use std::min instead")
  int Min(int a, int b);
  VTK_DEPRECATED_IN_9_5_0("Use std::max instead")
  int Max(int a, int b);

  // Internal implementation data.
  vtkExtentSplitterInternals* Internal;

  // On if reading only all points (but not always all cells) is
  // necessary.  Used for reading volumes of planar slices storing
  // only point data.
  vtkTypeBool PointMode;

private:
  vtkExtentSplitter(const vtkExtentSplitter&) = delete;
  void operator=(const vtkExtentSplitter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
