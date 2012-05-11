/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoostGraphAdapter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkDirectedGraph*ToBoostAdapter - adapter to the boost graph library (www.boost.org)
//
// .SECTION Description
// Including this header allows you to use a vtkDirectedGraph* in boost algorithms.
// To do this, first wrap the class in a vtkDirectedGraph* or
// vtkUndirectedGraph* depending on whether your graph is directed or undirected.
// You may then use these objects directly in boost graph algorithms.

#ifndef __vtkBoostGraphAdapter_h
#define __vtkBoostGraphAdapter_h

#include "vtkAbstractArray.h"
#include "vtkDirectedGraph.h"
#include "vtkDistributedGraphHelper.h"
#include "vtkDataObject.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkTree.h"
#include "vtkUndirectedGraph.h"
#include "vtkVariant.h"

#include <boost/version.hpp>

namespace boost {
  //===========================================================================
  // VTK arrays as property maps
  // These need to be defined before including other boost stuff

  // Forward declarations are required here, so that we aren't forced
  // to include boost/property_map.hpp.
  template<typename> struct property_traits;
  struct read_write_property_map_tag;

#define vtkPropertyMapMacro(T, V)                       \
  template <>                                           \
  struct property_traits<T*>                            \
    {                                                   \
    typedef V value_type;                               \
    typedef V reference;                                \
    typedef vtkIdType key_type;                         \
    typedef read_write_property_map_tag category;       \
    };                                                  \
                                                        \
  inline property_traits<T*>::reference                 \
  get(                                                  \
    T* const & arr,                                     \
    property_traits<T*>::key_type key)                  \
  {                                                     \
    return arr->GetValue(key);                          \
  }                                                     \
                                                        \
  inline void                                           \
  put(                                                  \
    T* arr,                                             \
    property_traits<T*>::key_type key,                  \
    const property_traits<T*>::value_type & value)      \
  {                                                     \
    arr->InsertValue(key, value);                       \
  }

  vtkPropertyMapMacro(vtkIntArray, int)
  vtkPropertyMapMacro(vtkIdTypeArray, vtkIdType)
  vtkPropertyMapMacro(vtkDoubleArray, double)
  vtkPropertyMapMacro(vtkFloatArray, float)

  // vtkDataArray
  template<>
  struct property_traits<vtkDataArray*>
  {
    typedef double value_type;
    typedef double reference;
    typedef vtkIdType  key_type;
    typedef read_write_property_map_tag category;
  };

  inline double
  get(vtkDataArray * const& arr, vtkIdType key)
  {
    return arr->GetTuple1(key);
  }

  inline void
  put(vtkDataArray *arr, vtkIdType key, const double& value)
  {
    arr->SetTuple1(key, value);
  }

  // vtkAbstractArray as a property map of vtkVariants
  template<>
  struct property_traits<vtkAbstractArray*>
  {
    typedef vtkVariant value_type;
    typedef vtkVariant reference;
    typedef vtkIdType  key_type;
    typedef read_write_property_map_tag category;
  };

  inline vtkVariant
  get(vtkAbstractArray * const& arr, vtkIdType key)
  {
    return arr->GetVariantValue(key);
  }

  inline void
  put(vtkAbstractArray *arr, vtkIdType key, const vtkVariant& value)
  {
    arr->InsertVariantValue(key, value);
  }
}

#include <vtksys/stl/utility> // STL Header

#include <boost/config.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/adjacency_iterator.hpp>

// The functions and classes in this file allows the user to
// treat a vtkDirectedGraph or vtkUndirectedGraph object
// as a boost graph "as is".

namespace boost {

  class vtk_vertex_iterator :
    public iterator_facade<vtk_vertex_iterator,
                           vtkIdType,
                           bidirectional_traversal_tag,
                           vtkIdType,
                           vtkIdType>
    {
    public:
      explicit vtk_vertex_iterator(vtkIdType i = 0) : index(i) {}

    private:
      vtkIdType dereference() const { return index; }

      bool equal(const vtk_vertex_iterator& other) const
        { return index == other.index; }

      void increment() { index++; }
      void decrement() { index--; }

      vtkIdType index;

      friend class iterator_core_access;
    };

