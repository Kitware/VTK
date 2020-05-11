// Copyright(C) 1999-2017 National Technology & Engineering Solutions
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

#ifndef IOSS_Ioss_State_h
#define IOSS_Ioss_State_h

#include "vtk_ioss_mangle.h"

namespace Ioss {

  /** \brief Access states for a database.
   *
   *  All access states except STATE_INVALID, STATE_UNKNOWN, and STATE_READONLY are only
   *  for output databases.
   */
  enum State {
    STATE_INVALID = -1, /**< Error state if something goes wrong. */
    STATE_UNKNOWN,      /**< Typically used at the very beginning of the database's existence
                             when the class has been created, but no reading or writing has
                             occurred. */
    STATE_READONLY,     /**< An input database that is not in STATE_UNKNOWN is in this
                             state, which means that it cannot be written to or changed. */
    STATE_CLOSED,       /**< The sates are not nested, so each state must end with a transition
                             to this state prior to entering the next state. */
    STATE_DEFINE_MODEL, /**< Defining the metadata, which defines the topology of the model
                             (nontransient, geometry and topology). */
    STATE_MODEL,        /**< Outputting the bulk data (mesh_model_coordinates, ids, connectivity)
                             relating to the model portion. */
    STATE_DEFINE_TRANSIENT, /**< Defining the metadata relating to the transient data. For example,
                                 the element or nodal fields. */
    STATE_TRANSIENT,        /**< Outputting the transient bulk data. */
    STATE_LAST_ENTRY
  };
} // namespace Ioss
#endif
