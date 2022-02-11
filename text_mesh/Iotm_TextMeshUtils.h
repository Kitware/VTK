#pragma once

// #######################  Start Clang Header Tool Managed Headers ########################
// clang-format off
#include <stddef.h>                                  // for size_t
#include <algorithm>                                 // for remove, etc
#include <cctype>                                    // for toupper, isdigit
#include <iterator>                                  // for insert_iterator
#include <map>
#include <set>                                       // for set
#include <sstream>                                   // for operator<<, etc
#include <string>                                    // for basic_string, etc
#include <utility>                                   // for pair
#include <vector>                                    // for vector
#include <unordered_map>
#include <sstream>                       // for ostringstream
#include <functional>
#include <stdexcept>

// clang-format on
// #######################   End Clang Header Tool Managed Headers  ########################
namespace Iotm {
  namespace text_mesh {
    using ErrorHandler = std::function<void(const std::ostringstream &)>;

    template <class EXCEPTION> void handle_error(const std::ostringstream &message)
    {
      throw EXCEPTION((message).str());
    }

    inline void default_error_handler(const std::ostringstream &message)
    {
      handle_error<std::logic_error>(message);
    }

    template <typename T> class TopologyMapping
    {
    public:
      using Topology = T;

      virtual ~TopologyMapping() {}

      Topology topology(const std::string &name) const
      {
        auto it = m_nameToTopology.find(name);
        return (it != m_nameToTopology.end() ? it->second : invalid_topology());
      }

      virtual Topology invalid_topology() const  = 0;
      virtual void     initialize_topology_map() = 0;

    protected:
      std::unordered_map<std::string, Topology> m_nameToTopology;
    };

    class PartIdMapping
    {
    public:
      PartIdMapping() : m_idsAssigned(false)
      {
        set_error_handler([](const std::ostringstream &errmsg) { default_error_handler(errmsg); });
      }

      void register_part_name(const std::string &name)
      {
        m_partNames.push_back(name);
        handle_block_part(name);
      }

      void register_part_name_with_id(const std::string &name, unsigned id)
      {
        register_part_name(name);
        assign(name, id);
      }

      unsigned get(const std::string &name) const
      {
        if (!m_idsAssigned)
          assign_ids();
        return get_part_id(name);
      }

      std::string get(unsigned id) const
      {
        if (!m_idsAssigned)
          assign_ids();
        return get_part_name(id);
      }

      unsigned size() const
      {
        if (!m_idsAssigned)
          assign_ids();
        return m_ids.size();
      }

      std::vector<std::string> get_part_names_sorted_by_id() const
      {
        if (!m_idsAssigned)
          assign_ids();

        std::vector<std::string> names;
        names.reserve(m_parts.size());

        for (auto iter : m_parts) {
          names.push_back(iter.second);
        }

        return names;
      }

      void set_error_handler(ErrorHandler errorHandler) { m_errorHandler = errorHandler; }

    private:
      void handle_block_part(const std::string &name)
      {
        const std::string blockPrefix  = "BLOCK_";
        const unsigned    prefixLength = blockPrefix.length();

        if (name.length() < prefixLength + 1)
          return;

        const std::string namePrefix = name.substr(0, prefixLength);
        const std::string nameSuffix = name.substr(prefixLength);

        if (namePrefix != blockPrefix)
          return;

        unsigned           id;
        std::istringstream nameSuffixStream(nameSuffix);
        nameSuffixStream >> id;
        if (nameSuffixStream.fail()) {
          return;
        }
        assign(name, id);
      }

      void assign_ids() const
      {
        unsigned nextPartId = 1;
        for (const std::string &name : m_partNames) {
          if (m_ids.find(name) == m_ids.end()) {
            while (is_assigned(nextPartId))
              nextPartId++;

            assign(name, nextPartId);
          }
        }

        m_idsAssigned = true;
      }

      void assign(const std::string &name, unsigned id) const
      {
        validate_name_and_id(name, id);
        m_ids[name] = id;
        m_parts[id] = name;
      }

