// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Iotm_TextMesh.h"

#include <Ioss_Utils.h>

#include "vtk_ioss_fmt.h"
#include VTK_FMT(fmt/ostream.h)

#include <algorithm>
#include <array>
#include <cassert> // for assert
#include <cmath>   // for atan2, cos, sin
#include <cstdlib> // for nullptr, exit, etc
#include <iostream>
#include <numeric>
#include <string>
#include <vector> // for vector

#define ThrowRequireMsg(expr, message)                                                             \
  do {                                                                                             \
    if (!(expr)) {                                                                                 \
      std::ostringstream internal_throw_require_oss;                                               \
      internal_throw_require_oss << message;                                                       \
      throw std::logic_error(internal_throw_require_oss.str());                                    \
    }                                                                                              \
  } while (false)

namespace Iotm {
  void error_handler(const std::ostringstream &message) { throw std::logic_error((message).str()); }

  TextMesh::TextMesh(int proc_count, int my_proc)
      : m_processorCount(proc_count), m_myProcessor(my_proc)
  {
    m_errorHandler = [](const std::ostringstream &errmsg) { error_handler(errmsg); };
    initialize();
  }

  TextMesh::TextMesh(const std::string &parameters, int proc_count, int my_proc)
      : m_processorCount(proc_count), m_myProcessor(my_proc)
  {
    m_errorHandler = [](const std::ostringstream &errmsg) { error_handler(errmsg); };

    if (!parameters.empty()) {
      TextMeshParser parser;
      parser.set_error_handler(m_errorHandler);
      m_data = parser.parse(parameters);
    };

    initialize();
  }

  TextMesh::TextMesh()
  {
    m_errorHandler = [](const std::ostringstream &errmsg) { error_handler(errmsg); };
    initialize();
  }

  unsigned TextMesh::spatial_dimension() const { return m_data.spatialDim; }

  void TextMesh::initialize()
  {
    build_part_to_topology_map();
    build_block_partition_map();
    build_element_connectivity_map();

    m_variableCount[Ioss::NODESET]      = 0;
    m_variableCount[Ioss::SIDESET]      = 0;
    m_variableCount[Ioss::COMMSET]      = 0;
    m_variableCount[Ioss::ELEMENTBLOCK] = 0;
    m_variableCount[Ioss::INVALID_TYPE] = 0;
    m_variableCount[Ioss::NODEBLOCK]    = 0;
    m_variableCount[Ioss::REGION]       = 0;
    m_variableCount[Ioss::ASSEMBLY]     = 0;
  }

  int64_t TextMesh::node_count() const { return m_data.nodeIds.size(); }

  int64_t TextMesh::node_count_proc() const { return m_data.num_nodes_on_proc(m_myProcessor); }

  int64_t TextMesh::block_count() const { return m_data.partIds.size(); }

  int64_t TextMesh::nodeset_count() const { return m_data.nodesets.get_group_data().size(); }

  int64_t TextMesh::nodeset_node_count(int64_t id) const
  {
    int64_t count = 0;

    const NodesetData *nodeset = m_data.nodesets.get_group_data(id);
    if (nullptr != nodeset) {
      count = nodeset->data.size();
    }
    return count;
  }

  int64_t TextMesh::nodeset_node_count_proc(int64_t id) const
  {
    int64_t count = 0;

    const NodesetData       *nodeset = m_data.nodesets.get_group_data(id);
    const std::set<int64_t> &myNodes = m_data.nodes_on_proc(m_myProcessor);

    if (nullptr != nodeset) {
      for (int64_t nodeId : nodeset->data) {
        if (myNodes.count(nodeId) > 0) {
          count++;
        }
      }
    }
    return count;
  }

  int64_t TextMesh::sideset_count() const { return m_data.sidesets.get_group_data().size(); }

  int64_t TextMesh::sideset_side_count(int64_t id) const
  {
    int64_t count = 0;

    const SidesetData *sideset = m_data.sidesets.get_group_data(id);
    if (nullptr != sideset) {
      count = sideset->data.size();
    }
    return count;
  }

