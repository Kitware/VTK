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
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/parallel/container_traits.hpp>
#include <boost/serialization/base_object.hpp>
//ETX

#include "vtkBoostGraphAdapter.h" // for the sequential BGL adapters
#include "vtkPBGLDistributedGraphHelper.h"

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

//----------------------------------------------------------------------------
// Extract the process group from a vtkGraph
//----------------------------------------------------------------------------

namespace boost { namespace graph { namespace parallel { 
  template<>
  struct process_group_type<vtkGraph *> 
  {
    typedef boost::graph::distributed::mpi_process_group type;
  };

  template<>
  struct process_group_type<vtkDirectedGraph *> 
    : process_group_type<vtkGraph *> { };

  template<>
  struct process_group_type<vtkUndirectedGraph *> 
    : process_group_type<vtkGraph *> { };
} } } // end namespace boost::graph::parallel

boost::graph::distributed::mpi_process_group process_group(vtkGraph *graph);

inline boost::graph::distributed::mpi_process_group 
process_group(vtkDirectedGraph *graph)
{
  return process_group(static_cast<vtkGraph *>(graph));
}

inline boost::graph::distributed::mpi_process_group
process_group(vtkUndirectedGraph *graph)
{
  return process_group(static_cast<vtkGraph *>(graph));
}

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

//----------------------------------------------------------------------------
// Simplified tools to build distributed property maps
//----------------------------------------------------------------------------

// Description:
// Retrieves the type of the distributed property map indexed by the
// vertices of a distributed graph.
template<typename DataArray>
struct vtkDistributedVertexPropertyMapType
{
  typedef boost::parallel::distributed_property_map<
            boost::graph::distributed::mpi_process_group,
            boost::vtkVertexGlobalMap,
            DataArray*> type;
};

// Description:
// Build a distributed property map indexed by the vertices of the
// given graph, using storage from the given array.
template<typename DataArray>
inline typename vtkDistributedVertexPropertyMapType<DataArray>::type
MakeDistributedVertexPropertyMap(vtkGraph* graph, DataArray* array)
{
  typedef typename vtkDistributedVertexPropertyMapType<DataArray>::type MapType;
  
  vtkDistributedGraphHelper *helper = graph->GetDistributedGraphHelper();
  if (!helper)
    {
    vtkErrorWithObjectMacro(graph, "A vtkGraph without a distributed graph helper is not a distributed graph");
    return MapType();
    }

  vtkPBGLDistributedGraphHelper *pbglHelper 
    = vtkPBGLDistributedGraphHelper::SafeDownCast(helper);
  if (!pbglHelper)
    {
    vtkErrorWithObjectMacro(graph, "A vtkGraph with a non-Parallel BGL distributed graph helper cannot be used with the Parallel BGL");
    return MapType();
    }

  return MapType(pbglHelper->GetProcessGroup(),
                 boost::vtkVertexGlobalMap(graph),
                 array);
}

// Description:
// Retrieves the type of the distributed property map indexed by the
// edges of a distributed graph.
template<typename DataArray>
struct vtkDistributedEdgePropertyMapType
{
  typedef boost::parallel::distributed_property_map<
            boost::graph::distributed::mpi_process_group,
            boost::vtkEdgeGlobalMap,
            DataArray*> type;
};

// Description:
// Build a distributed property map indexed by the edges of the
// given graph, using storage from the given array.
template<typename DataArray>
inline typename vtkDistributedEdgePropertyMapType<DataArray>::type
MakeDistributedEdgePropertyMap(vtkGraph* graph, DataArray* array)
{
  typedef typename vtkDistributedEdgePropertyMapType<DataArray>::type MapType;

  vtkDistributedGraphHelper *helper = graph->GetDistributedGraphHelper();
  if (!helper)
    {
    vtkErrorWithObjectMacro(graph, "A vtkGraph without a distributed graph helper is not a distributed graph");
    return MapType();
    }

  vtkPBGLDistributedGraphHelper *pbglHelper 
    = vtkPBGLDistributedGraphHelper::SafeDownCast(helper);
  if (!pbglHelper)
    {
    vtkErrorWithObjectMacro(graph, "A vtkGraph with a non-Parallel BGL distributed graph helper cannot be used with the Parallel BGL");
    return MapType();
    }

  return MapType(pbglHelper->GetProcessGroup(),
                 boost::vtkEdgeGlobalMap(graph),
                 array);
}

#endif // __vtkPBGLGraphAdapter_h
