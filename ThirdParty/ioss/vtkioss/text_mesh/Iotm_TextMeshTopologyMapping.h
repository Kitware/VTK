// Copyright(C) 1999-2020, 2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include <Ioss_CodeTypes.h>
#include <Ioss_EntityType.h> // for EntityType

#include <cstddef> // for size_t
#include <cstdint> // for int64_t
#include <map>     // for map, etc
#include <string>  // for string
#include <unordered_map>
#include <utility> // for pair
#include <vector>  // for vector

#include "Ioss_ElementTopology.h"
#include "Ioss_StandardElementTypes.h"

#include "Iotm_TextMeshUtils.h"

namespace Iotm {
  struct TopologyMapEntry
  {
    using DimensionArray = bool[4];

    unsigned int           id;
    Ioss::ElementTopology *topology;

    // Defines what spatial dimension the topology is valid on
    DimensionArray validSpatialDimensions;

    TopologyMapEntry()
        : id(Ioss::ElementTopology::get_unique_id(Ioss::Unknown::name)),
          topology(Ioss::ElementTopology::factory(Ioss::Unknown::name))
    {
      set_valid_spatial_dimensions({false, false, false, false});
    }

    TopologyMapEntry(const std::string &name, const DimensionArray &validSpatialDimensions_)
        : id(Ioss::ElementTopology::get_unique_id(name)),
          topology(Ioss::ElementTopology::factory(name))
    {
      set_valid_spatial_dimensions(validSpatialDimensions_);
    }

    TopologyMapEntry(const TopologyMapEntry &topo) : id(topo.id), topology(topo.topology)
    {
      set_valid_spatial_dimensions(topo.validSpatialDimensions);
    }

    void set_valid_spatial_dimensions(const DimensionArray &validSpatialDimensions_)
    {
      validSpatialDimensions[0] = validSpatialDimensions_[0];
      validSpatialDimensions[1] = validSpatialDimensions_[1];
      validSpatialDimensions[2] = validSpatialDimensions_[2];
      validSpatialDimensions[3] = validSpatialDimensions_[3];
    }

    bool defined_on_spatial_dimension(const unsigned spatialDim) const
    {
      if (spatialDim > 3) {
        return false;
      }
      return validSpatialDimensions[spatialDim];
    }

    const std::string &name() const { return topology->name(); }

    int num_nodes() const { return topology->number_nodes(); }

    bool equivalent_valid_spatial_dimensions(const DimensionArray &validSpatialDimensions_) const
    {
      return validSpatialDimensions[0] == validSpatialDimensions_[0] &&
             validSpatialDimensions[1] == validSpatialDimensions_[1] &&
             validSpatialDimensions[2] == validSpatialDimensions_[2] &&
             validSpatialDimensions[3] == validSpatialDimensions_[3];
    }

    bool operator==(const TopologyMapEntry &rhs) const
    {
      return id == rhs.id && topology == rhs.topology &&
             equivalent_valid_spatial_dimensions(rhs.validSpatialDimensions);
    }

    bool operator!=(const TopologyMapEntry &rhs) const { return !(*this == rhs); }
  };

  class IossTopologyMapping : public text_mesh::TopologyMapping<TopologyMapEntry>
  {
  public:
    TopologyMapEntry invalid_topology() const override { return TopologyMapEntry(); }