  class vtk_edge_iterator :
    public iterator_facade<vtk_edge_iterator,
                           vtkEdgeType,
                           forward_traversal_tag,
                           vtkEdgeType,
                           vtkIdType>
    {
    public:
      explicit vtk_edge_iterator(vtkGraph *g = 0, vtkIdType v = 0) :
        directed(false), vertex(v), lastVertex(v), iter(0), end(0), graph(g)
        {
        if (graph)
          {
          lastVertex = graph->GetNumberOfVertices();
          }

        vtkIdType myRank = -1;
        vtkDistributedGraphHelper *helper
          = this->graph? this->graph->GetDistributedGraphHelper() : 0;
        if (helper)
          {
          myRank = this->graph->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
          vertex = helper->MakeDistributedId(myRank, vertex);
          lastVertex = helper->MakeDistributedId(myRank, lastVertex);
          }

        if (graph != 0)
          {
          directed = (vtkDirectedGraph::SafeDownCast(graph) != 0);
          while (vertex < lastVertex && this->graph->GetOutDegree(vertex) == 0)
            {
            ++vertex;
            }

          if (vertex < lastVertex)
            {
            // Get the outgoing edges of the first vertex that has outgoing
            // edges
            vtkIdType nedges;
            graph->GetOutEdges(vertex, iter, nedges);
            end = iter + nedges;

            if (!directed)
              {
              while(iter != 0
                    && (// Skip non-local edges
                        (helper && helper->GetEdgeOwner(iter->Id) != myRank)
                        // Skip entirely-local edges where Source > Target
                        || (((helper
                              && myRank == helper->GetVertexOwner(iter->Target))
                             || !helper)
                            && vertex > iter->Target)))
                {
                this->inc();
                }
              }
            }
          else
            {
            iter = 0;
            }
          }
        }

    private:
      vtkEdgeType dereference() const
        { return vtkEdgeType(vertex, iter->Target, iter->Id); }

      bool equal(const vtk_edge_iterator& other) const
        { return vertex == other.vertex && iter == other.iter; }

      void increment()
        {
        inc();
        if (!directed)
          {
          vtkIdType myRank = -1;
          vtkDistributedGraphHelper *helper
            = this->graph? this->graph->GetDistributedGraphHelper() : 0;
          if (helper)
            {
            myRank = this->graph->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
            }

          while (iter != 0
                 && (// Skip non-local edges
                     (helper && helper->GetEdgeOwner(iter->Id) != myRank)
                     // Skip entirely-local edges where Source > Target
                     || (((helper
                           && myRank == helper->GetVertexOwner(iter->Target))
                          || !helper)
                         && vertex > iter->Target)))
            {
            inc();
            }
          }
        }

      void inc()
        {
        ++iter;
        if (iter == end)
          {
          // Find a vertex with nonzero out degree.
          ++vertex;
          while (vertex < lastVertex && this->graph->GetOutDegree(vertex) == 0)
            {
            ++vertex;
            }

          if (vertex < lastVertex)
            {
            vtkIdType nedges;
            graph->GetOutEdges(vertex, iter, nedges);
            end = iter + nedges;
            }
          else
            {
            iter = 0;
            }
          }
        }

      bool directed;
      vtkIdType vertex;
      vtkIdType lastVertex;
      const vtkOutEdgeType * iter;
      const vtkOutEdgeType * end;
      vtkGraph *graph;

      friend class iterator_core_access;
    };

  class vtk_out_edge_pointer_iterator :
    public iterator_facade<vtk_out_edge_pointer_iterator,
                           vtkEdgeType,
                           bidirectional_traversal_tag,
                           vtkEdgeType,
                           ptrdiff_t>
    {
    public:
      explicit vtk_out_edge_pointer_iterator(vtkGraph *g = 0, vtkIdType v = 0, bool end = false) :
        vertex(v)
      {
        if (g)
          {
          vtkIdType nedges;
          g->GetOutEdges(vertex, iter, nedges);
          if (end)
            {
            iter += nedges;
            }
          }
      }

    private:
      vtkEdgeType dereference() const { return vtkEdgeType(vertex, iter->Target, iter->Id); }

      bool equal(const vtk_out_edge_pointer_iterator& other) const
      { return iter == other.iter; }

      void increment() { iter++; }
      void decrement() { iter--; }

      vtkIdType vertex;
      const vtkOutEdgeType *iter;

      friend class iterator_core_access;
    };

