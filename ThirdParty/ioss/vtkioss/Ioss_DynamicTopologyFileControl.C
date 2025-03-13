// Copyright(C) 2024, 2025 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_Assembly.h"
#include "Ioss_Blob.h"
#include "Ioss_ChangeSet.h"
#include "Ioss_ChangeSetFactory.h"
#include "Ioss_CodeTypes.h"
#include "Ioss_CommSet.h"
#include "Ioss_DBUsage.h"
#include "Ioss_DatabaseIO.h"
#include "Ioss_DynamicTopology.h"
#include "Ioss_DynamicTopologyFileControl.h"
#include "Ioss_EdgeBlock.h"
#include "Ioss_EdgeSet.h"
#include "Ioss_ElementBlock.h"
#include "Ioss_ElementSet.h"
#include "Ioss_EntityBlock.h"
#include "Ioss_EntityType.h"
#include "Ioss_FaceBlock.h"
#include "Ioss_FaceSet.h"
#include "Ioss_Field.h"
#include "Ioss_FileInfo.h"
#include "Ioss_GroupingEntity.h"
#include "Ioss_IOFactory.h"
#include "Ioss_NodeBlock.h"
#include "Ioss_NodeSet.h"
#include "Ioss_ParallelUtils.h"
#include "Ioss_Region.h"
#include "Ioss_SideBlock.h"
#include "Ioss_SideSet.h"
#include "Ioss_StructuredBlock.h"

#include "vtk_fmt.h"
#include VTK_FMT(fmt/core.h)
#include VTK_FMT(fmt/format.h)
#include VTK_FMT(fmt/ostream.h)

#include <assert.h>
#include <climits>
#include <cstddef>
#include <functional>
#include <iomanip>
#include <sstream>

namespace {

  bool file_exists(const Ioss::ParallelUtils &util, const std::string &filename,
                   const std::string &db_type, Ioss::DatabaseUsage db_usage)
  {
    int         par_size      = util.parallel_size();
    int         par_rank      = util.parallel_rank();
    bool        is_parallel   = par_size > 1;
    std::string full_filename = filename;
    if (is_parallel && (db_type == "exodusII" || db_type == "cgns") &&
        (db_usage != Ioss::WRITE_HISTORY)) {
      full_filename = Ioss::Utils::decode_filename(filename, par_rank, par_size);
    }

    std::string    message;
    Ioss::FileInfo file = Ioss::FileInfo(full_filename);
    return file.parallel_exists(util.communicator(), message);
  }

  template <typename T>
  void update_database_for_grouping_entities(const T &container, Ioss::DatabaseIO *db)
  {
    for (auto *entity : container) {
      Ioss::GroupingEntity *ge = dynamic_cast<Ioss::GroupingEntity *>(entity);
      assert(ge != nullptr);

      if (ge->type() == Ioss::SIDESET) {
        Ioss::SideSet *sset = dynamic_cast<Ioss::SideSet *>(ge);
        assert(sset != nullptr);

        sset->reset_database(db);
        const auto &sblocks = sset->get_side_blocks();
        for (const auto &sblock : sblocks) {
          sblock->reset_database(db);
        }
      }
      else {
        ge->reset_database(db);
      }
    }
  }

} // namespace

namespace Ioss {

  DynamicTopologyFileControl::DynamicTopologyFileControl(Region *region) : m_region(region)
  {
    if (nullptr == region) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: null region passed in as argument to DynamicTopologyFileControl");
      IOSS_ERROR(errmsg);
    }

