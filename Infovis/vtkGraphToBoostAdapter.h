/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphToBoostAdapter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkDirectedGraph*ToBoostAdapter - adapter to the boost graph library (www.boost.org)
//
// .SECTION Description
// Including this header allows you to use a vtkDirectedGraph* in boost algorithms.
// To do this, first wrap the class in a vtkDirectedGraph* or
// vtkUndirectedGraph* depending on whether your graph is directed or undirected.
// You may then use these objects directly in boost graph algorithms.

#ifndef __vtkGraphToBoostAdapter_h
#define __vtkGraphToBoostAdapter_h

#include "vtkDirectedGraph.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkTree.h"
#include "vtkUndirectedGraph.h"

// Moving these functions before any boost includes to get rid of some linux
// compile errors.
namespace boost {
  inline int
  get(
    vtkIntArray* & arr,
    vtkIdType& key)
  {
    return arr->GetValue(key);
  }

  inline void
  put(
    vtkIntArray* arr,
    vtkIdType key,
    const int & value)
  {
    arr->InsertValue(key, value);
  }
}

#include <vtksys/stl/utility>

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
                           const vtkIdType*>
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
                           const vtkEdgeType*>
    {
    public:
      explicit vtk_edge_iterator(vtkGraph *g = 0, vtkIdType v = 0) :
        graph(g), vertex(v), iter(0), end(0), directed(false)
        {
        if (graph != 0 && vertex < graph->GetNumberOfVertices())
          {
          directed = (vtkDirectedGraph::SafeDownCast(graph) != 0);
          vtkIdType nedges;
          graph->GetOutEdges(vertex, iter, nedges);
          end = iter + nedges;
          }
        }


    private:
      vtkEdgeType dereference() const
        { return vtkEdgeType(vertex, iter->Target, iter->Id); }

      bool equal(const vtk_edge_iterator& other) const
        { return iter == other.iter; }

      void increment()
        {
        inc();
        // If it is undirected, skip edges 
        // whose source is greater than the target.
        if (!directed)
          {
          while (iter != 0 && vertex > iter->Target)
            {
            inc();
            }
          }
        }

      void inc()
        {
        ++iter;
        if (iter != 0 && iter == end)
          {
          ++vertex;
          if (graph != 0 && vertex < graph->GetNumberOfVertices())
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
                           const vtkEdgeType*>
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
                           const vtkEdgeType*>
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

  inline graph_traits< vtkGraph* >::vertex_descriptor
  source(graph_traits< vtkGraph* >::edge_descriptor e,
         vtkGraph *)
  {
    return e.Source;
  }

  inline graph_traits< vtkGraph* >::vertex_descriptor
  target(graph_traits< vtkGraph* >::edge_descriptor e,
         vtkGraph *)
  {
    return e.Target;
  }

  inline vtksys_stl::pair<
    graph_traits< vtkGraph* >::vertex_iterator,
    graph_traits< vtkGraph* >::vertex_iterator >  
  vertices(vtkGraph *g)
  {
    typedef graph_traits< vtkGraph* >::vertex_iterator Iter;
    return vtksys_stl::make_pair( Iter(0), Iter(g->GetNumberOfVertices()) );
  }

  inline vtksys_stl::pair<
    graph_traits< vtkGraph* >::edge_iterator,
    graph_traits< vtkGraph* >::edge_iterator >  
  edges(vtkGraph *g)
  {
    typedef graph_traits< vtkGraph* >::edge_iterator Iter;
    return vtksys_stl::make_pair( Iter(g), Iter(g, g->GetNumberOfVertices()) );
  }

  inline vtksys_stl::pair<
    graph_traits< vtkGraph* >::out_edge_iterator,
    graph_traits< vtkGraph* >::out_edge_iterator >  
  out_edges(
    graph_traits< vtkGraph* >::vertex_descriptor u, 
    vtkGraph *g)
  {
    typedef graph_traits< vtkGraph* >::out_edge_iterator Iter;
    vtksys_stl::pair<Iter, Iter> p = vtksys_stl::make_pair( Iter(g, u), Iter(g, u, true) );
    return p;
  }

  inline vtksys_stl::pair<
    graph_traits< vtkGraph* >::in_edge_iterator,
    graph_traits< vtkGraph* >::in_edge_iterator >  
  in_edges(
    graph_traits< vtkGraph* >::vertex_descriptor u, 
    vtkGraph *g)
  {
    typedef graph_traits< vtkGraph* >::in_edge_iterator Iter;
    vtksys_stl::pair<Iter, Iter> p = vtksys_stl::make_pair( Iter(g, u), Iter(g, u, true) );
    return p;
  }

  inline vtksys_stl::pair<
    graph_traits< vtkGraph* >::adjacency_iterator,
    graph_traits< vtkGraph* >::adjacency_iterator >
  adjacent_vertices(
    graph_traits< vtkGraph* >::vertex_descriptor u, 
    vtkGraph *g)
  {
    typedef graph_traits< vtkGraph* >::adjacency_iterator Iter;
    typedef graph_traits< vtkGraph* >::out_edge_iterator OutEdgeIter;
    vtksys_stl::pair<OutEdgeIter, OutEdgeIter> out = out_edges(u, g);
    return vtksys_stl::make_pair( Iter(out.first, &g), Iter(out.second, &g) );
  }

  inline graph_traits< vtkGraph* >::vertices_size_type
  num_vertices(vtkGraph *g)
  {
    return g->GetNumberOfVertices();
  }

  inline graph_traits< vtkGraph* >::edges_size_type
  num_edges(vtkGraph *g)
  {
    return g->GetNumberOfEdges();
  }  

  inline graph_traits< vtkGraph* >::degree_size_type
  out_degree(
    graph_traits< vtkGraph* >::vertex_descriptor u, 
    vtkGraph *g)
  {
    return g->GetOutDegree(u);
  }

  inline graph_traits< vtkDirectedGraph* >::degree_size_type
  in_degree(
    graph_traits< vtkDirectedGraph* >::vertex_descriptor u, 
    vtkDirectedGraph *g)
  {
    return g->GetInDegree(u);
  }

  inline graph_traits< vtkGraph* >::degree_size_type
  degree(
    graph_traits< vtkGraph* >::vertex_descriptor u, 
    vtkGraph *g)
  {
    return g->GetDegree(u);
  }
  
  inline graph_traits< vtkMutableDirectedGraph* >::vertex_descriptor
  add_vertex(vtkMutableDirectedGraph *g)
  {
    return g->AddVertex();
  }

  inline vtksys_stl::pair<
    graph_traits< vtkMutableDirectedGraph* >::edge_descriptor,
    bool>
  add_edge(
    graph_traits< vtkMutableDirectedGraph* >::vertex_descriptor u,
    graph_traits< vtkMutableDirectedGraph* >::vertex_descriptor v,
    vtkMutableDirectedGraph *g)
  {
    graph_traits< vtkMutableDirectedGraph* >::edge_descriptor e = g->AddEdge(u, v);
    return vtksys_stl::make_pair(e, true);
  }

  inline graph_traits< vtkMutableUndirectedGraph* >::vertex_descriptor
  add_vertex(vtkMutableUndirectedGraph *g)
  {
    return g->AddVertex();
  }

  inline vtksys_stl::pair<
    graph_traits< vtkMutableUndirectedGraph* >::edge_descriptor,
    bool>
  add_edge(
    graph_traits< vtkMutableUndirectedGraph* >::vertex_descriptor u,
    graph_traits< vtkMutableUndirectedGraph* >::vertex_descriptor v,
    vtkMutableUndirectedGraph *g)
  {
    graph_traits< vtkMutableUndirectedGraph* >::edge_descriptor e = g->AddEdge(u, v);
    return vtksys_stl::make_pair(e, true);
  }

  //===========================================================================
  // VTK arrays as property maps
  // These need to be defined before including other boost stuff

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

  template <>
  struct property_traits<vtkIntArray*>
    {
    typedef int value_type;
    typedef int reference;
    typedef vtkIdType key_type;
    typedef read_write_property_map_tag category;
    };

  vtkPropertyMapMacro(vtkIdTypeArray, vtkIdType)
  vtkPropertyMapMacro(vtkDoubleArray, double)
  vtkPropertyMapMacro(vtkFloatArray, float)

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

} // namespace boost


#endif // __vtkGraphToBoostAdapter_h