  class vtk_in_edge_pointer_iterator :
    public iterator_facade<vtk_in_edge_pointer_iterator,
                           vtkEdgeType,
                           bidirectional_traversal_tag,
                           vtkEdgeType,
                           ptrdiff_t>
    {
    public:
      explicit vtk_in_edge_pointer_iterator(vtkGraph *g = 0, vtkIdType v = 0, bool end = false) :
        vertex(v)
      {
        if (g)
          {
          vtkIdType nedges;
          g->GetInEdges(vertex, iter, nedges);
          if (end)
            {
            iter += nedges;
            }
          }
      }

    private:
      vtkEdgeType dereference() const { return vtkEdgeType(iter->Source, vertex, iter->Id); }

      bool equal(const vtk_in_edge_pointer_iterator& other) const
      { return iter == other.iter; }

      void increment() { iter++; }
      void decrement() { iter--; }

      vtkIdType vertex;
      const vtkInEdgeType *iter;

      friend class iterator_core_access;
    };

  //===========================================================================
  // vtkGraph
  // VertexAndEdgeListGraphConcept
  // BidirectionalGraphConcept
  // AdjacencyGraphConcept

  struct vtkGraph_traversal_category :
    public virtual bidirectional_graph_tag,
    public virtual edge_list_graph_tag,
    public virtual vertex_list_graph_tag,
    public virtual adjacency_graph_tag { };

  template <>
  struct graph_traits<vtkGraph*> {
    typedef vtkIdType vertex_descriptor;
    static vertex_descriptor null_vertex() { return -1; }
    typedef vtkEdgeType edge_descriptor;
    static edge_descriptor null_edge() { return vtkEdgeType(-1, -1, -1); }
    typedef vtk_out_edge_pointer_iterator out_edge_iterator;
    typedef vtk_in_edge_pointer_iterator in_edge_iterator;

    typedef vtk_vertex_iterator vertex_iterator;
    typedef vtk_edge_iterator edge_iterator;

    typedef allow_parallel_edge_tag edge_parallel_category;
    typedef vtkGraph_traversal_category traversal_category;
    typedef vtkIdType vertices_size_type;
    typedef vtkIdType edges_size_type;
    typedef vtkIdType degree_size_type;

    typedef adjacency_iterator_generator<vtkGraph*,
      vertex_descriptor, out_edge_iterator>::type adjacency_iterator;
  };

#if BOOST_VERSION >= 104500
  template<>
  struct graph_property_type< vtkGraph* > {
    typedef no_property type;
  };
#endif

  template<>
  struct vertex_property_type< vtkGraph* > {
    typedef no_property type;
  };

  template<>
  struct edge_property_type< vtkGraph* > {
    typedef no_property type;
  };

#if BOOST_VERSION >= 104500
  template<>
  struct graph_bundle_type< vtkGraph* > {
    typedef no_property type;
  };
#endif

  template<>
  struct vertex_bundle_type< vtkGraph* > {
    typedef no_property type;
  };

  template<>
  struct edge_bundle_type< vtkGraph* > {
    typedef no_property type;
  };

  inline bool has_no_edges(vtkGraph* g)
    {
      return ((g->GetNumberOfEdges() > 0) ? false : true);
    }

  inline void remove_edge(graph_traits<vtkGraph*>::edge_descriptor e,
                          vtkGraph* g)
    {
    if(vtkMutableDirectedGraph::SafeDownCast(g))
      {
      vtkMutableDirectedGraph::SafeDownCast(g)->RemoveEdge(e.Id);
      }
    else if(vtkMutableUndirectedGraph::SafeDownCast(g))
      {
      vtkMutableUndirectedGraph::SafeDownCast(g)->RemoveEdge(e.Id);
      }
    }

  //===========================================================================
  // vtkDirectedGraph

  template <>
  struct graph_traits<vtkDirectedGraph*> : graph_traits<vtkGraph*>
  {
    typedef directed_tag directed_category;
  };

  // The graph_traits for a const graph are the same as a non-const graph.
  template <>
  struct graph_traits<const vtkDirectedGraph*> : graph_traits<vtkDirectedGraph*> { };

  // The graph_traits for a const graph are the same as a non-const graph.
  template <>
  struct graph_traits<vtkDirectedGraph* const> : graph_traits<vtkDirectedGraph*> { };

#if BOOST_VERSION >= 104500
  // Internal graph properties
  template<>
  struct graph_property_type< vtkDirectedGraph* >
    : graph_property_type< vtkGraph* > { };

  // Internal graph properties
  template<>
  struct graph_property_type< vtkDirectedGraph* const >
    : graph_property_type< vtkGraph* > { };
#endif