      void validate_name_and_id(const std::string &name, unsigned id) const
      {
        if (is_registered(name)) {
          if (m_ids[name] != id) {
            std::ostringstream errmsg;
            errmsg << "Cannot assign part '" << name << "' two different ids: " << m_ids[name]
                   << " and " << id;
            m_errorHandler(errmsg);
          }
        }
        else {
          if (is_assigned(id)) {
            std::ostringstream errmsg;
            errmsg << "Part id " << id << " has already been assigned, cannot assign it to part '"
                   << name << "'";
            m_errorHandler(errmsg);
          }
        }
      }

      bool is_registered(const std::string &name) const { return m_ids.count(name) > 0; }

      bool is_assigned(unsigned id) const { return m_parts.count(id) > 0; }

      unsigned get_part_id(const std::string &name) const
      {
        auto it = m_ids.find(name);
        if (it == m_ids.end()) {
          std::ostringstream errmsg;
          errmsg << "PartIdMapping has no ID for invalid part name " << name;
          m_errorHandler(errmsg);
        }
        return it->second;
      }

      std::string get_part_name(unsigned id) const
      {
        auto it = m_parts.find(id);
        if (it == m_parts.end()) {
          std::ostringstream errmsg;
          errmsg << "PartIdMapping has no part name for invalid id " << id;
          m_errorHandler(errmsg);
        }
        return it->second;
      }

      std::vector<std::string>                          m_partNames;
      mutable std::unordered_map<std::string, unsigned> m_ids;
      mutable std::map<unsigned, std::string>           m_parts;
      mutable bool                                      m_idsAssigned;

      ErrorHandler m_errorHandler;
    };

    template <typename EntityId, typename Topology> struct ElementData
    {
      int                   proc;
      EntityId              identifier;
      Topology              topology;
      std::vector<EntityId> nodeIds;
      std::string           partName = "";
    };

    template <typename EntityId, typename Topology> struct TextMeshData
    {
      unsigned                                     spatialDim;
      std::vector<ElementData<EntityId, Topology>> elementDataVec;
      PartIdMapping                                partIds;
      std::set<EntityId>                           nodeIds;

      void add_element(const ElementData<EntityId, Topology> &elem)
      {
        elementDataVec.push_back(elem);
        for (const EntityId &nodeId : elem.nodeIds) {
          nodeIds.insert(nodeId);
          associate_node_with_proc(nodeId, elem.proc);
        }
      }

      const std::set<EntityId> &nodes_on_proc(int proc) const
      {
        auto it = m_nodesOnProc.find(proc);
        return it != m_nodesOnProc.end() ? it->second : m_emptyNodes;
      }

      unsigned num_nodes_on_proc(int proc) const
      {
        auto it = m_nodesOnProc.find(proc);
        return it != m_nodesOnProc.end() ? it->second.size() : 0;
      }

      const std::set<int> &procs_for_node(const EntityId nodeId) const
      {
        auto it = m_procsForNode.find(nodeId);
        return it != m_procsForNode.end() ? it->second : m_emptyProcs;
      }

    private:
      void associate_node_with_proc(const EntityId nodeId, int proc)
      {
        m_procsForNode[nodeId].insert(proc);
        m_nodesOnProc[proc].insert(nodeId);
      }

      std::unordered_map<EntityId, std::set<int>> m_procsForNode;
      std::unordered_map<int, std::set<EntityId>> m_nodesOnProc;

      std::set<int>      m_emptyProcs;
      std::set<EntityId> m_emptyNodes;
    };

    class TextMeshLexer
    {
    public:
      TextMeshLexer() : m_currentIndex(0), m_token(""), m_isNumber(false) {}

      void set_input_string(const std::string &input)
      {
        m_input        = input;
        m_currentIndex = 0;
        read_next_token();
      }

      int get_int()
      {
        read_next_token();
        return std::stoi(m_oldToken);
      }

      unsigned get_unsigned()
      {
        read_next_token();
        return std::stoul(m_oldToken);
      }

      std::string get_string()
      {
        read_next_token();
        return make_upper_case(m_oldToken);
      }

      void get_newline() { read_next_token(); }

      bool has_token() const { return m_token != ""; }
      bool has_newline() const { return m_token == "\n"; }
      bool has_number() const { return has_token() && m_isNumber; }
      bool has_string() const { return has_token() && !has_number() && !has_newline(); }

