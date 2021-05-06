// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#ifndef IOSS_Ioss_ElementBlock_h
#define IOSS_Ioss_ElementBlock_h

#include "vtk_ioss_mangle.h"

#include <Ioss_BoundingBox.h>
#include <Ioss_CodeTypes.h>
#include <Ioss_EntityBlock.h>
#include <Ioss_Property.h>
#include <cassert>
#include <string>

namespace Ioss {
  class DatabaseIO;

  /** \brief A collection of elements having the same topology.
   */
  class ElementBlock : public EntityBlock
  {
  public:
    ElementBlock(DatabaseIO *io_database, const std::string &my_name,
                 const std::string &element_type, int64_t number_elements);

    ElementBlock(const ElementBlock &) = default;
    ~ElementBlock() override;

    std::string type_string() const override { return "ElementBlock"; }
    std::string short_type_string() const override { return "block"; }
    std::string contains_string() const override { return "Element"; }
    EntityType  type() const override { return ELEMENTBLOCK; }

    // Handle implicit properties -- These are calcuated from data stored
    // in the grouping entity instead of having an explicit value assigned.
    // An example would be 'element_block_count' for a region.
    Property get_implicit_property(const std::string &my_name) const override;

    void                   get_block_adjacencies(std::vector<std::string> &block_adjacency) const;
    AxisAlignedBoundingBox get_bounding_box() const;
    bool                   operator==(const Ioss::ElementBlock &rhs) const;
    bool                   operator!=(const Ioss::ElementBlock &rhs) const;
    bool                   equal(const Ioss::ElementBlock &rhs) const;

  protected:
    int64_t internal_get_field_data(const Field &field, void *data,
                                    size_t data_size) const override;

    int64_t internal_put_field_data(const Field &field, void *data,
                                    size_t data_size) const override;

  private:
  };
} // namespace Ioss
#endif
