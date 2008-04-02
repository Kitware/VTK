/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPBGLGraphAdapter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/* 
 * Copyright (C) 2008 The Trustees of Indiana University.
 * Use, modification and distribution is subject to the Boost Software
 * License, Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt)
 */
// .NAME vtkPBGLGraphAdapter - adapter to the Parallel Boost Graph Library (http://www.osl.iu.edu/research/pbgl)
//
// .SECTION Description
// Including this header allows you to use a vtk*Graph in Parallel BGL algorithms.

#ifndef __vtkPBGLGraphAdapter_h
#define __vtkPBGLGraphAdapter_h

//BTX
#include <boost/parallel/mpi/bsp_process_group.hpp>
#include <boost/graph/properties.hpp>
#include <boost/parallel/container_traits.hpp>
#include <boost/serialization/base_object.hpp>
//ETX

#include "vtkBoostGraphAdapter.h" // for the sequential BGL adapters

// So that we can specialize the hash function for the hash table
// Parallel BGL is using (a C++ Standard Library extension)
#ifdef PBGL_HAS_HASH_TABLES
#  include PBGL_HASH_MAP_HEADER
#endif

namespace boost {

// Define specializations of class template property_map for
// vtkDirectedGraph and vtkUndirectedGraph, based on the
// specialization for vtkGraph.
#define SUBCLASS_PROPERTY_MAP_SPECIALIZATIONS(Property) \
  template<>                                            \
  struct property_map<vtkDirectedGraph *, Property>     \
    : property_map<vtkGraph *, Property> { };           \
                                                        \
  template<>                                            \
  struct property_map<vtkUndirectedGraph *, Property>   \
    : property_map<vtkGraph *, Property> { }

  // Property map from a vertex descriptor to the owner of the vertex
  struct vtkVertexOwnerMap 
  {
    // Default-construct an empty (useless!) vertex-owner map
    vtkVertexOwnerMap() : graph(0) { }
    
    // Construct a vertex-owner map for a specific vtkGraph
    explicit vtkVertexOwnerMap(vtkGraph* graph) : graph(graph) { }
    
    // The graph for which we will map vertices to their owners
    vtkGraph* graph;
  };
  
  // Property map traits for the vertex-owner map
  template<>
  struct property_traits<vtkVertexOwnerMap> 
  {
    typedef vtkIdType value_type;
    typedef vtkIdType reference;
    typedef vtkIdType key_type;
    typedef readable_property_map_tag category;
  };
  
  // Retrieve the owner of the given vertex (the key)
  inline property_traits<vtkVertexOwnerMap>::reference
  get(
    vtkVertexOwnerMap owner_map,
    property_traits<vtkVertexOwnerMap>::key_type key)
  {
    return owner_map.graph->GetVertexOwner(key);
  }
   
   // State that the vertex owner property map of a vtkGraph is the
   // vtkVertexOwnerMap
   template<>
   struct property_map<vtkGraph*, vertex_owner_t>
   {
     typedef vtkVertexOwnerMap type;
     typedef vtkVertexOwnerMap const_type;
   };
   
  SUBCLASS_PROPERTY_MAP_SPECIALIZATIONS(vertex_owner_t);

    // Retrieve the vertex-owner property map from a vtkGraph
   inline vtkVertexOwnerMap
   get(vertex_owner_t, vtkGraph* graph)
   {
     return vtkVertexOwnerMap(graph);
   }
   
   // Map from vertex descriptor to (owner, local descriptor)
   struct vtkVertexGlobalMap 
   {
     vtkVertexGlobalMap() : graph(0) { }
          
     explicit vtkVertexGlobalMap(vtkGraph* graph) 
       : graph(graph) { }
          
     vtkGraph* graph;
   };
   
   template<>
   struct property_traits<vtkVertexGlobalMap> 
     {
     typedef vtkstd::pair<int, vtkIdType> value_type;
     typedef value_type reference;
     typedef vtkIdType key_type;
     typedef readable_property_map_tag category;
     };
   
   inline property_traits<vtkVertexGlobalMap>::reference
   get(
     vtkVertexGlobalMap global_map,
     property_traits<vtkVertexGlobalMap>::key_type key)
     {
     return vtkstd::pair<int, vtkIdType>(global_map.graph->GetVertexOwner(key),
                                         global_map.graph->GetVertexIndex(key));
     }
    