  // Internal vertex properties
  template<>
  struct vertex_property_type< vtkDirectedGraph* >
    : vertex_property_type< vtkGraph* > { };

  // Internal vertex properties
  template<>
  struct vertex_property_type< vtkDirectedGraph* const >
    : vertex_property_type< vtkGraph* > { };

  // Internal edge properties
  template<>
  struct edge_property_type< vtkDirectedGraph* >
    : edge_property_type< vtkGraph* > { };

  // Internal edge properties
  template<>
  struct edge_property_type< vtkDirectedGraph* const >
    : edge_property_type< vtkGraph* > { };

#if BOOST_VERSION >= 104500
  // Internal graph properties
  template<>
  struct graph_bundle_type< vtkDirectedGraph* >
    : graph_bundle_type< vtkGraph* > { };

  // Internal graph properties
  template<>
  struct graph_bundle_type< vtkDirectedGraph* const >
    : graph_bundle_type< vtkGraph* > { };
#endif

  // Internal vertex properties
  template<>
  struct vertex_bundle_type< vtkDirectedGraph* >
    : vertex_bundle_type< vtkGraph* > { };

  // Internal vertex properties
  template<>
  struct vertex_bundle_type< vtkDirectedGraph* const >
    : vertex_bundle_type< vtkGraph* > { };

  // Internal edge properties
  template<>
  struct edge_bundle_type< vtkDirectedGraph* >
    : edge_bundle_type< vtkGraph* > { };

  // Internal edge properties
  template<>
  struct edge_bundle_type< vtkDirectedGraph* const >
    : edge_bundle_type< vtkGraph* > { };

  //===========================================================================
  // vtkTree

  template <>
  struct graph_traits<vtkTree*> : graph_traits<vtkDirectedGraph*> { };

  // The graph_traits for a const graph are the same as a non-const graph.
  template <>
  struct graph_traits<const vtkTree*> : graph_traits<vtkTree*> { };

  // The graph_traits for a const graph are the same as a non-const graph.
  template <>
  struct graph_traits<vtkTree* const> : graph_traits<vtkTree*> { };

  //===========================================================================
  // vtkUndirectedGraph
  template <>
  struct graph_traits<vtkUndirectedGraph*> : graph_traits<vtkGraph*>
  {
    typedef undirected_tag directed_category;
  };

  // The graph_traits for a const graph are the same as a non-const graph.
  template <>
  struct graph_traits<const vtkUndirectedGraph*> : graph_traits<vtkUndirectedGraph*> { };

  // The graph_traits for a const graph are the same as a non-const graph.
  template <>
  struct graph_traits<vtkUndirectedGraph* const> : graph_traits<vtkUndirectedGraph*> { };

#if BOOST_VERSION >= 104500
  // Internal graph properties
  template<>
  struct graph_property_type< vtkUndirectedGraph* >
    : graph_property_type< vtkGraph* > { };

  // Internal graph properties
  template<>
  struct graph_property_type< vtkUndirectedGraph* const >
    : graph_property_type< vtkGraph* > { };
#endif

  // Internal vertex properties
  template<>
  struct vertex_property_type< vtkUndirectedGraph* >
    : vertex_property_type< vtkGraph* > { };

  // Internal vertex properties
  template<>
  struct vertex_property_type< vtkUndirectedGraph* const >
    : vertex_property_type< vtkGraph* > { };

  // Internal edge properties
  template<>
  struct edge_property_type< vtkUndirectedGraph* >
    : edge_property_type< vtkGraph* > { };

  // Internal edge properties
  template<>
  struct edge_property_type< vtkUndirectedGraph* const >
    : edge_property_type< vtkGraph* > { };

#if BOOST_VERSION >= 104500
  // Internal graph properties
  template<>
  struct graph_bundle_type< vtkUndirectedGraph* >
    : graph_bundle_type< vtkGraph* > { };

  // Internal graph properties
  template<>
  struct graph_bundle_type< vtkUndirectedGraph* const >
    : graph_bundle_type< vtkGraph* > { };
#endif

  // Internal vertex properties
  template<>
  struct vertex_bundle_type< vtkUndirectedGraph* >
    : vertex_bundle_type< vtkGraph* > { };

  // Internal vertex properties
  template<>
  struct vertex_bundle_type< vtkUndirectedGraph* const >
    : vertex_bundle_type< vtkGraph* > { };

