// Copyright(C) 2021, 2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <Ioss_CodeTypes.h>
#include <Ioss_CopyDatabase.h>
#include <Ioss_DataPool.h>
#include <Ioss_FaceGenerator.h>
#include <Ioss_MeshCopyOptions.h>
#include <Ioss_SubSystem.h>

#include "vtk_fmt.h"
#include VTK_FMT(fmt/ostream.h)
#include <limits>

// For Sleep...
#include <chrono>
#include <thread>

// For copy_database...
namespace {
  std::vector<int> get_selected_steps(Ioss::Region &region, const Ioss::MeshCopyOptions &options);
  void show_step(int istep, double time, const Ioss::MeshCopyOptions &options, int rank);
  std::vector<Ioss::Face> generate_boundary_faces(Ioss::Region                &region,
                                                  const Ioss::MeshCopyOptions &options);
  void define_model(Ioss::Region &region, Ioss::Region &output_region, Ioss::DataPool &data_pool,
                    const std::vector<Ioss::Face> &boundary, const Ioss::MeshCopyOptions &options,
                    int rank);
  void transfer_model(Ioss::Region &region, Ioss::Region &output_region, Ioss::DataPool &pool,
                      const std::vector<Ioss::Face> &boundary, const Ioss::MeshCopyOptions &options,
                      int rank);
  void define_transient_fields(Ioss::Region &region, Ioss::Region &output_region,
                               const Ioss::MeshCopyOptions &options, int rank);
  void transfer_step(Ioss::Region &region, Ioss::Region &output_region, Ioss::DataPool &pool, int istep,
                     const Ioss::MeshCopyOptions &options, int rank);

  void transfer_nodeblock(Ioss::Region &region, Ioss::Region &output_region, Ioss::DataPool &pool,
                          const Ioss::MeshCopyOptions &options, int rank);
  void transfer_structuredblocks(Ioss::Region &region, Ioss::Region &output_region,
                                 const Ioss::MeshCopyOptions &options, int rank);
  void transfer_elementblocks(Ioss::Region &region, Ioss::Region &output_region,
                              const Ioss::MeshCopyOptions &options, int rank);
  void transfer_edgeblocks(Ioss::Region &region, Ioss::Region &output_region,
                           const Ioss::MeshCopyOptions &options, int rank);
  void transfer_faceblocks(Ioss::Region &region, Ioss::Region &output_region,
                           const Ioss::MeshCopyOptions &options, int rank);
  void transfer_nodesets(Ioss::Region &region, Ioss::Region &output_region,
                         const Ioss::MeshCopyOptions &options, int rank);
  void transfer_edgesets(Ioss::Region &region, Ioss::Region &output_region,
                         const Ioss::MeshCopyOptions &options, int rank);
  void transfer_facesets(Ioss::Region &region, Ioss::Region &output_region,
                         const Ioss::MeshCopyOptions &options, int rank);
  void transfer_elemsets(Ioss::Region &region, Ioss::Region &output_region,
                         const Ioss::MeshCopyOptions &options, int rank);
  void transfer_sidesets(Ioss::Region &region, Ioss::Region &output_region,
                         const Ioss::MeshCopyOptions &options, int rank);
  void transfer_commsets(Ioss::Region &region, Ioss::Region &output_region,
                         const Ioss::MeshCopyOptions &options, int rank);

  void transfer_fields(const Ioss::GroupingEntity *ige, Ioss::GroupingEntity *oge,
                       Ioss::Field::RoleType role, const std::string &prefix = "");

  void add_proc_id(Ioss::Region &region, int rank);

  template <typename T>
  void transfer_field_data(const std::vector<T *> &entities, Ioss::Region &output_region,
                           Ioss::DataPool &pool, Ioss::Field::RoleType role,
                           const Ioss::MeshCopyOptions &options);

  void transfer_field_data(Ioss::GroupingEntity *ige, Ioss::GroupingEntity *oge, Ioss::DataPool &pool,
                           Ioss::Field::RoleType role, const Ioss::MeshCopyOptions &options,
                           const std::string &prefix = "");

  void transfer_properties(const Ioss::GroupingEntity *ige, Ioss::GroupingEntity *oge);

  void transfer_qa_info(Ioss::Region &in, Ioss::Region &out);

  void transfer_field_data_internal(Ioss::GroupingEntity *ige, Ioss::GroupingEntity *oge,
                                    Ioss::DataPool &pool, const std::string &field_name,
                                    const Ioss::MeshCopyOptions &options);

#ifdef SEACAS_HAVE_MPI
  template <typename INT>
  void set_owned_node_count(Ioss::Region &region, int my_processor, INT dummy);
#endif

  template <typename T>
  std::pair<size_t, std::string>
  calculate_maximum_field_size(const std::vector<T>           &entities,
                               std::pair<size_t, std::string> &max_field)
  {
    size_t      max_size = max_field.first;
    std::string max_name = max_field.second;
    for (const auto &entity : entities) {
      Ioss::NameList fields = entity->field_describe();
      for (const auto &field_name : fields) {
        Ioss::Field field = entity->get_field(field_name);
        if (field.get_size() > max_size) {
          max_size = field.get_size();
          max_name = field_name;
        }
      }
    }
    return std::make_pair(max_size, max_name);
  }

  std::pair<size_t, std::string> calculate_maximum_field_size(const Ioss::Region &region)
  {
    std::pair<size_t, std::string> max_field{};
    max_field = calculate_maximum_field_size(region.get_node_blocks(), max_field);
    max_field = calculate_maximum_field_size(region.get_edge_blocks(), max_field);
    max_field = calculate_maximum_field_size(region.get_face_blocks(), max_field);
    max_field = calculate_maximum_field_size(region.get_element_blocks(), max_field);
    max_field = calculate_maximum_field_size(region.get_sidesets(), max_field);
    max_field = calculate_maximum_field_size(region.get_nodesets(), max_field);
    max_field = calculate_maximum_field_size(region.get_edgesets(), max_field);
    max_field = calculate_maximum_field_size(region.get_facesets(), max_field);
    max_field = calculate_maximum_field_size(region.get_elementsets(), max_field);
    max_field = calculate_maximum_field_size(region.get_commsets(), max_field);
    max_field = calculate_maximum_field_size(region.get_structured_blocks(), max_field);
    max_field = calculate_maximum_field_size(region.get_assemblies(), max_field);
    max_field = calculate_maximum_field_size(region.get_blobs(), max_field);
    return max_field;
  }

  template <typename INT>
  void output_boundary_sideset(Ioss::SideBlock *sb, const std::vector<Ioss::Face> &boundary,
                               INT /* dummy */)
  {
    std::vector<INT> el_side;
    el_side.reserve(boundary.size() * 2);
    for (const auto &face : boundary) {
      el_side.push_back(face.element[0] / 10);
      el_side.push_back(face.element[0] % 10 + 1);
    }
    sb->put_field_data("element_side", el_side);
  }
} // namespace

void Ioss::transfer_coordinate_frames(Ioss::Region &region, Ioss::Region &output_region)
{
  const Ioss::CoordinateFrameContainer &cf = region.get_coordinate_frames();
  for (const auto &frame : cf) {
    output_region.add(frame);
  }
}

