// Copyright(C) 2024, 2025 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

#include "Ioss_DBUsage.h"
#include "Ioss_DatabaseIO.h"
#include "Ioss_ParallelUtils.h"
#include "Ioss_PropertyManager.h"
#include "Ioss_Region.h"

#include <cstddef>
#include <functional>
#include <sstream>
#include <string>
#include <utility>

namespace Ioss {

  using FileNameGenerator =
      std::function<std::string(const std::string &baseFileName, unsigned step)>;

  enum ChangeSetFormat {
    CHANGE_SET_NONE               = (0),
    CHANGE_SET_INTERNAL_FILES     = (1U << 0),
    CHANGE_SET_LINEAR_MULTI_FILES = (1U << 1),
    CHANGE_SET_CYCLIC_MULTI_FILES = (1U << 2)
  };

  std::pair<std::string, Ioss::DatabaseIO *>
  expand_topology_files(FileNameGenerator generator, const Ioss::ParallelUtils &util,
                        const std::string &basename, const std::string &db_type,
                        const Ioss::PropertyManager &properties, Ioss::DatabaseUsage usage,
                        int step);

  std::string expand_topology_files(FileNameGenerator generator, const Ioss::ParallelUtils &util,
                                    const std::string           &basename,
                                    const Ioss::PropertyManager &properties,
                                    Ioss::DatabaseUsage usage, int step);

  FileNameGenerator construct_cyclic_filename_generator(unsigned cyclicCount);
  FileNameGenerator construct_linear_filename_generator();

  class IOSS_EXPORT ChangeSet
  {
  public:
    explicit ChangeSet(Ioss::Region *region);
    ChangeSet(Ioss::DatabaseIO *db, const std::string &dbName, const std::string &dbType,
              unsigned fileCyclicCount);

    virtual ~ChangeSet();
    ChangeSet()                  = delete;
    ChangeSet(const ChangeSet &) = delete;

    IOSS_NODISCARD unsigned supported_formats() const { return m_supportedFormats; }
    IOSS_NODISCARD unsigned database_format() const { return m_databaseFormat; }

    virtual void populate_change_sets(bool loadAllFiles = true);

    IOSS_NODISCARD virtual DatabaseIO *open_change_set(unsigned index, Ioss::DatabaseUsage usage);
    virtual void                       close_change_set(unsigned index);

    IOSS_NODISCARD size_t size() const { return m_changeSetNames.size(); }
    IOSS_NODISCARD const std::vector<std::string> &names() const { return m_changeSetNames; }
    IOSS_NODISCARD std::string get_change_set_name(unsigned index) const;

    IOSS_NODISCARD unsigned get_file_cyclic_count() const { return m_fileCyclicCount; }

    static std::string get_cyclic_database_filename(const std::string &baseFileName,
                                                    unsigned int       fileCyclicCount,
                                                    unsigned int       step);

    static std::string get_linear_database_filename(const std::string &baseFileName,
                                                    unsigned int       step);

  private:
    std::vector<DatabaseIO *> m_changeSetDatabases;

  protected:
    void get_cyclic_multi_file_change_sets();
    void get_linear_multi_file_change_sets();

    void verify_change_set_index(unsigned index) const;

    virtual void clear_change_sets();

    IOSS_NODISCARD DatabaseIO          *get_database() const;
    IOSS_NODISCARD const ParallelUtils &util() const;

    Ioss::DatabaseIO *m_database{nullptr};
    std::string       m_ioDB;
    std::string       m_dbType;
    unsigned          m_fileCyclicCount{0};

    unsigned m_supportedFormats{CHANGE_SET_LINEAR_MULTI_FILES | CHANGE_SET_CYCLIC_MULTI_FILES};
    unsigned m_databaseFormat{CHANGE_SET_NONE};

    std::vector<std::string> m_changeSetNames;
  };

} // namespace Ioss
