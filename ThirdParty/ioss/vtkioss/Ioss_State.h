// Copyright(C) 1999-2020, 2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

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
    STATE_CLOSED,       /**< The states are not nested, so each state must end with a transition
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