void Ioss::transfer_assemblies(Ioss::Region &region, Ioss::Region &output_region,
                               const Ioss::MeshCopyOptions &options, int rank)
{
  const auto &assem = region.get_assemblies();
  if (!assem.empty()) {
    for (const auto &assm : assem) {
      const std::string &name = assm->name();
      if (options.debug && rank == 0) {
        fmt::print(Ioss::DebugOut(), "{}, ", name);
      }

      // NOTE: Can't totally use the copy constructor as it will
      // create a members list containing entities from input
      // database.  We need corresponding entities from output
      // database...
      auto o_assem = new Ioss::Assembly(*assm);
      o_assem->remove_members();

      // Now, repopulate member list with corresponding entities from output database...
      const auto &members = assm->get_members();
      for (const auto &member : members) {
        const auto *entity = output_region.get_entity(member->name(), member->type());
        if (entity != nullptr) {
          o_assem->add(entity);
        }
      }
      output_region.add(o_assem);
    }

    if (options.output_summary && rank == 0) {
      fmt::print(Ioss::DebugOut(), " Number of {:20s} = {:14}\n", "Assemblies",
                 fmt::group_digits(assem.size()));
    }
    if (options.debug && rank == 0) {
      fmt::print(Ioss::DebugOut(), "\n");
    }
  }
}

void Ioss::transfer_blobs(Ioss::Region &region, Ioss::Region &output_region,
                          const Ioss::MeshCopyOptions &options, int rank)
{
  const auto &blobs = region.get_blobs();
  if (!blobs.empty()) {
    size_t total_entities = 0;
    for (const auto &blob : blobs) {
      const std::string &name = blob->name();
      if (options.debug && rank == 0) {
        fmt::print(Ioss::DebugOut(), "{}, ", name);
      }
      size_t count = blob->entity_count();
      total_entities += count;
      auto o_blob = new Ioss::Blob(*blob);
      output_region.add(o_blob);
    }

    if (options.output_summary && rank == 0) {
      fmt::print(Ioss::DebugOut(), " Number of {:20s} = {:14}",
                 (*blobs.begin())->type_string() + "s", fmt::group_digits(blobs.size()));
      fmt::print(Ioss::DebugOut(), "\tLength of entity list = {:14}\n",
                 fmt::group_digits(total_entities));
    }
    if (options.debug && rank == 0) {
      fmt::print(Ioss::DebugOut(), "\n");
    }
  }
}

void Ioss::copy_database(Ioss::Region &region, Ioss::Region &output_region,
                         Ioss::MeshCopyOptions &options)
{

  Ioss::DatabaseIO *dbi  = region.get_database();
  int               rank = dbi->util().parallel_rank();

  // Minimize number of times that we grow the memory buffer used for transferring field data.
  auto max_field = calculate_maximum_field_size(region);
  if (options.verbose && rank == 0) {
    std::string label = "MiB";
    double      size  = (double)max_field.first / 1024 / 1024;
    if (size > 1024.0) {
      label = "GiB";
      size /= 1024.0;
    }
    fmt::print(Ioss::DebugOut(), "\n Maximum Field size = {} bytes ({:.3} {}) for field '{}'.\n",
               fmt::group_digits(max_field.first), size, label, max_field.second);
  }

  DataPool data_pool;
  data_pool.data.resize(max_field.first);
  if (options.verbose && rank == 0) {
    fmt::print(Ioss::DebugOut(), " Resize finished...\n");
  }

  std::vector<Ioss::Face> boundary = generate_boundary_faces(region, options);
  if (options.define_geometry) {
    define_model(region, output_region, data_pool, boundary, options, rank);
  }
  bool appending = output_region.get_database()->open_create_behavior() == Ioss::DB_APPEND;
  if (!appending) {
    transfer_model(region, output_region, data_pool, boundary, options, rank);

    if (options.add_proc_id) {
      Ioss::Utils::clear(data_pool.data);
      add_proc_id(output_region, rank);
      return;
    }

    if (options.delete_timesteps) {
      Ioss::Utils::clear(data_pool.data);
      return;
    }
  } // !appending

  if (options.define_geometry) {
    define_transient_fields(region, output_region, options, rank);
  }

  output_region.begin_mode(Ioss::STATE_TRANSIENT);
  if (options.debug && rank == 0) {
    fmt::print(Ioss::DebugOut(), "TRANSFERRING TRANSIENT FIELDS ... \n");
  }
  dbi->progress("TRANSFERRING TRANSIENT FIELDS... ");

  // Get the timesteps from the input database.  Step through them
  // and transfer fields to output database...
  // `selected_steps` specifies whether an input step should be transferred
  // to the output region based on values in `options`
  std::vector<int> selected_steps = get_selected_steps(region, options);

  int step_count = region.get_property("state_count").get_int();
#ifdef SEACAS_HAVE_MPI
  int min_step_count = dbi->util().global_minmax(step_count, Ioss::ParallelUtils::DO_MIN);
  int max_step_count = dbi->util().global_minmax(step_count, Ioss::ParallelUtils::DO_MAX);
  if (min_step_count != max_step_count) {
    std::ostringstream errmsg;
    fmt::print(errmsg,
               "ERROR: Number of timesteps does not match on all ranks.  Range from {} to {}.\n",
               min_step_count, max_step_count);
    IOSS_ERROR(errmsg);
  }
#endif
  for (int istep = 1; istep <= step_count; istep++) {
    if (selected_steps[istep] == 1) {
      transfer_step(region, output_region, data_pool, istep, options, rank);
    }
  }

  if (options.debug && rank == 0) {
    fmt::print(Ioss::DebugOut(), "END STATE_TRANSIENT... \n");
  }
  dbi->progress("END STATE_TRANSIENT (begin) ... ");

  output_region.end_mode(Ioss::STATE_TRANSIENT);
  dbi->progress("END STATE_TRANSIENT (end) ... ");
  Ioss::Utils::clear(data_pool.data);

  if (rank == 0 && options.output_summary) {
    fmt::print(std::cout, "\n\n Output Region summary for rank 0:");
    output_region.output_summary(std::cout);
  }
}

namespace {
  std::vector<int> get_selected_steps(Ioss::Region &region, const Ioss::MeshCopyOptions &options)
  {
    // This routine checks all steps of the input database and selects those which
    // meet the requirements specified in `options`.  The returned (1-based) vector will have a
    // value of `1` if the step is to be output and `0` if skipped.
    int              step_count = region.get_property("state_count").get_int();
    std::vector<int> selected_steps(step_count + 1);

    // If user specified a list of times to transfer to output database,
    // process the list and find the times on the input database that are
    // closest to the times in the list.
    if (!options.selected_times.empty()) {
      int selected_step = 0;
      for (auto time : options.selected_times) {
        double diff = std::numeric_limits<double>::max();
        for (int step = 1; step <= step_count; step++) {
          double db_time  = region.get_state_time(step);
          double cur_diff = std::abs(db_time - time);
          if (cur_diff < diff) {
            diff          = std::abs(db_time - time);
            selected_step = step;
          }
        }
        if (selected_step > 0) {
          selected_steps[selected_step] = 1;
        }
      }
    }
    else {
      // User did not select specific times to be output...
      // Just select them all
      for (int i = 1; i <= step_count; i++) {
        selected_steps[i] = 1;
      }
    }

    // Now, filter by min and max time...
    for (int istep = 1; istep <= step_count; istep++) {
      double time = region.get_state_time(istep);
      if (time < options.minimum_time) {
        selected_steps[istep] = 0;
      }
      if (time > options.maximum_time) {
        selected_steps[istep] = 0;
      }
    }
    return selected_steps;
  }