  int64_t TextMesh::sideset_side_count_proc(int64_t id) const
  {
    int64_t count = 0;

    const SidesetData *sideset = m_data.sidesets.get_group_data(id);
    int                myProc  = m_myProcessor;
    if (nullptr != sideset) {
      for (const std::pair<int64_t, int> &elemSidePair : sideset->data) {
        int64_t elemId = elemSidePair.first;
        auto iter = std::find(m_data.elementDataVec.begin(), m_data.elementDataVec.end(), elemId);
        if (iter != m_data.elementDataVec.end() && iter->proc == myProc) {
          count++;
        }
      }
    }
    return count;
  }

  int64_t TextMesh::element_count() const { return m_data.elementDataVec.size(); }

  int64_t TextMesh::element_count_proc() const
  {
    int64_t count = 0;

    for (auto iter = m_blockPartition.begin(); iter != m_blockPartition.end(); iter++) {
      count += iter->second.elemIds.size();
    }

    return count;
  }

  int64_t TextMesh::element_count(int64_t id) const
  {
    int64_t count = 0;
    for (const auto &elementData : m_data.elementDataVec) {
      if (get_part_id(elementData.partName) == id) {
        count++;
      }
    }

    return count;
  }

  int64_t TextMesh::element_count_proc(int64_t id) const
  {
    int64_t count  = 0;
    int     myProc = m_myProcessor;
    for (const auto &elementData : m_data.elementDataVec) {
      if (get_part_id(elementData.partName) == id && elementData.proc == myProc) {
        count++;
      }
    }

    return count;
  }

  Topology TextMesh::get_topology_for_part(int64_t id) const
  {
    unsigned    partId   = id;
    std::string partName = m_data.partIds.get(partId);

    auto iter = m_partToTopology.find(partName);
    ThrowRequireMsg(iter != m_partToTopology.end(),
                    "Could not find a topology associated with part: " << partName);

    Topology topo = iter->second;
    return topo;
  }

  std::pair<std::string, int> TextMesh::topology_type(int64_t id) const
  {
    Topology topo = get_topology_for_part(id);
    return std::make_pair(topo.name(), topo.num_nodes());
  }

  template <typename INT> void TextMesh::raw_node_map(std::vector<INT> &map) const
  {
    map.resize(node_count_proc());
    INT offset = 0;

    const auto &nodeIds = m_data.nodes_on_proc(m_myProcessor);
    for (auto id : nodeIds) {
      map[offset++] = id;
    }
  }

  void TextMesh::node_map(Ioss::Int64Vector &map) const { raw_node_map(map); }

  void TextMesh::node_map(Ioss::IntVector &map) const { raw_node_map(map); }

  int64_t TextMesh::communication_node_count_proc() const
  {
    int64_t     count   = 0;
    const auto &nodeIds = m_data.nodes_on_proc(m_myProcessor);

    for (auto id : nodeIds) {
      size_t numProcsForNode = m_data.procs_for_node(id).size();
      ThrowRequireMsg(numProcsForNode > 0, "Invalid node sharing for id: " << id);
      count += numProcsForNode - 1;
      ;
    }
    return count;
  }

  void TextMesh::owning_processor(int *owner, int64_t num_node)
  {
    const auto &nodeIds = m_data.nodes_on_proc(m_myProcessor);
    auto        iter    = nodeIds.begin();

    ThrowRequireMsg(num_node == (int64_t)nodeIds.size(),
                    "Unmatched data sizes in TextMesh::owning_processor()");

    for (int64_t i = 0; i < num_node; i++) {
      const auto &procs = m_data.procs_for_node(*iter);
      owner[i]          = *procs.begin();
      iter++;
    }
  }

  class NodeCommunicationMap
  {
  public:
    NodeCommunicationMap(int myProc, Ioss::Int64Vector &map, std::vector<int> &processors)
        : m_myProcessor(myProc), m_nodeMap(map), m_processorMap(processors)
    {
    }

