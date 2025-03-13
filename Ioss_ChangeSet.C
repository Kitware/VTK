// Copyright(C) 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_ChangeSet.h"

#include "Ioss_CodeTypes.h"
#include "Ioss_DBUsage.h"

#include "Ioss_DynamicTopology.h"
#include "Ioss_EntityBlock.h"
#include "Ioss_EntityType.h"
#include "Ioss_Field.h"
#include "Ioss_FileInfo.h"
#include "Ioss_GroupingEntity.h"
#include "Ioss_IOFactory.h"

#include "vtk_fmt.h"
#include VTK_FMT(fmt/core.h)
#include VTK_FMT(fmt/format.h)
#include VTK_FMT(fmt/ostream.h)

#include <assert.h>
#include <iomanip>

namespace {
  int file_exists(const Ioss::ParallelUtils &util, const std::string &filename,
                  std::string &message, bool filePerRank)
  {
    std::string filenameBase = filename;
    const int   par_size     = util.parallel_size();
    const int   par_rank     = util.parallel_rank();

    if (par_size > 1 && !filePerRank) {
      filenameBase = Ioss::Utils::decode_filename(filenameBase, par_rank, par_size);
    }

    Ioss::FileInfo file = Ioss::FileInfo(filenameBase);
    return file.parallel_exists(util.communicator(), message);
  }