  template <typename T> void transfer_mesh_info(const T *input, T *output)
  {
    transfer_properties(input, output);
    transfer_fields(input, output, Ioss::Field::MESH);
    transfer_fields(input, output, Ioss::Field::MAP);
    transfer_fields(input, output, Ioss::Field::ATTRIBUTE);
    transfer_fields(input, output, Ioss::Field::MESH_REDUCTION);
  }

  std::vector<Ioss::Face> generate_boundary_faces(Ioss::Region                &region,
                                                  const Ioss::MeshCopyOptions &options)
  {
    std::vector<Ioss::Face> boundary;
    if (options.define_geometry && options.boundary_sideset) {
      Ioss::FaceGenerator face_generator(region);
      if (region.get_database()->int_byte_size_api() == 4) {
        face_generator.generate_faces((int)0, false);
      }
      else {
        face_generator.generate_faces((int64_t)0, false);
      }

      // Get vector of all boundary faces which will be output as the skin...
      auto &faces = face_generator.faces("ALL");
      for (auto &face : faces) {
        if (face.elementCount_ == 1) {
          boundary.push_back(face);
        }
      }
    }
    return boundary;
  }

  void define_model(Ioss::Region &region, Ioss::Region &output_region, Ioss::DataPool &data_pool,
                    const std::vector<Ioss::Face> &boundary, const Ioss::MeshCopyOptions &options,
                    int rank)
  {
    if (options.debug && rank == 0) {
      fmt::print(Ioss::DebugOut(), "DEFINING MODEL ... \n");
    }
    Ioss::DatabaseIO *dbi = region.get_database();
    dbi->progress("DEFINING MODEL");
    if (!output_region.begin_mode(Ioss::STATE_DEFINE_MODEL)) {
      if (options.verbose) {
        std::ostringstream errmsg;
        fmt::print(errmsg, "ERROR: Could not put output region into define model state\n");
        IOSS_ERROR(errmsg);
      }
      else {
        std::exit(EXIT_FAILURE);
      }
    }

    // Get all properties of input database...
    transfer_properties(&region, &output_region);
    transfer_qa_info(region, output_region);

    if (rank == 0 && options.output_summary) {
      fmt::print(std::cout, "\n\n Input Region summary for rank 0:\n");
    }
    transfer_nodeblock(region, output_region, data_pool, options, rank);

#ifdef SEACAS_HAVE_MPI
    // This also assumes that the node order and count is the same for input
    // and output regions... (This is checked during nodeset output)
    if (output_region.get_database()->needs_shared_node_information()) {
      if (options.ints_64_bit)
        set_owned_node_count(region, rank, (int64_t)0);
      else
        set_owned_node_count(region, rank, (int)0);
    }
#endif

    transfer_edgeblocks(region, output_region, options, rank);
    transfer_faceblocks(region, output_region, options, rank);
    transfer_elementblocks(region, output_region, options, rank);
    transfer_structuredblocks(region, output_region, options, rank);

    transfer_nodesets(region, output_region, options, rank);
    transfer_edgesets(region, output_region, options, rank);
    transfer_facesets(region, output_region, options, rank);
    transfer_elemsets(region, output_region, options, rank);

    transfer_sidesets(region, output_region, options, rank);

    if (options.define_geometry && options.boundary_sideset) {
      // Get topology of the sideset faces. Using just block[0] since for what we are doing, doesn't
      // really matter.
      const auto &blocks    = region.get_element_blocks();
      auto        topo      = blocks[0]->topology();
      auto        elem_topo = topo->name();
      auto        face_topo = topo->boundary_type(0)->name();

      auto ss = new Ioss::SideSet(output_region.get_database(), "boundary");
      output_region.add(ss);
      auto sb = new Ioss::SideBlock(output_region.get_database(), "boundary", face_topo, elem_topo,
                                    boundary.size());
      ss->add(sb);
    }

    transfer_commsets(region, output_region, options, rank);

    transfer_coordinate_frames(region, output_region);
    transfer_blobs(region, output_region, options, rank);

    // This must be last...
    transfer_assemblies(region, output_region, options, rank);

    if (options.debug && rank == 0) {
      fmt::print(Ioss::DebugOut(), "END STATE_DEFINE_MODEL...\n");
    }
    dbi->progress("END STATE_DEFINE_MODEL");

    output_region.end_mode(Ioss::STATE_DEFINE_MODEL);
    dbi->progress("output_region.end_mode(Ioss::STATE_DEFINE_MODEL) finished");
  }

