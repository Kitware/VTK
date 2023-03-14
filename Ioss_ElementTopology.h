// Copyright(C) 1999-2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "ioss_export.h"

#include "vtk_ioss_mangle.h"

#include <Ioss_CodeTypes.h>
#include <Ioss_ElementPermutation.h> // for ElementPermutation
#include <map>                       // for map, map<>::value_compare
#include <set>                       // for set
#include <string>                    // for string, operator<
#include <vector>                    // for vector

namespace Ioss {
  class ElementTopology;
  class ElementPermutation;
} // namespace Ioss

namespace Ioss {
  enum class ElementShape : unsigned {
    UNKNOWN,
    POINT,
    SPHERE,
    LINE,
    SPRING,
    TRI,
    QUAD,
    TET,
    PYRAMID,
    WEDGE,
    HEX,
    SUPER
  };

  using ElementShapeMap    = std::map<ElementShape, std::string>;
  using ElementTopologyMap = std::map<std::string, ElementTopology *, std::less<std::string>>;
  using ETM_VP             = ElementTopologyMap::value_type;

  class IOSS_EXPORT ETRegistry
  {
  public:
    void                         insert(const Ioss::ETM_VP &value, bool delete_me);
    ElementTopologyMap::iterator begin() { return m_registry.begin(); }
    ElementTopologyMap::iterator end() { return m_registry.end(); }
    ElementTopologyMap::iterator find(const std::string &type) { return m_registry.find(type); }

    ~ETRegistry();
    std::map<std::string, std::string> customFieldTypes;

  private:
    Ioss::ElementTopologyMap             m_registry;
    std::vector<Ioss::ElementTopology *> m_deleteThese;
  };

  // ========================================================================

  /** \brief Represents an element topology.
   *
   *  Defines node, edge, and face connectivity information of an element.
   */
  class IOSS_EXPORT ElementTopology
  {
  public:
    void alias(const std::string &base, const std::string &syn);
    bool is_alias(const std::string &my_alias) const;

    ElementTopology(const ElementTopology &)            = delete;
    ElementTopology &operator=(const ElementTopology &) = delete;

    virtual ~ElementTopology();

    const std::string &name() const { return name_; }

    //: Return the Sierra master element name corresponding to this
    //: element topology.  Somewhat klugy coupling between IO subsystem
    //: and Sierra, but least klugy I could think of...
    std::string master_element_name() const { return masterElementName_; }

    //: Return basic shape...
    virtual ElementShape shape() const = 0;

    //: Return whether the topology describes an "element". If it
    //: isn't an element, then it is a component of an element.  For
    // example, a quadrilateral Shell is an element, but a QuadFace is
    // not.
    //
    // Default implementation returns true if spatial_dimension() ==
    // parametric_dimension(), otherwise returns false;
    // "Structural" elements (shells, rods, trusses, particles) need
    // to override.
    virtual bool is_element() const;
    virtual bool is_shell() const             = 0;
    virtual int  spatial_dimension() const    = 0;
    virtual int  parametric_dimension() const = 0;
    virtual int  order() const                = 0;

    virtual bool edges_similar() const; // true if all edges have same topology
    virtual bool faces_similar() const; // true if all faces have same topology

    virtual int number_corner_nodes() const = 0;
    virtual int number_nodes() const        = 0;
    virtual int number_edges() const        = 0;
    virtual int number_faces() const        = 0;
    int         number_boundaries() const;

    virtual int number_nodes_edge(int edge = 0) const = 0;
    virtual int number_nodes_face(int face = 0) const = 0;
    virtual int number_edges_face(int face = 0) const = 0;

    IntVector         boundary_connectivity(int edge_number) const;
    virtual IntVector edge_connectivity(int edge_number) const = 0;
    virtual IntVector face_connectivity(int face_number) const = 0;
    virtual IntVector element_connectivity() const             = 0;

    // These have default implementations in ElementTopology.
    // The defaults simply fill in the vector with 0..num.
    // For 'face_edge_connectivity', this is sufficient for 2d
    // elements, 3d need to override.
    // For 'element_edge_connectivity', this works for all elements.
    virtual IntVector face_edge_connectivity(int face_number) const;
    IntVector         element_edge_connectivity() const;

    ElementTopology         *boundary_type(int face_number = 0) const;
    virtual ElementTopology *face_type(int face_number = 0) const = 0;
    virtual ElementTopology *edge_type(int edge_number = 0) const = 0;

    static ElementTopology *factory(const std::string &type, bool ok_to_fail = false);
    static ElementTopology *factory(unsigned int unique_id);
    static unsigned int     get_unique_id(const std::string &type);
    static int              describe(NameList *names);
    static NameList         describe();

    bool operator==(const Ioss::ElementTopology &rhs) const;
    bool operator!=(const Ioss::ElementTopology &rhs) const;
    bool equal(const Ioss::ElementTopology &rhs) const;

    ElementPermutation        *permutation() const;
    virtual const std::string &base_topology_permutation_name() const;

  protected:
    ElementTopology(std::string type, std::string master_elem_name, bool delete_me = false);
    virtual bool validate_permutation_nodes() const { return true; }

  private:
    bool              equal_(const Ioss::ElementTopology &rhs, bool quiet) const;
    const std::string name_;
    const std::string masterElementName_;

    static const std::string &topology_shape_to_permutation_name(Ioss::ElementShape topoShape);

    static ETRegistry &registry();
  };
} // namespace Ioss
