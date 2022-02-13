// Copyright(C) 1999-2022 National Technology & Engineering Solutions
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

#include "Iotm_TextMeshTopologyMapping.h"
#include "Iotm_TextMeshUtils.h"

namespace Iotm {
  using Topology       = TopologyMapEntry;
  using TextMeshData   = text_mesh::TextMeshData<int64_t, TopologyMapEntry>;
  using ElementData    = text_mesh::ElementData<int64_t, TopologyMapEntry>;
  using Coordinates    = text_mesh::Coordinates<int64_t, TopologyMapEntry>;
  using TextMeshParser = text_mesh::TextMeshParser<int64_t, IossTopologyMapping>;

  inline std::ostream &operator<<(std::ostream &out, Topology t) { return out << t.name(); }

  struct BlockPartition
  {
    size_t            offset;
    std::string       name;
    std::set<int64_t> elemIds;

    BlockPartition() : offset(0), name("") {}

    BlockPartition(size_t offset_, const std::string &name_, const std::set<int64_t> &elemIds_)
        : offset(offset_), name(name_), elemIds(elemIds_)
    {
    }
  };

  class TextMesh
  {
  public:
    explicit TextMesh(const std::string &parameters, int proc_count = 1, int my_proc = 0);
    TextMesh(int proc_count = 1, int my_proc = 0);
    TextMesh();
    TextMesh(const TextMesh &) = delete;
    TextMesh &operator=(const TextMesh &) = delete;

    virtual ~TextMesh() = default;

    /**
     * Return number of nodes in the entire model.
     */
    virtual int64_t node_count() const;

    /**
     * Return number of nodes on this processor.
     */
    virtual int64_t node_count_proc() const;

    /**
     * Return number of element blocks in the entire model.
     */
    virtual int64_t block_count() const;

    /**
     * Return number of elements in all element blocks in the model.
     */
    virtual int64_t element_count() const;

    /**
     * Return number of elements in all element blocks on this processor.
     */
    virtual int64_t element_count_proc() const;

    int64_t timestep_count() const { return m_timestepCount; }
    /**
     * Return number of elements in the element block with id
     * 'block_number'.
     */
    virtual int64_t element_count(int64_t block_number) const;

    /**
     * Return number of elements on this processor in the element
     * block with id 'block_number'.
     */
    virtual int64_t element_count_proc(int64_t block_number) const;

    /**
     * Returns pair containing "topology type string" and "number of
     * nodes / element". The topology type string will be "hex8" for
     * the hex element block and "shell4" for the shell element blocks.
     */
    virtual std::pair<std::string, int> topology_type(int64_t block_number) const;

    virtual int64_t communication_node_count_proc() const;
    virtual void    node_communication_map(Ioss::Int64Vector &map, std::vector<int> &proc);
    virtual void    owning_processor(int *owner, int64_t num_node);
    /**
     * Fill the passed in 'map' argument with the node map
     * "map[local_position] = global_id" for the nodes on this
     * processor.
     */
    virtual void node_map(Ioss::Int64Vector &map) const;
    virtual void node_map(Ioss::IntVector &map) const;

    /**
     * Fill the passed in 'map' argument with the element map
     * "map[local_position] = global_id" for the elements on this
     * processor in block "block_number".
     */
    virtual void element_map(int64_t block_number, Ioss::Int64Vector &map) const;
    virtual void element_map(int64_t block_number, Ioss::IntVector &map) const;

    /**
     * Fill the passed in 'map' argument with the element map
     * "map[local_position] = global_id" for all elements on this
     * processor
     */
    virtual void element_map(Ioss::Int64Vector &map) const;
    virtual void element_map(Ioss::IntVector &map) const;

    /**
     * Return the connectivity for the elements on this processor in
     * the block with id 'block_number'. If the elements in this block
     * have 'npe' nodes per element, then the first 'npe' entries in
     * the 'conn' vector will be the nodal connectivity for the first
     * element; the next 'npe' entries are the nodal connectivity for
     * the second element.  The 'connect' vector will be resized to the
     * size required to contain the nodal connectivity for the
     * specified block; all information in 'connect' will be overwritten.
     */
    void         connectivity(int64_t block_number, Ioss::Int64Vector &connect) const;
    void         connectivity(int64_t block_number, Ioss::IntVector &connect) const;
    void         connectivity(int64_t block_number, int64_t *connect) const;
    virtual void connectivity(int64_t block_number, int *connect) const;

    /**
     * Return the coordinates for all nodes on this processor.  The
     * first 3 entries in the 'coord' vector are the x, y, and z
     * coordinates of the first node, etc.  The 'coord' vector will be
     * resized to the size required to contain the nodal coordinates;
     * all information in 'coord' will be overwritten.
     */
    virtual void coordinates(std::vector<double> &coord) const;
    virtual void coordinates(double *coord) const;

    /**
     * Return the coordinates for all nodes on this processor in
     * separate vectors. The vectors will be resized to the size
     * required to contain the nodal coordinates; all information in
     * the vectors will be overwritten.
     */
    virtual void coordinates(std::vector<double> &x, std::vector<double> &y,
                             std::vector<double> &z) const;

    /**
     * Return the coordinates for componenet 'comp' (1=x, 2=y, 3=z)
     * for all nodes on this processor. The
     * vector will be resized to the size required to contain the
     * nodal coordinates; all information in the vector will be
     * overwritten.
     * It is an error to request the coordinates via this function
     * if a rotation is defined.
     */
    virtual void coordinates(int component, std::vector<double> &xyz) const;
    virtual void coordinates(int component, double *xyz) const;

    size_t get_variable_count(Ioss::EntityType type) const
    {
      return m_variableCount.find(type) != m_variableCount.end()
                 ? m_variableCount.find(type)->second
                 : 0;
    }

    std::vector<std::string> get_part_names() const;
    int64_t                  get_part_id(const std::string &name) const;

    unsigned spatial_dimension() const;

  private:
    template <typename INT> void raw_element_map(int64_t block_number, std::vector<INT> &map) const;
    template <typename INT> void raw_element_map(std::vector<INT> &map) const;
    template <typename INT> void raw_connectivity(int64_t block_number, INT *connect) const;
    template <typename INT> void raw_node_map(std::vector<INT> &map) const;

    void set_variable_count(const std::string &type, size_t count);
    void parse_options(const std::vector<std::string> &groups);
    void parse_coordinates_option(const std::vector<std::string> &option);
    void parse_dimension_option(const std::vector<std::string> &option);

    void initialize();

    void build_part_to_topology_map();
    void build_block_partition_map();
    void build_element_connectivity_map();

    std::vector<int64_t> get_part_ids(const std::vector<std::string> &partNames);
    std::vector<size_t>  get_part_offsets(const std::vector<int64_t> &partIds);

    Topology get_topology_for_part(int64_t id) const;

    std::set<int64_t> get_local_element_ids_for_block(int64_t id) const;

    unsigned m_dimension{3};
    size_t   m_processorCount{0};
    size_t   m_myProcessor{0};

    size_t                             m_timestepCount{0};
    std::map<Ioss::EntityType, size_t> m_variableCount;

    bool                m_coordinatesParsed{false};
    std::vector<double> m_rawCoordinates;

    TextMeshData m_data{};
    Coordinates  m_coordinates{};

    text_mesh::ErrorHandler m_errorHandler;

    std::unordered_map<std::string, Topology> m_partToTopology;

    std::unordered_map<int64_t, BlockPartition> m_blockPartition;

    std::unordered_map<int64_t, std::vector<int64_t>> m_elementConnectivity;
  };
} // namespace Iotm