  void transfer_model(Ioss::Region &region, Ioss::Region &output_region, Ioss::DataPool &data_pool,
                      const std::vector<Ioss::Face> &boundary, const Ioss::MeshCopyOptions &options,
                      int rank)
  {
    if (options.debug && rank == 0) {
      fmt::print(Ioss::DebugOut(), "TRANSFERRING MESH FIELD DATA ...\n");
    }
    Ioss::DatabaseIO *dbi = region.get_database();
    dbi->progress("TRANSFERRING MESH FIELD DATA ... ");

    // Model defined, now fill in the model data...
    output_region.begin_mode(Ioss::STATE_MODEL);

    // Transfer MESH field_data from input to output...
    // Transfer MESH field_data from input to output...
    bool node_major = output_region.node_major();

    if (!node_major) {
      transfer_field_data(region.get_element_blocks(), output_region, data_pool, Ioss::Field::MESH,
                          options);
      transfer_field_data(region.get_element_blocks(), output_region, data_pool,
                          Ioss::Field::ATTRIBUTE, options);
      transfer_field_data(region.get_element_blocks(), output_region, data_pool, Ioss::Field::MAP,
                          options);
    }

    if (region.mesh_type() != Ioss::MeshType::STRUCTURED) {
      transfer_field_data(region.get_node_blocks(), output_region, data_pool, Ioss::Field::MESH,
                          options);
      transfer_field_data(region.get_node_blocks(), output_region, data_pool,
                          Ioss::Field::ATTRIBUTE, options);
      transfer_field_data(region.get_node_blocks(), output_region, data_pool, Ioss::Field::MAP,
                          options);
    }

    if (node_major) {
      transfer_field_data(region.get_element_blocks(), output_region, data_pool, Ioss::Field::MESH,
                          options);
      transfer_field_data(region.get_element_blocks(), output_region, data_pool,
                          Ioss::Field::ATTRIBUTE, options);
      transfer_field_data(region.get_element_blocks(), output_region, data_pool, Ioss::Field::MAP,
                          options);
    }

    // Structured Blocks -- Contain a NodeBlock that also needs its field data transferred...
    const auto &sbs = region.get_structured_blocks();
    for (const auto &isb : sbs) {
      const std::string &name = isb->name();
      if (options.debug && rank == 0) {
        fmt::print(Ioss::DebugOut(), "{}, ", name);
      }
      // Find matching output structured block
      Ioss::StructuredBlock *osb = output_region.get_structured_block(name);
      if (osb != nullptr) {
        transfer_field_data(isb, osb, data_pool, Ioss::Field::MESH, options);
        transfer_field_data(isb, osb, data_pool, Ioss::Field::ATTRIBUTE, options);

        auto &inb = isb->get_node_block();
        auto &onb = osb->get_node_block();
        if (options.debug && rank == 0) {
          fmt::print(Ioss::DebugOut(), "NB: {}, ", inb.name());
        }

        transfer_field_data(&inb, &onb, data_pool, Ioss::Field::MESH, options);
        transfer_field_data(&inb, &onb, data_pool, Ioss::Field::ATTRIBUTE, options);
      }
    }

    transfer_field_data(region.get_assemblies(), output_region, data_pool, Ioss::Field::MESH,
                        options);
    transfer_field_data(region.get_assemblies(), output_region, data_pool, Ioss::Field::ATTRIBUTE,
                        options);

    transfer_field_data(region.get_blobs(), output_region, data_pool, Ioss::Field::MESH, options);
    transfer_field_data(region.get_blobs(), output_region, data_pool, Ioss::Field::ATTRIBUTE,
                        options);

    transfer_field_data(region.get_edge_blocks(), output_region, data_pool, Ioss::Field::MESH,
                        options);
    transfer_field_data(region.get_edge_blocks(), output_region, data_pool, Ioss::Field::ATTRIBUTE,
                        options);
    transfer_field_data(region.get_edge_blocks(), output_region, data_pool, Ioss::Field::MAP,
                        options);

    transfer_field_data(region.get_face_blocks(), output_region, data_pool, Ioss::Field::MESH,
                        options);
    transfer_field_data(region.get_face_blocks(), output_region, data_pool, Ioss::Field::ATTRIBUTE,
                        options);
    transfer_field_data(region.get_face_blocks(), output_region, data_pool, Ioss::Field::MAP,
                        options);

    transfer_field_data(region.get_nodesets(), output_region, data_pool, Ioss::Field::MESH,
                        options);
    transfer_field_data(region.get_nodesets(), output_region, data_pool, Ioss::Field::ATTRIBUTE,
                        options);

    transfer_field_data(region.get_edgesets(), output_region, data_pool, Ioss::Field::MESH,
                        options);
    transfer_field_data(region.get_edgesets(), output_region, data_pool, Ioss::Field::ATTRIBUTE,
                        options);

    transfer_field_data(region.get_facesets(), output_region, data_pool, Ioss::Field::MESH,
                        options);
    transfer_field_data(region.get_facesets(), output_region, data_pool, Ioss::Field::ATTRIBUTE,
                        options);

    transfer_field_data(region.get_elementsets(), output_region, data_pool, Ioss::Field::MESH,
                        options);
    transfer_field_data(region.get_elementsets(), output_region, data_pool, Ioss::Field::ATTRIBUTE,
                        options);

    transfer_field_data(region.get_commsets(), output_region, data_pool, Ioss::Field::MESH,
                        options);
    transfer_field_data(region.get_commsets(), output_region, data_pool, Ioss::Field::ATTRIBUTE,
                        options);
    transfer_field_data(region.get_commsets(), output_region, data_pool, Ioss::Field::COMMUNICATION,
                        options);

    // Side Sets
    if (region.mesh_type() == Ioss::MeshType::UNSTRUCTURED) {
      const auto &fss = region.get_sidesets();
      for (const auto &ifs : fss) {
        const std::string &name = ifs->name();
        if (options.debug && rank == 0) {
          fmt::print(Ioss::DebugOut(), "{}, ", name);
        }
        // Find matching output sideset
        Ioss::SideSet *ofs = output_region.get_sideset(name);

        if (ofs != nullptr) {
          transfer_field_data(ifs, ofs, data_pool, Ioss::Field::MESH, options);
          transfer_field_data(ifs, ofs, data_pool, Ioss::Field::ATTRIBUTE, options);

          const auto &fbs = ifs->get_side_blocks();
          for (const auto &ifb : fbs) {

            // Find matching output sideblock
            const std::string &fbname = ifb->name();
            if (options.debug && rank == 0) {
              fmt::print(Ioss::DebugOut(), "{}, ", fbname);
            }
            Ioss::SideBlock *ofb = ofs->get_side_block(fbname);

            if (ofb != nullptr) {
              transfer_field_data(ifb, ofb, data_pool, Ioss::Field::MESH, options);
              transfer_field_data(ifb, ofb, data_pool, Ioss::Field::ATTRIBUTE, options);
            }
          }
        }
      }
      if (options.debug && rank == 0) {
        fmt::print(Ioss::DebugOut(), "\n");
      }

      if (options.define_geometry && options.boundary_sideset) {
        auto *ss = output_region.get_sideset("boundary");
        if (ss != nullptr) {
          auto sb = ss->get_side_block("boundary");
          if (output_region.get_database()->int_byte_size_api() == 4) {
            output_boundary_sideset(sb, boundary, (int)0);
          }
          else {
            output_boundary_sideset(sb, boundary, (int64_t)0);
          }
        }
      }
    }
    if (options.debug && rank == 0) {
      fmt::print(Ioss::DebugOut(), "END STATE_MODEL... \n");
    }
    dbi->progress("END STATE_MODEL... ");
    output_region.end_mode(Ioss::STATE_MODEL);
  }

