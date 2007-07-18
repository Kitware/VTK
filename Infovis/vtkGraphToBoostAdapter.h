#ifndef __vtkGraphToBoostAdapter_h
#define __vtkGraphToBoostAdapter_h

#include <vtksys/stl/utility>

#include <boost/config.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/adjacency_iterator.hpp>
#include <boost/property_map.hpp>
#include <boost/vector_property_map.hpp>

#include "vtkGraph.h"
#include "vtkIntArray.h"
#include "vtkIdTypeArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"

// The functions and classes in this file allows the user to
// treat a vtkBoostDirectedGraph object as a boost graph "as is". No
// wrapper is needed for the vtkAbstractGraph object.

namespace boost {

  struct vtkGraph_traversal_category : 
    public virtual bidirectional_graph_tag,
    public virtual edge_list_graph_tag,
    public virtual vertex_list_graph_tag,
    public virtual adjacency_graph_tag { };
    
  struct vtkBoostDirectedGraph;
  
  template <>
  struct graph_traits< vtkBoostDirectedGraph > {
    typedef vtkIdType vertex_descriptor;
    static vertex_descriptor null_vertex() { return -1; }
    typedef vtkIdType edge_descriptor;
    typedef vtkIdType* out_edge_iterator;
    typedef vtkIdType* in_edge_iterator;

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
    typedef vtkGraph_traversal_category traversal_category;
    typedef vtkIdType vertices_size_type;
    typedef vtkIdType edges_size_type;
    typedef vtkIdType degree_size_type;

    typedef adjacency_iterator_generator<vtkBoostDirectedGraph, vertex_descriptor, out_edge_iterator>::type adjacency_iterator;
  };


  //===========================================================================
  // vtkBoostDirectedGraph
  // VertexAndEdgeListGraphConcept
  // BidirectionalGraphConcept
  // MutableGraphConcept
  // AdjacencyGraphConcept
  struct vtkBoostDirectedGraph
  {
    vtkBoostDirectedGraph() 
    {
      this->Graph = vtkGraph::New();
      this->Graph->SetDirected(true);
    }

    vtkBoostDirectedGraph(vtkGraph* g) : Graph(g) 
    {
      assert(g->GetDirected());
      this->Graph->Register(NULL);
    }

    vtkBoostDirectedGraph(vtkBoostDirectedGraph& other)
    {
      this->Graph = other.Graph;
      this->Graph->Register(NULL);
    }

    ~vtkBoostDirectedGraph()
    {
      if (this->Graph != NULL)
        {
        this->Graph->Delete();
        this->Graph = NULL;
        }
    }

    vtkGraph* Graph;
  };

  // The graph_traits for a const graph are the same as a non-const graph.
  template <>
  struct graph_traits<const vtkBoostDirectedGraph> : graph_traits<vtkBoostDirectedGraph> { };
  
  template <>
  class vertex_property< vtkBoostDirectedGraph > {
  public:
    typedef vtkIdType type;
  };

  template <>
  class edge_property< vtkBoostDirectedGraph > {
  public:
    typedef vtkIdType type;
  };

  inline graph_traits< vtkBoostDirectedGraph >::vertex_descriptor
  source(graph_traits< vtkBoostDirectedGraph >::edge_descriptor e,
         const vtkBoostDirectedGraph& g)
  {
    return g.Graph->GetSourceVertex(e);
  }

  inline graph_traits< vtkBoostDirectedGraph >::vertex_descriptor
  target(graph_traits< vtkBoostDirectedGraph >::edge_descriptor e,
         const vtkBoostDirectedGraph& g)
  {
    return g.Graph->GetTargetVertex(e);
  }

  inline vtksys_stl::pair<
    graph_traits< vtkBoostDirectedGraph >::vertex_iterator,
    graph_traits< vtkBoostDirectedGraph >::vertex_iterator >  
  vertices(const vtkBoostDirectedGraph& g)
  {
    typedef graph_traits< vtkBoostDirectedGraph >::vertex_iterator Iter;
    return vtksys_stl::make_pair( 0, g.Graph->GetNumberOfVertices() );
  }

  inline vtksys_stl::pair<
    graph_traits< vtkBoostDirectedGraph >::edge_iterator,
    graph_traits< vtkBoostDirectedGraph >::edge_iterator >  
  edges(const vtkBoostDirectedGraph& g)
  {
    typedef graph_traits< vtkBoostDirectedGraph >::edge_iterator
      Iter;
    return vtksys_stl::make_pair( Iter(0), Iter(g.Graph->GetNumberOfEdges()) );
  }

