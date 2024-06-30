// Copyright(C) 1999-2023 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "exonull/Ioexnl_DatabaseIO.h" // for Ioexnl DatabaseIO
#include "exonull/Ioexnl_IOFactory.h"  // for Ioexnl IOFactory

#if defined(PARALLEL_AWARE_EXODUS)             // Defined in exodusII.h
#include "exonull/Ioexnl_ParallelDatabaseIO.h" // for Ioexnl ParallelDatabaseIO
#endif
#include <string> // for string

#include "Ioss_CodeTypes.h" // for Ioss_MPI_Comm
#include "Ioss_DBUsage.h"   // for DatabaseUsage
#include "Ioss_IOFactory.h" // for IOFactory

#if !defined(NO_PARMETIS_SUPPORT)
#include <parmetis.h>
#endif

#if defined(PARALLEL_AWARE_EXODUS)
namespace {
  std::string check_decomposition_property(const Ioss::PropertyManager &properties,
                                           Ioss::DatabaseUsage          db_usage);
  bool        check_composition_property(const Ioss::PropertyManager &properties,
                                         Ioss::DatabaseUsage          db_usage);
} // namespace
#endif

namespace Ioexnl {

  const IOFactory *IOFactory::factory()
  {
    static IOFactory registerThis;
    return &registerThis;
  }

  IOFactory::IOFactory() : Ioss::IOFactory("exonull") {}

  Ioss::DatabaseIO *IOFactory::make_IO(const std::string &filename, Ioss::DatabaseUsage db_usage,
                                       Ioss_MPI_Comm                communicator,
                                       const Ioss::PropertyManager &properties) const
  {
#if defined(PARALLEL_AWARE_EXODUS)
    // The "exodus" and "parallel_exodus" databases can both be accessed
    // from this factory.  The "parallel_exodus" is returned only if the following
    // are true:
    // 0. The db_usage is 'READ_MODEL' (not officially supported for READ_RESTART yet)
    // 1. Parallel run with >1 processor
    // 2. There is a DECOMPOSITION_METHOD specified in 'properties'
    // 3. The decomposition method is not "EXTERNAL"

    Ioss::ParallelUtils pu(communicator);
    int                 proc_count = pu.parallel_size();

    bool decompose = false;
    if (proc_count > 1) {
      if (db_usage == Ioss::READ_MODEL || db_usage == Ioss::READ_RESTART) {
        std::string method = check_decomposition_property(properties, db_usage);
        if (!method.empty() && method != "EXTERNAL") {
          decompose = true;
        }
      }
      else if (db_usage == Ioss::WRITE_RESULTS || db_usage == Ioss::WRITE_RESTART) {
        if (check_composition_property(properties, db_usage)) {
          decompose = true;
        }
      }
    }

    // Could call Ioexnl::ParallelDatabaseIO constructor directly, but that leads to some circular
    // dependencies and other yuks.
    if (decompose)
      return new Ioexnl::ParallelDatabaseIO(nullptr, filename, db_usage, communicator, properties);
    else
#endif
      return new Ioexnl::DatabaseIO(nullptr, filename, db_usage, communicator, properties);
  }

  std::string IOFactory::show_config() const { return ""; }
} // namespace Ioexnl

#if defined(PARALLEL_AWARE_EXODUS)
namespace {
  std::string check_decomposition_property(const Ioss::PropertyManager &properties,
                                           Ioss::DatabaseUsage          db_usage)
  {
    std::string decomp_method;
    std::string decomp_property;
    if (db_usage == Ioss::READ_MODEL) {
      decomp_property = "MODEL_DECOMPOSITION_METHOD";
    }
    else if (db_usage == Ioss::READ_RESTART) {
      decomp_property = "RESTART_DECOMPOSITION_METHOD";
    }

    // Applies to either read_model or read_restart
    if (properties.exists("DECOMPOSITION_METHOD")) {
      std::string method = properties.get("DECOMPOSITION_METHOD").get_string();
      return Ioss::Utils::uppercase(method);
    }

    // Check for property...
    if (properties.exists(decomp_property)) {
      std::string method = properties.get(decomp_property).get_string();
      return Ioss::Utils::uppercase(method);
    }
    return decomp_method;
  }

  bool check_composition_property(const Ioss::PropertyManager &properties,
                                  Ioss::DatabaseUsage          db_usage)
  {
    bool        compose          = false;
    std::string compose_property = "COMPOSE_INVALID";
    if (db_usage == Ioss::WRITE_RESULTS) {
      compose_property = "COMPOSE_RESULTS";
    }
    else if (db_usage == Ioss::WRITE_RESTART) {
      compose_property = "COMPOSE_RESTART";
    }

    Ioss::Utils::check_set_bool_property(properties, compose_property, compose);
    return compose;
  }
} // namespace
#endif