    void fill_map_from_data(const TextMeshData &data)
    {
      m_fillIndex         = 0;
      const auto &nodeIds = data.nodes_on_proc(m_myProcessor);

      for (const auto &id : nodeIds) {
        fill_map_for_node(id, data);
      }
    }

    void verify_map_size(const size_t minimumSize)
    {
      ThrowRequireMsg(m_nodeMap.size() >= minimumSize, "Insufficient size in entity vector");
      ThrowRequireMsg(m_processorMap.size() >= minimumSize,
                      "Insufficient size in processor vector");
    }

  private:
    NodeCommunicationMap();
    NodeCommunicationMap(const NodeCommunicationMap &);

    void add_comm_map_pair(int64_t id, int proc)
    {
      m_nodeMap[m_fillIndex]      = id;
      m_processorMap[m_fillIndex] = proc;

      m_fillIndex++;
    }

    void fill_map_for_node(int64_t id, const TextMeshData &data)
    {
      const std::set<int> &procs = data.procs_for_node(id);

      for (int proc : procs) {
        if (proc != m_myProcessor) {
          add_comm_map_pair(id, proc);
        }
      }
    }

    int                m_myProcessor;
    Ioss::Int64Vector &m_nodeMap;
    std::vector<int>  &m_processorMap;

    size_t m_fillIndex = 0;
  };

  void TextMesh::node_communication_map(Ioss::Int64Vector &map, std::vector<int> &processors)
  {
    NodeCommunicationMap commMap(m_myProcessor, map, processors);
    commMap.verify_map_size(communication_node_count_proc());
    commMap.fill_map_from_data(m_data);
  }

  void TextMesh::element_map(int64_t block_number, Ioss::Int64Vector &map) const
  {
    raw_element_map(block_number, map);
  }

  void TextMesh::element_map(int64_t block_number, Ioss::IntVector &map) const
  {
    raw_element_map(block_number, map);
  }

  template <typename INT> void TextMesh::raw_element_map(int64_t id, std::vector<INT> &map) const
  {
    auto iter = m_blockPartition.find(id);
    ThrowRequireMsg(iter != m_blockPartition.end(),
                    "Could not find block with id: " << id << " in block partition");

    map.reserve(iter->second.elemIds.size());

    for (const auto &elemId : iter->second.elemIds) {
      map.push_back(elemId);
    }
  }

  void TextMesh::element_map(Ioss::Int64Vector &map) const { raw_element_map(map); }

  void TextMesh::element_map(Ioss::IntVector &map) const { raw_element_map(map); }

  template <typename INT> void TextMesh::raw_element_map(std::vector<INT> &map) const
  {
    INT count = element_count_proc();
    map.resize(count);

    for (auto iter = m_blockPartition.begin(); iter != m_blockPartition.end(); iter++) {
      size_t offset     = iter->second.offset;
      size_t blockCount = 0;
      for (const auto &elemId : iter->second.elemIds) {
        map[offset + blockCount++] = elemId;
      }
    }
  }

  void TextMesh::coordinates(std::vector<double> &coord) const
  {
    /* create global coordinates */
    int64_t count = node_count_proc();
    coord.resize(count * spatial_dimension());
    coordinates(&coord[0]);
  }

  void TextMesh::coordinates(double *coord) const
  {
    if (!m_data.coords.has_coordinate_data())
      return;

    /* create global coordinates */
    const auto &nodes  = m_data.nodes_on_proc(m_myProcessor);
    unsigned    offset = 0;

    for (auto node : nodes) {
      const std::vector<double> &coords = m_data.coords[node];
      for (unsigned i = 0; i < coords.size(); i++) {
        coord[offset++] = coords[i];
      }
    }
  }