  inline vtksys_stl::pair<
    graph_traits< vtkBoostDirectedGraph >::out_edge_iterator,
    graph_traits< vtkBoostDirectedGraph >::out_edge_iterator >  
  out_edges(
    graph_traits< vtkBoostDirectedGraph >::vertex_descriptor u, 
    const vtkBoostDirectedGraph& g)
  {
    typedef graph_traits< vtkBoostDirectedGraph >::out_edge_iterator Iter;
    vtkIdType nedges;
    const vtkIdType* edges;
    g.Graph->GetOutEdges(u, nedges, edges);
    return vtksys_stl::make_pair( Iter(edges), Iter(edges + nedges) );
  }

  inline vtksys_stl::pair<
    graph_traits< vtkBoostDirectedGraph >::in_edge_iterator,
    graph_traits< vtkBoostDirectedGraph >::in_edge_iterator >  
  in_edges(
    graph_traits< vtkBoostDirectedGraph >::vertex_descriptor u, 
    const vtkBoostDirectedGraph& g)
  {
    typedef graph_traits< vtkBoostDirectedGraph >::in_edge_iterator Iter;
    vtkIdType nedges;
    const vtkIdType* edges;
    g.Graph->GetInEdges(u, nedges, edges);
    return vtksys_stl::make_pair( Iter(edges), Iter(edges + nedges) );
  }

  inline vtksys_stl::pair<
    graph_traits< vtkBoostDirectedGraph >::adjacency_iterator,
    graph_traits< vtkBoostDirectedGraph >::adjacency_iterator >
  adjacent_vertices(
    graph_traits< vtkBoostDirectedGraph >::vertex_descriptor u, 
    const vtkBoostDirectedGraph& g)
  {
    typedef graph_traits< vtkBoostDirectedGraph >::adjacency_iterator Iter;
    typedef graph_traits< vtkBoostDirectedGraph >::out_edge_iterator OutEdgeIter;
    vtksys_stl::pair<OutEdgeIter, OutEdgeIter> out = out_edges(u, g);
    return vtksys_stl::make_pair( Iter(out.first, &g), Iter(out.second, &g) );
  }

  inline graph_traits< vtkBoostDirectedGraph >::vertices_size_type
  num_vertices(const vtkBoostDirectedGraph& g)
  {
    return g.Graph->GetNumberOfVertices();
  }  

  inline graph_traits< vtkBoostDirectedGraph >::edges_size_type
  num_edges(const vtkBoostDirectedGraph& g)
  {
    return g.Graph->GetNumberOfEdges();
  }  

  inline graph_traits< vtkBoostDirectedGraph >::degree_size_type
  out_degree(
    graph_traits< vtkBoostDirectedGraph >::vertex_descriptor u, 
    const vtkBoostDirectedGraph& g)
  {
    return g.Graph->GetOutDegree(u);
  }

  inline graph_traits< vtkBoostDirectedGraph >::degree_size_type
  in_degree(
    graph_traits< vtkBoostDirectedGraph >::vertex_descriptor u, 
    const vtkBoostDirectedGraph& g)
  {
    return g.Graph->GetInDegree(u);
  }

  inline graph_traits< vtkBoostDirectedGraph >::degree_size_type
  degree(
    graph_traits< vtkBoostDirectedGraph >::vertex_descriptor u, 
    const vtkBoostDirectedGraph& g)
  {
    return g.Graph->GetDegree(u);
  }
  
  inline graph_traits< vtkBoostDirectedGraph >::vertex_descriptor
  add_vertex(vtkBoostDirectedGraph& g)
  {
    return g.Graph->AddVertex();
  }

  inline void clear_vertex(
    graph_traits< vtkBoostDirectedGraph >::vertex_descriptor u,
    vtkBoostDirectedGraph& g)
  {
    g.Graph->ClearVertex(u);
  }

  inline void remove_vertex(
    graph_traits< vtkBoostDirectedGraph >::vertex_descriptor u,
    vtkBoostDirectedGraph& g)
  {
    g.Graph->RemoveVertex(u);
  }

  inline vtksys_stl::pair<
    graph_traits< vtkBoostDirectedGraph >::edge_descriptor,
    bool>
  add_edge(
    graph_traits< vtkBoostDirectedGraph >::vertex_descriptor u,
    graph_traits< vtkBoostDirectedGraph >::vertex_descriptor v,
    vtkBoostDirectedGraph& g)
  {
    return vtksys_stl::make_pair(g.Graph->AddEdge(u, v), true);
  }

