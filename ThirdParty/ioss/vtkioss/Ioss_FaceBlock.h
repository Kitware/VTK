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

#ifndef IOSS_Ioss_FaceBlock_h
#define IOSS_Ioss_FaceBlock_h

#include "vtk_ioss_mangle.h"

#include "Ioss_EntityType.h" // for EntityType, etc
#include <Ioss_EntityBlock.h>
#include <Ioss_Property.h> // for Property
#include <cstddef>         // for size_t
#include <cstdint>         // for int64_t
#include <string>          // for string
namespace Ioss {
  class DatabaseIO;
} // namespace Ioss
namespace Ioss {
  class Field;
} // namespace Ioss

namespace Ioss {

  /** \brief A collection of element faces with the same topology.
   */
  class FaceBlock : public EntityBlock
  {
  public:
    FaceBlock(DatabaseIO *io_database, const std::string &my_name, const std::string &face_type,
              int64_t number_faces);

    FaceBlock(const FaceBlock &) = default;
    ~FaceBlock() override;

    std::string type_string() const override { return "FaceBlock"; }
    std::string short_type_string() const override { return "faceblock"; }
    std::string contains_string() const override { return "Face"; }
    EntityType  type() const override { return FACEBLOCK; }

    // Handle implicit properties -- These are calcuated from data stored
    // in the grouping entity instead of having an explicit value assigned.
    // An example would be 'face_block_count' for a region.
    Property get_implicit_property(const std::string &my_name) const override;

  protected:
    int64_t internal_get_field_data(const Field &field, void *data,
                                    size_t data_size) const override;

    int64_t internal_put_field_data(const Field &field, void *data,
                                    size_t data_size) const override;
  };
} // namespace Ioss
#endif
