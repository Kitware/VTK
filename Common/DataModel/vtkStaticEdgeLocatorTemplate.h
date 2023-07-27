// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStaticEdgeLocatorTemplate
 * @brief   templated locator for managing edges and associated data on edges
 *
 *
 * vtkStaticEdgeLocatorTemplate provides methods for ordering and tracking
 * edges, as well as associating data with edges. (An edge is a tuple (v0,v1)
 * with v0 < v1.) Typically this class may be used for isocontouring or any
 * operation that operates on edges and needs to determine whether duplicates
 * exist. The class is templated on 1) the type used to represent the id
 * tuple; and 2) the data associated with the edge.
 *
 * This class is non-incremental (i.e., static). That is, an array of edges
 * must be provided and the locator is built from this array. Once the locator
 * is built, incremental additions of new edges is not allowed (analogoues to
 * vtkStaticPointLocator and vtkStaticCellLocator).
 *
 * Finally, there are two distinct usage patterns for this class. One is to
 * inject edges and then later search for them. This pattern begins with
 * BuildLocator() and then is followed by repeated calls to IsInsertedEdge().
 * Internally this operates on an array of EdgeTupleType. The second pattern
 * also operates on an array of EdgeTupleType. It simply sorts an array of
 * vtkEdgeTupleType using MergeEdges(), thereby grouping identical edges.
 * An offset array is created that refers to the beginning of each group,
 * hence indirectly indicating the number of unique edges, and providing O(1)
 * access to each edge. Typically the offset array can be uses to renumber
 * duplicate edges and/or data (such as points) associated with the edge.
 *
 * @warning
 * The id tuple type can be specified via templating to reduce memory and
 * speed processing.
 *
 * @warning
 * Data is associated with edges (e.g., a parametric coordinate, or original
 * id). By using the appropriate template parameter it is possible to
 * associate other data with each edge. Note however that this data is not
 * used when comparing and sorting the edges. (This could be changed - define
 * appropriate comparison operators.)
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkEdgeTable vtkStaticPointLocator vtkStaticCellLocator
 */

#ifndef vtkStaticEdgeLocatorTemplate_h
#define vtkStaticEdgeLocatorTemplate_h

#include "vtkABINamespace.h"

#include <algorithm>
#include <vector>

/**
 * Definition of an edge tuple. Note that the TId template type may be
 * specified to manage memory resources, and provide increased speeds (e.g.,
 * sort) by using smaller types (32-int versus 64-bit vtkIdType). It is
 * required that V0 < V1; the tuple constructor enforces this.
 */
VTK_ABI_NAMESPACE_BEGIN
template <typename TId, typename TED>
struct EdgeTuple
{
  TId V0;
  TId V1;
  TED Data;

  // Default constructor - nothing needs to be done
  EdgeTuple() = default;

  // Construct an edge and ensure that the edge tuple (vo,v1) is
  // specified such that (v0<v1).
  EdgeTuple(TId v0, TId v1, TED data)
    : V0(v0)
    , V1(v1)
    , Data(data)
  {
    if (this->V0 > this->V1)
    {
      std::swap(this->V0, this->V1);
    }
  }

  void Define(TId v0, TId v1)
  {
    if (v0 < v1)
    {
      this->V0 = v0;
      this->V1 = v1;
    }
    else
    {
      this->V0 = v1;
      this->V1 = v0;
    }
  }

  bool operator==(const EdgeTuple& et) const { return this->V0 == et.V0 && this->V1 == et.V1; }

  bool operator!=(const EdgeTuple& et) const { return this->V0 != et.V0 || this->V1 != et.V1; }

  bool IsEdge(TId v0, TId v1) const
  {
    if (v0 < v1) // ordered properly
    {
      return this->V0 == v0 && this->V1 == v1;
    }
    else // swap comparison required
    {
      return this->V0 == v1 && this->V1 == v0;
    }
  }
  // Sort on v0 first, then v1.
  bool operator<(const EdgeTuple& tup) const
  {
    if (this->V0 < tup.V0)
      return true;
    if (tup.V0 < this->V0)
      return false;
    if (this->V1 < tup.V1)
      return true;
    return false;
  }
};

/**
 * Templated on types of ids defining an edge, and any data associated with
 * the edge.
 */
template <typename IDType, typename EdgeData>
class vtkStaticEdgeLocatorTemplate
{
public:
  ///@{
  /**
   * Some convenient typedefs.
   */
  typedef EdgeTuple<IDType, EdgeData> EdgeTupleType;
  ///@)

  /**
   * Construct an empty edge locator.
   */
  vtkStaticEdgeLocatorTemplate()
    : NumEdges(0)
    , NumEdgesPerBin(5)
    , EdgeArray(nullptr)
    , EdgeOffsets(nullptr)
    , MinV0(-1)
    , MaxV0(-1)
    , V0Range(0)
    , NDivs(0)
    , MergeArray(nullptr)
  {
  }

