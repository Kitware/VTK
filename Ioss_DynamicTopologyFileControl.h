// Copyright(C) 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "Ioss_DynamicTopology.h"

#include "Ioss_CodeTypes.h"
#include "Ioss_DBUsage.h"
#include "Ioss_DatabaseIO.h"      // for DatabaseIO
#include "Ioss_ParallelUtils.h"   // for ParallelUtils
#include "Ioss_PropertyManager.h" // for PropertyManager
#include "Ioss_Utils.h"
#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

#include <cstddef> // for size_t, nullptr
#include <cstdint> // for int64_t
#include <iomanip>
#include <sstream>
#include <string> // for string, operator<

namespace Ioss {
  class Region;

  class IOSS_EXPORT DynamicTopologyFileControl
  {
  public:
    explicit DynamicTopologyFileControl(Region *region);

    void clone_and_replace_output_database(int steps = 0);
    void add_output_database_change_set(int steps = 0);

    static std::string change_set_prefix() { return "IOSS_FILE_GROUP-"; }

    DatabaseIO *get_database() const;

    static std::string get_cyclic_database_filename(const std::string &baseFileName,
                                                    unsigned int       fileCyclicCount,
                                                    unsigned int       step);

    static std::string get_linear_database_filename(const std::string &baseFileName,
                                                    unsigned int       step);

    static std::string get_internal_file_change_set_name(unsigned int step);

    unsigned int             get_topology_change_count() const { return m_dbChangeCount; }
    unsigned int             get_file_cyclic_count() const { return m_fileCyclicCount; }
    IfDatabaseExistsBehavior get_if_database_exists_behavior() const { return m_ifDatabaseExists; }

  private:
    Region     *m_region{nullptr};
    std::string m_ioDB;
    std::string m_dbType;

    PropertyManager m_properties;

    unsigned int             m_fileCyclicCount;
    IfDatabaseExistsBehavior m_ifDatabaseExists;
    unsigned int             m_dbChangeCount;

    IOSS_NODISCARD const ParallelUtils &util() const;

    std::string get_unique_linear_filename(DatabaseUsage db_usage);
    std::string construct_database_filename(int &step, DatabaseUsage db_usage);
    bool        file_exists(const std::string &filename, const std::string &db_type,
                            DatabaseUsage db_usage);
    bool        abort_if_exists(const std::string &filename, const std::string &db_type,
                                DatabaseUsage db_usage);

    DatabaseIO *clone_output_database(int steps);
    bool        replace_output_database(DatabaseIO *db);
  };

} // namespace Ioss