    // If we are checking for `region == nullptr` above, we cannot
    // do these initializations above and must do them here...
    m_fileCyclicCount  = region->get_file_cyclic_count();
    m_ifDatabaseExists = region->get_if_database_exists_behavior();
    m_dbChangeCount    = region->get_topology_change_count();
    m_ioDB             = region->get_property("base_filename").get_string();
    m_dbType           = region->get_property("database_type").get_string();
  }

  const ParallelUtils &DynamicTopologyFileControl::util() const
  {
    return m_region->get_database()->util();
  }

  bool DynamicTopologyFileControl::file_exists(const std::string  &filename,
                                               const std::string  &db_type,
                                               Ioss::DatabaseUsage db_usage)
  {
    return ::file_exists(util(), filename, db_type, db_usage);
  }

  std::string DynamicTopologyFileControl::get_unique_linear_filename(Ioss::DatabaseUsage db_usage)
  {
    std::string filename = m_ioDB;

    do {
      // Run this loop at least once for all files.  If this is an automatic
      // restart, then make sure that the generated file does not already exist,
      // so keep running the loop until we generate a filename that doesn't exist...
      std::ostringstream tmp_filename;
      tmp_filename << m_ioDB;

      // Don't append the "-s000X" the first time in case the base filename doesn't
      // exist -- we want write to the name specified by the user if at all possible and
      // once that exists, then start adding on the suffix...
      if (m_dbChangeCount > 1) {
        tmp_filename << "-s" << std::setw(4) << std::setfill('0') << m_dbChangeCount;
      }
      filename = tmp_filename.str();
      ++m_dbChangeCount;
    } while (file_exists(filename, m_dbType, db_usage));
    --m_dbChangeCount;
    return filename;
  }

  std::string DynamicTopologyFileControl::get_internal_file_change_set_name(unsigned int step)
  {
    std::ostringstream change_setname;
    change_setname << change_set_prefix();
    change_setname << step;
    return change_setname.str();
  }

  std::string DynamicTopologyFileControl::get_cyclic_database_filename(
      const std::string &baseFileName, unsigned int fileCyclicCount, unsigned int step)
  {
    return ChangeSet::get_cyclic_database_filename(baseFileName, fileCyclicCount, step);
  }

  std::string
  DynamicTopologyFileControl::get_linear_database_filename(const std::string &baseFileName,
                                                           unsigned int       step)
  {
    return ChangeSet::get_linear_database_filename(baseFileName, step);
  }

  std::string DynamicTopologyFileControl::construct_database_filename(int                &step,
                                                                      Ioss::DatabaseUsage db_usage)
  {
    // Filename will be of the form -- ioDB-sxxxx where xxxx is step
    // number.  Assume maximum of 9999 steps (will do more, but won't have
    // good lineup of step numbers.
    // Check database for validity (filename and a type)
    if (m_ioDB.empty() || m_dbType.empty()) {
      std::string error_message;
      if (m_dbType.empty())
        error_message += "The database TYPE has not been defined\n";

      if (m_ioDB.empty()) {
        error_message += "The database FILENAME has not been defined\n";
      }
      std::ostringstream errmsg;
      errmsg << error_message;
      IOSS_ERROR(errmsg);
    }
    assert(!m_ioDB.empty());
    assert(!m_dbType.empty());
    std::string filename = m_ioDB;
    if (m_fileCyclicCount > 0) {
      // In this mode, we close the old file and open a new file
      // every time this is called. The file suffix cycles through
      // the first fileCyclicCount'th entries in A,B,C,D,E,F,...

      //    filename = get_cyclic_database_filename(m_ioDB, m_fileCyclicCount, step);
      if (step == 0)
        step++;

      static std::string suffix = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
      std::string        tmp    = "-" + suffix.substr((step - 1) % m_fileCyclicCount, 1);
      filename += tmp;
      m_properties.add(Ioss::Property("APPEND_OUTPUT", Ioss::DB_OVERWRITE));
    }
    else {
      if (m_region->model_is_written()) {
        // After the initial open, we want to add suffix if the topology changes
        // during the run
        m_ifDatabaseExists = Ioss::DB_ADD_SUFFIX_OVERWRITE;
      }

      // Handle complications of DB_APPEND mode...
      // If in DB_APPEND mode, then we don't output metadata
      // information, so some knowledge is needed at this level if
      // we are appending.  If user specified APPEND, but the file
      // doesn't yet exist OR it does exist and we are not
      // restarting, change the mode to OVERWRITE.
      // 0. Must be restarting; either manual or automatic.
      std::shared_ptr<DynamicTopologyObserver> observer =
          m_region->get_mesh_modification_observer();

      if (m_ifDatabaseExists == Ioss::DB_APPEND) {
        if (!observer->is_restart_requested()) {
          // Not restarting
          m_ifDatabaseExists = Ioss::DB_OVERWRITE;
        }
        else if (!file_exists(m_ioDB, m_dbType, db_usage)) {
          m_ifDatabaseExists = Ioss::DB_OVERWRITE;
        }
      }
      if (step > 1 || (m_dbChangeCount > 1)) {
        // Use the !is_input_event test since restart input files already have the
        // -s000x extension...
        if (m_ifDatabaseExists == Ioss::DB_APPEND) {
          std::ostringstream tmp_filename;
          tmp_filename << m_ioDB;
          filename = m_ioDB;
          if (m_dbChangeCount > 1) {
            tmp_filename << "-s" << std::setw(4) << std::setfill('0') << m_dbChangeCount;
          }
          size_t inc = 0;
          while (file_exists(tmp_filename.str(), m_dbType, db_usage)) {
            filename = tmp_filename.str();
            tmp_filename.clear();
            tmp_filename.str("");
            tmp_filename << m_ioDB << "-s" << std::setw(4) << std::setfill('0')
                         << m_dbChangeCount + (++inc);
          }
          if (inc > 0) {
            m_dbChangeCount += (inc - 1);
          }
          else {
            m_ifDatabaseExists = Ioss::DB_OVERWRITE;
          }
        }
        else if (m_ifDatabaseExists == Ioss::DB_ADD_SUFFIX) {
          filename = get_unique_linear_filename(db_usage);
        }
        else if (m_ifDatabaseExists == Ioss::DB_ADD_SUFFIX_OVERWRITE) {
          if (m_dbChangeCount > 0) {
            std::ostringstream tmp_filename;
            tmp_filename << m_ioDB << "-s" << std::setw(4) << std::setfill('0')
                         << ++m_dbChangeCount;
            filename = tmp_filename.str();
          }
          else {
            filename = m_ioDB;
          }
        }
        else {
          filename = m_ioDB;
        }
      }
      else if (m_ifDatabaseExists == Ioss::DB_ADD_SUFFIX) {
        filename = get_unique_linear_filename(db_usage);
      }
      else {
        filename = m_ioDB;
      }

      m_properties.add(Ioss::Property("APPEND_OUTPUT", m_ifDatabaseExists));
      // A little complicated on deciding whether we are actually
      // overwriting the database.  The 'validate' routine for Results and
      // History will call create_database once the parser block is
      // ended. This routine will then create the database and the
      // ioRegion_. However, the database will not really be opened or
      // written to at this time.  If the code is auto-restarting, then it will
      // detect that the database exists and create a database with the
      // -s000x extension.
      // At this point, we need to skip the 'abort_if_exists' test if we
      // are in this routine from the 'validate' and we are restarting
      // since we won't really write to the file.  So, the cases where we
      // *don't* check are:
      // -- is_input_event(db_usage)
      // -- ifExists_ == DB_OVERWRITE || DB_ADD_SUFFIX_OVERWRITE || DB_APPEND
      // -- is_automatic_restart() && step == 0 (coming from validate)
      if (m_ifDatabaseExists != DB_OVERWRITE && m_ifDatabaseExists != DB_APPEND &&
          m_ifDatabaseExists != DB_ADD_SUFFIX_OVERWRITE &&
          !(step == 0 && observer->is_automatic_restart())) {
        abort_if_exists(filename, m_dbType, db_usage);
      }
    }
    return filename;
  }

  bool DynamicTopologyFileControl::abort_if_exists(const std::string  &filename,
                                                   const std::string  &db_type,
                                                   Ioss::DatabaseUsage db_usage)
  {
    // Check whether file with same name as database already exists.  If so,
    // print error message and stop...
    // At the current time, only check on processor 0 and assume if it doesn't exist
    // there, then it doesn't exist on other processors.  Or, if it doesn't exist on
    // processor 0, then it doesn't matter if it doesn't exist on other processors
    // since we don't have all pieces...

    bool exists = file_exists(filename, db_type, db_usage);
    if (exists) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: The database file named '{} exists"
                 "and would be overwritten if the code continued.\n\n"
                 "Input options specified that this file *not* be overwritten,\n"
                 "\tso you must rename or remove this file and restart the code.\n",
                 filename);
      IOSS_ERROR(errmsg);
    }
    return exists;
  }

  Ioss::DatabaseIO *DynamicTopologyFileControl::clone_output_database(int steps)
  {
    auto current_db = m_region->get_database();

    if (current_db->is_input())
      return nullptr;

    const Ioss::PropertyManager &current_properties = current_db->get_property_manager();
    Ioss::NameList               names              = current_properties.describe();

    // Iterate through properties and transfer to new output database...
    for (const auto &name : names) {
      if (!m_properties.exists(name))
        m_properties.add(current_properties.get(name));
    }

    auto db_usage = current_db->usage();

    std::string filename = construct_database_filename(steps, db_usage);

    Ioss::DatabaseIO *db = Ioss::IOFactory::create(m_dbType, filename, db_usage,
                                                   current_db->util().communicator(), m_properties);

    if (nullptr == db) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: unable to create output database named '{}'"
                 " of type '{}'",
                 filename, m_dbType);
      IOSS_ERROR(errmsg);
    }

    assert(db != nullptr);
    if (!db->ok(true)) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: unable to validate output database named '{}'"
                 " of type '{}'",
                 filename, m_dbType);
      IOSS_ERROR(errmsg);
    }

    db->set_field_separator(current_db->get_field_separator());
    db->set_surface_split_type(current_db->get_surface_split_type());
    db->set_maximum_symbol_length(current_db->maximum_symbol_length());
    db->set_int_byte_size_api(current_db->int_byte_size_data_size());

    return db;
  }

  bool DynamicTopologyFileControl::replace_output_database(Ioss::DatabaseIO *db)
  {
    auto current_db = m_region->get_database();

    if (current_db->is_input())
      return false;

    current_db->finalize_database();
    current_db->closeDatabase();
    delete current_db;

    m_region->reset_database(db);
    db->set_region(m_region);

    update_database_for_grouping_entities(m_region->get_node_blocks(), db);
    update_database_for_grouping_entities(m_region->get_edge_blocks(), db);
    update_database_for_grouping_entities(m_region->get_face_blocks(), db);
    update_database_for_grouping_entities(m_region->get_element_blocks(), db);
    update_database_for_grouping_entities(m_region->get_sidesets(), db);
    update_database_for_grouping_entities(m_region->get_nodesets(), db);
    update_database_for_grouping_entities(m_region->get_edgesets(), db);
    update_database_for_grouping_entities(m_region->get_facesets(), db);
    update_database_for_grouping_entities(m_region->get_elementsets(), db);
    update_database_for_grouping_entities(m_region->get_commsets(), db);
    update_database_for_grouping_entities(m_region->get_structured_blocks(), db);
    update_database_for_grouping_entities(m_region->get_assemblies(), db);
    update_database_for_grouping_entities(m_region->get_blobs(), db);

    return true;
  }

  void DynamicTopologyFileControl::clone_and_replace_output_database(int steps)
  {
    auto db = clone_output_database(steps);

    if (nullptr != db)
      replace_output_database(db);
  }

  void DynamicTopologyFileControl::add_output_database_change_set(IOSS_MAYBE_UNUSED int steps)
  {
    auto current_db = get_database();

    std::ostringstream oss;
    oss << change_set_prefix();
    oss << m_dbChangeCount;

    current_db->release_memory();
    current_db->create_internal_change_set(oss.str());

    m_dbChangeCount++;
  }

  DatabaseIO *DynamicTopologyFileControl::get_database() const { return m_region->get_database(); }

} // namespace Ioss
