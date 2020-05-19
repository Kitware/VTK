// Copyright(C) 1999-2017, 2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//
//     * Neither the name of NTESS nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef IOSS_Ioss_EntitySet_h
#define IOSS_Ioss_EntitySet_h

#include "vtk_ioss_mangle.h"

#include <Ioss_GroupingEntity.h> // for GroupingEntity
#include <Ioss_Property.h>       // for Property
#include <cstddef>               // for size_t
#include <string>                // for string
namespace Ioss {
  class DatabaseIO;
} // namespace Ioss

namespace Ioss {
  class ElementSet;

  /** \brief Base class for all 'set'-type grouping entities, which means that members
   *         of the set are not necessarily similar, or do not necessarily have the
   *         same topology.
   *
   *   The following derived classes are typical:
   *
   *   -- NodeSet  -- grouping of nodes (0d topology)
   *
   *   -- EdgeSet  -- grouping of edges (1d topology)
   *
   *   -- FaceSet  -- grouping of faces (2d topology) [Surface]
   *
   *
   */
  class EntitySet : public GroupingEntity
  {
  public:
    Property get_implicit_property(const std::string &my_name) const override = 0;

  protected:
    EntitySet(DatabaseIO *io_database, const std::string &my_name, size_t entity_cnt);
    EntitySet(const EntitySet &) = default;
    EntitySet &operator=(const EntitySet &) = delete;
    ~EntitySet() override                   = default;

  protected:
    void count_attributes() const;
  };
} // namespace Ioss
#endif