    // clang-format off
  void initialize_topology_map() override
  {
    m_nameToTopology = {
        {"NODE",         TopologyMapEntry(Ioss::Node::name,        {false,true ,true ,true })},
        {"LINE_2",       TopologyMapEntry(Ioss::Edge2::name,       {false,false,true ,true })},
        {"LINE_3",       TopologyMapEntry(Ioss::Edge3::name,       {false,false,true ,true })},
        {"TRI_3",        TopologyMapEntry(Ioss::Tri3::name,        {false,false,false,true })},
        {"TRI_4",        TopologyMapEntry(Ioss::Tri4::name,        {false,false,false,true })},
        {"TRI_6",        TopologyMapEntry(Ioss::Tri6::name,        {false,false,false,true })},
        {"QUAD_4",       TopologyMapEntry(Ioss::Quad4::name,       {false,false,false,true })},
        {"QUAD_6",       TopologyMapEntry(Ioss::Quad6::name,       {false,false,false,true })},
        {"QUAD_8",       TopologyMapEntry(Ioss::Quad8::name,       {false,false,false,true })},
        {"QUAD_9",       TopologyMapEntry(Ioss::Quad9::name,       {false,false,false,true })},
        {"PARTICLE",     TopologyMapEntry(Ioss::Sphere::name,      {false,true ,true ,true })},
        {"LINE_2_1D",    TopologyMapEntry(Ioss::Edge2::name,       {false,true ,false,false})},
        {"LINE_3_1D",    TopologyMapEntry(Ioss::Edge2::name,       {false,true ,false,false})},
        {"BEAM_2",       TopologyMapEntry(Ioss::Beam2::name,       {false,false,true ,true })},
        {"BEAM_3",       TopologyMapEntry(Ioss::Beam3::name,       {false,false,true ,true })},
        {"SHELL_LINE_2", TopologyMapEntry(Ioss::ShellLine2D2::name,{false,false,true ,false})},
        {"SHELL_LINE_3", TopologyMapEntry(Ioss::ShellLine2D3::name,{false,false,true ,false})},
        {"SPRING_2",     TopologyMapEntry(Ioss::Spring2::name,     {false,true ,true ,true })},
        {"SPRING_3",     TopologyMapEntry(Ioss::Spring3::name,     {false,true ,true ,true })},
        {"TRI_3_2D",     TopologyMapEntry(Ioss::Tri3::name,        {false,false,true ,false})},
        {"TRI_4_2D",     TopologyMapEntry(Ioss::Tri4::name,        {false,false,true ,false})},
        {"TRI_6_2D",     TopologyMapEntry(Ioss::Tri6::name,        {false,false,true ,false})},
        {"QUAD_4_2D",    TopologyMapEntry(Ioss::Quad4::name,       {false,false,true ,false})},
        {"QUAD_8_2D",    TopologyMapEntry(Ioss::Quad8::name,       {false,false,true ,false})},
        {"QUAD_9_2D",    TopologyMapEntry(Ioss::Quad9::name,       {false,false,true ,false})},
        {"SHELL_TRI_3",  TopologyMapEntry(Ioss::TriShell3::name,   {false,false,false,true })},
        {"SHELL_TRI_4",  TopologyMapEntry(Ioss::TriShell4::name,   {false,false,false,true })},
        {"SHELL_TRI_6",  TopologyMapEntry(Ioss::TriShell6::name,   {false,false,false,true })},
        {"SHELL_QUAD_4", TopologyMapEntry(Ioss::Shell4::name,      {false,false,false,true })},
        {"SHELL_QUAD_8", TopologyMapEntry(Ioss::Shell8::name,      {false,false,false,true })},
        {"SHELL_QUAD_9", TopologyMapEntry(Ioss::Shell9::name,      {false,false,false,true })},
        {"TET_4",        TopologyMapEntry(Ioss::Tet4::name,        {false,false,false,true })},
        {"TET_8",        TopologyMapEntry(Ioss::Tet8::name,        {false,false,false,true })},
        {"TET_10",       TopologyMapEntry(Ioss::Tet10::name,       {false,false,false,true })},
        {"TET_11",       TopologyMapEntry(Ioss::Tet11::name,       {false,false,false,true })},
        {"PYRAMID_5",    TopologyMapEntry(Ioss::Pyramid5::name,    {false,false,false,true })},
        {"PYRAMID_13",   TopologyMapEntry(Ioss::Pyramid13::name,   {false,false,false,true })},
        {"PYRAMID_14",   TopologyMapEntry(Ioss::Pyramid14::name,   {false,false,false,true })},
        {"WEDGE_6",      TopologyMapEntry(Ioss::Wedge6::name,      {false,false,false,true })},
        {"WEDGE_12",     TopologyMapEntry(Ioss::Wedge12::name,     {false,false,false,true })},
        {"WEDGE_15",     TopologyMapEntry(Ioss::Wedge15::name,     {false,false,false,true })},
        {"WEDGE_18",     TopologyMapEntry(Ioss::Wedge18::name,     {false,false,false,true })},
        {"HEX_8",        TopologyMapEntry(Ioss::Hex8::name,        {false,false,false,true })},
        {"HEX_20",       TopologyMapEntry(Ioss::Hex20::name,       {false,false,false,true })},
        {"HEX_27",       TopologyMapEntry(Ioss::Hex27::name,       {false,false,false,true })}
    };
  }
    // clang-format on
  };
} // namespace Iotm
