#ifndef __vtkTreeToBoostAdapter_h
#define __vtkTreeToBoostAdapter_h

#include <vtksys/stl/utility>

#include <boost/config.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/adjacency_iterator.hpp>

#include "vtkTree.h"

// The functions and classes in this file allows the user to
// treat a vtkTree* object as a boost graph "as is". No
// wrapper is needed for the vtkAbstractTree object.

namespace boost {

  //===========================================================================
  // vtkTree*
  // VertexAndEdgeListGraphConcept
  // BidirectionalGraphConcept
  // AdjacencyGraaphConcept

  struct vtkTree_traversal_category : 
    public virtual bidirectional_graph_tag,
    public virtual edge_list_graph_tag,
    public virtual vertex_list_graph_tag,
    public virtual adjacency_graph_tag { };
    
  template <>
  struct graph_traits< vtkTree* > {
    typedef vtkIdType vertex_descriptor;
    static vertex_descriptor null_vertex() { return -1; }
    typedef vtkIdType edge_descriptor;

    class tree_edge_iterator
      : public iterator_facade<tree_edge_iterator,
                               vtkIdType,
                               bidirectional_traversal_tag,
                               vtkIdType,
                               const vtkIdType*>
    {
    public:
      explicit tree_edge_iterator(vtkTree* t, vtkIdType s, const vtkIdType* a)
        : tree(t), source(s), adj(a), cur(0) {}

      explicit tree_edge_iterator()
        : tree(NULL), source(0), adj(NULL), cur(0) {}

      vtkIdType dereference() const 
        {
        vtkIdType v = adj[cur];
        if (v == tree->GetParent(source))
          {
          v = source;
          }
        if (v == 0)
          {
          return tree->GetRoot() - 1;
          }
        return v - 1;
        }

      bool equal(const tree_edge_iterator& other) const
        { 
        return tree == other.tree &&
               adj == other.adj &&
               source == other.source &&
               cur == other.cur; 
        }

      void increment() { cur++; }
      void decrement() { cur--; }

    private:
      vtkTree* tree;
      const vtkIdType* adj;
      vtkIdType source;
      vtkIdType cur;
      friend class iterator_core_access;
    };


    typedef tree_edge_iterator out_edge_iterator;
    typedef tree_edge_iterator in_edge_iterator;

    class index_iterator
      : public iterator_facade<index_iterator,
                               vtkIdType,
                               bidirectional_traversal_tag,
                               vtkIdType,
                               const vtkIdType*>
    {
    public:
      explicit index_iterator(vtkIdType i = 0) : index(i) {}

    private:
      vtkIdType dereference() const { return index; }

      bool equal(const index_iterator& other) const
      { return index == other.index; }

      void increment() { index++; }
      void decrement() { index--; }

      vtkIdType index;

      friend class iterator_core_access;
    };

    typedef index_iterator vertex_iterator;
    typedef index_iterator edge_iterator;

    typedef directed_tag directed_category;
    typedef allow_parallel_edge_tag edge_parallel_category;
    typedef vtkTree_traversal_category traversal_category;
    typedef vtkIdType vertices_size_type;
    typedef vtkIdType edges_size_type;
    typedef vtkIdType degree_size_type;

    typedef adjacency_iterator_generator<vtkTree*, vertex_descriptor, out_edge_iterator>::type adjacency_iterator;
  };

  // The graph_traits for a const graph are the same as a non-const graph.
  template <>
  struct graph_traits<const vtkTree*> : graph_traits<vtkTree*> { };
  
  template <>
  class vertex_property< vtkTree* > {
  public:
    typedef vtkIdType type;
  };

  template <>
  class edge_property< vtkTree* > {
  public:
    typedef vtkIdType type;
  };

  inline vtkIdType
  vtkTree_edge_id(
         graph_traits< vtkTree* >::edge_descriptor e,
         const vtkTree*& g)
  {
    return e;
  }

  inline graph_traits< vtkTree* >::vertex_descriptor
  source(graph_traits< vtkTree* >::edge_descriptor e,
         vtkTree* g)
  {
    return g->GetSourceVertex(e);
  }