  // Internal edge properties
  template<>
  struct edge_bundle_type< vtkUndirectedGraph* >
    : edge_bundle_type< vtkGraph* > { };

  // Internal edge properties
  template<>
  struct edge_bundle_type< vtkUndirectedGraph* const >
    : edge_bundle_type< vtkGraph* > { };

  //===========================================================================
  // vtkMutableDirectedGraph

  template <>
  struct graph_traits<vtkMutableDirectedGraph*> : graph_traits<vtkDirectedGraph*> { };

  // The graph_traits for a const graph are the same as a non-const graph.
  template <>
  struct graph_traits<const vtkMutableDirectedGraph*> : graph_traits<vtkMutableDirectedGraph*> { };

  // The graph_traits for a const graph are the same as a non-const graph.
  template <>
  struct graph_traits<vtkMutableDirectedGraph* const> : graph_traits<vtkMutableDirectedGraph*> { };

#if BOOST_VERSION >= 104500
  // Internal graph properties
  template<>
  struct graph_property_type< vtkMutableDirectedGraph* >
    : graph_property_type< vtkDirectedGraph* > { };

  // Internal graph properties
  template<>
  struct graph_property_type< vtkMutableDirectedGraph* const >
    : graph_property_type< vtkDirectedGraph* > { };
#endif

  // Internal vertex properties
  template<>
  struct vertex_property_type< vtkMutableDirectedGraph* >
    : vertex_property_type< vtkDirectedGraph* > { };

  // Internal vertex properties
  template<>
  struct vertex_property_type< vtkMutableDirectedGraph* const >
    : vertex_property_type< vtkDirectedGraph* > { };

  // Internal edge properties
  template<>
  struct edge_property_type< vtkMutableDirectedGraph* >
    : edge_property_type< vtkDirectedGraph* > { };

  // Internal edge properties
  template<>
  struct edge_property_type< vtkMutableDirectedGraph* const >
    : edge_property_type< vtkDirectedGraph* > { };

#if BOOST_VERSION >= 104500
  // Internal graph properties
  template<>
  struct graph_bundle_type< vtkMutableDirectedGraph* >
    : graph_bundle_type< vtkDirectedGraph* > { };

  // Internal graph properties
  template<>
  struct graph_bundle_type< vtkMutableDirectedGraph* const >
    : graph_bundle_type< vtkDirectedGraph* > { };
#endif

  // Internal vertex properties
  template<>
  struct vertex_bundle_type< vtkMutableDirectedGraph* >
    : vertex_bundle_type< vtkDirectedGraph* > { };

  // Internal vertex properties
  template<>
  struct vertex_bundle_type< vtkMutableDirectedGraph* const >
    : vertex_bundle_type< vtkDirectedGraph* > { };

  // Internal edge properties
  template<>
  struct edge_bundle_type< vtkMutableDirectedGraph* >
    : edge_bundle_type< vtkDirectedGraph* > { };

  // Internal edge properties
  template<>
  struct edge_bundle_type< vtkMutableDirectedGraph* const >
    : edge_bundle_type< vtkDirectedGraph* > { };

  //===========================================================================
  // vtkMutableUndirectedGraph

  template <>
  struct graph_traits<vtkMutableUndirectedGraph*> : graph_traits<vtkUndirectedGraph*> { };

  // The graph_traits for a const graph are the same as a non-const graph.
  template <>
  struct graph_traits<const vtkMutableUndirectedGraph*> : graph_traits<vtkMutableUndirectedGraph*> { };

  // The graph_traits for a const graph are the same as a non-const graph.
  template <>
  struct graph_traits<vtkMutableUndirectedGraph* const> : graph_traits<vtkMutableUndirectedGraph*> { };

#if BOOST_VERSION >= 104500
  // Internal graph properties
  template<>
  struct graph_property_type< vtkMutableUndirectedGraph* >
    : graph_property_type< vtkUndirectedGraph* > { };

  // Internal graph properties
  template<>
  struct graph_property_type< vtkMutableUndirectedGraph* const >
    : graph_property_type< vtkUndirectedGraph* > { };
#endif

  // Internal vertex properties
  template<>
  struct vertex_property_type< vtkMutableUndirectedGraph* >
    : vertex_property_type< vtkUndirectedGraph* > { };

  // Internal vertex properties
  template<>
  struct vertex_property_type< vtkMutableUndirectedGraph* const >
    : vertex_property_type< vtkUndirectedGraph* > { };