  inline void
  remove_edge(
    graph_traits< vtkBoostDirectedGraph >::vertex_descriptor u,
    graph_traits< vtkBoostDirectedGraph >::vertex_descriptor v,
    vtkBoostDirectedGraph& g)
  {
    // TODO: This may fail if multiple edges are removed and renumbered
    graph_traits< vtkBoostDirectedGraph >::out_edge_iterator 
      i,iend;
    for (boost::tie(i,iend) = out_edges(u,g); i != iend; ++i)
      {
      if (target(*i,g) == v)
        {
        g.Graph->RemoveEdge(*i);
        }
      }
  }

  inline void
  remove_edge(
    graph_traits< vtkBoostDirectedGraph >::edge_descriptor e,
    vtkBoostDirectedGraph& g)
  {
    g.Graph->RemoveEdge(e);
  }

  struct vtkBoostUndirectedGraph;
  
    template <>
  struct graph_traits< vtkBoostUndirectedGraph > {
    typedef vtkIdType vertex_descriptor;
    static vertex_descriptor null_vertex() { return -1; }

    typedef vtkIdType edge_descriptor;

    class edge_flip_iterator
      : public iterator_facade<edge_flip_iterator,
                               vtkIdType,
                               bidirectional_traversal_tag,
                               vtkIdType,
                               const vtkIdType*>
    {
    public:
      explicit edge_flip_iterator()
      : graph(NULL), ptr(NULL), ref(0), outgoing(true) {}

      explicit edge_flip_iterator(
        vtkGraph* g, const vtkIdType* p, vtkIdType r, bool out) 
      : graph(g), ptr(p), ref(r), outgoing(out) {}

    private:
      vtkIdType dereference() const
      {
        vtkIdType edge_id = *ptr;
        bool flip = outgoing ? ref == graph->GetTargetVertex(edge_id) : ref == graph->GetSourceVertex(edge_id);
        return flip ? -(edge_id + 1) : edge_id;
      }

      bool equal(const edge_flip_iterator& other) const
      {
        return graph == other.graph &&
          ptr == other.ptr &&
          ref == other.ref &&
          outgoing == other.outgoing;
      }

      void increment() { ptr++; }
      void decrement() { ptr--; }

      vtkGraph* graph;
      const vtkIdType* ptr;
      vtkIdType ref;
      bool outgoing;

      friend class iterator_core_access;
    };

    typedef edge_flip_iterator out_edge_iterator;
    typedef edge_flip_iterator in_edge_iterator;

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

    typedef undirected_tag directed_category;
    typedef allow_parallel_edge_tag edge_parallel_category;
    typedef vtkGraph_traversal_category traversal_category;
    typedef vtkIdType vertices_size_type;
    typedef vtkIdType edges_size_type;
    typedef vtkIdType degree_size_type;

    typedef adjacency_iterator_generator<vtkBoostUndirectedGraph, vertex_descriptor, out_edge_iterator>::type adjacency_iterator;
  };


  
  //===========================================================================
  // vtkBoostUndirectedGraph
  // VertexAndEdgeListGraphConcept
  // BidirectionalGraphConcept
  // MutableGraphConcept
  // AdjacencyGraphConcept
  struct vtkBoostUndirectedGraph
  {
    vtkBoostUndirectedGraph() 
    {
      this->Graph = vtkGraph::New();
      this->Graph->SetDirected(true);
    }

    vtkBoostUndirectedGraph(vtkGraph* g) : Graph(g) 
    {
      assert(!g->GetDirected());
      this->Graph->Register(NULL);
    }

    vtkBoostUndirectedGraph(vtkBoostUndirectedGraph& other)
    {
      this->Graph = other.Graph;
      this->Graph->Register(NULL);
    }

    ~vtkBoostUndirectedGraph()
    {
      if (this->Graph != NULL)
        {
        this->Graph->Delete();
        this->Graph = NULL;
        }
    }

    vtkGraph* Graph;
  };

  // The graph_traits for a const graph are the same as a non-const graph.
  template <>
  struct graph_traits<const vtkBoostUndirectedGraph> : graph_traits<vtkBoostUndirectedGraph> { };

  template <>
  class vertex_property< vtkBoostUndirectedGraph > {
  public:
    typedef vtkIdType type;
  };

  template <>
  class edge_property< vtkBoostUndirectedGraph > {
  public:
    typedef vtkIdType type;
  };

  inline vtkIdType
  vtkGraph_edge_id(graph_traits< vtkBoostUndirectedGraph >::edge_descriptor e)
  {
    if (e < 0)
      {
      return -e-1;
      }
    return e;
  }