  /**
   * Delete internal offset array. The edgeArray is provided from outside the
   * class and so not deleted.
   */
  ~vtkStaticEdgeLocatorTemplate() { delete[] this->EdgeOffsets; }

  /**
   * Return the number of edges in the edge array.
   */
  IDType GetNumberOfEdges() { return this->NumEdges; }

  /**
   * This method sorts (in place) an array of EdgeTupleType (of length
   * numEdges) into separate groups, and allocates and returns an offset
   * array providing access to each group. Each grouping is a list of
   * duplicate edges. The method indicates the number of unique edges
   * numUniqueEdges. Note that the offset array end value
   * offsets[numUniqueEdges] = numEdges, i.e., total allocation of the
   * offsets array is numUniqueEdges+1. Also note that the EId contained in
   * the sorted EdgeTuples can be used to represent data on edges, and
   * perform operations such as renumber edges from initial edge ids
   * (possibly one of several duplicates) to unique edge ids.
   */
  const IDType* MergeEdges(vtkIdType numEdges, EdgeTupleType* edgeArray, vtkIdType& numUniqueEdges);

  /**
   * This method constructs the edge locator to be used when searching for
   * edges. Basically it does a sort of the provided numEdges edges (which
   * likely contains duplicates), and builds an offset table to provide rapid
   * access to edge (v0,v1). The sort is performed via a parallel
   * vtkSMPTools::Sort(). The provided array is modified in place. The
   * method returns the number of unique edges.
   */
  vtkIdType BuildLocator(vtkIdType numEdges, EdgeTupleType* edgeArray);

  /**
   * Return the id of the edge indicated. If the edge has not been inserted
   * return <0. Note that the vertices (v0,v1) do not have to be in any
   * particular (ascending/descending) order. BuildLocator() should be called
   * prior to using this method.
   */
  IDType IsInsertedEdge(IDType v0, IDType v1) const
  {
    // Ensure that BuildLocator has been called by checking MinV0, MaxV0
    if (this->MinV0 < 0 || this->MaxV0 < 0)
    {
      return -1;
    }
    // Ensure that data is consistent with what is expected.
    if (v0 > v1)
    {
      std::swap(v0, v1);
    }
    if (v0 < this->MinV0 || v0 > this->MaxV0)
    {
      return -1;
    }

    // Bin and search for matching edge
    const IDType curBin = this->HashBin(v0);
    const IDType num = this->GetNumberOfEdgesInBin(curBin);
    // check if there are no edges
    if (num < 1)
    {
      return -1;
    }
    IDType curId = this->EdgeOffsets[curBin];
    IDType curV0 = this->EdgeArray[curId].V0;
    while (curV0 < v0)
    {
      curId++;
      curV0 = this->EdgeArray[curId].V0;
    }
    if (curV0 > v0)
    {
      return -1;
    }
    else // matched v0, now find v1
    {
      IDType curV1 = this->EdgeArray[curId].V1;
      while (curV1 < v1)
      {
        curId++;
        curV1 = this->EdgeArray[curId].V1;
      }
      if (curV1 > v1)
      {
        return -1;
      }
      else
      {
        return curId;
      }
    }
  }

  /**
   * Return the ith edge in the edge array. Either obtain a non-negative
   * value i from IsInsertedEdge(); or use 0<=i<NumberOfEdges().
   */
  const EdgeTupleType& GetEdge(IDType i) const { return (*this->EdgeArray)[i]; }

protected:
  vtkIdType NumEdges;

  // Support BuildLocator usage pattern
  vtkIdType NumEdgesPerBin;
  EdgeTupleType* EdgeArray;
  IDType* EdgeOffsets;
  IDType MinV0;
  IDType MaxV0;
  IDType V0Range;
  int NDivs;

  IDType HashBin(IDType v) const { return ((v - this->MinV0) / this->NumEdgesPerBin); }

  IDType GetNumberOfEdgesInBin(IDType bin) const
  {
    return (this->EdgeOffsets[bin + 1] - this->EdgeOffsets[bin]);
  }

  // Support MergeEdges usage pattern
  EdgeTupleType* MergeArray;
  std::vector<IDType> MergeOffsets;

private:
  vtkStaticEdgeLocatorTemplate(const vtkStaticEdgeLocatorTemplate&) = delete;
  void operator=(const vtkStaticEdgeLocatorTemplate&) = delete;
};

VTK_ABI_NAMESPACE_END
#include "vtkStaticEdgeLocatorTemplate.txx"

#endif
// VTK-HeaderTest-Exclude: vtkStaticEdgeLocatorTemplate.h