  void TextMesh::coordinates(std::vector<double> &x, std::vector<double> &y,
                             std::vector<double> &z) const
  {
    if (!m_data.coords.has_coordinate_data())
      return;

    /* create global coordinates */
    int64_t count = node_count_proc();
    x.reserve(count);
    y.reserve(count);
    z.reserve(count);

    const auto &nodes = m_data.nodes_on_proc(m_myProcessor);

    for (auto node : nodes) {
      const std::vector<double> &coords = m_data.coords[node];

      x.push_back(coords[0]);
      y.push_back(coords[1]);
      z.push_back((spatial_dimension() == 3) ? coords[2] : 0.0);
    }
  }

  void TextMesh::coordinates(int component, std::vector<double> &xyz) const
  {
    /* create global coordinates */
    size_t count = node_count_proc();
    xyz.resize(count);
    coordinates(component, xyz.data());
  }

  void TextMesh::coordinates(int component, double *xyz) const
  {
    const auto &nodes  = m_data.nodes_on_proc(m_myProcessor);
    unsigned    offset = 0;

    if (component == 1) {
      for (auto node : nodes) {
        const std::vector<double> &coords = m_data.coords[node];
        xyz[offset++]                     = coords[0];
      }
    }
    else if (component == 2) {
      for (auto node : nodes) {
        const std::vector<double> &coords = m_data.coords[node];
        xyz[offset++]                     = coords[1];
      }
    }
    else if (component == 3) {
      for (auto node : nodes) {
        const std::vector<double> &coords = m_data.coords[node];
        xyz[offset++]                     = (spatial_dimension() == 3) ? coords[2] : 0.0;
      }
    }
  }

  void TextMesh::nodeset_nodes(int64_t id, Ioss::Int64Vector &nodes) const
  {
    const NodesetData *nodeset = m_data.nodesets.get_group_data(id);
    if (nullptr == nodeset)
      return;

    nodes.resize(nodeset_node_count_proc(id));

    int64_t                  count   = 0;
    const std::set<int64_t> &myNodes = m_data.nodes_on_proc(m_myProcessor);

    for (int64_t nodeId : nodeset->data) {
      if (myNodes.count(nodeId) > 0) {
        nodes[count++] = nodeId;
      }
    }
  }

  void TextMesh::sideset_elem_sides(int64_t id, Ioss::Int64Vector &elemSides) const
  {
    const SidesetData *sideset = m_data.sidesets.get_group_data(id);
    if (nullptr == sideset)
      return;

    elemSides.resize(2 * sideset_side_count_proc(id));

    int64_t count  = 0;
    int     myProc = m_myProcessor;

    for (const std::pair<int64_t, int> &elemSidePair : sideset->data) {
      int64_t elemId = elemSidePair.first;
      int     side   = elemSidePair.second;
      auto    iter = std::find(m_data.elementDataVec.begin(), m_data.elementDataVec.end(), elemId);
      if (iter != m_data.elementDataVec.end() && iter->proc == myProc) {
        elemSides[count++] = elemId;
        elemSides[count++] = side;
      }
    }
  }

  std::set<std::string> TextMesh::get_blocks_touched_by_sideset(const SidesetData *sideset) const
  {
    std::set<std::string> touchedBlocks;

    int myProc = m_myProcessor;

    for (const std::pair<int64_t, int> &elemSidePair : sideset->data) {
      int64_t elemId = elemSidePair.first;
      auto    iter = std::find(m_data.elementDataVec.begin(), m_data.elementDataVec.end(), elemId);
      if (iter != m_data.elementDataVec.end() && iter->proc == myProc) {
        touchedBlocks.insert(iter->partName);
      }
    }

    return touchedBlocks;
  }

  std::vector<std::string> TextMesh::sideset_touching_blocks(int64_t setId) const
  {
    std::vector<std::string> touchedBlocks;

    const SidesetData *sideset = m_data.sidesets.get_group_data(setId);
    if (nullptr != sideset) {
      std::set<std::string> blockList = get_blocks_touched_by_sideset(sideset);
      touchedBlocks.reserve(blockList.size());
      for (const std::string &block : blockList) {
        touchedBlocks.push_back(block);
      }
    }

    return touchedBlocks;
  }

