// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <exodus/Ioex_SuperElement.h> // for SuperElement

#include <Ioss_Field.h>
#include <Ioss_Property.h>
#include <Ioss_Utils.h>

#include <cassert>
#include <cstddef>
#include "vtk_fmt.h"
#include VTK_FMT(fmt/ostream.h)
#include <vtk_netcdf.h>
#include <string>

#include <Ioss_FieldManager.h>
#include <Ioss_GroupingEntity.h>
#include <Ioss_PropertyManager.h>

namespace {
  int nc_get_array(int ncid, const char *name, double *data)
  {
    // NOTE: size of data array is validated in
    // calling code.  Just read and store here.

    int varid  = 0;
    int status = nc_inq_varid(ncid, name, &varid);
    if (status != NC_NOERR) {
      return status;
    }

    status = nc_get_var_double(ncid, varid, data);
    return status;
  }

  int nc_get_dimension(int ncid, const char *DIMENSION, const char *label, size_t *count)
  {
    std::ostringstream errmsg;

    *count    = 0;
    int dimid = -1;

    int status = nc_inq_dimid(ncid, DIMENSION, &dimid);
    if (status != NC_NOERR) {
      if (status == NC_EBADDIM) {
        // Value is zero if the dimension is not defined.
        *count = 0;
        return 0;
      }
      fmt::print(errmsg, "ERROR: Failed to locate number of {} in superelement file.", label);
      IOSS_ERROR(errmsg);
    }

    status = nc_inq_dimlen(ncid, dimid, count);
    if (status != NC_NOERR) {
      fmt::print(errmsg, "ERROR: Failed to get number of {} in superelement file.", label);
      IOSS_ERROR(errmsg);
    }
    return status;
  }
} // namespace

Ioex::SuperElement::SuperElement(std::string filename, const std::string &my_name)
    : Ioss::GroupingEntity(nullptr, my_name, 1), fileName(std::move(filename))
{

  // For now, we will open the raw netcdf file here and parse the
  // dimensions. This is probably not how this should be done long
  // term, but is better than putting netcdf calls in application...

  // Check that file specified by filename exists...
  // Add working directory if needed.
  std::string local_filename = fileName;

  int status = nc_open(local_filename.c_str(), NC_NOWRITE, &filePtr);
  if (status != NC_NOERR) {
    std::ostringstream errmsg;
    fmt::print(errmsg, "ERROR: Failed to open superelement file '{}'.", local_filename);
    IOSS_ERROR(errmsg);
  }

  // At this point have a valid netcdf file handle.
  // Read some dimensions to determine size of Mass and Stiffness
  // matrix.
  nc_get_dimension(filePtr, "NumDof", "number of degrees of freedom", &numDOF);

  nc_get_dimension(filePtr, "num_nodes", "number of nodes", &num_nodes);

  nc_get_dimension(filePtr, "NumEig", "number of eigenvalues", &numEIG);

  nc_get_dimension(filePtr, "NumRbm", "number of rigid body modes", &numRBM);

  nc_get_dimension(filePtr, "num_dim", "number of dimensions", &num_dim);

  size_t num_constraints = 0;
  nc_get_dimension(filePtr, "NumConstraints", "number of interface dof", &num_constraints);
  assert(num_constraints == numDOF - numEIG);

  // Add the standard properties...
  properties.add(Ioss::Property(this, "numDOF", Ioss::Property::INTEGER));
  if (num_nodes > 0) {
    properties.add(Ioss::Property(this, "num_nodes", Ioss::Property::INTEGER));
  }
  properties.add(Ioss::Property(this, "numEIG", Ioss::Property::INTEGER));

  properties.add(Ioss::Property(this, "numRBM", Ioss::Property::INTEGER));

  properties.add(Ioss::Property(this, "numDIM", Ioss::Property::INTEGER));

  properties.add(Ioss::Property(this, "numConstraints", Ioss::Property::INTEGER));

  // Add the standard fields...
  if (num_nodes > 0) {
    fields.add(
        Ioss::Field("coordx", Ioss::Field::REAL, IOSS_SCALAR(), Ioss::Field::MESH, num_nodes));
    fields.add(
        Ioss::Field("coordy", Ioss::Field::REAL, IOSS_SCALAR(), Ioss::Field::MESH, num_nodes));
    fields.add(
        Ioss::Field("coordz", Ioss::Field::REAL, IOSS_SCALAR(), Ioss::Field::MESH, num_nodes));
    fields.add(Ioss::Field("node_num_map", Ioss::Field::REAL, IOSS_SCALAR(), Ioss::Field::MESH,
                           num_nodes));
    fields.add(Ioss::Field("cbmap", Ioss::Field::REAL, IOSS_SCALAR(), Ioss::Field::MESH,
                           2 * num_nodes * num_dim));
  }

  fields.add(
      Ioss::Field("Kr", Ioss::Field::REAL, IOSS_SCALAR(), Ioss::Field::MESH, numDOF * numDOF));

  fields.add(
      Ioss::Field("Mr", Ioss::Field::REAL, IOSS_SCALAR(), Ioss::Field::MESH, numDOF * numDOF));

  if (numRBM > 0) {
    fields.add(Ioss::Field("InertiaTensor", Ioss::Field::REAL, IOSS_SCALAR(), Ioss::Field::MESH,
                           numDOF * numRBM));
    fields.add(Ioss::Field("MassInertia", Ioss::Field::REAL, IOSS_SCALAR(), Ioss::Field::MESH,
                           numDOF * numRBM));
  }

  // There are additional properties and fields on the netcdf file,
  // but for now we only need "Kr", "Mr", and "InertiaTensor"
}