  void define_transient_fields(Ioss::Region &region, Ioss::Region &output_region,
                               const Ioss::MeshCopyOptions &options, int rank)
  {
    if (options.debug && rank == 0) {
      fmt::print(Ioss::DebugOut(), "DEFINING TRANSIENT FIELDS ... \n");
    }

    Ioss::DatabaseIO *dbi = region.get_database();
    dbi->progress("DEFINING TRANSIENT FIELDS ... ");

    if (region.property_exists("state_count") && region.get_property("state_count").get_int() > 0) {
      if (options.output_summary && rank == 0) {
        fmt::print(Ioss::DebugOut(), "\n Number of time steps on database = {}\n",
                   region.get_property("state_count").get_int());
      }

      output_region.begin_mode(Ioss::STATE_DEFINE_TRANSIENT);

      // NOTE: For most types, the fields are transferred from input to output
      //       via the copy constructor.  The "special" ones are handled here.
      // The below lines handle both methods of handling global variables...
      transfer_fields(&region, &output_region, Ioss::Field::REDUCTION);
      transfer_fields(&region, &output_region, Ioss::Field::TRANSIENT);

      // Structured Blocks -- Contain a NodeBlock that also needs its fields transferred...
      const auto &sbs = region.get_structured_blocks();
      for (const auto &isb : sbs) {

        // Find matching output structured block
        const std::string     &name = isb->name();
        Ioss::StructuredBlock *osb  = output_region.get_structured_block(name);
        if (osb != nullptr) {
          transfer_fields(isb, osb, Ioss::Field::TRANSIENT);
          transfer_fields(isb, osb, Ioss::Field::REDUCTION);

          auto &inb = isb->get_node_block();
          auto &onb = osb->get_node_block();
          transfer_fields(&inb, &onb, Ioss::Field::TRANSIENT);
          transfer_fields(&inb, &onb, Ioss::Field::REDUCTION);
        }
      }

      if (options.debug && rank == 0) {
        fmt::print(Ioss::DebugOut(), "END STATE_DEFINE_TRANSIENT... \n");
      }
      dbi->progress("END STATE_DEFINE_TRANSIENT... ");
      output_region.end_mode(Ioss::STATE_DEFINE_TRANSIENT);
    }
  }
  void transfer_step(Ioss::Region &region, Ioss::Region &output_region, Ioss::DataPool &data_pool,
                     int istep, const Ioss::MeshCopyOptions &options, int rank)
  {
    double time  = region.get_state_time(istep);
    int    ostep = output_region.add_state(time);
    show_step(istep, time, options, rank);

    output_region.begin_state(ostep);
    region.begin_state(istep);

    for (int i = 0; i < 2; i++) {
      auto field_type = Ioss::Field::TRANSIENT;
      if (i > 0) {
        field_type = Ioss::Field::REDUCTION;
      }

      transfer_field_data(&region, &output_region, data_pool, field_type, options);

      transfer_field_data(region.get_assemblies(), output_region, data_pool, field_type, options);
      transfer_field_data(region.get_blobs(), output_region, data_pool, field_type, options);

      if (region.mesh_type() != Ioss::MeshType::STRUCTURED) {
        transfer_field_data(region.get_node_blocks(), output_region, data_pool, field_type,
                            options);
      }
      transfer_field_data(region.get_edge_blocks(), output_region, data_pool, field_type, options);
      transfer_field_data(region.get_face_blocks(), output_region, data_pool, field_type, options);
      transfer_field_data(region.get_element_blocks(), output_region, data_pool, field_type,
                          options);

      {
        // Structured Blocks -- handle embedded NodeBlock also.
        const auto &sbs = region.get_structured_blocks();
        for (const auto &isb : sbs) {
          const std::string &name = isb->name();
          if (options.debug && rank == 0) {
            fmt::print(Ioss::DebugOut(), "{}, ", name);
          }
          // Find matching output structured block
          Ioss::StructuredBlock *osb = output_region.get_structured_block(name);
          if (osb != nullptr) {
            transfer_field_data(isb, osb, data_pool, field_type, options);

            auto &inb = isb->get_node_block();
            auto &onb = osb->get_node_block();
            transfer_field_data(&inb, &onb, data_pool, field_type, options);
          }
        }
      }

      transfer_field_data(region.get_nodesets(), output_region, data_pool, field_type, options);
      transfer_field_data(region.get_edgesets(), output_region, data_pool, field_type, options);
      transfer_field_data(region.get_facesets(), output_region, data_pool, field_type, options);
      transfer_field_data(region.get_elementsets(), output_region, data_pool, field_type, options);

      // Side Sets
      {
        const auto &fss = region.get_sidesets();
        for (const auto &ifs : fss) {
          const std::string &name = ifs->name();
          if (options.debug && rank == 0) {
            fmt::print(Ioss::DebugOut(), "{}, ", name);
          }

          // Find matching output sideset
          Ioss::SideSet *ofs = output_region.get_sideset(name);
          if (ofs != nullptr) {
            transfer_field_data(ifs, ofs, data_pool, field_type, options);

            const auto &fbs = ifs->get_side_blocks();
            for (const auto &ifb : fbs) {

              // Find matching output sideblock
              const std::string &fbname = ifb->name();
              if (options.debug && rank == 0) {
                fmt::print(Ioss::DebugOut(), "{}, ", fbname);
              }

              Ioss::SideBlock *ofb = ofs->get_side_block(fbname);
              if (ofb != nullptr) {
                transfer_field_data(ifb, ofb, data_pool, field_type, options);
              }
            }
          }
        }
      }
    }
    region.end_state(istep);
    output_region.end_state(ostep);

    if (options.delay > 0.0) {
      std::this_thread::sleep_for(
          std::chrono::milliseconds(static_cast<int>(options.delay * 1000)));
    }
  }

  void transfer_nodeblock(Ioss::Region &region, Ioss::Region &output_region, Ioss::DataPool &pool,
                          const Ioss::MeshCopyOptions &options, int rank)
  {
    const auto &nbs = region.get_node_blocks();
    for (const auto &inb : nbs) {
      const std::string &name = inb->name();
      if (options.debug && rank == 0) {
        fmt::print(Ioss::DebugOut(), "{}, ", name);
      }
      size_t num_nodes = inb->entity_count();
      size_t degree    = inb->get_property("component_degree").get_int();
      if (options.output_summary && rank == 0) {
        fmt::print(Ioss::DebugOut(), " Number of Coordinates per Node = {:14}\n",
                   fmt::group_digits(degree));
        fmt::print(Ioss::DebugOut(), " Number of Nodes                = {:14}\n",
                   fmt::group_digits(num_nodes));
      }
      auto *nb = new Ioss::NodeBlock(*inb);
      output_region.add(nb);

      if (output_region.get_database()->needs_shared_node_information()) {
        // If the "owning_processor" field exists on the input
        // nodeblock, transfer it and the "ids" field to the output
        // nodeblock at this time since it is used to determine
        // per-processor sizes of nodeblocks and nodesets.
        if (inb->field_exists("owning_processor")) {
          size_t isize = inb->get_field("ids").get_size();
          pool.data.resize(isize);
          inb->get_field_data("ids", pool.data.data(), isize);
          nb->put_field_data("ids", pool.data.data(), isize);

          isize = inb->get_field("owning_processor").get_size();
          pool.data.resize(isize);
          inb->get_field_data("owning_processor", pool.data.data(), isize);
          nb->put_field_data("owning_processor", pool.data.data(), isize);
        }
      }
    }
    if (options.debug && rank == 0) {
      fmt::print(Ioss::DebugOut(), "\n");
    }
  }

  template <typename T>
  void transfer_field_data(const std::vector<T *> &entities, Ioss::Region &output_region,
                           Ioss::DataPool &pool, Ioss::Field::RoleType role,
                           const Ioss::MeshCopyOptions &options)
  {
    for (const auto &entity : entities) {
      const std::string &name = entity->name();

      // Find the corresponding output block...
      Ioss::GroupingEntity *output = output_region.get_entity(name, entity->type());
      if (output != nullptr) {
        transfer_field_data(entity, output, pool, role, options);
      }
    }
  }

  template <typename T>
  void transfer_blocks(const std::vector<T *> &blocks, Ioss::Region &output_region,
                       const Ioss::MeshCopyOptions &options, int rank)
  {
    if (!blocks.empty()) {
      size_t total_entities = 0;
      for (const auto &iblock : blocks) {
        const std::string &name = iblock->name();
        if (options.debug && rank == 0) {
          fmt::print(Ioss::DebugOut(), "{}, ", name);
        }
        size_t count = iblock->entity_count();
        total_entities += count;

        auto block = new T(*iblock);
        output_region.add(block);
      }
      if (options.output_summary && rank == 0) {
        fmt::print(Ioss::DebugOut(), " Number of {:20s} = {:14}\n",
                   (*blocks.begin())->type_string() + "s", fmt::group_digits(blocks.size()));
        fmt::print(Ioss::DebugOut(), " Number of {:20s} = {:14}\n",
                   (*blocks.begin())->contains_string() + "s", fmt::group_digits(total_entities));
      }
      if (options.debug && rank == 0) {
        fmt::print(Ioss::DebugOut(), "\n");
      }
    }
  }