  // Internal edge properties
  template<>
  struct edge_property_type< vtkMutableUndirectedGraph* >
    : edge_property_type< vtkUndirectedGraph* > { };

  // Internal edge properties
  template<>
  struct edge_property_type< vtkMutableUndirectedGraph* const >
    : edge_property_type< vtkUndirectedGraph* > { };

#if BOOST_VERSION >= 104500
  // Internal graph properties
  template<>
  struct graph_bundle_type< vtkMutableUndirectedGraph* >
    : graph_bundle_type< vtkUndirectedGraph* > { };

  // Internal graph properties
  template<>
  struct graph_bundle_type< vtkMutableUndirectedGraph* const >
    : graph_bundle_type< vtkUndirectedGraph* > { };
#endif

  // Internal vertex properties
  template<>
  struct vertex_bundle_type< vtkMutableUndirectedGraph* >
    : vertex_bundle_type< vtkUndirectedGraph* > { };

  // Internal vertex properties
  template<>
  struct vertex_bundle_type< vtkMutableUndirectedGraph* const >
    : vertex_bundle_type< vtkUndirectedGraph* > { };

  // Internal edge properties
  template<>
  struct edge_bundle_type< vtkMutableUndirectedGraph* >
    : edge_bundle_type< vtkUndirectedGraph* > { };

  // Internal edge properties
  template<>
  struct edge_bundle_type< vtkMutableUndirectedGraph* const >
    : edge_bundle_type< vtkUndirectedGraph* > { };

  //===========================================================================
  // API implementation
  template <>
  class vertex_property< vtkGraph* > {
  public:
    typedef vtkIdType type;
  };

  template <>
  class edge_property< vtkGraph* > {
  public:
    typedef vtkIdType type;
  };
} // end namespace boost

inline boost::graph_traits< vtkGraph* >::vertex_descriptor
source(boost::graph_traits< vtkGraph* >::edge_descriptor e,
       vtkGraph *)
{
  return e.Source;
}

inline boost::graph_traits< vtkGraph* >::vertex_descriptor
target(boost::graph_traits< vtkGraph* >::edge_descriptor e,
       vtkGraph *)
{
  return e.Target;
}

inline vtksys_stl::pair<
  boost::graph_traits< vtkGraph* >::vertex_iterator,
  boost::graph_traits< vtkGraph* >::vertex_iterator >
vertices(vtkGraph *g)
{
  typedef boost::graph_traits< vtkGraph* >::vertex_iterator Iter;
  vtkIdType start = 0;
  if (vtkDistributedGraphHelper *helper = g->GetDistributedGraphHelper())
    {
    int rank =
      g->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
    start = helper->MakeDistributedId(rank, start);
    }

  return vtksys_stl::make_pair( Iter(start),
                                Iter(start + g->GetNumberOfVertices()) );
}

inline vtksys_stl::pair<
  boost::graph_traits< vtkGraph* >::edge_iterator,
  boost::graph_traits< vtkGraph* >::edge_iterator >
edges(vtkGraph *g)
{
  typedef boost::graph_traits< vtkGraph* >::edge_iterator Iter;
  return vtksys_stl::make_pair( Iter(g), Iter(g, g->GetNumberOfVertices()) );
}

inline vtksys_stl::pair<
  boost::graph_traits< vtkGraph* >::out_edge_iterator,
  boost::graph_traits< vtkGraph* >::out_edge_iterator >
out_edges(
  boost::graph_traits< vtkGraph* >::vertex_descriptor u,
  vtkGraph *g)
{
  typedef boost::graph_traits< vtkGraph* >::out_edge_iterator Iter;
  vtksys_stl::pair<Iter, Iter> p = vtksys_stl::make_pair( Iter(g, u), Iter(g, u, true) );
  return p;
}

inline vtksys_stl::pair<
  boost::graph_traits< vtkGraph* >::in_edge_iterator,
  boost::graph_traits< vtkGraph* >::in_edge_iterator >
in_edges(
  boost::graph_traits< vtkGraph* >::vertex_descriptor u,
  vtkGraph *g)
{
  typedef boost::graph_traits< vtkGraph* >::in_edge_iterator Iter;
  vtksys_stl::pair<Iter, Iter> p = vtksys_stl::make_pair( Iter(g, u), Iter(g, u, true) );
  return p;
}

inline vtksys_stl::pair<
  boost::graph_traits< vtkGraph* >::adjacency_iterator,
  boost::graph_traits< vtkGraph* >::adjacency_iterator >