  inline graph_traits< vtkTree* >::vertex_descriptor
  target(graph_traits< vtkTree* >::edge_descriptor e,
         vtkTree* g)
  {
    return g->GetTargetVertex(e);
  }

  inline vtksys_stl::pair<
    graph_traits< vtkTree* >::vertex_iterator,
    graph_traits< vtkTree* >::vertex_iterator >  
  vertices(vtkTree* g)
  {
    return vtksys_stl::make_pair( 0, g->GetNumberOfVertices() );
  }

  inline vtksys_stl::pair<
    graph_traits< vtkTree* >::edge_iterator,
    graph_traits< vtkTree* >::edge_iterator >  
  edges(vtkTree* g)
  {
    typedef graph_traits< vtkTree* >::edge_iterator
      Iter;
    return vtksys_stl::make_pair( Iter(0), Iter(g->GetNumberOfEdges()) );
  }

  inline vtksys_stl::pair<
    graph_traits< vtkTree* >::out_edge_iterator,
    graph_traits< vtkTree* >::out_edge_iterator >  
  out_edges(
    graph_traits< vtkTree* >::vertex_descriptor u, 
    vtkTree* g)
  {
    typedef graph_traits< vtkTree* >::out_edge_iterator Iter;
    vtkIdType nverts;
    const vtkIdType* verts;
    g->GetOutVertices(u, nverts, verts);
    return vtksys_stl::make_pair( Iter(g, u, verts), Iter(g, u, verts + nverts) );
  }

  inline vtksys_stl::pair<
    graph_traits< vtkTree* >::in_edge_iterator,
    graph_traits< vtkTree* >::in_edge_iterator >  
  in_edges(
    graph_traits< vtkTree* >::vertex_descriptor u, 
    vtkTree* g)
  {
    typedef graph_traits< vtkTree* >::in_edge_iterator Iter;
    vtkIdType nverts;
    const vtkIdType* verts;
    g->GetInVertices(u, nverts, verts);
    if (u == g->GetRoot())
      {
      return vtksys_stl::make_pair( Iter(g, u, NULL), Iter(g, u, NULL) );
      }
    return vtksys_stl::make_pair( Iter(g, u, verts), Iter(g, u, verts + 1) );
  }

  inline vtksys_stl::pair<
    graph_traits< vtkTree* >::adjacency_iterator,
    graph_traits< vtkTree* >::adjacency_iterator >
  adjacent_vertices(
    graph_traits< vtkTree* >::vertex_descriptor u, 
    vtkTree* g)
  {
    typedef graph_traits< vtkTree* >::adjacency_iterator Iter;
    typedef graph_traits< vtkTree* >::out_edge_iterator OutEdgeIter;
    vtksys_stl::pair<OutEdgeIter, OutEdgeIter> out = out_edges(u, g);
    return vtksys_stl::make_pair( Iter(out.first, &g), Iter(out.second, &g) );
  }

  inline graph_traits< vtkTree* >::vertices_size_type
  num_vertices(vtkTree* g)
  {
    return g->GetNumberOfVertices();
  }  

  inline graph_traits< vtkTree* >::edges_size_type
  num_edges(vtkTree* g)
  {
    return g->GetNumberOfEdges();
  }  

  inline graph_traits< vtkTree* >::degree_size_type
  out_degree(
    graph_traits< vtkTree* >::vertex_descriptor u, 
    vtkTree* g)
  {
    return g->GetOutDegree(u);
  }

  inline graph_traits< vtkTree* >::degree_size_type
  in_degree(
    graph_traits< vtkTree* >::vertex_descriptor u, 
    vtkTree* g)
  {
    return g->GetInDegree(u);
  }

  inline graph_traits< vtkTree* >::degree_size_type
  degree(
    graph_traits< vtkTree* >::vertex_descriptor u, 
    vtkTree* g)
  {
    return g->GetDegree(u);
  }
  
} // namespace boost

#endif // __vtkTreeToBoostAdapter_h