  void transfer_structuredblocks(Ioss::Region &region, Ioss::Region &output_region,
                                 const Ioss::MeshCopyOptions &options, int rank)
  {
    const auto &blocks = region.get_structured_blocks();
    if (!blocks.empty()) {
      size_t total_entities = 0;
      if (options.reverse) {
        // Defines the CGNS zones in the reverse order they
        // were read from the input mesh.  This is used in
        // testing to verify that we handle zone reordering
        // correctly.
        for (int i = blocks.size() - 1; i >= 0; i--) {
          const auto        &iblock = blocks[i];
          const std::string &name   = iblock->name();
          if (options.debug && rank == 0) {
            fmt::print(Ioss::DebugOut(), "{}, ", name);
          }
          size_t count = iblock->entity_count();
          total_entities += count;

          auto block = iblock->clone(output_region.get_database());
          output_region.add(block);
          transfer_mesh_info(iblock, block);

          // Now do the transfer on the NodeBlock contained in the StructuredBlock
          auto &inb = iblock->get_node_block();
          auto &onb = block->get_node_block();
          if (options.debug && rank == 0) {
            fmt::print(Ioss::DebugOut(), "(NB: {}), ", inb.name());
          }
          transfer_mesh_info(&inb, &onb);
        }
      }
      else {
        for (const auto &iblock : blocks) {
          const std::string &name = iblock->name();
          if (options.debug && rank == 0) {
            fmt::print(Ioss::DebugOut(), "{}, ", name);
          }
          size_t count = iblock->entity_count();
          total_entities += count;

          auto block = iblock->clone(output_region.get_database());
          output_region.add(block);
          transfer_mesh_info(iblock, block);

          // Now do the transfer on the NodeBlock contained in the StructuredBlock
          auto &inb = iblock->get_node_block();
          auto &onb = block->get_node_block();
          if (options.debug && rank == 0) {
            fmt::print(Ioss::DebugOut(), "(NB: {}), ", inb.name());
          }
          transfer_mesh_info(&inb, &onb);
        }
      }

      if (options.output_summary && rank == 0) {
        fmt::print(Ioss::DebugOut(), " Number of {:20s} = {:14}\n",
                   (*blocks.begin())->type_string() + "s", fmt::group_digits(blocks.size()));
        fmt::print(Ioss::DebugOut(), " Number of {:20s} = {:14}\n",
                   (*blocks.begin())->contains_string() + "s", fmt::group_digits(total_entities));
      }
      if (options.debug && rank == 0) {
        fmt::print(Ioss::DebugOut(), "\n");
      }
    }
  }

  void transfer_elementblocks(Ioss::Region &region, Ioss::Region &output_region,
                              const Ioss::MeshCopyOptions &options, int rank)
  {
    const auto &ebs = region.get_element_blocks();
    transfer_blocks(ebs, output_region, options, rank);
  }

  void transfer_edgeblocks(Ioss::Region &region, Ioss::Region &output_region,
                           const Ioss::MeshCopyOptions &options, int rank)
  {
    const auto &ebs = region.get_edge_blocks();
    transfer_blocks(ebs, output_region, options, rank);
  }

  void transfer_faceblocks(Ioss::Region &region, Ioss::Region &output_region,
                           const Ioss::MeshCopyOptions &options, int rank)
  {
    const auto &ebs = region.get_face_blocks();
    transfer_blocks(ebs, output_region, options, rank);
  }

  void transfer_sidesets(Ioss::Region &region, Ioss::Region &output_region,
                         const Ioss::MeshCopyOptions &options, int rank)
  {
    const auto &fss = region.get_sidesets();
    for (const auto &ss : fss) {
      const std::string &name = ss->name();
      if (options.debug && rank == 0) {
        fmt::print(Ioss::DebugOut(), "{}, ", name);
      }
      auto surf = new Ioss::SideSet(*ss);
      output_region.add(surf);

      // Fix up the optional 'owner_block' in copied SideBlocks...
      const auto &fbs = ss->get_side_blocks();
      for (const auto &ifb : fbs) {
        if (ifb->parent_block() != nullptr) {
          auto  fb_name = ifb->parent_block()->name();
          auto *parent  = dynamic_cast<Ioss::EntityBlock *>(
              output_region.get_entity(fb_name, Ioss::ELEMENTBLOCK));
          if (parent == nullptr) {
            parent = dynamic_cast<Ioss::EntityBlock *>(
                output_region.get_entity(fb_name, Ioss::STRUCTUREDBLOCK));
          }

          auto *ofb = surf->get_side_block(ifb->name());
          ofb->set_parent_block(parent);
        }
      }
    }

    if (options.output_summary && rank == 0 && !fss.empty()) {
      fmt::print(Ioss::DebugOut(), " Number of {:20s} = {:14}\n",
                 (*fss.begin())->type_string() + "s", fmt::group_digits(fss.size()));
    }
    if (options.debug && rank == 0) {
      fmt::print(Ioss::DebugOut(), "\n");
    }
  }

  template <typename T>
  void transfer_sets(const std::vector<T *> &sets, Ioss::Region &output_region,
                     const Ioss::MeshCopyOptions &options, int rank)
  {
    if (!sets.empty()) {
      size_t total_entities = 0;
      for (const auto &set : sets) {
        const std::string &name = set->name();
        if (options.debug && rank == 0) {
          fmt::print(Ioss::DebugOut(), "{}, ", name);
        }
        size_t count = set->entity_count();
        total_entities += count;
        auto o_set = new T(*set);
        output_region.add(o_set);
      }

      if (options.output_summary && rank == 0) {
        fmt::print(Ioss::DebugOut(), " Number of {:20s} = {:14}",
                   (*sets.begin())->type_string() + "s", fmt::group_digits(sets.size()));
        fmt::print(Ioss::DebugOut(), "\tLength of entity list = {:14}\n",
                   fmt::group_digits(total_entities));
      }
      if (options.debug && rank == 0) {
        fmt::print(Ioss::DebugOut(), "\n");
      }
    }
  }

  void transfer_nodesets(Ioss::Region &region, Ioss::Region &output_region,
                         const Ioss::MeshCopyOptions &options, int rank)
  {
    const auto &nss = region.get_nodesets();
    transfer_sets(nss, output_region, options, rank);
  }

  void transfer_edgesets(Ioss::Region &region, Ioss::Region &output_region,
                         const Ioss::MeshCopyOptions &options, int rank)
  {
    const auto &nss = region.get_edgesets();
    transfer_sets(nss, output_region, options, rank);
  }

  void transfer_facesets(Ioss::Region &region, Ioss::Region &output_region,
                         const Ioss::MeshCopyOptions &options, int rank)
  {
    const auto &nss = region.get_facesets();
    transfer_sets(nss, output_region, options, rank);
  }

  void transfer_elemsets(Ioss::Region &region, Ioss::Region &output_region,
                         const Ioss::MeshCopyOptions &options, int rank)
  {
    const auto &nss = region.get_elementsets();
    transfer_sets(nss, output_region, options, rank);
  }

  void transfer_commsets(Ioss::Region &region, Ioss::Region &output_region,
                         const Ioss::MeshCopyOptions &options, int rank)
  {
    const auto &css = region.get_commsets();
    for (const auto &ics : css) {
      if (options.debug && rank == 0) {
        const std::string &name = ics->name();
        fmt::print(Ioss::DebugOut(), "{}, ", name);
      }
      auto cs = new Ioss::CommSet(*ics);
      output_region.add(cs);
    }
    if (options.debug && rank == 0) {
      fmt::print(Ioss::DebugOut(), "\n");
    }
  }