    private:
      void read_next_token()
      {
        m_oldToken = m_token;

        if (char_is_newline()) {
          m_isNumber = false;
          m_token    = "\n";
          m_currentIndex++;
          return;
        }

        m_token    = "";
        m_isNumber = true;

        while (has_more_input()) {
          if (char_is_whitespace()) {
            m_currentIndex++;
            continue;
          }

          if (char_is_comma()) {
            m_currentIndex++;
            break;
          }

          if (char_is_newline()) {
            break;
          }

          m_isNumber &= char_is_digit();
          m_token += current_char();
          m_currentIndex++;
        }
      }

      bool has_more_input() { return m_currentIndex < m_input.size(); }

      bool char_is_whitespace() { return current_char() == ' '; }
      bool char_is_comma() { return current_char() == ','; }
      bool char_is_newline() { return current_char() == '\n'; }
      bool char_is_digit() { return std::isdigit(static_cast<unsigned char>(current_char())); }

      char current_char() { return m_input[m_currentIndex]; }

      std::string make_upper_case(std::string str)
      {
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        return str;
      }

      std::string m_input;
      unsigned    m_currentIndex;

      std::string m_oldToken;
      std::string m_token;

      bool m_isNumber;
    };

    template <typename EntityId, typename TopologyMapping> class TextMeshParser
    {
    private:
      using Topology = typename TopologyMapping::Topology;

    public:
      explicit TextMeshParser(unsigned dim)
      {
        set_error_handler([](const std::ostringstream &errmsg) { default_error_handler(errmsg); });
        m_data.spatialDim = dim;
        m_topologyMapping.initialize_topology_map();
      }

      TextMeshData<EntityId, Topology> parse(const std::string &meshDescription)
      {
        initialize_parse(meshDescription);
        parse_description();
        return m_data;
      }

      void set_error_handler(ErrorHandler errorHandler)
      {
        m_errorHandler = errorHandler;
        m_data.partIds.set_error_handler(errorHandler);
      }

    private:
      void initialize_parse(const std::string &meshDescription)
      {
        m_lexer.set_input_string(meshDescription);
        m_lineNumber = 1;
        validate_required_field(m_lexer.has_token());
      }

      void parse_description()
      {
        while (m_lexer.has_token()) {
          ElementData<EntityId, Topology> elemData = parse_element();
          m_data.add_element(elemData);

          validate_no_extra_fields();
          parse_newline();
        }
      }

      ElementData<EntityId, Topology> parse_element()
      {
        ElementData<EntityId, Topology> elem;
        elem.proc       = parse_proc_id();
        elem.identifier = parse_elem_id();
        elem.topology   = parse_topology();
        elem.nodeIds    = parse_node_ids(elem.topology);
        elem.partName   = parse_part(elem.topology);
        return elem;
      }

      int parse_proc_id()
      {
        validate_required_field(m_lexer.has_number());
        return parse_int();
      }

      EntityId parse_elem_id()
      {
        validate_required_field(m_lexer.has_number());
        return parse_unsigned();
      }

      Topology parse_topology()
      {
        validate_required_field(m_lexer.has_string());
        std::string topologyName = parse_string();

        Topology topology = m_topologyMapping.topology(topologyName);
        validate_topology(topology, topologyName);

        return topology;
      }

      std::vector<EntityId> parse_node_ids(const Topology &topology)
      {
        std::vector<EntityId> nodeIds;
        while (m_lexer.has_number()) {
          nodeIds.push_back(parse_unsigned());
        }
        validate_node_count(topology, nodeIds.size());
        return nodeIds;
      }

      std::string parse_part(const Topology &topology)
      {
        std::string partName;

        if (m_lexer.has_string()) {
          partName = parse_string();
        }
        else {
          partName = "block_" + topology.name();
        }

        if (m_lexer.has_number()) {
          unsigned partId = parse_unsigned();
          m_data.partIds.register_part_name_with_id(partName, partId);
        }
        else {
          m_data.partIds.register_part_name(partName);
        }

        return partName;
      }

      int         parse_int() { return m_lexer.get_int(); }
      unsigned    parse_unsigned() { return m_lexer.get_unsigned(); }
      std::string parse_string() { return m_lexer.get_string(); }

      void parse_newline()
      {
        m_lexer.get_newline();
        m_lineNumber++;
      }

      void validate_required_field(bool hasNextRequiredField)
      {
        if (!hasNextRequiredField) {
          std::ostringstream errmsg;
          errmsg
              << "Error!  Each line must contain the following fields (with at least one node):  "
                 "Processor, GlobalId, Element Topology, NodeIds.  Error on line "
              << m_lineNumber << ".";
          m_errorHandler(errmsg);
        }
      }

      void validate_no_extra_fields()
      {
        bool requiredCondition = !m_lexer.has_token() || m_lexer.has_newline();
        if (!requiredCondition) {
          std::ostringstream errmsg;
          errmsg << "Error!  Each line should not contain more than the following fields (with at "
                    "least one node):  "
                    "Processor, GlobalId, Element Topology, NodeIds, Part Name, PartId.  "
                    "Error on line "
                 << m_lineNumber << ".";
          m_errorHandler(errmsg);
        }
      }

      void validate_topology(const Topology &topology, const std::string &providedName)
      {
        if (topology == m_topologyMapping.invalid_topology()) {
          std::ostringstream errmsg;
          errmsg << "Error!  Topology = >>" << providedName << "<< is invalid from line "
                 << m_lineNumber << ".";
          m_errorHandler(errmsg);
        }

        if (!topology.defined_on_spatial_dimension(m_data.spatialDim)) {
          std::ostringstream errmsg;
          errmsg << "Error on input line " << m_lineNumber << ".  Topology = " << topology
                 << " is not defined on spatial dimension = " << m_data.spatialDim
                 << " set in parser.";
          m_errorHandler(errmsg);
        }
      }

      void validate_node_count(const Topology &topology, size_t numNodes)
      {
        size_t numTopologyNodes = topology.num_nodes();
        if (numNodes != numTopologyNodes) {
          std::ostringstream errmsg;
          errmsg << "Error!  The input line appears to contain " << numNodes
                 << " nodes, but the topology " << topology << " needs " << numTopologyNodes
                 << " nodes on line " << m_lineNumber << ".";
          m_errorHandler(errmsg);
        }
      }

      unsigned                         m_lineNumber{0};
      TextMeshData<EntityId, Topology> m_data;
      TextMeshLexer                    m_lexer;
      TopologyMapping                  m_topologyMapping;

      ErrorHandler m_errorHandler;
    };

