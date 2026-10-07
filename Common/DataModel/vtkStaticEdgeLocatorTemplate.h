// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStaticEdgeLocatorTemplate
 * @brief   templated locator for managing edges and associated data on edges
 *
 * vtkStaticEdgeLocatorTemplate is a vtkStaticElementLocator of EdgeTuple, the edges (v0,v1) with
 * v0 < v1 and their data, with its former API: BuildLocator() then IsInsertedEdge() to search the
 * unique edges, or MergeEdges() to group the identical edges, whose offsets give the number of
 * unique edges and O(1) access to every group, e.g., to renumber duplicate edges or their data.
 * The data of the edges is not used when comparing and sorting them.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @deprecated
 * vtkStaticEdgeLocatorTemplate is deprecated: use vtkStaticElementLocator with EdgeTuple
 * instead, whose GroupElements() replaces MergeEdges() and gives the groups as vtkIdType, so
 * that there can be more edges than IDType holds.
 *
 * @sa
 * vtkEdgeTable vtkStaticPointLocator vtkStaticCellLocator vtkStaticElementLocator
 */

#ifndef vtkStaticEdgeLocatorTemplate_h
#define vtkStaticEdgeLocatorTemplate_h

#include "vtkABINamespace.h"
#include "vtkDeprecation.h" // For VTK_DEPRECATED_IN_9_8_0
#include "vtkSMPTools.h"    // For the copy of MergeEdges()
#include "vtkStaticElementLocator.h"
#include "vtkType.h" // For vtkIdType

#include <vector>

VTK_ABI_NAMESPACE_BEGIN

// Templated on the type of the ids of the edges, and their data.
template <typename IDType, typename EdgeData>
class VTK_DEPRECATED_IN_9_8_0(
  "Use vtkStaticElementLocator with EdgeTuple instead.") vtkStaticEdgeLocatorTemplate
  : public vtkStaticEdgeLocator<IDType, EdgeData>
{
public:
  ///@{
  /**
   * The edges, and the element locator of the edges.
   */
  using EdgeTupleType = EdgeTuple<IDType, EdgeData>;
  using Superclass = vtkStaticElementLocator<IDType, EdgeTupleType>;
  ///@}

  /**
   * Construct an empty edge locator.
   */
  vtkStaticEdgeLocatorTemplate() = default;
  ~vtkStaticEdgeLocatorTemplate() = default;

  /**
   * Return the number of unique edges, after BuildLocator().
   */
  IDType GetNumberOfEdges() const { return this->GetNumberOfElements(); }

  /**
   * Sort edgeArray so that identical edges are contiguous, and return the start of every group
   * of identical edges, of size (numGroups + 1). The offsets are owned by the locator and remain
   * valid until the next call of MergeEdges(). This does not affect BuildLocator().
   *
   * @warning
   * The offsets are of IDType, so the number of edges must fit IDType, see GroupElements().
   */
  const IDType* MergeEdges(vtkIdType numEdges, EdgeTupleType* edgeArray, vtkIdType& numGroups)
  {
    const auto& groups = this->GroupElements(numEdges, edgeArray);
    numGroups = groups.GetNumberOfGroups();
    if (numGroups <= 0)
    {
      this->MergeOffsets.clear();
      return nullptr;
    }
    // The start of every group, of IDType, which is assumed to hold the number of edges.
    this->MergeOffsets.resize(numGroups + 1);
    vtkSMPTools::For(0, numGroups + 1,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType g = begin; g < end; ++g)
        {
          this->MergeOffsets[g] = static_cast<IDType>(groups.GetGroupOffset(g));
        }
      });
    return this->MergeOffsets.data();
  }

  /**
   * Return the id of the edge (v0,v1), in any order, or a negative value if it was not inserted,
   * after BuildLocator().
   */
  IDType IsInsertedEdge(IDType v0, IDType v1) const { return this->IsInsertedElement(v0, v1); }

  /**
   * Return the ith unique edge, e.g., of a non-negative IsInsertedEdge(), 0 <= i <
   * GetNumberOfEdges().
   */
  const EdgeTupleType& GetEdge(IDType i) const { return this->GetElement(i); }

protected:
  std::vector<IDType> MergeOffsets; // The start of every group of MergeEdges()

private:
  vtkStaticEdgeLocatorTemplate(const vtkStaticEdgeLocatorTemplate&) = delete;
  void operator=(const vtkStaticEdgeLocatorTemplate&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkStaticEdgeLocatorTemplate.h
