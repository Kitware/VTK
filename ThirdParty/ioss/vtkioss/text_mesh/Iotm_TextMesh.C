// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Iotm_TextMesh.h"

#include <Ioss_Utils.h>
#include "vtk_fmt.h"
#include VTK_FMT(fmt/ostream.h)
#include <tokenize.h> // for tokenize

#include <algorithm>
#include <array>
#include <cassert> // for assert
#include <cmath>   // for atan2, cos, sin
#include <cstdlib> // for nullptr, exit, etc
#include <iostream>
#include <numeric>
#include <string>
#include <vector> // for vector

#include <Ioss_Utils.h>

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
      auto groups = Ioss::tokenize(parameters, "|+");
      parse_options(groups);

      TextMeshParser parser(m_dimension);
      parser.set_error_handler(m_errorHandler);
      m_data = parser.parse(groups[0]);

      m_coordinates.set_error_handler(m_errorHandler);
      m_coordinates.set_coordinate_data(m_data, m_rawCoordinates);
      m_rawCoordinates.clear();
    };

    initialize();
  }

  TextMesh::TextMesh()
  {
    m_errorHandler = [](const std::ostringstream &errmsg) { error_handler(errmsg); };
    initialize();
  }

  unsigned TextMesh::spatial_dimension() const { return m_dimension; }

  void TextMesh::initialize()
  {
    build_part_to_topology_map();
    build_block_partition_map();
    build_element_connectivity_map();

    m_variableCount[Ioss::COMMSET]      = 0;
    m_variableCount[Ioss::ELEMENTBLOCK] = 0;
    m_variableCount[Ioss::INVALID_TYPE] = 0;
    m_variableCount[Ioss::NODEBLOCK]    = 0;
    m_variableCount[Ioss::REGION]       = 0;
  }

  void TextMesh::parse_coordinates_option(const std::vector<std::string> &option)
  {
    ThrowRequireMsg(!m_coordinatesParsed, "Coordinates have already been parsed! Check syntax.");

    if (option.size() > 1) {
      const auto &tokens = Ioss::tokenize(option[1], ",");
      m_rawCoordinates.reserve(tokens.size());
      for (const auto &token : tokens) {
        double coord = std::stod(token);
        m_rawCoordinates.push_back(coord);
      }

      m_coordinatesParsed = true;
    }
  }

  void TextMesh::parse_dimension_option(const std::vector<std::string> &option)
  {
    if (option.size() > 1) {
      m_dimension = std::stoull(option[1]);
      ThrowRequireMsg(m_dimension == 2 || m_dimension == 3,
                      "Error!  Spatial dimension not defined to be 2 or 3!");
    }
  }

  void TextMesh::parse_options(const std::vector<std::string> &groups)
  {
    for (size_t i = 1; i < groups.size(); i++) {
      auto        option     = Ioss::tokenize(groups[i], ":");
      std::string optionType = option[0];

      if (optionType == "coordinates") {
        parse_coordinates_option(option);
      }
      else if (optionType == "dimension") {
        parse_dimension_option(option);
      }
      else if (optionType == "help") {
        fmt::print(
            Ioss::OUTPUT(),
            "\nValid Options for TextMesh parameter string:\n"
            "\tPROC_ID,ELEM_ID,TOPOLOGY,{NODE CONNECTIVITY LIST}[,PART_NAME[,PART_ID]] (specifies "
            "element list .. first "
            "argument)\n"
            "\tcoordinates:x1,y1[,z1], x2,y2[,z2], ...., xn,yn[,zn] (specifies coordinate data)\n"
            "\tdimension:spatialDimension (specifies spatial dimension .. default is 3)\n"
            "\thelp -- show this list\n\n");
      }
      else {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: Unrecognized option '{}'.  It will be ignored.\n", optionType);
        IOSS_ERROR(errmsg);
      }
    }
  }

  int64_t TextMesh::node_count() const { return m_data.nodeIds.size(); }

  int64_t TextMesh::node_count_proc() const { return m_data.num_nodes_on_proc(m_myProcessor); }

  int64_t TextMesh::block_count() const { return m_data.partIds.size(); }

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
    coord.resize(count * m_dimension);
    coordinates(&coord[0]);
  }

  void TextMesh::coordinates(double *coord) const
  {
    if (!m_coordinatesParsed)
      return;

    /* create global coordinates */
    const auto &nodes  = m_data.nodes_on_proc(m_myProcessor);
    unsigned    offset = 0;

    for (auto node : nodes) {
      const std::vector<double> &coords = m_coordinates[node];
      for (unsigned i = 0; i < coords.size(); i++) {
        coord[offset++] = coords[i];
      }
    }
  }

  void TextMesh::coordinates(std::vector<double> &x, std::vector<double> &y,
                             std::vector<double> &z) const
  {
    if (!m_coordinatesParsed)
      return;

    /* create global coordinates */
    int64_t count = node_count_proc();
    x.reserve(count);
    y.reserve(count);
    z.reserve(count);

    const auto &nodes = m_data.nodes_on_proc(m_myProcessor);

    for (auto node : nodes) {
      const std::vector<double> &coords = m_coordinates[node];

      x.push_back(coords[0]);
      y.push_back(coords[1]);
      z.push_back((m_dimension == 3) ? coords[2] : 0.0);
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
        const std::vector<double> &coords = m_coordinates[node];
        xyz[offset++]                     = coords[0];
      }
    }
    else if (component == 2) {
      for (auto node : nodes) {
        const std::vector<double> &coords = m_coordinates[node];
        xyz[offset++]                     = coords[1];
      }
    }
    else if (component == 3) {
      for (auto node : nodes) {
        const std::vector<double> &coords = m_coordinates[node];
        xyz[offset++]                     = (m_dimension == 3) ? coords[2] : 0.0;
      }
    }
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
    else {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: (Iotm::TextMesh::set_variable_count)\n"
                 "       Unrecognized variable type '{}'. Valid types are:\n"
                 "       global, element, node, nodal, nodeset, surface, sideset.\n",
                 type);
      IOSS_ERROR(errmsg);
    }
  }

  std::vector<std::string> TextMesh::get_part_names() const
  {
    return m_data.partIds.get_part_names_sorted_by_id();
  }

  int64_t TextMesh::get_part_id(const std::string &name) const { return m_data.partIds.get(name); }

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

} // namespace Iotm
