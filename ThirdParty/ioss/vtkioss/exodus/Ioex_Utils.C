// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_ElementTopology.h"
#include "Ioss_Region.h"
#include "Ioss_SmartAssert.h"
#include "Ioss_Utils.h"
#include "Ioss_VariableType.h"
#include "exodus/Ioex_Utils.h"
#include <cstring>
#include "vtk_fmt.h"
#include VTK_FMT(fmt/core.h)
#include VTK_FMT(fmt/format.h)
#include VTK_FMT(fmt/ostream.h)
#include <iosfwd>
#include <vtk_netcdf.h> // for NC_NOERR, nc_def_var, etc
#if defined(_WIN32) && !defined(__MINGW32__)
#include <string.h>
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#else
#include <strings.h>
#endif
#include <tokenize.h>

#include "Ioss_BasisVariableType.h"
#include "Ioss_CoordinateFrame.h"
#include "Ioss_DatabaseIO.h"
#include "Ioss_ElementBlock.h"
#include "Ioss_Field.h"
#include "Ioss_GroupingEntity.h"
#include "Ioss_NamedSuffixVariableType.h"
#include "Ioss_ParallelUtils.h"
#include "Ioss_Property.h"
#include "Ioss_QuadratureVariableType.h"
#include "vtk_exodusII.h"
#include "exodusII_int.h"

namespace {
  size_t match(const std::string &name1, const std::string &name2)
  {
    size_t l1  = name1.size();
    size_t l2  = name2.size();
    size_t len = std::min(l1, l2);
    for (size_t i = 0; i < len; i++) {
      if (name1[i] != name2[i]) {
        while (i > 0 && (isdigit(name1[i - 1]) != 0) && (isdigit(name2[i - 1]) != 0)) {
          i--;
          // Back up to first non-digit so to handle "evar0000, evar0001, ..., evar 1123"
        }
        return i;
      }
    }
    return len;
  }

  template <typename INT>
  void internal_write_coordinate_frames(int exoid, const Ioss::CoordinateFrameContainer &frames,
                                        INT /*dummy*/)
  {
    // Query number of coordinate frames...
    int nframes = static_cast<int>(frames.size());
    if (nframes > 0) {
      std::vector<char>   tags(nframes);
      std::vector<double> coordinates(nframes * 9);
      std::vector<INT>    ids(nframes);

      for (size_t i = 0; i < frames.size(); i++) {
        ids[i]              = frames[i].id();
        tags[i]             = frames[i].tag();
        const double *coord = frames[i].coordinates();
        for (size_t j = 0; j < 9; j++) {
          coordinates[9 * i + j] = coord[j];
        }
      }
      int ierr = ex_put_coordinate_frames(exoid, nframes, Data(ids), Data(coordinates), Data(tags));
      if (ierr < 0) {
        Ioex::exodus_error(exoid, __LINE__, __func__, __FILE__);
      }
    }
  }

  template <typename INT>
  void internal_add_coordinate_frames(int exoid, Ioss::Region *region, INT /*dummy*/)
  {
    // Query number of coordinate frames...
    int nframes = 0;
    int ierr    = ex_get_coordinate_frames(exoid, &nframes, nullptr, nullptr, nullptr);
    if (ierr < 0) {
      Ioex::exodus_error(exoid, __LINE__, __func__, __FILE__);
    }

    if (nframes > 0) {
      std::vector<char>   tags(nframes);
      std::vector<double> coord(nframes * 9);
      std::vector<INT>    ids(nframes);
      ierr = ex_get_coordinate_frames(exoid, &nframes, Data(ids), Data(coord), Data(tags));
      if (ierr < 0) {
        Ioex::exodus_error(exoid, __LINE__, __func__, __FILE__);
      }

      for (int i = 0; i < nframes; i++) {
        Ioss::CoordinateFrame cf(ids[i], tags[i], &coord[9 * i]);
        region->add(cf);
      }
    }
  }
} // namespace