  inline graph_traits< vtkBoostUndirectedGraph >::vertex_descriptor
  source(graph_traits< vtkBoostUndirectedGraph >::edge_descriptor e,
         const vtkBoostUndirectedGraph& g)
  {
    // If outgoing edge, return reference vertex; otherwise, return opposite vertex
    if (e < 0)
      {
      return g.Graph->GetTargetVertex(-e-1);
      }
    return g.Graph->GetSourceVertex(e);
  }

  inline graph_traits< vtkBoostUndirectedGraph >::vertex_descriptor
  target(graph_traits< vtkBoostUndirectedGraph >::edge_descriptor e,
         const vtkBoostUndirectedGraph& g)
  {
    // If outgoing edge, return opposite vertex; otherwise, return reference vertex
    if (e < 0)
      {
      return g.Graph->GetSourceVertex(-e-1);
      }
    return g.Graph->GetTargetVertex(e);
  }

  inline vtksys_stl::pair<
    graph_traits< vtkBoostUndirectedGraph >::vertex_iterator,
    graph_traits< vtkBoostUndirectedGraph >::vertex_iterator >  
  vertices(const vtkBoostUndirectedGraph& g)
  {
    typedef graph_traits< vtkBoostUndirectedGraph >::vertex_iterator Iter;
    return vtksys_stl::make_pair( Iter(0), Iter(g.Graph->GetNumberOfVertices()) );
  }

  inline vtksys_stl::pair<
    graph_traits< vtkBoostUndirectedGraph >::edge_iterator,
    graph_traits< vtkBoostUndirectedGraph >::edge_iterator >  
  edges(const vtkBoostUndirectedGraph& g)
  {
    typedef graph_traits< vtkBoostUndirectedGraph >::edge_iterator
      Iter;
    return vtksys_stl::make_pair( Iter(0), Iter(g.Graph->GetNumberOfEdges()) );
  }

  inline vtksys_stl::pair<
    graph_traits< vtkBoostUndirectedGraph >::out_edge_iterator,
    graph_traits< vtkBoostUndirectedGraph >::out_edge_iterator >  
  out_edges(
    graph_traits< vtkBoostUndirectedGraph >::vertex_descriptor u, 
    const vtkBoostUndirectedGraph& g)
  {
    typedef graph_traits< vtkBoostUndirectedGraph >::out_edge_iterator Iter;
    vtkIdType nedges;
    const vtkIdType* edges;
    g.Graph->GetIncidentEdges(u, nedges, edges);
    return vtksys_stl::make_pair( Iter(g.Graph, edges, u, true), Iter(g.Graph, edges + nedges, u, true) );
  }

  inline vtksys_stl::pair<
    graph_traits< vtkBoostUndirectedGraph >::in_edge_iterator,
    graph_traits< vtkBoostUndirectedGraph >::in_edge_iterator >  
  in_edges(
    graph_traits< vtkBoostUndirectedGraph >::vertex_descriptor u, 
    const vtkBoostUndirectedGraph& g)
  {
    typedef graph_traits< vtkBoostUndirectedGraph >::in_edge_iterator Iter;
    vtkIdType nedges;
    const vtkIdType* edges;
    g.Graph->GetIncidentEdges(u, nedges, edges);
    return vtksys_stl::make_pair( Iter(g.Graph, edges, u, false), Iter(g.Graph, edges + nedges, u, false) );
  }

  inline vtksys_stl::pair<
    graph_traits< vtkBoostUndirectedGraph >::adjacency_iterator,
    graph_traits< vtkBoostUndirectedGraph >::adjacency_iterator >
  adjacent_vertices(
    graph_traits< vtkBoostUndirectedGraph >::vertex_descriptor u, 
    const vtkBoostUndirectedGraph& g)
  {
    typedef graph_traits< vtkBoostUndirectedGraph >::adjacency_iterator Iter;
    typedef graph_traits< vtkBoostUndirectedGraph >::out_edge_iterator OutEdgeIter;
    vtksys_stl::pair<OutEdgeIter, OutEdgeIter> out = out_edges(u, g);
    return vtksys_stl::make_pair( Iter(out.first, &g), Iter(out.second, &g) );
  }

  inline graph_traits< vtkBoostUndirectedGraph >::vertices_size_type
  num_vertices(const vtkBoostUndirectedGraph& g)
  {
    return g.Graph->GetNumberOfVertices();
  }  

  inline graph_traits< vtkBoostUndirectedGraph >::edges_size_type
  num_edges(const vtkBoostUndirectedGraph& g)
  {
    return g.Graph->GetNumberOfEdges();
  }  