  std::string get_decomposition_property(const Ioss::PropertyManager &properties,
                                         Ioss::DatabaseUsage          db_usage)
  {
    std::string decomp_method;
    std::string decomp_property;
    if (db_usage == Ioss::READ_MODEL) {
      decomp_property = "MODEL_DECOMPOSITION_METHOD";
    }
    else if (db_usage == Ioss::READ_RESTART || db_usage == Ioss::QUERY_TIMESTEPS_ONLY) {
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

  bool internal_decomp_specified(const Ioss::PropertyManager &props, Ioss::DatabaseUsage usage)
  {
    bool internalDecompSpecified = false;

    std::string method = get_decomposition_property(props, usage);
    if (!method.empty() && method != "EXTERNAL") {
      internalDecompSpecified = true;
    }

    return internalDecompSpecified;
  }

} // namespace

namespace Ioss {

  ChangeSet::ChangeSet(Ioss::Region *region)
      : m_database(region->get_database()),
        m_ioDB(region->get_property("base_filename").get_string()),
        m_dbType(region->get_property("database_type").get_string()),
        m_fileCyclicCount(region->get_file_cyclic_count())
  {
  }

  ChangeSet::ChangeSet(Ioss::DatabaseIO *db, const std::string &dbName, const std::string &dbType,
                       unsigned fileCyclicCount)
      : m_database(db), m_ioDB(dbName), m_dbType(dbType), m_fileCyclicCount(fileCyclicCount)
  {
  }

  ChangeSet::~ChangeSet() { clear_change_sets(); }

  DatabaseIO *ChangeSet::get_database() const { return m_database; }

  const ParallelUtils &ChangeSet::util() const { return get_database()->util(); }

  void ChangeSet::get_cyclic_multi_file_change_sets()
  {
    auto db = get_database();

    m_databaseFormat = CHANGE_SET_CYCLIC_MULTI_FILES;

    Ioss::FileNameGenerator generator =
        Ioss::construct_cyclic_filename_generator(m_fileCyclicCount);

    bool found           = true;
    int  step            = 0;
    int  fileCyclicCount = m_fileCyclicCount;

    while (found && (step < fileCyclicCount)) {
      ++step;

      std::string expanded = Ioss::expand_topology_files(
          generator, util(), m_ioDB, db->get_property_manager(), db->usage(), step);
      if (!expanded.empty()) {
        m_changeSetNames.push_back(expanded);
      }
      else {
        found = false;
      }
    }

    m_changeSetDatabases.resize(m_changeSetNames.size(), nullptr);
  }

  void ChangeSet::get_linear_multi_file_change_sets()
  {
    auto db = get_database();

    m_databaseFormat = CHANGE_SET_LINEAR_MULTI_FILES;

    Ioss::FileNameGenerator generator = Ioss::construct_linear_filename_generator();

    bool found = true;
    int  step  = 0;

    while (found) {
      ++step;

      std::string expanded = Ioss::expand_topology_files(
          generator, util(), m_ioDB, db->get_property_manager(), db->usage(), step);
      if (!expanded.empty()) {
        m_changeSetNames.push_back(expanded);
      }
      else {
        found = false;
      }
    }

    m_changeSetDatabases.resize(m_changeSetNames.size(), nullptr);
  }

  void ChangeSet::populate_change_sets(bool loadAllFiles)
  {
    clear_change_sets();

    if (!loadAllFiles) {
      // Load only the current db file
      m_databaseFormat = CHANGE_SET_LINEAR_MULTI_FILES;
      m_changeSetNames.push_back(get_database()->get_filename());
      m_changeSetDatabases.resize(m_changeSetNames.size(), nullptr);
      return;
    }

    if (get_file_cyclic_count() > 0) {
      get_cyclic_multi_file_change_sets();
    }
    else {
      get_linear_multi_file_change_sets();
    }
  }

  void ChangeSet::verify_change_set_index(unsigned index) const
  {
    if (index >= m_changeSetNames.size()) {
      IOSS_ERROR(fmt::format("Invalid change set index {} with a max value of {}\n", index,
                             m_changeSetNames.size() - 1));
    }
  }

  std::string ChangeSet::get_change_set_name(unsigned index) const
  {
    verify_change_set_index(index);

    return m_changeSetNames[index];
  }

  void ChangeSet::close_change_set(unsigned index)
  {
    verify_change_set_index(index);

    auto db = m_changeSetDatabases[index];
    if (nullptr != db) {
      db->closeDatabase();
      delete db;
      m_changeSetDatabases[index] = nullptr;
    }
  }

  Ioss::DatabaseIO *ChangeSet::open_change_set(unsigned index, Ioss::DatabaseUsage usage)
  {
    verify_change_set_index(index);

    Ioss::DatabaseIO *db = nullptr;

    auto csdb = m_changeSetDatabases[index];
    if (nullptr != csdb) {
      if (csdb->usage() == usage) {
        return csdb;
      }
      else {
        csdb->closeDatabase();
        delete csdb;
        m_changeSetDatabases[index] = nullptr;
      }
    }

    // open db
    const std::string &ioDB   = m_changeSetNames[index];
    const std::string  dbType = m_dbType;

    db = Ioss::IOFactory::create(dbType, ioDB, usage, util().communicator(),
                                 get_database()->get_property_manager());
    std::string error_message;
    bool        is_valid_file = db != nullptr && db->ok(false, &error_message, nullptr);
    if (!is_valid_file) {
      delete db;
      std::ostringstream errmsg;
      errmsg << error_message;
      errmsg << __FILE__ << ", " << __FUNCTION__ << ", filename " << ioDB
             << " is not a valid file\n";
      IOSS_ERROR(errmsg);
    }

    m_changeSetDatabases[index] = db;

    return db;
  }

  void ChangeSet::clear_change_sets()
  {
    m_changeSetNames.clear();

    for (size_t i = 0; i < m_changeSetDatabases.size(); i++) {
      auto db = m_changeSetDatabases[i];
      if (nullptr != db) {
        db->closeDatabase();
        delete db;
        m_changeSetDatabases[i] = nullptr;
      }
    }

    m_changeSetDatabases.clear();
  }

  std::string ChangeSet::get_cyclic_database_filename(const std::string &baseFileName,
                                                      unsigned int       fileCyclicCount,
                                                      unsigned int       step)
  {
    Ioss::FileNameGenerator generator = Ioss::construct_cyclic_filename_generator(fileCyclicCount);
    return generator(baseFileName, step);
  }

  std::string ChangeSet::get_linear_database_filename(const std::string &baseFileName,
                                                      unsigned int       step)
  {
    Ioss::FileNameGenerator generator = Ioss::construct_linear_filename_generator();
    return generator(baseFileName, step);
  }

  std::string expand_topology_files(FileNameGenerator generator, const Ioss::ParallelUtils &util,
                                    const std::string           &basename,
                                    const Ioss::PropertyManager &properties,
                                    Ioss::DatabaseUsage usage, int step)
  {
    // See if there are multiple topology files

    // If the file exists on all processors, return the filename.
    // If the file does not exist on any processors, return "";
    // If the file exists on some, but not all, throw an exception.

    std::string filename = generator(basename, step);

    bool        internalDecompSpecified = internal_decomp_specified(properties, usage);
    std::string message;
    int         exists = ::file_exists(util, filename, message, internalDecompSpecified);

    int par_size = util.parallel_size();
    int par_rank = util.parallel_rank();

    if (exists > 0 && exists < par_size) {
      std::ostringstream errmsg;
      errmsg << "ERROR: Unable to open input database";
      if (par_rank == 0) {
        errmsg << " '" << filename << "'" << "\n\ton processor(s): " << message;
      }
      else {
        errmsg << ". See processor 0 output for more details.\n";
      }
      IOSS_ERROR(errmsg);
    }

    if (exists == par_size) {
      return filename;
    }

    // Does not exist on any processors
    return std::string();
  }

  std::pair<std::string, Ioss::DatabaseIO *>
  expand_topology_files(FileNameGenerator generator, const Ioss::ParallelUtils &util,
                        const std::string &basename, const std::string &db_type,
                        const Ioss::PropertyManager &properties, Ioss::DatabaseUsage usage,
                        int step)
  {
    // See if there are multiple topology files

    // If the file exists on all processors, return the filename.
    // If the file does not exist on any processors, return "";
    // If the file exists on some, but not all, throw an exception.

    Ioss::DatabaseIO *db = nullptr;
    std::string       filename =
        expand_topology_files(generator, util, basename, properties, usage, step);

    if (!filename.empty()) {
      db = Ioss::IOFactory::create(db_type, filename, usage, util.communicator(), properties);
      int         bad_count = 0;
      std::string error_message;
      bool        is_valid_file = db != nullptr && db->ok(false, &error_message, &bad_count);
      if (is_valid_file) {
        return std::make_pair(filename, db);
      }
      else {
        delete db;
        std::ostringstream errmsg;
        errmsg << error_message;
        errmsg << __FILE__ << ", " << __FUNCTION__ << ", filename " << filename
               << " is not a valid file\n";
        IOSS_ERROR(errmsg);
      }
    }

    // Does not exist on any processors
    return std::make_pair(filename, db);
  }

  Ioss::FileNameGenerator construct_cyclic_filename_generator(unsigned cyclicCount)
  {
    if (cyclicCount > 26) {
      cyclicCount = 26;
    }

    Ioss::FileNameGenerator generator = [cyclicCount](const std::string &baseFileName,
                                                      unsigned           step) {
      static std::string suffix   = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
      std::string        filename = baseFileName;
      if (step == 0)
        step++;
      filename += "-" + suffix.substr((step - 1) % cyclicCount, 1);
      return filename;
    };

    return generator;
  }

  Ioss::FileNameGenerator construct_linear_filename_generator()
  {
    Ioss::FileNameGenerator generator = [](const std::string &baseFileName, unsigned step) {
      std::ostringstream filename;
      filename << baseFileName;
      if (step > 1) {
        filename << "-s" << std::setw(4) << std::setfill('0') << step;
      }
      return filename.str();
    };

    return generator;
  }
} // namespace Ioss