namespace Ioex {
  void update_last_time_attribute(int exodusFilePtr, double value)
  {
    double tmp    = 0.0;
    int    rootid = static_cast<unsigned>(exodusFilePtr) & EX_FILE_ID_MASK;
    int    status = nc_get_att_double(rootid, NC_GLOBAL, "last_written_time", &tmp);

    if (status == NC_NOERR && value > tmp) {
      status = nc_put_att_double(rootid, NC_GLOBAL, "last_written_time", NC_DOUBLE, 1, &value);
      if (status != NC_NOERR) {
        ex_opts(EX_VERBOSE);
        auto errmsg = fmt::format(
            "Error: failed to define 'last_written_time' attribute to file id {}", exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
      }
    }
  }

  std::string map_ioss_field_type(ex_field_type type)
  {
    if (type == EX_VECTOR_2D)
      return "vector_2d";
    if (type == EX_VECTOR_3D)
      return "vector_3d";
    if (type == EX_SCALAR)
      return "scalar";
    if (type == EX_VECTOR_1D)
      return "vector_1d";
    if (type == EX_QUATERNION_2D)
      return "quaternion_2d";
    if (type == EX_QUATERNION_3D)
      return "quaternion_3d";
    if (type == EX_FULL_TENSOR_36)
      return "full_tensor_36";
    if (type == EX_FULL_TENSOR_32)
      return "full_tensor_32";
    if (type == EX_FULL_TENSOR_22)
      return "full_tensor_22";
    if (type == EX_FULL_TENSOR_16)
      return "full_tensor_16";
    if (type == EX_FULL_TENSOR_12)
      return "full_tensor_12";
    if (type == EX_SYM_TENSOR_33)
      return "sym_tensor_33";
    if (type == EX_SYM_TENSOR_31)
      return "sym_tensor_31";
    if (type == EX_SYM_TENSOR_21)
      return "sym_tensor_21";
    if (type == EX_SYM_TENSOR_13)
      return "sym_tensor_13";
    if (type == EX_SYM_TENSOR_11)
      return "sym_tensor_11";
    if (type == EX_SYM_TENSOR_10)
      return "sym_tensor_10";
    if (type == EX_ASYM_TENSOR_03)
      return "asym_tensor_03";
    if (type == EX_ASYM_TENSOR_02)
      return "asym_tensor_02";
    if (type == EX_ASYM_TENSOR_01)
      return "asym_tensor_01";
    if (type == EX_MATRIX_2X2)
      return "matrix_22";
    if (type == EX_MATRIX_3X3)
      return "matrix_33";
    if (type == EX_FIELD_TYPE_SEQUENCE)
      return "Real";
    if (type == EX_BASIS)
      return "Basis";
    if (type == EX_QUADRATURE)
      return "Quadrature";
    return "invalid";
  }

  ex_field_type map_ioss_field_type(const Ioss::VariableType *type)
  {
    if (type->name() == "vector_2d")
      return EX_VECTOR_2D;
    if (type->name() == "vector_3d")
      return EX_VECTOR_3D;
    if (type->name() == "scalar")
      return EX_SCALAR;
    if (type->name() == "vector_1d")
      return EX_VECTOR_1D;
    if (type->name() == "quaternion_2d")
      return EX_QUATERNION_2D;
    if (type->name() == "quaternion_3d")
      return EX_QUATERNION_3D;
    if (type->name() == "full_tensor_36")
      return EX_FULL_TENSOR_36;
    if (type->name() == "full_tensor_32")
      return EX_FULL_TENSOR_32;
    if (type->name() == "full_tensor_22")
      return EX_FULL_TENSOR_22;
    if (type->name() == "full_tensor_16")
      return EX_FULL_TENSOR_16;
    if (type->name() == "full_tensor_12")
      return EX_FULL_TENSOR_12;
    if (type->name() == "sym_tensor_33")
      return EX_SYM_TENSOR_33;
    if (type->name() == "sym_tensor_31")
      return EX_SYM_TENSOR_31;
    if (type->name() == "sym_tensor_21")
      return EX_SYM_TENSOR_21;
    if (type->name() == "sym_tensor_13")
      return EX_SYM_TENSOR_13;
    if (type->name() == "sym_tensor_11")
      return EX_SYM_TENSOR_11;
    if (type->name() == "sym_tensor_10")
      return EX_SYM_TENSOR_10;
    if (type->name() == "asym_tensor_03")
      return EX_ASYM_TENSOR_03;
    if (type->name() == "asym_tensor_02")
      return EX_ASYM_TENSOR_02;
    if (type->name() == "asym_tensor_01")
      return EX_ASYM_TENSOR_01;
    if (type->name() == "matrix_22")
      return EX_MATRIX_2X2;
    if (type->name() == "matrix_33")
      return EX_MATRIX_3X3;

    if (Ioss::Utils::substr_equal("Real", type->name()))
      return EX_FIELD_TYPE_SEQUENCE;

    // This may be a basis, quadratur, or user type...
    auto nsvt = dynamic_cast<const Ioss::NamedSuffixVariableType *>(type);
    if (nsvt != nullptr) {
      return EX_FIELD_TYPE_USER_DEFINED;
    }
    auto bvt = dynamic_cast<const Ioss::BasisVariableType *>(type);
    if (bvt != nullptr) {
      return EX_BASIS;
    }
    auto qvt = dynamic_cast<const Ioss::QuadratureVariableType *>(type);
    if (qvt != nullptr) {
      return EX_QUADRATURE;
    }

    return EX_FIELD_TYPE_INVALID;
  }

  Ioss::EntityType map_exodus_type(ex_entity_type type)
  {
    switch (type) {
    case EX_ASSEMBLY: return Ioss::ASSEMBLY;
    case EX_BLOB: return Ioss::BLOB;
    case EX_EDGE_BLOCK: return Ioss::EDGEBLOCK;
    case EX_EDGE_SET: return Ioss::EDGESET;
    case EX_ELEM_BLOCK: return Ioss::ELEMENTBLOCK;
    case EX_ELEM_SET: return Ioss::ELEMENTSET;
    case EX_FACE_BLOCK: return Ioss::FACEBLOCK;
    case EX_FACE_SET: return Ioss::FACESET;
    case EX_NODAL: return Ioss::NODEBLOCK;
    case EX_NODE_SET: return Ioss::NODESET;
    case EX_SIDE_SET: return Ioss::SIDESET;
    case EX_GLOBAL: return Ioss::REGION;
    default: return Ioss::INVALID_TYPE;
    }
  }

  int read_exodus_basis(int exoid)
  {
    auto bas_cnt = ex_get_basis_count(exoid);
    if (bas_cnt == 0) {
      return 0;
    }
    std::vector<ex_basis> exo_basis(bas_cnt);
    auto                 *pbasis = Data(exo_basis);
    ex_get_basis(exoid, &pbasis, &bas_cnt);

    for (const auto &ebasis : exo_basis) {
      Ioss::Basis basis;
      for (int i = 0; i < ebasis.cardinality; i++) {
        Ioss::BasisComponent bc{
            ebasis.subc_dim[i],     ebasis.subc_ordinal[i], ebasis.subc_dof_ordinal[i],
            ebasis.subc_num_dof[i], ebasis.xi[i],           ebasis.eta[i],
            ebasis.zeta[i]};
        basis.basies.push_back(bc);
      }
      Ioss::VariableType::create_basis_type(ebasis.name, basis);
    }

    // deallocate any memory allocated in the 'ex_basis' structs.
    ex_initialize_basis_struct(Data(exo_basis), exo_basis.size(), -1);

    return bas_cnt;
  }

  int read_exodus_quadrature(int exoid)
  {
    auto quad_cnt = ex_get_quadrature_count(exoid);
    if (quad_cnt == 0) {
      return quad_cnt;
    }

    std::vector<ex_quadrature> exo_quadrature(quad_cnt);
    auto                      *pquad = Data(exo_quadrature);
    ex_get_quadrature(exoid, &pquad, &quad_cnt);

    for (const auto &equadrature : exo_quadrature) {
      std::vector<Ioss::QuadraturePoint> quadrature;
      quadrature.reserve(equadrature.cardinality);
      for (int i = 0; i < equadrature.cardinality; i++) {
        Ioss::QuadraturePoint q{equadrature.xi[i], equadrature.eta[i], equadrature.zeta[i],
                                equadrature.weight[i]};
        quadrature.push_back(q);
      }
      Ioss::VariableType::create_quadrature_type(equadrature.name, quadrature);
    }

    // deallocate any memory allocated in the 'ex_quadrature' structs.
    ex_initialize_quadrature_struct(Data(exo_quadrature), exo_quadrature.size(), -1);

    return quad_cnt;
  }

  ex_entity_type map_exodus_type(Ioss::EntityType type)
  {
    switch (type) {
    case Ioss::REGION: return EX_GLOBAL;
    case Ioss::ASSEMBLY: return EX_ASSEMBLY;
    case Ioss::BLOB: return EX_BLOB;
    case Ioss::EDGEBLOCK: return EX_EDGE_BLOCK;
    case Ioss::EDGESET: return EX_EDGE_SET;
    case Ioss::ELEMENTBLOCK: return EX_ELEM_BLOCK;
    case Ioss::ELEMENTSET: return EX_ELEM_SET;
    case Ioss::FACEBLOCK: return EX_FACE_BLOCK;
    case Ioss::FACESET: return EX_FACE_SET;
    case Ioss::NODEBLOCK: return EX_NODAL;
    case Ioss::NODESET: return EX_NODE_SET;
    case Ioss::SIDESET: return EX_SIDE_SET;
    case Ioss::SIDEBLOCK: return EX_SIDE_SET;
    case Ioss::COMMSET: return static_cast<ex_entity_type>(0);
    default: return EX_INVALID;
    }
  }

  char **get_name_array(size_t count, int size)
  {
    auto *names = new char *[count];
    for (size_t i = 0; i < count; i++) {
      names[i] = new char[size + 1];
      std::memset(names[i], '\0', size + 1);
    }
    return names;
  }

  void delete_name_array(char **names, int count)
  {
    for (int i = 0; i < count; i++) {
      delete[] names[i];
    }
    delete[] names;
  }

  Ioss::NameList get_variable_names(int nvar, int maximumNameLength, int exoid, ex_entity_type type)
  {
    char **names = get_name_array(nvar, maximumNameLength);
    int    ierr  = ex_get_variable_names(exoid, type, nvar, names);
    if (ierr < 0) {
      Ioex::exodus_error(exoid, __LINE__, __func__, __FILE__);
    }

    Ioss::NameList name_str;
    name_str.reserve(nvar);
    for (int i = 0; i < nvar; i++) {
      name_str.emplace_back(names[i]);
    }
    delete_name_array(names, nvar);
    return name_str;
  }

  Ioss::NameList get_reduction_variable_names(int nvar, int maximumNameLength, int exoid,
                                              ex_entity_type type)
  {
    char **names = get_name_array(nvar, maximumNameLength);
    int    ierr  = ex_get_reduction_variable_names(exoid, type, nvar, names);
    if (ierr < 0) {
      Ioex::exodus_error(exoid, __LINE__, __func__, __FILE__);
    }

    Ioss::NameList name_str;
    name_str.reserve(nvar);
    for (int i = 0; i < nvar; i++) {
      name_str.emplace_back(names[i]);
    }
    delete_name_array(names, nvar);
    return name_str;
  }

  bool read_last_time_attribute(int exodusFilePtr, double *value)
  {
    // Check whether the "last_written_time" attribute exists.  If it does,
    // return the value of the attribute in 'value' and return 'true'.
    // If not, don't change 'value' and return 'false'.
    bool found = false;

    int     rootid   = static_cast<unsigned>(exodusFilePtr) & EX_FILE_ID_MASK;
    nc_type att_type = NC_NAT;
    size_t  att_len  = 0;
    int     status   = nc_inq_att(rootid, NC_GLOBAL, "last_written_time", &att_type, &att_len);
    if (status == NC_NOERR && att_type == NC_DOUBLE) {
      // Attribute exists on this database, read it...
      double tmp = 0.0;
      status     = nc_get_att_double(rootid, NC_GLOBAL, "last_written_time", &tmp);
      if (status == NC_NOERR) {
        *value = tmp;
        found  = true;
      }
      else {
        ex_opts(EX_VERBOSE);
        auto errmsg = fmt::format(
            "Error: failed to read last_written_time attribute from file id {}", exodusFilePtr);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        found = false;
      }
    }
    return found;
  }

  bool check_processor_info(const std::string &filename, int exodusFilePtr, int processor_count,
                            int processor_id)
  {
    // A restart file may contain an attribute which contains
    // information about the processor count and current processor id
    // when the file was written.  This code checks whether that
    // information matches the current processor count and id.  If it
    // exists, but doesn't match, a warning message is printed.
    // Eventually, this will be used to determine whether certain
    // decomposition-related data in the file is valid or has been
    // invalidated by a join/re-spread to a different number of
    // processors.
    bool matches = true;

    nc_type att_type = NC_NAT;
    size_t  att_len  = 0;
    int     status   = nc_inq_att(exodusFilePtr, NC_GLOBAL, "processor_info", &att_type, &att_len);
    if (status == NC_NOERR && att_type == NC_INT) {
      // Attribute exists on this database, read it and check that the information
      // matches the current processor count and processor id.
      int proc_info[2];
      status = nc_get_att_int(exodusFilePtr, NC_GLOBAL, "processor_info", proc_info);
      if (status == NC_NOERR) {
        if (proc_info[0] != processor_count && proc_info[0] > 1) {
          fmt::print(Ioss::WarnOut(),
                     "Processor decomposition count in file ({}) does not match current "
                     "processor count ({}) in file named '{}'.\n",
                     proc_info[0], processor_count, filename);
          matches = false;
        }
        if (proc_info[1] != processor_id) {
          fmt::print(
              Ioss::WarnOut(),
              "The file '{}' was originally written on processor {}, but is now being read on "
              "processor {}.\n"
              "This may cause problems if there is any processor-dependent data on the file.\n",
              filename, proc_info[1], processor_id);
          matches = false;
        }
      }
      else {
        ex_opts(EX_VERBOSE);
        auto errmsg =
            fmt::format("Error: failed to read processor info attribute from file {}", filename);
        ex_err_fn(exodusFilePtr, __func__, errmsg.c_str(), status);
        return (EX_FATAL) != 0;
      }
    }
    return matches;
  }

  bool type_match(const std::string &type, const char *substring)
  {
    // Returns true if 'substring' is a sub-string of 'type'.
    // The comparisons are case-insensitive
    // 'substring' is required to be in all lowercase.
    const char *s = substring;
    const char *t = type.c_str();

    SMART_ASSERT(s != nullptr);
    while (*s != '\0' && *t != '\0') {
      if (*s++ != tolower(*t++)) {
        return false;
      }
    }
    return true;
  }

  void decode_surface_name(Ioex::SideSetMap &fs_map, Ioex::SideSetSet &fs_set,
                           const std::string &name)
  {
    auto tokens = Ioss::tokenize(name, "_");
    if (tokens.size() >= 4) {
      // Name of form: "name_eltopo_sidetopo_id" or
      // "name_block_id_sidetopo_id" "name" is typically "surface".
      // The sideset containing this should then be called "name_id"

      // Check whether the second-last token is a side topology and
      // the third-last token is an element topology.
      const Ioss::ElementTopology *side_topo =
          Ioss::ElementTopology::factory(tokens[tokens.size() - 2], true);
      if (side_topo != nullptr) {
        const Ioss::ElementTopology *element_topo =
            Ioss::ElementTopology::factory(tokens[tokens.size() - 3], true);
        if (element_topo != nullptr || tokens[tokens.size() - 4] == "block") {
          // The remainder of the tokens will be used to create
          // a side set name and then this sideset will be
          // a side block in that set.
          std::string fs_name;
          size_t      last_token = tokens.size() - 3;
          if (element_topo == nullptr) {
            last_token--;
          }
          for (size_t tok = 0; tok < last_token; tok++) {
            fs_name += tokens[tok];
          }
          fs_name += "_";
          fs_name += tokens[tokens.size() - 1]; // Add on the id.

          fs_set.insert(fs_name);
          fs_map.insert(Ioex::SideSetMap::value_type(name, fs_name));
        }
      }
    }
  }

  bool set_id(const Ioss::GroupingEntity *entity, Ioex::EntityIdSet *idset)
  {
    // See description of 'get_id' function.  This function just primes
    // the idset with existing ids so that when we start generating ids,
    // we don't overwrite an existing one.

    // Avoid a few string constructors/destructors
    static std::string id_prop("id");

    bool succeed = false;
    if (entity->property_exists(id_prop)) {
      int64_t id = entity->get_property(id_prop).get_int();

      // See whether it already exists...
      auto type = map_exodus_type(entity->type());
      succeed   = idset->insert(std::make_pair(static_cast<int>(type), id)).second;
      if (!succeed) {
        // Need to remove the property so it doesn't cause problems
        // later...
        auto *new_entity = const_cast<Ioss::GroupingEntity *>(entity);
        new_entity->property_erase(id_prop);
        SMART_ASSERT(!entity->property_exists(id_prop))(id_prop);
      }
    }
    return succeed;
  }

  // Potentially extract the id from a name possibly of the form name_id.
  // If not of this form, return 0;
  int64_t extract_id(const std::string &name_id)
  {
    auto tokens = Ioss::tokenize(name_id, "_");

    if (tokens.size() == 1) {
      return 0;
    }

    // Check whether last token is an integer...
    std::string str_id = tokens.back();
    std::size_t found  = str_id.find_first_not_of("0123456789");
    if (found == std::string::npos) {
      // All digits...
      return std::stoll(str_id);
    }

    return 0;
  }

  int64_t get_id(const Ioss::GroupingEntity *entity, Ioex::EntityIdSet *idset)
  {
    // Sierra uses names to refer to grouping entities; however,
    // exodusII requires integer ids.  When reading an exodusII file,
    // the DatabaseIO creates a name by concatenating the entity
    // type (e.g., 'block') and the id separated by an underscore.  For
    // example, an exodusII element block with an id of 100 would be
    // encoded into "block_100"

    // This routine tries to determine the id of the entity using 3
    // approaches:
    //
    // 1. If the entity contains a property named 'id', this is used.
    // The DatabaseIO actually stores the id in the "id" property;
    // however, other grouping entity creators are not required to do
    // this so the property is not guaranteed to exist.
    //
    // 2.If property does not exist, it tries to decode the entity name
    // based on the above encoding.  Again, it is not required that the
    // name follow this convention so success is not guaranteed.
    //
    // 3. If all other schemes fail, the routine picks an id for the entity
    // and returns it.  It also stores this id in the "id" property so an
    // entity will always return the same id for multiple calls.
    // Note that this violates the 'const'ness of the entity so we use
    // a const-cast.

    // Avoid a few string constructors/destructors
    static std::string prop_name("name");
    static std::string id_prop("id");

    int64_t id = 1;

    if (entity->property_exists(id_prop)) {
      id = entity->get_property(id_prop).get_int();
      return id;
    }

    // Try to decode an id from the name.
    std::string name_string = entity->get_property(prop_name).get_string();
    std::string type_name   = entity->short_type_string();
    if (strncasecmp(type_name.c_str(), name_string.c_str(), type_name.size()) == 0) {
      id = extract_id(name_string);
      if (id <= 0) {
        id = 1;
      }
    }

    // At this point, we either have an id equal to '1' or we have an id
    // extracted from the entities name. Increment it until it is
    // unique...
    ex_entity_type type = map_exodus_type(entity->type());
    while (idset->find(std::make_pair(int(type), id)) != idset->end()) {
      ++id;
    }

    // 'id' is a unique id for this entity type...
    idset->insert(std::make_pair(static_cast<int>(type), id));
    auto *new_entity = const_cast<Ioss::GroupingEntity *>(entity);
    new_entity->property_add(Ioss::Property(id_prop, id));
    new_entity->property_update("guid", entity->get_database()->util().generate_guid(id));
    return id;
  }

  bool find_displacement_field(Ioss::NameList &fields, const Ioss::GroupingEntity *block, int ndim,
                               std::string *disp_name)
  {
    // This is a kluge to work with many of the SEACAS codes.  The
    // convention used (in Blot and others) is that the first 'ndim'
    // nodal variables are assumed to be displacements *if* the first
    // character of the names is 'D' and the last characters match the
    // coordinate labels (typically 'X', 'Y', and 'Z').  This routine
    // looks for the field that has the longest match with the string
    // "displacement" and is of the correct storage type (VECTOR_2D or
    // VECTOR_3D).  If found, it returns the name.
    //

    static const std::string displace = "displacement";

    size_t max_span = 0;
    for (const auto &name : fields) {
      std::string lc_name(name);

      Ioss::Utils::fixup_name(lc_name);
      size_t span = match(lc_name, displace);
      if (span > max_span) {
        const Ioss::VariableType *var_type   = block->get_field(name).transformed_storage();
        int                       comp_count = var_type->component_count();
        if (comp_count == ndim) {
          max_span   = span;
          *disp_name = name;
        }
      }
    }
    return max_span > 0;
  }

  void fix_bad_name(char *name)
  {
    SMART_ASSERT(name != nullptr);

    size_t len = std::strlen(name);
    for (size_t i = 0; i < len; i++) {
      if (name[i] < 32 || name[i] > 126) {
        // Zero out entire name if a bad character found anywhere in the name.
        for (size_t j = 0; j < len; j++) {
          name[j] = '\0';
        }
        return;
      }
    }
  }

  std::string get_entity_name(int exoid, ex_entity_type type, int64_t id,
                              const std::string &basename, int length, bool &db_has_name)
  {
    std::vector<char> buffer(length + 1);
    buffer[0] = '\0';
    int error = ex_get_name(exoid, type, id, Data(buffer));
    if (error < 0) {
      exodus_error(exoid, __LINE__, __func__, __FILE__);
    }
    if (buffer[0] != '\0') {
      Ioss::Utils::fixup_name(Data(buffer));
      // Filter out names of the form "basename_id" if the name
      // id doesn't match the id in the name...
      size_t base_size = basename.size();
      if (std::strncmp(basename.c_str(), Data(buffer), base_size) == 0) {
        int64_t name_id = extract_id(Data(buffer));

        // See if name is truly of form "basename_name_id" (e.g. "surface_{id}")
        std::string tmp_name = Ioss::Utils::encode_entity_name(basename, name_id);
        if (tmp_name == Data(buffer)) {
          if (name_id > 0) {
            db_has_name = false;
            if (name_id != id) {
              std::string new_name = Ioss::Utils::encode_entity_name(basename, id);
              fmt::print(Ioss::WarnOut(),
                         "The entity named '{}' has the id {} which does not match the "
                         "embedded id {}.\n"
                         "         This can cause issues later; the entity will be renamed to '{}' "
                         "(IOSS)\n\n",
                         Data(buffer), id, name_id, new_name);
              return new_name;
            }
            return tmp_name;
          }
        }
      }
      db_has_name = true;
      return {Data(buffer)};
    }
    db_has_name = false;
    return Ioss::Utils::encode_entity_name(basename, id);
  }

  void exodus_error(int exoid, int lineno, const char *function, const char *filename)
  {
    std::string empty{};
    exodus_error(exoid, lineno, function, filename, empty);
  }

  void exodus_error(int exoid, int lineno, const char *function, const char *filename,
                    const std::string &extra)
  {
    std::ostringstream errmsg;
    // Create errmsg here so that the exerrval doesn't get cleared by
    // the ex_close call.
    int status;
    ex_get_err(nullptr, nullptr, &status);
    fmt::print(errmsg, "Exodus error ({}) {} at line {} of file '{}' in function '{}'.", status,
               ex_strerror(status), lineno, filename, function);

    if (!extra.empty()) {
      fmt::print(errmsg, " {}", extra);
    }
    fmt::print(errmsg, " Please report to gdsjaar@sandia.gov if you need help.");

    ex_err_fn(exoid, nullptr, nullptr, EX_PRTLASTMSG);
    IOSS_ERROR(errmsg);
  }

  int add_map_fields(int exoid, Ioss::ElementBlock *block, int64_t my_element_count,
                     size_t name_length)
  {
    // Check for optional element maps...
    int map_count = ex_inquire_int(exoid, EX_INQ_ELEM_MAP);
    if (map_count <= 0) {
      return map_count;
    }

    // Get the names of the maps...
    char **names = get_name_array(map_count, name_length);
    int    ierr  = ex_get_names(exoid, EX_ELEM_MAP, names);
    if (ierr < 0) {
      Ioex::exodus_error(exoid, __LINE__, __func__, __FILE__);
    }

    // Convert to lowercase.
    for (int i = 0; i < map_count; i++) {
      Ioss::Utils::fixup_name(names[i]);
    }

    for (int i = 0; i < map_count; i++) {
      // If the name does *not* contain a `:`, then assume that this is a scalar map and add to the
      // block.
      std::string name{names[i]};
      if (name.find(':') == std::string::npos) {
        auto field = Ioss::Field(name, block->field_int_type(), IOSS_SCALAR(), Ioss::Field::MAP,
                                 my_element_count)
                         .set_index(i + 1);
        block->field_add(std::move(field));
        continue;
      }

      // Name does contain a `:` which is a loose convention for naming of maps in IOSS.
      // If multiple maps start with the same substring before the `:`, then they are considered
      // components of the same Ioss::Field::MAP field.
      // Count the number of names that begin with the same substring...
      auto base = name.substr(0, name.find(':'));

      // Now see if the following name(s) contain the same substring...
      int ii = i;
      while (++ii < map_count) {
        std::string next{names[ii]};
        std::string next_base = next.substr(0, next.find(':'));
        if (base != next_base) {
          break;
        }
      }

      int comp_count = ii - i;

      std::string storage = fmt::format("Real[{}]", comp_count);
      auto        field =
          Ioss::Field(base, block->field_int_type(), storage, Ioss::Field::MAP, my_element_count)
              .set_index(i + 1);
      block->field_add(std::move(field));

      i = ii - 1;
    }

    delete_name_array(names, map_count);
    return map_count;
  }

  void write_coordinate_frames(int exoid, const Ioss::CoordinateFrameContainer &frames)
  {
    if ((ex_int64_status(exoid) & EX_BULK_INT64_API) != 0) {
      internal_write_coordinate_frames(exoid, frames, static_cast<int64_t>(0));
    }
    else {
      internal_write_coordinate_frames(exoid, frames, 0);
    }
  }

  void add_coordinate_frames(int exoid, Ioss::Region *region)
  {
    if ((ex_int64_status(exoid) & EX_BULK_INT64_API) != 0) {
      internal_add_coordinate_frames(exoid, region, static_cast<int64_t>(0));
    }
    else {
      internal_add_coordinate_frames(exoid, region, 0);
    }
  }

  bool filter_node_list(Ioss::Int64Vector                &nodes,
                        const std::vector<unsigned char> &node_connectivity_status)
  {
    // Iterate through 'nodes' and determine which of the nodes are
    // not connected to any non-omitted blocks. The index of these
    // nodes is then put in the 'nodes' list.
    // Assumes that there is at least one omitted element block.  The
    // 'nodes' list on entry contains 1-based local node ids, not global.
    // On return, the nodes list contains indices.  To filter a nodeset list:
    // for (size_t i = 0; i < nodes.size(); i++) {
    //    active_values[i] = some_nset_values[nodes[i]];
    // }

    size_t orig_size = nodes.size();
    size_t active    = 0;
    for (size_t i = 0; i < orig_size; i++) {
      if (node_connectivity_status[nodes[i] - 1] >= 2) {
        // Node is connected to at least 1 active element...
        nodes[active++] = i;
      }
    }
    nodes.resize(active);
    nodes.shrink_to_fit(); // shrink to fit
    return (active != orig_size);
  }

  void filter_element_list(Ioss::Region *region, Ioss::Int64Vector &elements,
                           Ioss::Int64Vector &sides, bool remove_omitted_elements)
  {
    // Iterate through 'elements' and remove the elements which are in an omitted block.
    // Precondition is that there is at least one omitted element block.
    // The 'elements' list contains local element ids, not global.
    // Since there are typically a small number of omitted blocks, do
    // the following:
    // For each omitted block, determine the min and max element id in
    // that block.  Iterate 'elements' vector and set the id to zero if
    // min <= id <= max.  Once all omitted blocks have been processed,
    // then iterate the vector and compress out all zeros.  Keep 'sides'
    // array consistent.

    // Get all element blocks in region...
    bool                               omitted        = false;
    const Ioss::ElementBlockContainer &element_blocks = region->get_element_blocks();
    for (const auto &block : element_blocks) {

      if (Ioss::Utils::block_is_omitted(block)) {
        int64_t min_id = block->get_offset() + 1;
        int64_t max_id = min_id + block->entity_count() - 1;
        for (size_t i = 0; i < elements.size(); i++) {
          if (min_id <= elements[i] && elements[i] <= max_id) {
            omitted     = true;
            elements[i] = 0;
            sides[i]    = 0;
          }
        }
      }
    }
    if (remove_omitted_elements && omitted) {
      elements.erase(std::remove(elements.begin(), elements.end(), 0), elements.end());
      sides.erase(std::remove(sides.begin(), sides.end(), 0), sides.end());
    }
  }

  void separate_surface_element_sides(Ioss::Int64Vector &element, Ioss::Int64Vector &sides,
                                      Ioss::Region *region, Ioex::TopologyMap &topo_map,
                                      Ioex::TopologyMap     &side_map,
                                      Ioss::SurfaceSplitType split_type,
                                      const std::string     &surface_name)
  {
    if (!element.empty()) {
      Ioss::ElementBlock *block = nullptr;
      // Topology of sides in current element block
      const Ioss::ElementTopology *common_ftopo = nullptr;
      const Ioss::ElementTopology *topo         = nullptr; // Topology of current side
      int64_t                      current_side = -1;

      for (size_t iel = 0; iel < element.size(); iel++) {
        int64_t elem_id = element[iel];
        if (elem_id <= 0) {
          IOSS_ERROR(fmt::format(
              "ERROR: In sideset/surface '{}' an element with id {} is specified.  Element "
              "ids must be greater than zero. ({})",
              surface_name, elem_id, __func__));
        }
        if (block == nullptr || !block->contains(elem_id)) {
          block = region->get_element_block(elem_id);
          SMART_ASSERT(block != nullptr);
          SMART_ASSERT(!Ioss::Utils::block_is_omitted(block)); // Filtered out above.

          // nullptr if hetero sides on element
          common_ftopo = block->topology()->boundary_type(0);
          if (common_ftopo != nullptr) {
            topo = common_ftopo;
          }
          current_side = -1;
        }

        if (common_ftopo == nullptr && sides[iel] != current_side) {
          current_side = sides[iel];
          if (current_side <= 0 || current_side > block->topology()->number_boundaries()) {
            IOSS_ERROR(fmt::format(
                "ERROR: In sideset/surface '{}' for the element with id {} of topology '{}';\n\t"
                "an invalid face index '{}' is specified.\n\tFace indices "
                "must be between 1 and {}. ({})",
                surface_name, fmt::group_digits(elem_id), block->topology()->name(), current_side,
                block->topology()->number_boundaries(), __func__));
          }
          topo = block->topology()->boundary_type(sides[iel]);
          SMART_ASSERT(topo != nullptr);
        }
        std::pair<std::string, const Ioss::ElementTopology *> name_topo;
        if (split_type == Ioss::SPLIT_BY_TOPOLOGIES) {
          name_topo = std::make_pair(block->topology()->name(), topo);
        }
        else if (split_type == Ioss::SPLIT_BY_ELEMENT_BLOCK) {
          name_topo = std::make_pair(block->name(), topo);
        }
        topo_map[name_topo]++;
        if (side_map[name_topo] == 0) {
          side_map[name_topo] = sides[iel];
        }
        else if (side_map[name_topo] != sides[iel]) {
          // Not a consistent side for all sides in this
          // sideset. Set to large number. Note that maximum
          // sides/element is 6, so don't have to worry about
          // a valid element having 999 sides (unless go to
          // arbitrary polyhedra some time...) Using a large
          // number instead of -1 makes it easier to check the
          // parallel consistency...
          side_map[name_topo] = 999;
        }
      }
    }
  }

  void write_reduction_attributes(int exoid, const Ioss::GroupingEntity *ge)
  {
    Ioss::NameList properties = ge->property_describe(Ioss::Property::Origin::ATTRIBUTE);

    auto type = Ioex::map_exodus_type(ge->type());
    auto id   = ge->get_optional_property("id", 0);

    double  rval = 0.0;
    int64_t ival = 0;
    for (const auto &property_name : properties) {
      auto prop = ge->get_property(property_name);

      switch (prop.get_type()) {
      case Ioss::Property::BasicType::REAL:
        rval = prop.get_real();
        ex_put_double_attribute(exoid, type, id, property_name.c_str(), 1, &rval);
        break;
      case Ioss::Property::BasicType::INTEGER:
        ival = prop.get_int();
        ex_put_integer_attribute(exoid, type, id, property_name.c_str(), 1, &ival);
        break;
      case Ioss::Property::BasicType::STRING:
        ex_put_text_attribute(exoid, type, id, property_name.c_str(), prop.get_string().c_str());
        break;
      case Ioss::Property::BasicType::VEC_INTEGER:
        ex_put_integer_attribute(exoid, type, id, property_name.c_str(), prop.get_vec_int().size(),
                                 Data(prop.get_vec_int()));
        break;
      case Ioss::Property::BasicType::VEC_DOUBLE:
        ex_put_double_attribute(exoid, type, id, property_name.c_str(),
                                prop.get_vec_double().size(), Data(prop.get_vec_double()));
        break;
      default:; // Do nothing
      }
    }
  }
} // namespace Ioex