adjacent_vertices(
  boost::graph_traits< vtkGraph* >::vertex_descriptor u,
  vtkGraph *g)
{
  typedef boost::graph_traits< vtkGraph* >::adjacency_iterator Iter;
  typedef boost::graph_traits< vtkGraph* >::out_edge_iterator OutEdgeIter;
  vtksys_stl::pair<OutEdgeIter, OutEdgeIter> out = out_edges(u, g);
  return vtksys_stl::make_pair( Iter(out.first, &g), Iter(out.second, &g) );
}

inline boost::graph_traits< vtkGraph* >::vertices_size_type
num_vertices(vtkGraph *g)
{
  return g->GetNumberOfVertices();
}

inline boost::graph_traits< vtkGraph* >::edges_size_type
num_edges(vtkGraph *g)
{
  return g->GetNumberOfEdges();
}

inline boost::graph_traits< vtkGraph* >::degree_size_type
out_degree(
  boost::graph_traits< vtkGraph* >::vertex_descriptor u,
  vtkGraph *g)
{
  return g->GetOutDegree(u);
}

inline boost::graph_traits< vtkDirectedGraph* >::degree_size_type
in_degree(
  boost::graph_traits< vtkDirectedGraph* >::vertex_descriptor u,
  vtkDirectedGraph *g)
{
  return g->GetInDegree(u);
}

inline boost::graph_traits< vtkGraph* >::degree_size_type
degree(
  boost::graph_traits< vtkGraph* >::vertex_descriptor u,
  vtkGraph *g)
{
  return g->GetDegree(u);
}

inline boost::graph_traits< vtkMutableDirectedGraph* >::vertex_descriptor
add_vertex(vtkMutableDirectedGraph *g)
{
  return g->AddVertex();
}

inline vtksys_stl::pair<
  boost::graph_traits< vtkMutableDirectedGraph* >::edge_descriptor,
  bool>
add_edge(
  boost::graph_traits< vtkMutableDirectedGraph* >::vertex_descriptor u,
  boost::graph_traits< vtkMutableDirectedGraph* >::vertex_descriptor v,
  vtkMutableDirectedGraph *g)
{
  boost::graph_traits< vtkMutableDirectedGraph* >::edge_descriptor e = g->AddEdge(u, v);
  return vtksys_stl::make_pair(e, true);
}

inline boost::graph_traits< vtkMutableUndirectedGraph* >::vertex_descriptor
add_vertex(vtkMutableUndirectedGraph *g)
{
  return g->AddVertex();
}

inline vtksys_stl::pair<
  boost::graph_traits< vtkMutableUndirectedGraph* >::edge_descriptor,
  bool>
add_edge(
  boost::graph_traits< vtkMutableUndirectedGraph* >::vertex_descriptor u,
  boost::graph_traits< vtkMutableUndirectedGraph* >::vertex_descriptor v,
  vtkMutableUndirectedGraph *g)
{
  boost::graph_traits< vtkMutableUndirectedGraph* >::edge_descriptor e = g->AddEdge(u, v);
  return vtksys_stl::make_pair(e, true);
}

namespace boost {
  //===========================================================================
  // An edge map for vtkGraph.
  // This is a common input needed for algorithms.

  struct vtkGraphEdgeMap { };

  template <>
  struct property_traits<vtkGraphEdgeMap>
    {
    typedef vtkIdType value_type;
    typedef vtkIdType reference;
    typedef vtkEdgeType key_type;
    typedef readable_property_map_tag category;
    };

  inline property_traits<vtkGraphEdgeMap>::reference
  get(
    vtkGraphEdgeMap vtkNotUsed(arr),
    property_traits<vtkGraphEdgeMap>::key_type key)
  {
    return key.Id;
  }

  //===========================================================================
  // Helper for vtkGraph edge property maps
  // Automatically converts boost edge ids to vtkGraph edge ids.

  template<typename PMap>
  class vtkGraphEdgePropertyMapHelper
  {
  public:
    vtkGraphEdgePropertyMapHelper(PMap m) : pmap(m) { }
    PMap pmap;
    typedef typename property_traits<PMap>::value_type value_type;
    typedef typename property_traits<PMap>::reference reference;
    typedef vtkEdgeType key_type;
    typedef typename property_traits<PMap>::category category;
  };