  void TextMesh::connectivity(int64_t id, Ioss::Int64Vector &connect) const
  {
    Topology topo = get_topology_for_part(id);

    int64_t npe = topo.num_nodes();
    connect.resize(element_count_proc(id) * npe);

    raw_connectivity(id, &connect[0]);
  }

  void TextMesh::connectivity(int64_t id, Ioss::IntVector &connect) const
  {
    Topology topo = get_topology_for_part(id);

    int64_t npe = topo.num_nodes();
    connect.resize(element_count_proc(id) * npe);

    raw_connectivity(id, &connect[0]);
  }

  void TextMesh::connectivity(int64_t id, int64_t *connect) const { raw_connectivity(id, connect); }

  void TextMesh::connectivity(int64_t id, int *connect) const { raw_connectivity(id, connect); }

  template <typename INT> void TextMesh::raw_connectivity(int64_t id, INT *connect) const
  {
    unsigned offset    = 0;
    auto     blockIter = m_blockPartition.find(id);
    ThrowRequireMsg(blockIter != m_blockPartition.end(),
                    "Could not find block with id: " << id << " in block partition");

    for (const auto &elemId : blockIter->second.elemIds) {
      auto connIter = m_elementConnectivity.find(elemId);
      ThrowRequireMsg(connIter != m_elementConnectivity.end(),
                      "Could not find element with id: " << id << " in connectivity map");

      for (auto nodeId : connIter->second) {
        connect[offset++] = nodeId;
      }
    }
  }