  void transfer_fields(const Ioss::GroupingEntity *ige, Ioss::GroupingEntity *oge,
                       Ioss::Field::RoleType role, const std::string &prefix)
  {
    // Check for transient fields...
    Ioss::NameList fields = ige->field_describe(role);

    // Iterate through results fields and transfer to output
    // database...  If a prefix is specified, only transfer fields
    // whose names begin with the prefix
    for (const auto &field_name : fields) {
      Ioss::Field field = ige->get_field(field_name);
      if (field_name != "ids" && !oge->field_exists(field_name) &&
          Ioss::Utils::substr_equal(prefix, field_name)) {
        // If the field does not already exist, add it to the output node block
        oge->field_add(field);
      }
    }
  }

  void transfer_field_data(Ioss::GroupingEntity *ige, Ioss::GroupingEntity *oge, Ioss::DataPool &pool,
                           Ioss::Field::RoleType role, const Ioss::MeshCopyOptions &options,
                           const std::string &prefix)
  {
    // Iterate through the TRANSIENT-role fields of the input
    // database and transfer to output database.
    Ioss::NameList state_fields = ige->field_describe(role);

    // Complication here is that if the 'role' is 'Ioss::Field::MESH',
    // then the 'ids' field must be transferred first...
    if (role == Ioss::Field::MESH && ige->field_exists("ids")) {
      assert(oge->field_exists("ids"));
      transfer_field_data_internal(ige, oge, pool, "ids", options);
    }

    for (const auto &field_name : state_fields) {
      // All of the 'Ioss::EntityBlock' derived classes have a
      // 'connectivity' field, but it is only interesting on the
      // Ioss::ElementBlock class. On the other classes, it just
      // generates overhead...

      if (field_name == "connectivity" && ige->type() != Ioss::ELEMENTBLOCK) {
        continue;
      }
      if (field_name == "ids") {
        continue;
      }

      if (Ioss::Utils::substr_equal(prefix, field_name)) {
        assert(oge->field_exists(field_name));
        transfer_field_data_internal(ige, oge, pool, field_name, options);
      }
    }
  }

  void transfer_field_data_internal(Ioss::GroupingEntity *ige, Ioss::GroupingEntity *oge,
                                    Ioss::DataPool &pool, const std::string &field_name,
                                    const Ioss::MeshCopyOptions &options)
  {

    size_t isize = ige->get_field(field_name).get_size();
    assert(isize == oge->get_field(field_name).get_size());

    int basic_type = ige->get_field(field_name).get_type();

    if (field_name == "mesh_model_coordinates_x") {
      return;
    }
    if (field_name == "mesh_model_coordinates_y") {
      return;
    }
    if (field_name == "mesh_model_coordinates_z") {
      return;
    }
    if (field_name == "connectivity_raw") {
      return;
    }
    if (field_name == "element_side_raw") {
      return;
    }
    if (field_name == "ids_raw") {
      return;
    }
    if (field_name == "implicit_ids") {
      return;
    }
    if (field_name == "node_connectivity_status") {
      return;
    }
    if (field_name == "owning_processor") {
      return;
    }
    if (field_name == "entity_processor_raw") {
      return;
    }
    if (field_name == "ids" && ige->type() == Ioss::SIDEBLOCK) {
      return;
    }
    if (field_name == "ids" && ige->type() == Ioss::STRUCTUREDBLOCK) {
      return;
    }
    if (field_name == "cell_ids" && ige->type() == Ioss::STRUCTUREDBLOCK) {
      return;
    }
    if (field_name == "cell_node_ids" && ige->type() == Ioss::STRUCTUREDBLOCK) {
      return;
    }

    if (options.data_storage_type == 1 || options.data_storage_type == 2) {
      if (pool.data.size() < isize) {
        pool.data.resize(isize);
      }
    }
    else {
    }

    assert(pool.data.size() >= isize);

    switch (options.data_storage_type) {
    case 1: ige->get_field_data(field_name, pool.data.data(), isize); break;
    case 2:
      if ((basic_type == Ioss::Field::CHARACTER) || (basic_type == Ioss::Field::STRING)) {
        ige->get_field_data(field_name, pool.data);
      }
      else if (basic_type == Ioss::Field::INT32) {
        ige->get_field_data(field_name, pool.data_int);
      }
      else if (basic_type == Ioss::Field::INT64) {
        ige->get_field_data(field_name, pool.data_int64);
      }
      else if (basic_type == Ioss::Field::REAL) {
        ige->get_field_data(field_name, pool.data_double);
      }
      else if (basic_type == Ioss::Field::COMPLEX) {
        ige->get_field_data(field_name, pool.data_complex);
      }
      else {
      }
      break;
#ifdef SEACAS_HAVE_KOKKOS
    case 3:
      if ((basic_type == Ioss::Field::CHARACTER) || (basic_type == Ioss::Field::STRING)) {
        ige->get_field_data<char>(field_name, pool.data_view_char);
      }
      else if (basic_type == Ioss::Field::INT32) {
        ige->get_field_data<int>(field_name, pool.data_view_int);
      }
      else if (basic_type == Ioss::Field::INT64) {
        ige->get_field_data<int64_t>(field_name, pool.data_view_int64);
      }
      else if (basic_type == Ioss::Field::REAL) {
        ige->get_field_data<double>(field_name, pool.data_view_double);
      }
      else if (basic_type == Ioss::Field::COMPLEX) {
        // Since data_view_complex cannot be a global variable.
        ige->get_field_data(field_name, pool.data.data(), isize);
      }
      else {
      }
      break;
    case 4:
      if ((basic_type == Ioss::Field::CHARACTER) || (basic_type == Ioss::Field::STRING)) {
        ige->get_field_data<char>(field_name, pool.data_view_2D_char);
      }
      else if (basic_type == Ioss::Field::INT32) {
        ige->get_field_data<int>(field_name, pool.data_view_2D_int);
      }
      else if (basic_type == Ioss::Field::INT64) {
        ige->get_field_data<int64_t>(field_name, pool.data_view_2D_int64);
      }
      else if (basic_type == Ioss::Field::REAL) {
        ige->get_field_data<double>(field_name, pool.data_view_2D_double);
      }
      else if (basic_type == Ioss::Field::COMPLEX) {
        // Since data_view_complex cannot be a global variable.
        ige->get_field_data(field_name, pool.data.data(), isize);
      }
      else {
      }
      break;
    case 5:
      if ((basic_type == Ioss::Field::CHARACTER) || (basic_type == Ioss::Field::STRING)) {
        ige->get_field_data<char, Kokkos::LayoutRight, Kokkos::HostSpace>(
            field_name, pool.data_view_2D_char_layout_space);
      }
      else if (basic_type == Ioss::Field::INT32) {
        ige->get_field_data<int, Kokkos::LayoutRight, Kokkos::HostSpace>(
            field_name, pool.data_view_2D_int_layout_space);
      }
      else if (basic_type == Ioss::Field::INT64) {
        ige->get_field_data<int64_t, Kokkos::LayoutRight, Kokkos::HostSpace>(
            field_name, pool.data_view_2D_int64_layout_space);
      }
      else if (basic_type == Ioss::Field::REAL) {
        ige->get_field_data<double, Kokkos::LayoutRight, Kokkos::HostSpace>(
            field_name, pool.data_view_2D_double_layout_space);
      }
      else if (basic_type == Ioss::Field::COMPLEX) {
        // Since data_view_complex cannot be a global variable.
        ige->get_field_data(field_name, pool.data.data(), isize);
      }
      else {
      }
      break;
#endif
    default:
      if (field_name == "mesh_model_coordinates") {
        fmt::print(Ioss::DebugOut(), "data_storage option not recognized.");
      }
      return;
    }

    switch (options.data_storage_type) {
    case 1: oge->put_field_data(field_name, pool.data.data(), isize); break;
    case 2:
      if ((basic_type == Ioss::Field::CHARACTER) || (basic_type == Ioss::Field::STRING)) {
        oge->put_field_data(field_name, pool.data);
      }
      else if (basic_type == Ioss::Field::INT32) {
        oge->put_field_data(field_name, pool.data_int);
      }
      else if (basic_type == Ioss::Field::INT64) {
        oge->put_field_data(field_name, pool.data_int64);
      }
      else if (basic_type == Ioss::Field::REAL) {
        oge->put_field_data(field_name, pool.data_double);
      }
      else if (basic_type == Ioss::Field::COMPLEX) {
        oge->put_field_data(field_name, pool.data_complex);
      }
      else {
      }
      break;
#ifdef SEACAS_HAVE_KOKKOS
    case 3:
      if ((basic_type == Ioss::Field::CHARACTER) || (basic_type == Ioss::Field::STRING)) {
        oge->put_field_data<char>(field_name, pool.data_view_char);
      }
      else if (basic_type == Ioss::Field::INT32) {
        oge->put_field_data<int>(field_name, pool.data_view_int);
      }
      else if (basic_type == Ioss::Field::INT64) {
        oge->put_field_data<int64_t>(field_name, pool.data_view_int64);
      }
      else if (basic_type == Ioss::Field::REAL) {
        oge->put_field_data<double>(field_name, pool.data_view_double);
      }
      else if (basic_type == Ioss::Field::COMPLEX) {
        // Since data_view_complex cannot be a global variable.
        oge->put_field_data(field_name, pool.data.data(), isize);
      }
      else {
      }
      break;
    case 4:
      if ((basic_type == Ioss::Field::CHARACTER) || (basic_type == Ioss::Field::STRING)) {
        oge->put_field_data<char>(field_name, pool.data_view_2D_char);
      }
      else if (basic_type == Ioss::Field::INT32) {
        oge->put_field_data<int>(field_name, pool.data_view_2D_int);
      }
      else if (basic_type == Ioss::Field::INT64) {
        oge->put_field_data<int64_t>(field_name, pool.data_view_2D_int64);
      }
      else if (basic_type == Ioss::Field::REAL) {
        oge->put_field_data<double>(field_name, pool.data_view_2D_double);
      }
      else if (basic_type == Ioss::Field::COMPLEX) {
        // Since data_view_complex cannot be a global variable.
        oge->put_field_data(field_name, pool.data.data(), isize);
      }
      else {
      }
      break;
    case 5:
      if ((basic_type == Ioss::Field::CHARACTER) || (basic_type == Ioss::Field::STRING)) {
        oge->put_field_data<char, Kokkos::LayoutRight, Kokkos::HostSpace>(
            field_name, pool.data_view_2D_char_layout_space);
      }
      else if (basic_type == Ioss::Field::INT32) {
        oge->put_field_data<int, Kokkos::LayoutRight, Kokkos::HostSpace>(
            field_name, pool.data_view_2D_int_layout_space);
      }
      else if (basic_type == Ioss::Field::INT64) {
        oge->put_field_data<int64_t, Kokkos::LayoutRight, Kokkos::HostSpace>(
            field_name, pool.data_view_2D_int64_layout_space);
      }
      else if (basic_type == Ioss::Field::REAL) {
        oge->put_field_data<double, Kokkos::LayoutRight, Kokkos::HostSpace>(
            field_name, pool.data_view_2D_double_layout_space);
      }
      else if (basic_type == Ioss::Field::COMPLEX) {
        // Since data_view_complex cannot be a global variable.
        oge->put_field_data(field_name, pool.data.data(), isize);
      }
      else {
      }
      break;
#endif
    default: return;
    }
  }