  template<typename PMap>
  inline typename property_traits<PMap>::reference
  get(
    vtkGraphEdgePropertyMapHelper<PMap> helper,
    vtkEdgeType key)
  {
    return get(helper.pmap, key.Id);
  }

  template<typename PMap>
  inline void
  put(
    vtkGraphEdgePropertyMapHelper<PMap> helper,
    vtkEdgeType key,
    const typename property_traits<PMap>::value_type & value)
  {
    put(helper.pmap, key.Id, value);
  }

  //===========================================================================
  // An index map for vtkGraph
  // This is a common input needed for algorithms

  struct vtkGraphIndexMap { };

  template <>
  struct property_traits<vtkGraphIndexMap>
    {
    typedef vtkIdType value_type;
    typedef vtkIdType reference;
    typedef vtkIdType key_type;
    typedef readable_property_map_tag category;
    };

  inline property_traits<vtkGraphIndexMap>::reference
  get(
    vtkGraphIndexMap vtkNotUsed(arr),
    property_traits<vtkGraphIndexMap>::key_type key)
  {
    return key;
  }

  //===========================================================================
  // Helper for vtkGraph property maps
  // Automatically multiplies the property value by some value (default 1)
  template<typename PMap>
  class vtkGraphPropertyMapMultiplier
  {
  public:
    vtkGraphPropertyMapMultiplier(PMap m, float multi=1) : pmap(m),multiplier(multi){}
    PMap pmap;
    float multiplier;
    typedef typename property_traits<PMap>::value_type value_type;
    typedef typename property_traits<PMap>::reference reference;
    typedef typename property_traits<PMap>::key_type key_type;
    typedef typename property_traits<PMap>::category category;
  };

  template<typename PMap>
  inline typename property_traits<PMap>::reference
  get(
    vtkGraphPropertyMapMultiplier<PMap> multi,
    const typename property_traits<PMap>::key_type key)
  {
    return multi.multiplier * get(multi.pmap, key);
  }

  template<typename PMap>
  inline void
  put(
    vtkGraphPropertyMapMultiplier<PMap> multi,
    const typename property_traits<PMap>::key_type key,
    const typename property_traits<PMap>::value_type & value)
  {
    put(multi.pmap, key, value);
  }

  // Allow algorithms to automatically extract vtkGraphIndexMap from a
  // VTK graph
  template<>
  struct property_map<vtkGraph*, vertex_index_t>
  {
    typedef vtkGraphIndexMap type;
    typedef vtkGraphIndexMap const_type;
  };

  template<>
  struct property_map<vtkDirectedGraph*, vertex_index_t>
    : property_map<vtkGraph*, vertex_index_t> { };

  template<>
  struct property_map<vtkUndirectedGraph*, vertex_index_t>
    : property_map<vtkGraph*, vertex_index_t> { };

  inline vtkGraphIndexMap get(vertex_index_t, vtkGraph*) { return vtkGraphIndexMap(); }

  template<>
  struct property_map<vtkGraph*, edge_index_t>
  {
    typedef vtkGraphIndexMap type;
    typedef vtkGraphIndexMap const_type;
  };

  template<>
  struct property_map<vtkDirectedGraph*, edge_index_t>
    : property_map<vtkGraph*, edge_index_t> { };

  template<>
  struct property_map<vtkUndirectedGraph*, edge_index_t>
    : property_map<vtkGraph*, edge_index_t> { };

  inline vtkGraphIndexMap get(edge_index_t, vtkGraph*) { return vtkGraphIndexMap(); }

  // property_map specializations for const-qualified graphs
  template<>
  struct property_map<vtkDirectedGraph* const, vertex_index_t>
    : property_map<vtkDirectedGraph*, vertex_index_t> { };

  template<>
  struct property_map<vtkUndirectedGraph* const, vertex_index_t>
    : property_map<vtkUndirectedGraph*, vertex_index_t> { };

  template<>
  struct property_map<vtkDirectedGraph* const, edge_index_t>
    : property_map<vtkDirectedGraph*, edge_index_t> { };

  template<>
  struct property_map<vtkUndirectedGraph* const, edge_index_t>
    : property_map<vtkUndirectedGraph*, edge_index_t> { };
} // namespace boost

#if BOOST_VERSION > 104000
#include <boost/property_map/vector_property_map.hpp>
#else
#include <boost/vector_property_map.hpp>
#endif


#endif // __vtkBoostGraphAdapter_h
// VTK-HeaderTest-Exclude: vtkBoostGraphAdapter.h