  void TextMesh::set_variable_count(const std::string &type, size_t count)
  {
    if (type == "global") {
      m_variableCount[Ioss::REGION] = count;
    }
    else if (type == "element") {
      m_variableCount[Ioss::ELEMENTBLOCK] = count;
    }
    else if (type == "nodal" || type == "node") {
      m_variableCount[Ioss::NODEBLOCK] = count;
    }
    else if (type == "nodeset") {
      m_variableCount[Ioss::NODESET] = count;
    }
    else if (type == "surface" || type == "sideset") {
      m_variableCount[Ioss::SIDEBLOCK] = count;
    }
    else if (type == "assembly") {
      m_variableCount[Ioss::ASSEMBLY] = count;
    }
    else {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: (Iotm::TextMesh::set_variable_count)\n"
                 "       Unrecognized variable type '{}'. Valid types are:\n"
                 "       global, element, node, nodal, nodeset, surface, sideset, assembly.\n",
                 type);
      IOSS_ERROR(errmsg);
    }
  }

  std::vector<std::string> TextMesh::get_part_names() const
  {
    return m_data.partIds.get_part_names_sorted_by_id();
  }

  int64_t TextMesh::get_part_id(const std::string &name) const { return m_data.partIds.get(name); }

  std::vector<std::string> TextMesh::get_nodeset_names() const
  {
    return m_data.nodesets.get_part_names();
  }

  std::string TextMesh::get_nodeset_name(int64_t id) const
  {
    const NodesetData *nodeset = m_data.nodesets.get_group_data(id);
    ThrowRequireMsg(nullptr != nodeset, "Could not find nodeset with id" << id);
    return nodeset->name;
  }

  int64_t TextMesh::get_nodeset_id(const std::string &name) const
  {
    const NodesetData *nodeset = m_data.nodesets.get_group_data(name);
    ThrowRequireMsg(nullptr != nodeset, "Could not find nodeset with name" << name);
    return nodeset->id;
  }

  std::vector<std::string> TextMesh::get_sideset_names() const
  {
    return m_data.sidesets.get_part_names();
  }

  std::string TextMesh::get_sideset_name(int64_t id) const
  {
    const SidesetData *sideset = m_data.sidesets.get_group_data(id);
    ThrowRequireMsg(nullptr != sideset, "Could not find sideset with id" << id);
    return sideset->name;
  }

  int64_t TextMesh::get_sideset_id(const std::string &name) const
  {
    const SidesetData *sideset = m_data.sidesets.get_group_data(name);
    ThrowRequireMsg(nullptr != sideset, "Could not find sideset with name" << name);
    return sideset->id;
  }

  std::vector<std::string> TextMesh::get_assembly_names() const
  {
    return m_data.assemblies.get_part_names();
  }

  std::string TextMesh::get_assembly_name(int64_t id) const
  {
    const AssemblyData *assembly = m_data.assemblies.get_group_data(id);
    ThrowRequireMsg(nullptr != assembly, "Could not find assembly with id" << id);
    return assembly->name;
  }

  int64_t TextMesh::get_assembly_id(const std::string &name) const
  {
    const AssemblyData *assembly = m_data.assemblies.get_group_data(name);
    ThrowRequireMsg(nullptr != assembly, "Could not find assembly with name" << name);
    return assembly->id;
  }

  Ioss::EntityType TextMesh::assembly_type_to_entity_type(const AssemblyType type) const
  {
    if (type == AssemblyType::BLOCK) {
      return Ioss::ELEMENTBLOCK;
    }
    else if (type == AssemblyType::NODESET) {
      return Ioss::NODESET;
    }
    else if (type == AssemblyType::SIDESET) {
      return Ioss::SIDESET;
    }
    else if (type == AssemblyType::ASSEMBLY) {
      return Ioss::ASSEMBLY;
    }

    return Ioss::INVALID_TYPE;
  }

  Ioss::EntityType TextMesh::get_assembly_type(const std::string &name) const
  {
    const AssemblyData *assembly = m_data.assemblies.get_group_data(name);
    ThrowRequireMsg(nullptr != assembly, "Could not find assembly with name" << name);

    AssemblyType type = assembly->get_assembly_type();
    return assembly_type_to_entity_type(type);
  }

  std::vector<std::string> TextMesh::get_assembly_members(const std::string &name) const
  {
    const AssemblyData *assembly = m_data.assemblies.get_group_data(name);
    ThrowRequireMsg(nullptr != assembly, "Could not find assembly with name" << name);

    return assembly->data;
  }

  int64_t TextMesh::assembly_count() const { return m_data.assemblies.get_group_data().size(); }

  std::set<int64_t> TextMesh::get_local_element_ids_for_block(int64_t id) const
  {
    size_t            count = element_count_proc(id);
    std::set<int64_t> elemIds;

    int myProc = m_myProcessor;
    for (const auto &elementData : m_data.elementDataVec) {
      if (get_part_id(elementData.partName) == id && elementData.proc == myProc) {
        elemIds.insert(elementData.identifier);
      }
    }

    ThrowRequireMsg(elemIds.size() == count, "Elements in ElementData vector are not unique");
    return elemIds;
  }

  void TextMesh::build_part_to_topology_map()
  {
    for (const auto &elementData : m_data.elementDataVec) {
      auto iter = m_partToTopology.find(elementData.partName);
      if (iter == m_partToTopology.end()) {
        m_partToTopology[elementData.partName] = elementData.topology;
      }
      else {
        ThrowRequireMsg(iter->second == elementData.topology,
                        "Element with id: "
                            << elementData.identifier << " in part named: " << elementData.partName
                            << " is attempting to reset the part topology: " << iter->second
                            << " with: " << elementData.topology.name());
      }
    }
  }

  std::vector<int64_t> TextMesh::get_part_ids(const std::vector<std::string> &partNames)
  {
    std::vector<int64_t> partIds;

    size_t numParts = partNames.size();
    partIds.resize(numParts);

    for (size_t i = 0; i < numParts; ++i) {
      partIds[i] = get_part_id(partNames[i]);
    }

    return partIds;
  }

  std::vector<size_t> TextMesh::get_part_offsets(const std::vector<int64_t> &partIds)
  {
    std::vector<size_t> offsets;

    size_t numParts = partIds.size();
    offsets.resize(numParts);

    for (size_t i = 0; i < numParts; ++i) {
      offsets[i] = 0;
    }

    for (size_t i = 1; i < numParts; ++i) {
      offsets[i] = offsets[i - 1] + element_count_proc(partIds[i - 1]);
    }

    return offsets;
  }

  void TextMesh::build_block_partition_map()
  {
    std::vector<std::string> partNames = get_part_names();
    std::vector<int64_t>     partIds   = get_part_ids(partNames);
    std::vector<size_t>      offsets   = get_part_offsets(partIds);

    size_t numParts = partNames.size();

    for (size_t i = 0; i < numParts; ++i) {
      const std::string &name = partNames[i];
      int64_t            id   = partIds[i];

      BlockPartition partition(offsets[i], name, get_local_element_ids_for_block(id));
      m_blockPartition[id] = partition;
    }
  }

  void TextMesh::build_element_connectivity_map()
  {
    int                  myProc = m_myProcessor;
    std::vector<int64_t> nodeIds;

    for (const auto &elementData : m_data.elementDataVec) {
      if (elementData.proc == myProc) {
        nodeIds.clear();
        for (auto id : elementData.nodeIds) {
          nodeIds.push_back(id);
        }

        m_elementConnectivity[elementData.identifier] = nodeIds;
      }
    }
  }

  int64_t TextMesh::sideblock_side_count(int64_t id, const std::string &sideBlockName) const
  {
    int64_t count = 0;

    const SidesetData *sideset = m_data.sidesets.get_group_data(id);
    if (nullptr != sideset) {
      SideBlockInfo info = sideset->get_side_block_info(sideBlockName);
      count              = info.sideIndex.size();
    }
    return count;
  }

  int64_t TextMesh::sideblock_side_count_proc(int64_t id, const std::string &sideBlockName) const
  {
    int64_t count = 0;

    const SidesetData *sideset = m_data.sidesets.get_group_data(id);
    if (nullptr != sideset) {
      SideBlockInfo info = sideset->get_side_block_info(sideBlockName);
      count              = sideset->get_sideblock_indices_local_to_proc(info, m_myProcessor).size();
    }
    return count;
  }

  void TextMesh::sideblock_elem_sides(int64_t id, const std::string &sideBlockName,
                                      Ioss::Int64Vector &elemSides) const
  {
    const SidesetData *sideset = m_data.sidesets.get_group_data(id);
    if (nullptr == sideset)
      return;

    SideBlockInfo       info = sideset->get_side_block_info(sideBlockName);
    std::vector<size_t> localSideIndex =
        sideset->get_sideblock_indices_local_to_proc(info, m_myProcessor);
    elemSides.resize(2 * localSideIndex.size());

    int64_t count = 0;

    for (size_t sideIndex : localSideIndex) {
      const SidesetData::DataType &elemSidePair = sideset->data[sideIndex];
      int64_t                      elemId       = elemSidePair.first;
      int                          side         = elemSidePair.second;

      elemSides[count++] = elemId;
      elemSides[count++] = side;
    }
  }

  std::vector<SideBlockInfo>
  TextMesh::get_side_block_info_for_sideset(const std::string &name) const
  {
    const SidesetData *sideset = m_data.sidesets.get_group_data(name);
    ThrowRequireMsg(nullptr != sideset, "Could not find sideset with name" << name);
    return sideset->get_side_block_info();
  }

  std::vector<size_t> TextMesh::get_local_side_block_indices(const std::string   &name,
                                                             const SideBlockInfo &info) const
  {
    const SidesetData *sideset = m_data.sidesets.get_group_data(name);
    ThrowRequireMsg(nullptr != sideset, "Could not find sideset with name" << name);
    ThrowRequireMsg(name == info.parentName,
                    "SideBlock: " << info.name << " with parent: " << info.parentName
                                  << " was not created from sideset: " << name);
    return sideset->get_sideblock_indices_local_to_proc(info, m_myProcessor);
  }

  SplitType TextMesh::get_sideset_split_type(const std::string &name) const
  {
    const SidesetData *sideset = m_data.sidesets.get_group_data(name);
    ThrowRequireMsg(nullptr != sideset, "Could not find sideset with name" << name);

    return sideset->get_split_type();
  }
} // namespace Iotm