  void transfer_qa_info(Ioss::Region &in, Ioss::Region &out)
  {
    out.add_information_records(in.get_information_records());

    const std::vector<std::string> &qa = in.get_qa_records();
    for (size_t i = 0; i < qa.size(); i += 4) {
      out.add_qa_record(qa[i + 0], qa[i + 1], qa[i + 2], qa[i + 3]);
    }
  }

  void transfer_properties(const Ioss::GroupingEntity *ige, Ioss::GroupingEntity *oge)
  {
    Ioss::NameList properties = ige->property_describe();

    // Iterate through properties and transfer to output database...
    for (const auto &property : properties) {
      if (!oge->property_exists(property)) {
        oge->property_add(ige->get_property(property));
      }
    }
  }

  void show_step(int istep, double time, const Ioss::MeshCopyOptions &options, int rank)
  {
    if (options.output_summary && rank == 0) {
      fmt::print(Ioss::DebugOut(), "\r\tTime step {:5d} at time {:10.5e}", istep, time);
    }
  }

#ifdef SEACAS_HAVE_MPI
  template <typename INT>
  void set_owned_node_count(Ioss::Region &region, int my_processor, INT /*dummy*/)
  {
    Ioss::NodeBlock *nb = region.get_node_block("nodeblock_1");
    if (nb->field_exists("owning_processor")) {
      std::vector<int> my_data;
      nb->get_field_data("owning_processor", my_data);

      INT owned = std::count(my_data.begin(), my_data.end(), my_processor);
      nb->property_add(Ioss::Property("locally_owned_count", owned));

      // Set locally_owned_count property on all nodesets...
      const Ioss::NodeSetContainer &nss = region.get_nodesets();
      for (const auto &ns : nss) {

        std::vector<INT> ids;
        ns->get_field_data("ids_raw", ids);
        owned = 0;
        for (size_t n = 0; n < ids.size(); n++) {
          INT id = ids[n];
          if (my_data[id - 1] == my_processor) {
            ++owned;
          }
        }
        ns->property_add(Ioss::Property("locally_owned_count", owned));
      }
    }
  }
#endif

  void add_proc_id(Ioss::Region &region, int rank)
  {
    region.begin_mode(Ioss::STATE_DEFINE_TRANSIENT);
    auto &sblocks = region.get_structured_blocks();
    for (auto &sb : sblocks) {
      sb->field_add(
          Ioss::Field("processor_id", Ioss::Field::REAL, "scalar", Ioss::Field::TRANSIENT));
    }

    auto &eblocks = region.get_element_blocks();
    for (auto &eb : eblocks) {
      eb->field_add(
          Ioss::Field("processor_id", Ioss::Field::REAL, "scalar", Ioss::Field::TRANSIENT));
    }
    region.end_mode(Ioss::STATE_DEFINE_TRANSIENT);

    region.begin_mode(Ioss::STATE_TRANSIENT);

    auto step = region.add_state(0.0);
    region.begin_state(step);

    for (auto &sb : sblocks) {
      std::vector<double> proc_id(sb->entity_count(), rank);
      sb->put_field_data("processor_id", proc_id);
    }

    for (auto &eb : eblocks) {
      std::vector<double> proc_id(eb->entity_count(), rank);
      eb->put_field_data("processor_id", proc_id);
    }

    region.end_state(step);
    region.end_mode(Ioss::STATE_TRANSIENT);
  }
} // namespace