    // 
   template<>
   struct property_map<vtkGraph*, vertex_global_t>
     {
     typedef vtkVertexGlobalMap type;
     typedef vtkVertexGlobalMap const_type;
     };
    
  SUBCLASS_PROPERTY_MAP_SPECIALIZATIONS(vertex_global_t);

   inline vtkVertexGlobalMap
   get(vertex_global_t, vtkGraph* graph)
     {
     return vtkVertexGlobalMap(graph);
     }
   
   // Map from edge descriptor to (owner, local descriptor)
   struct vtkEdgeGlobalMap 
     {
     vtkEdgeGlobalMap() : graph(0) { }
          
     explicit vtkEdgeGlobalMap(vtkGraph* graph) 
       : graph(graph) { }
          
     vtkGraph* graph;
     };
      
   template<>
   struct property_traits<vtkEdgeGlobalMap> 
     {
     typedef vtkstd::pair<int, vtkEdgeType> value_type;
     typedef value_type reference;
     typedef vtkEdgeType key_type;
     typedef readable_property_map_tag category;
     };
      
   inline property_traits<vtkEdgeGlobalMap>::reference
   get(
     vtkEdgeGlobalMap global_map,
     property_traits<vtkEdgeGlobalMap>::key_type key)
     {
     return vtkstd::pair<int, vtkEdgeType>(global_map.graph->GetEdgeOwner(key.Id),
                                        key);
     }
       
   // 
   template<>
   struct property_map<vtkGraph*, edge_global_t>
     {
     typedef vtkEdgeGlobalMap type;
     typedef vtkEdgeGlobalMap const_type;
     };
       
  SUBCLASS_PROPERTY_MAP_SPECIALIZATIONS(edge_global_t);

   inline vtkEdgeGlobalMap
   get(edge_global_t, vtkGraph* graph)
     {
     return vtkEdgeGlobalMap(graph);
     }

#undef SUBCLASS_PROPERTY_MAP_SPECIALIZATIONS
} // namespace boost

namespace boost { namespace parallel { 
  //----------------------------------------------------------------------------
  // Extract the process group from a vtkGraph
  //----------------------------------------------------------------------------
  template<>
  struct process_group_type<vtkGraph *> 
  {
    typedef boost::parallel::mpi::bsp_process_group type;
  };

  process_group_type<vtkGraph *>::type process_group(vtkGraph *graph);

  template<>
  struct process_group_type<vtkDirectedGraph *> 
    : process_group_type<vtkGraph *> { };

  inline process_group_type<vtkGraph *>::type process_group(vtkDirectedGraph *graph)
  {
    return process_group(static_cast<vtkGraph *>(graph));
  }

  template<>
  struct process_group_type<vtkUndirectedGraph *> 
    : process_group_type<vtkGraph *> { };

  inline process_group_type<vtkGraph *>::type process_group(vtkUndirectedGraph *graph)
  {
    return process_group(static_cast<vtkGraph *>(graph));
  }

} } // end namespace boost::parallel

//----------------------------------------------------------------------------
// Serialization support for simple VTK structures
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
template<typename Archiver>
void serialize(Archiver& ar, vtkEdgeBase& edge, const unsigned int)
{
  ar & edge.Id;
}

template<typename Archiver>
void serialize(Archiver& ar, vtkOutEdgeType& edge, const unsigned int)
{
  ar & boost::serialization::base_object<vtkEdgeBase>(edge)
     & edge.Target;
}

template<typename Archiver>
void serialize(Archiver& ar, vtkInEdgeType& edge, const unsigned int)
{
  ar & boost::serialization::base_object<vtkEdgeBase>(edge)
     & edge.Source;
}

template<typename Archiver>
void serialize(Archiver& ar, vtkEdgeType& edge, const unsigned int)
{
  ar & boost::serialization::base_object<vtkEdgeBase>(edge)
     & edge.Source
     & edge.Target;
}

#if defined PBGL_HAS_HASH_TABLES && defined(__GNUC__) && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ > 0))
namespace PBGL_HASH_NAMESPACE {
  template<>
  struct hash<vtkIdType>
  {
    vtkstd::size_t operator()(vtkIdType x) const { return x; }
  };
}
#endif

#endif // __vtkPBGLGraphAdapter_h