    template <typename EntityId, typename Topology> class Coordinates
    {
    private:
      using Data = TextMeshData<EntityId, Topology>;

    public:
      Coordinates()
      {
        set_error_handler([](const std::ostringstream &errmsg) { default_error_handler(errmsg); });
      }

      const std::vector<double> &operator[](const EntityId nodeId) const
      {
        auto it(m_nodalCoords.find(nodeId));
        return it->second;
      }

      void set_coordinate_data(const Data &data, const std::vector<double> &coordinates)
      {
        if (!coordinates.empty()) {
          validate_num_coordinates(data, coordinates);
          fill_coordinate_map(data, coordinates);
        }
      }

      void set_error_handler(ErrorHandler errorHandler) { m_errorHandler = errorHandler; }

    private:
      void validate_num_coordinates(const Data &data, const std::vector<double> &coordinates)
      {
        if (coordinates.size() != data.nodeIds.size() * data.spatialDim) {
          std::ostringstream errmsg;
          errmsg << "Number of coordinates: " << coordinates.size()
                 << ", Number of nodes: " << data.nodeIds.size()
                 << ", Spatial dimension: " << data.spatialDim;
          m_errorHandler(errmsg);
        }
      }

      void fill_coordinate_map(const Data &data, const std::vector<double> &coordinates)
      {
        std::vector<double>::const_iterator coordIter = coordinates.begin();
        for (const EntityId &nodeId : data.nodeIds) {
          m_nodalCoords[nodeId] = std::vector<double>(coordIter, coordIter + data.spatialDim);
          coordIter += data.spatialDim;
        }
      }

      std::unordered_map<EntityId, std::vector<double>> m_nodalCoords;
      ErrorHandler                                      m_errorHandler;
    };

  } // namespace text_mesh
} // namespace Iotm