  inline graph_traits< vtkBoostUndirectedGraph >::degree_size_type
  out_degree(
    graph_traits< vtkBoostUndirectedGraph >::vertex_descriptor u, 
    const vtkBoostUndirectedGraph& g)
  {
    return g.Graph->GetDegree(u);
  }

  inline graph_traits< vtkBoostUndirectedGraph >::degree_size_type
  in_degree(
    graph_traits< vtkBoostUndirectedGraph >::vertex_descriptor u, 
    const vtkBoostUndirectedGraph& g)
  {
    return g.Graph->GetDegree(u);
  }

  inline graph_traits< vtkBoostUndirectedGraph >::degree_size_type
  degree(
    graph_traits< vtkBoostUndirectedGraph >::vertex_descriptor u, 
    const vtkBoostUndirectedGraph& g)
  {
    return g.Graph->GetDegree(u);
  }
  
  inline graph_traits< vtkBoostUndirectedGraph >::vertex_descriptor
  add_vertex(vtkBoostUndirectedGraph& g)
  {
    return g.Graph->AddVertex();
  }

  inline void clear_vertex(
    graph_traits< vtkBoostUndirectedGraph >::vertex_descriptor u,
    vtkBoostUndirectedGraph& g)
  {
    g.Graph->ClearVertex(u);
  }

  inline void remove_vertex(
    graph_traits< vtkBoostUndirectedGraph >::vertex_descriptor u,
    vtkBoostUndirectedGraph& g)
  {
    g.Graph->RemoveVertex(u);
  }

  inline vtksys_stl::pair<
    graph_traits< vtkBoostUndirectedGraph >::edge_descriptor,
    bool>
  add_edge(
    graph_traits< vtkBoostUndirectedGraph >::vertex_descriptor u,
    graph_traits< vtkBoostUndirectedGraph >::vertex_descriptor v,
    vtkBoostUndirectedGraph& g)
  {
    typedef graph_traits< vtkBoostUndirectedGraph >::edge_descriptor Edge;
    Edge e = g.Graph->AddEdge(u, v);
    return vtksys_stl::make_pair(e, true);
  }

  inline void
  remove_edge(
    graph_traits< vtkBoostUndirectedGraph >::vertex_descriptor u,
    graph_traits< vtkBoostUndirectedGraph >::vertex_descriptor v,
    vtkBoostUndirectedGraph& g)
  {
    // TODO: This may fail if multiple edges are removed and renumbered
    graph_traits< vtkBoostUndirectedGraph >::out_edge_iterator 
      i,iend;
    for (boost::tie(i,iend) = out_edges(u,g); i != iend; ++i)
      {
      if (target(*i,g) == v)
        {
        vtkIdType e = *i;
        if (e < 0)
          {
          g.Graph->RemoveEdge(-e-1);
          }
        else
          {
          g.Graph->RemoveEdge(e);
          }
        }
      }
  }

  inline void
  remove_edge(
    graph_traits< vtkBoostUndirectedGraph >::edge_descriptor e,
    vtkBoostUndirectedGraph& g)
  {
    if (e < 0)
      {
      g.Graph->RemoveEdge(-e-1);
      }
    else
      {
      g.Graph->RemoveEdge(e);
      }
  }


  //===========================================================================
  // VTK arrays as property maps

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
    T* arr,                                             \
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
    return arr->InsertValue(key, value);                \
  }

  vtkPropertyMapMacro(vtkIdTypeArray, vtkIdType)
  vtkPropertyMapMacro(vtkIntArray, int)
  vtkPropertyMapMacro(vtkDoubleArray, double)
  vtkPropertyMapMacro(vtkFloatArray, float)

  template<typename PMap>
  class vtkGraphEdgePropertyMapHelper
  {
  public:
    vtkGraphEdgePropertyMapHelper(PMap m) : pmap(m) { }
    PMap pmap;
    typedef typename property_traits<PMap>::value_type value_type;
    typedef typename property_traits<PMap>::reference reference;
    typedef typename property_traits<PMap>::key_type key_type;
    typedef typename property_traits<PMap>::category category;
  };

  template<typename PMap>
  inline typename property_traits<PMap>::reference
  get(
    vtkGraphEdgePropertyMapHelper<PMap> helper,
    typename property_traits<PMap>::key_type key)
  {
    return get(helper.pmap, vtkGraph_edge_id(key));
  } 

  template<typename PMap>
  inline void
  put(
    vtkGraphEdgePropertyMapHelper<PMap> helper,
    typename property_traits<PMap>::key_type key,
    const typename property_traits<PMap>::value_type & value)
  {
    put(helper.pmap, vtkGraph_edge_id(key), value);
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