int64_t Ioex::SuperElement::internal_get_field_data(const Ioss::Field &field, void *data,
                                                    size_t data_size) const
{
  size_t num_to_get = field.verify(data_size);

  if (field.get_name() == "cbmap") {
    assert(num_to_get == 2 * num_nodes * num_dim);
    int status = nc_get_array(filePtr, "cbmap", reinterpret_cast<double *>(data));
    if (status != 0) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: Could not load coordinate data field 'cbmap' from file '{}'.",
                 fileName);
      IOSS_ERROR(errmsg);
    }
  }
  else if (field.get_name() == "node_num_map") {
    assert(num_to_get == num_nodes);
    int status = nc_get_array(filePtr, "node_num_map", reinterpret_cast<double *>(data));

    if (status != 0) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: Could not load coordinate data field 'node_num_map' from file '{}'.",
                 fileName);
      IOSS_ERROR(errmsg);
    }
  }
  else if (field.get_name() == "coordx") {
    assert(num_to_get == num_nodes);
    int status = nc_get_array(filePtr, "coordx", reinterpret_cast<double *>(data));
    if (status != 0) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: Could not load coordinate data field 'coordx' from file '{}'.",
                 fileName);
      IOSS_ERROR(errmsg);
    }
  }
  else if (field.get_name() == "coordy") {
    assert(num_to_get == num_nodes);
    int status = nc_get_array(filePtr, "coordy", reinterpret_cast<double *>(data));
    if (status != 0) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: Could not load coordinate data field 'coordy' from file '{}'.",
                 fileName);
      IOSS_ERROR(errmsg);
    }
  }
  else if (field.get_name() == "coordz") {
    assert(num_to_get == num_nodes);
    int status = nc_get_array(filePtr, "coordz", reinterpret_cast<double *>(data));
    if (status != 0) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: Could not load coordinate data field 'coordz' from file '{}'.",
                 fileName);
      IOSS_ERROR(errmsg);
    }
  }
  else if (field.get_name() == "Kr") {
    assert(num_to_get == numDOF * numDOF);
    int status = nc_get_array(filePtr, "Kr", reinterpret_cast<double *>(data));
    if (status != 0) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: Could not load stiffness matrix field 'Kr' from file '{}'.",
                 fileName);
      IOSS_ERROR(errmsg);
    }
  }
  else if (field.get_name() == "Mr") {
    assert(num_to_get == numDOF * numDOF);
    int status = nc_get_array(filePtr, "Mr", reinterpret_cast<double *>(data));
    if (status != 0) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: Could not load mass matrix field 'Mr' from file '{}'.", fileName);
      IOSS_ERROR(errmsg);
    }
  }
  else if (field.get_name() == "InertiaTensor") {
    assert(num_to_get == numDOF * numRBM);
    int status = nc_get_array(filePtr, "InertiaTensor", reinterpret_cast<double *>(data));
    if (status != 0) {
      std::ostringstream errmsg;
      fmt::print(errmsg,
                 "ERROR: Could not load inertia matrix field 'InertialTensor' from file '{}'.",
                 fileName);
      IOSS_ERROR(errmsg);
    }
  }
  else if (field.get_name() == "MassInertia") {
    assert(num_to_get == numDOF * numRBM);
    int status = nc_get_array(filePtr, "MassInertia", reinterpret_cast<double *>(data));
    if (status != 0) {
      std::ostringstream errmsg;
      fmt::print(errmsg, "ERROR: Could not mass inertia matrix field 'MassInertia' from file '{}'.",
                 fileName);
      IOSS_ERROR(errmsg);
    }
  }
  else {
    fmt::print(Ioss::WarnOut(), "{} '{}'. Unknown input field '{}'", static_cast<int>(type()), name(),
               field.get_name());
    return -4;
  }
  return num_to_get;
}

Ioex::SuperElement::~SuperElement()
{
  if (filePtr != 0) {
    nc_close(filePtr);
  }
}

int64_t Ioex::SuperElement::internal_put_field_data(const Ioss::Field & /* field */,
                                                    void * /* data */, size_t /* data_size */) const
{
  return -1;
}

Ioss::Property Ioex::SuperElement::get_implicit_property(const std::string &the_name) const
{
  if (Ioss::Utils::str_equal(the_name, "numDOF")) {
    return Ioss::Property(the_name, static_cast<int>(numDOF));
  }
  if (Ioss::Utils::str_equal(the_name, "num_nodes")) {
    return Ioss::Property(the_name, static_cast<int>(num_nodes));
  }
  if (Ioss::Utils::str_equal(the_name, "numEIG")) {
    return Ioss::Property(the_name, static_cast<int>(numEIG));
  }
  if (Ioss::Utils::str_equal(the_name, "num_dim")) {
    return Ioss::Property(the_name, static_cast<int>(num_dim));
  }
  if (Ioss::Utils::str_equal(the_name, "numConstraints")) {
    return Ioss::Property(the_name, static_cast<int>(numDOF) - static_cast<int>(numEIG));
  }

  return Ioss::GroupingEntity::get_implicit_property(the_name);
}
