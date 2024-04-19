// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2021, Triad National Security, LLC
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-LANL-Triad-USGov

#include "vtkMath.h"
#include "vtkStdString.h"
#include <PIODataHDF5.h>
#include <cstdlib>
#include <iostream>
#include <vtk_hdf5.h>
#include <vtksys/FStream.hxx>

VTK_ABI_NAMESPACE_BEGIN

PIO_DATA_HDF5::PIO_DATA_HDF5(const char* piofile)
{
  // given a filename, attempt to load it as an hdf5 pio file
  cell_active_loaded = false;

  // open hdf5 file
  is_good_read = true;
  hdf5_file_id = H5Fopen(piofile, H5F_ACC_RDONLY, H5P_DEFAULT);
  if (hdf5_file_id < 0)
  {
    // error loading file
    is_good_read = false;
    return;
  }

  // iterate over all datasets and fill in VarMMap
  // all datasets are in the hdf5 root group
  H5G_info_t group_info;
  H5Gget_info(hdf5_file_id, &group_info);

  // fill in VarMMap and pio_field
  // iterate over all fields and store metadata about each field
  char buffer[255];
  VarMMap.clear();
  pio_field = new PIO_FIELD[group_info.nlinks];
  for (hsize_t i = 0; i < group_info.nlinks; i++)
  {
    // get the name of the dataset, and add it to VarMMap, along with its corresponding PIO_FIELD
    H5Lget_name_by_idx(hdf5_file_id, ".", H5_INDEX_NAME, H5_ITER_INC, i, buffer, 255, H5P_DEFAULT);
    size_t namelen = strlen(buffer);
    char* name = new char[namelen + 1];
    strcpy(name, buffer);
    VarMMap.insert(std::make_pair(name, &pio_field[i]));

    // obtain the name without the "..<index>" at the end
    char dotdot[3] = "..";
    char* suffix = strstr(name, dotdot);
    size_t suffixlen = strlen(suffix);
    size_t shortnamelen = namelen - suffixlen;
    char* shortname = new char[shortnamelen + 1];
    for (size_t j = 0; j < shortnamelen; j++)
    {
      shortname[j] = name[j];
    }
    shortname[shortnamelen] = '\0';
    pio_field[i].pio_name = shortname;

    // store the index as an integer
    char* index_str = new char[suffixlen - 2 + 1];
    for (size_t j = 0; j < suffixlen - 2; j++)
    {
      index_str[j] = suffix[j + 2];
    }
    index_str[suffixlen - 2] = '\0';
    pio_field[i].index = atoi(index_str);
    delete[] index_str;

    // data and cdata are always null. data is never stored there and instead read on demand using
    // the various set_scalar_field() or read_dataset() functions.
    pio_field[i].data = nullptr;
    pio_field[i].cdata = nullptr;

    // set the length and cdata_len, based on what the PIODATAPIO class would calculate.
    // for non-string fields, length is the size of the field in terms of
    // doubles. but if there is only one point, and the datatype size is
    // smaller than a double, then length could be 0. in this case, make it
    // one. this class will not use length, so it should be ok.
    // for string fields, length is the number of strings in the dataset.
    // for non-string fields, cdata_len is always 0.
    // for string fields, cdata_len is the length of each string, including the terminating null.
    hid_t dataset_id = H5Dopen(hdf5_file_id, name, hid_t(0));
    hid_t datatype_id = H5Dget_type(dataset_id);
    H5T_class_t datatype_class = H5Tget_class(datatype_id);
    hid_t dataspace_id = H5Dget_space(dataset_id);
    hssize_t npoints = H5Sget_simple_extent_npoints(dataspace_id);
    size_t datatype_size = H5Tget_size(datatype_id);
    if (datatype_class == H5T_STRING)
    {
      pio_field[i].length = npoints;
      if ((strcmp(name, "hist_dandt") == 0) || (strcmp(name, "hist_prbnm") == 0))
      {
        pio_field[i].cdata_len = sizeof(double) * 2 + 1;
      }
      else if (strcmp(name, "matident") == 0)
      {
        std::valarray<int> matident_len;
        set_scalar_field(matident_len, "MATIDENT_LEN");
        pio_field[i].cdata_len = matident_len[0] + 1;
      }
      else if (strcmp(name, "timertype") == 0)
      {
        std::valarray<int> timertype_len;
        set_scalar_field(timertype_len, "TIMERTYPE_LEN");
        pio_field[i].cdata_len = timertype_len[0] + 1;
      }
      else
      {
        pio_field[i].cdata_len = sizeof(double) + 1;
      }
    }
    else
    {
      pio_field[i].length = (npoints * datatype_size) / sizeof(double);
      if (pio_field[i].length == 0)
      {
        pio_field[i].length = 1;
      }
      pio_field[i].cdata_len = 0;
    }

    H5Sclose(dataspace_id);
    H5Tclose(datatype_id);
    H5Dclose(dataset_id);
  }

} // End PIO_DATA_HDF5::PIO_DATA_HDF5

PIO_DATA_HDF5::~PIO_DATA_HDF5()
{
  if (!is_good_read)
  {
    return;
  }

  H5Fclose(hdf5_file_id);

  for (size_t i = 0; i < VarMMap.size(); i++)
  {
    delete[] pio_field[i].pio_name;
  }
  delete[] pio_field;

  VarMMap.clear();

} // End PIO_DATA_HDF5::~PIO_DATA_HDF5()

bool PIO_DATA_HDF5::good_read()
{
  // returns whether the pio file opened successfully
  return is_good_read;
}

bool PIO_DATA_HDF5::set_scalar_field(std::valarray<int>& v, const char* fieldname)
{
  // read in a pio field as a scalar field. data will be placed in argument v.
  // for scalars, field names end with a suffix of 0 or -1. it is -1 if
  // it is a derived field.
  int index;
  if (!has_scalar(fieldname, index))
  {
    v.resize(0);
    return false;
  }

  // form the complete field name with index
  vtkStdString fieldname0(fieldname);
  fieldname0 = fieldname0 + ".." + std::to_string(index);

  // open the dataset
  hid_t dataset_id = H5Dopen(hdf5_file_id, fieldname0.c_str(), hid_t(0));
  hid_t datatype_id = H5Dget_type(dataset_id);
  H5T_class_t datatype_class = H5Tget_class(datatype_id);
  size_t datatype_size = H5Tget_size(datatype_id);

  // datatype class should be integer, and datatype size should be 4
  if (!((datatype_class == H5T_INTEGER) && (datatype_size == 4)))
  {
    H5Tclose(datatype_id);
    H5Dclose(dataset_id);
    v.resize(0);
    return false;
  }
  else
  {
    hid_t dataspace_id = H5Dget_space(dataset_id);
    hid_t ndims = H5Sget_simple_extent_ndims(dataspace_id);
    std::vector<hsize_t> dims_out(ndims);
    H5Sget_simple_extent_dims(dataspace_id, dims_out.data(), nullptr);
    hid_t memspace_id = H5Screate_simple(ndims, dims_out.data(), nullptr);
    v.resize(dims_out[0]);
    H5Dread(dataset_id, H5T_NATIVE_INT, memspace_id, dataspace_id, H5P_DEFAULT, &v[0]);

    H5Tclose(datatype_id);
    H5Dclose(dataset_id);
    H5Sclose(memspace_id);
    H5Sclose(dataspace_id);
    return true;
  }
} // End PIO_DATA_HDF5::set_scalar_field

bool PIO_DATA_HDF5::set_scalar_field(std::valarray<int64_t>& v, const char* fieldname)
{
  // read in a pio field as a scalar field. data will be placed in argument v.
  // for scalars, field names end with a suffix of 0 or -1. it is -1 if
  // it is a derived field.
  int index;
  if (!has_scalar(fieldname, index))
  {
    v.resize(0);
    return false;
  }

  // form the complete field name with index
  vtkStdString fieldname0(fieldname);
  fieldname0 = fieldname0 + ".." + std::to_string(index);

  // open the dataset
  hid_t dataset_id = H5Dopen(hdf5_file_id, fieldname0.c_str(), hid_t(0));
  hid_t datatype_id = H5Dget_type(dataset_id);
  H5T_class_t datatype_class = H5Tget_class(datatype_id);
  size_t datatype_size = H5Tget_size(datatype_id);

  // datatype class should be integer, and datatype size should be 8
  if (!((datatype_class == H5T_INTEGER) && (datatype_size == 8)))
  {
    H5Tclose(datatype_id);
    H5Dclose(dataset_id);
    v.resize(0);
    return false;
  }
  else
  {
    hid_t dataspace_id = H5Dget_space(dataset_id);
    hid_t ndims = H5Sget_simple_extent_ndims(dataspace_id);
    std::vector<hsize_t> dims_out(ndims);
    H5Sget_simple_extent_dims(dataspace_id, dims_out.data(), nullptr);
    hid_t memspace_id = H5Screate_simple(ndims, dims_out.data(), nullptr);
    v.resize(dims_out[0]);
    H5Dread(dataset_id, H5T_NATIVE_INT64, memspace_id, dataspace_id, H5P_DEFAULT, &v[0]);

    H5Tclose(datatype_id);
    H5Dclose(dataset_id);
    H5Sclose(memspace_id);
    H5Sclose(dataspace_id);
    return true;
  }
} // End PIO_DATA_HDF5::set_scalar_field

bool PIO_DATA_HDF5::set_scalar_field(std::valarray<double>& v, const char* fieldname)
{
  // read in a pio field as a scalar field. data will be placed in argument v.
  // for scalars, field names end with a suffix of 0 or -1. it is -1 if
  // it is a derived field.
  //
  // for some specific variables, if the variable is requested and is not
  // present, then derive these variables. these variables include xdt, ydt,
  // zdt, and rho.

  int index;
  bool field_present = has_scalar(fieldname, index);
  if (strcmp(fieldname, "xdt") == 0 && !field_present)
  {
    std::valarray<std::valarray<double>> cell_momentum;
    std::valarray<double> mass;
    set_vector_field(cell_momentum, "cell_momentum");
    set_scalar_field(mass, "mass");
    v = cell_momentum[0] / mass;
    return true;
  }
  if (strcmp(fieldname, "ydt") == 0 && !field_present)
  {
    std::valarray<std::valarray<double>> cell_momentum;
    std::valarray<double> mass;
    set_vector_field(cell_momentum, "cell_momentum");
    set_scalar_field(mass, "mass");
    if (cell_momentum.size() >= 2)
    {
      v = cell_momentum[1] / mass;
      return true;
    }
    else
    {
      v.resize(0);
      return false;
    }
  }
  if (strcmp(fieldname, "zdt") == 0 && !field_present)
  {
    std::valarray<std::valarray<double>> cell_momentum;
    std::valarray<double> mass;
    set_vector_field(cell_momentum, "cell_momentum");
    set_scalar_field(mass, "mass");
    if (cell_momentum.size() >= 3)
    {
      v = cell_momentum[2] / mass;
      return true;
    }
    else
    {
      v.resize(0);
      return false;
    }
  }
  if (strcmp(fieldname, "rho") == 0 && !field_present)
  {
    std::valarray<double> vcell;
    std::valarray<double> mass;
    set_scalar_field(vcell, "vcell");
    set_scalar_field(mass, "mass");
    v = mass / vcell;
    return true;
  }

  // fieldname is not a derived field, load it normally from the file.
  // use the cell_active array as a mask array.
  if (!field_present)
  {
    v.resize(0);
    return false;
  }

  if (!read_dataset(v, fieldname, index))
  {
    v.resize(0);
    return false;
  }

  if (!cell_active_loaded)
  {
    set_scalar_field(cell_active, "cell_active");
    cell_active_loaded = true;
  }

  if (cell_active.size() == v.size())
  {
    for (size_t i = 0; i < v.size(); i++)
    {
      if (cell_active[i] == 0)
      {
        v[i] = vtkMath::Nan();
      }
    }
  }

  return true;
} // End PIO_DATA_HDF5::set_scalar_field

bool PIO_DATA_HDF5::set_vector_field(std::valarray<std::valarray<double>>& v, const char* fieldname)
{
  // read in a pio field as a vector field. data will be placed in argument v.
  // for vectors, field names ends with a suffix of 1, and increases.
  // for example, each component of cell_center is spread over three fields:
  // cell_center..1, cell_center..2, cell_center..3
  int num_components = get_num_components(fieldname);
  if (num_components <= 0)
  {
    v.resize(0);
    return false;
  }

  // try loading each component dataset. if an issue occurs, free all data loaded so far.
  v.resize(num_components);
  for (int i = 0; i < num_components; i++)
  {
    if (!read_dataset(v[i], fieldname, i + 1))
    {
      for (int j = 0; j < i; j++)
      {
        v[j].resize(0);
      }
      v.resize(0);
      return false;
    }
  }

  // mask values based on cell_active
  if (!cell_active_loaded)
  {
    set_scalar_field(cell_active, "cell_active");
    cell_active_loaded = true;
  }

  // ensure array sizes are identical
  for (int i = 0; i < num_components; i++)
  {
    if (cell_active.size() != v[i].size())
    {
      // don't use mask values
      return true;
    }
  }

  // use cell_active to mask values
  for (size_t i = 0; i < v[0].size(); i++)
  {
    if (cell_active[i] == 0)
    {
      for (int d = 0; d < num_components; d++)
      {
        v[d][i] = vtkMath::Nan();
      }
    }
  }

  return true;
} // End PIO_DATA_HDF5::set_vector_field

bool PIO_DATA_HDF5::read_dataset(std::valarray<double>& v, const char* fieldname, int index)
{
  // read in the whole dataset, only performing basic checks.
  // differences between this and set_scalar_field(double):
  //   - this function requires an index value
  //   - this function does not calculate derived fields
  //   - this function does not mask values
  //
  // Note: if the field being asked to read is a 64 bit integer, then the
  // field is read in, and then converted to double. this is only done so
  // that the field cell_index can be read in. if the field is not double
  // or 64 bit integer, it is not read in.

  // add "..<index>" suffix to the field name
  vtkStdString fieldname0(fieldname);
  fieldname0 = fieldname0 + ".." + std::to_string(index);
  if (VarMMap.count(fieldname0.c_str()) != 1)
  {
    v.resize(0);
    return false;
  }

  // open the dataset
  hid_t dataset_id = H5Dopen(hdf5_file_id, fieldname0.c_str(), hid_t(0));
  hid_t datatype_id = H5Dget_type(dataset_id);
  H5T_class_t datatype_class = H5Tget_class(datatype_id);
  size_t datatype_size = H5Tget_size(datatype_id);

  // check for correct type class and size
  if (!((datatype_class == H5T_FLOAT) && (datatype_size == 8)))
  {
    if ((datatype_class == H5T_INTEGER) && (datatype_size == 8))
    {
      // field is a 64 bit integer. read it in, and then convert to double.
      std::valarray<int64_t> vint;
      hid_t dataspace_id = H5Dget_space(dataset_id);
      hid_t ndims = H5Sget_simple_extent_ndims(dataspace_id);
      std::vector<hsize_t> dims_out(ndims);
      H5Sget_simple_extent_dims(dataspace_id, dims_out.data(), nullptr);
      hid_t memspace_id = H5Screate_simple(ndims, dims_out.data(), nullptr);
      vint.resize(dims_out[0]);
      H5Dread(dataset_id, H5T_NATIVE_INT64, memspace_id, dataspace_id, H5P_DEFAULT, &vint[0]);

      H5Tclose(datatype_id);
      H5Dclose(dataset_id);
      H5Sclose(memspace_id);
      H5Sclose(dataspace_id);

      // convert from int to double
      v.resize(dims_out[0]);
      for (size_t i = 0; i < vint.size(); i++)
      {
        v[i] = static_cast<double>(vint[i]);
      }

      return true;
    }
    else
    {
      // field is unsupported type, return false
      H5Tclose(datatype_id);
      H5Dclose(dataset_id);
      v.resize(0);
      return false;
    }
  }
  else
  {
    // must be a double field. read in the field.
    hid_t dataspace_id = H5Dget_space(dataset_id);
    hid_t ndims = H5Sget_simple_extent_ndims(dataspace_id);
    std::vector<hsize_t> dims_out(ndims);
    H5Sget_simple_extent_dims(dataspace_id, dims_out.data(), nullptr);
    hid_t memspace_id = H5Screate_simple(ndims, dims_out.data(), nullptr);
    v.resize(dims_out[0]);
    H5Dread(dataset_id, H5T_NATIVE_DOUBLE, memspace_id, dataspace_id, H5P_DEFAULT, &v[0]);

    H5Tclose(datatype_id);
    H5Dclose(dataset_id);
    H5Sclose(memspace_id);
    H5Sclose(dataspace_id);
    return true;
  }
} // End PIO_DATA_HDF5::read_dataset

bool PIO_DATA_HDF5::read_dataset(std::valarray<std::string>& v, const char* fieldname, int index)
{
  // read the contents of a dataset storing a string or array of strings
  //
  // Assumptions:
  //   - all strings in the array have the same length (we know this from the xrage code)
  //   - strings are not null terminated (because of fortran)
  //   - all strings are padded with spaces at the end (since it is fortran)
  //
  // The output is the argument v, and will be an array of strings that are
  // null terminated and have trailing spaces trimmed off.

  // add "..<index>" suffix to the field name
  vtkStdString fieldname0(fieldname);
  fieldname0 = fieldname0 + ".." + std::to_string(index);
  if (VarMMap.count(fieldname0.c_str()) != 1)
  {
    v.resize(0);
    return false;
  }

  // open the dataset
  hid_t dataset_id = H5Dopen(hdf5_file_id, fieldname0.c_str(), hid_t(0));
  hid_t datatype_id = H5Dget_type(dataset_id);
  H5T_class_t datatype_class = H5Tget_class(datatype_id);

  // check for proper datatype class
  if (datatype_class != H5T_STRING)
  {
    H5Tclose(datatype_id);
    H5Dclose(dataset_id);
    v.resize(0);
    return false;
  }
  else
  {
    // obtain the native type, needed to read the dataset
    hid_t native_type;
    if (datatype_class == H5T_BITFIELD)
    {
      native_type = H5Tcopy(datatype_id);
    }
    else
    {
      native_type = H5Tget_native_type(datatype_id, H5T_DIR_DEFAULT);
    }

    // the type size is the length of each string
    size_t lenstr = static_cast<int>(H5Tget_size(native_type));

    // number of dimensions should be 1, and the size of the first dimension
    // is the number of strings in the dataset
    hid_t dataspace_id = H5Dget_space(dataset_id);
    hid_t ndims = H5Sget_simple_extent_ndims(dataspace_id);
    std::vector<hsize_t> dims_out(ndims);
    H5Sget_simple_extent_dims(dataspace_id, dims_out.data(), nullptr);
    int numstr = static_cast<int>(dims_out[0]);

    // allocate a buffer to read the entire dataset
    std::vector<char> buffer(lenstr * numstr);
    H5Dread(dataset_id, native_type, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer.data());

    // copy one string into another buffer, add the terminating null,
    // copy to output array, and trim away trailing spaces
    v.resize(numstr);
    std::vector<char> buffer2(lenstr + 1);
    buffer2[buffer2.size() - 1] = '\0'; // terminating null
    int curPos = 0;
    for (int i = 0; i < numstr; i++)
    {
      for (size_t j = 0; j < lenstr; j++)
      {
        buffer2[j] = buffer[curPos];
        curPos++;
      }
      v[i] = buffer2.data();

      // trim trailing spaces
      std::string space = " ";
      size_t end = v[i].find_last_not_of(space);
      if (end == std::string::npos)
      {
        // the whole string is spaces
        v[i] = "";
      }
      else
      {
        v[i] = v[i].substr(0, end + 1);
      }
    }

    H5Tclose(datatype_id);
    H5Dclose(dataset_id);
    H5Sclose(dataspace_id);
    return true;
  }
} // End PIO_DATA_HDF5::read_dataset

bool PIO_DATA_HDF5::reconstruct_chunk_field(
  int64_t numcell, std::valarray<double>& va, const char* prefix, const char* var, int materialId)
{
  // some variables are stored in a compressed format, referred to as chunked fields.
  // the compression is similar to Compressed Sparse Row (CSR) with some modifications.
  // this mainly applies to material variables. this function unpacks a chunked field.
  std::string PreFix = std::string(prefix);
  std::string matname = PreFix + "_" + var;
  std::string chunk_nummat_string = PreFix + "_nummat";
  std::string chunk_mat_string = PreFix + "_mat";

  int matname_index;
  int chunk_nummat_index;
  int chunk_mat_index;

  // check that fields exist and get correct index values
  bool exists1 = has_scalar(matname.c_str(), matname_index);
  bool exists2 = has_scalar(chunk_nummat_string.c_str(), chunk_nummat_index);
  bool exists3 = has_scalar(chunk_mat_string.c_str(), chunk_mat_index);

  if (!exists1 || !exists2 || !exists3)
  {
    return false;
  }

  std::valarray<double> cl;
  std::valarray<int> chunk_nummat;
  std::valarray<int> chunk_mat;

  // read in field data
  bool read1 = read_dataset(cl, matname.c_str(), matname_index);
  bool read2 = set_scalar_field(chunk_nummat, chunk_nummat_string.c_str());
  bool read3 = set_scalar_field(chunk_mat, chunk_mat_string.c_str());

  if (!read1 || !read2 || !read3)
  {
    cl.resize(0);
    chunk_nummat.resize(0);
    chunk_mat.resize(0);
    return false;
  }

  // perform the reconstruction
  va.resize(numcell);
  va = 0;
  int64_t cmi = 0;
  int64_t cli = 0;
  for (int64_t l = 0; l < numcell; ++l)
  {
    for (int j = 0; j < chunk_nummat[l]; ++j)
    {
      if ((int(chunk_mat[cmi])) == materialId)
      {
        va[l] = cl[cli];
      }
      cmi++;
      cli++;
    }
  }

  return true;
} // End PIO_DATA_HDF5::reconstruct_chunk_field

int PIO_DATA_HDF5::get_pio_num() const
{
  // return the number of fields in the pio file
  return static_cast<int>(VarMMap.size());
}

PIO_FIELD* PIO_DATA_HDF5::get_pio_field() const
{
  return pio_field;
}

int PIO_DATA_HDF5::get_num_components(const char* fieldname) const
{
  // check if there is one component by adding a "..0" or "..-1" suffix
  // to the field name
  vtkStdString fieldname_base(fieldname);
  vtkStdString fieldname0 = fieldname_base + "..0";
  if (VarMMap.count(fieldname0.c_str()) == 1)
  {
    return 1;
  }
  vtkStdString fieldname1 = fieldname_base + "..-1";
  if (VarMMap.count(fieldname1.c_str()) == 1)
  {
    return 1;
  }

  // check for a vector field by starting the index at one and incrementing
  int num_components = 0;
  while (true)
  {
    vtkStdString cur_fieldname(fieldname_base + ".." + std::to_string(num_components + 1));
    if (VarMMap.count(cur_fieldname.c_str()) != 1)
    {
      break;
    }
    num_components++;
  }

  return num_components;
} // End PIO_DATA_HDF5::get_num_components

int PIO_DATA_HDF5::get_num_materials() const
{
  // for each material, there is a "matdef" field, and matdef is numbered
  // like a vector field, (matdef..1, matdef..2, etc.). so the number of
  // materials is the same as the number of components of matdef.
  return get_num_components("matdef");
}

int64_t PIO_DATA_HDF5::get_num_cells()
{
  std::valarray<int64_t> histsize;
  set_scalar_field(histsize, "hist_size");
  return histsize[histsize.size() - 1];
}

bool PIO_DATA_HDF5::has_scalar(const char* fieldname)
{
  // check if there is a scalar field with the given fieldname by trying an index of 0 and -1.
  // if found, return true
  vtkStdString fieldname0(fieldname);
  fieldname0 = fieldname0 + "..0";
  if (VarMMap.count(fieldname0.c_str()) == 1)
  {
    return true;
  }

  vtkStdString fieldname1(fieldname);
  fieldname1 = fieldname1 + "..-1";
  if (VarMMap.count(fieldname1.c_str()) == 1)
  {
    return true;
  }

  // field not found
  return false;
}

bool PIO_DATA_HDF5::has_scalar(const char* fieldname, int& index)
{
  // check if there is a scalar field with the given fieldname by trying an index of 0 and -1.
  // if found, return true and place the correct index in the index argument.
  vtkStdString fieldname0(fieldname);
  fieldname0 = fieldname0 + "..0";
  if (VarMMap.count(fieldname0.c_str()) == 1)
  {
    index = 0;
    return true;
  }

  vtkStdString fieldname1(fieldname);
  fieldname1 = fieldname1 + "..-1";
  if (VarMMap.count(fieldname1.c_str()) == 1)
  {
    index = -1;
    return true;
  }

  // field not found
  return false;
}

bool PIO_DATA_HDF5::has_vector(const char* fieldname)
{
  // check if there is a vector field with the given fieldname by trying an index of 1.
  // if found, return true
  vtkStdString fieldname0(fieldname);
  fieldname0 = fieldname0 + "..1";
  if (VarMMap.count(fieldname0.c_str()) == 1)
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool PIO_DATA_HDF5::has_field(const char* fieldname)
{
  // check if there is a field with the given fieldname by checking if it is
  // a scalar or vector field. if found, return true.
  return (has_scalar(fieldname) || has_vector(fieldname));
}

int PIO_DATA_HDF5::get_cycle()
{
  // return the cycle number
  std::valarray<int> controller_i;
  if (set_scalar_field(controller_i, "controller_i"))
  {
    return controller_i[0];
  }
  else
  {
    return -1;
  }
}

double PIO_DATA_HDF5::get_simtime()
{
  // return the simulation time
  std::valarray<double> controller_r8;
  if (set_scalar_field(controller_r8, "controller_r8"))
  {
    return controller_r8[0];
  }
  else
  {
    return -1;
  }
}

int PIO_DATA_HDF5::get_dimension()
{
  // return the number of dimensions of the problem
  // whether the problem is 1D, 2D, or 3D
  std::valarray<int64_t> amhc_i;
  if (set_scalar_field(amhc_i, "amhc_i"))
  {
    // Nnumdim is an enum element in PIOData.h,
    // currently defined as 42
    return static_cast<int>(amhc_i[Nnumdim]);
  }
  else
  {
    return -1;
  }
}

bool PIO_DATA_HDF5::get_gridsize(std::valarray<int>& v)
{
  std::valarray<int64_t> amhc_i;
  if (set_scalar_field(amhc_i, "amhc_i"))
  {
    // Nmesh0, Nmesh1, Nmesh2 are enum elements in PIOData.h
    v.resize(3);
    v[0] = static_cast<int>(amhc_i[Nmesh0]);
    v[1] = static_cast<int>(amhc_i[Nmesh1]);
    v[2] = static_cast<int>(amhc_i[Nmesh2]);
    return true;
  }
  else
  {
    v.resize(0);
    return false;
  }
}

bool PIO_DATA_HDF5::get_gridscale(std::valarray<double>& v)
{
  std::valarray<double> amhc_r8;
  if (set_scalar_field(amhc_r8, "amhc_r8"))
  {
    // Nd0, Nd1, Nd2 are enum elements in PIOData.h
    v.resize(3);
    v[0] = amhc_r8[Nd0];
    v[1] = amhc_r8[Nd1];
    v[2] = amhc_r8[Nd2];
    return true;
  }
  else
  {
    v.resize(0);
    return false;
  }
}

bool PIO_DATA_HDF5::get_gridorigin(std::valarray<double>& v)
{
  std::valarray<double> amhc_r8;
  if (set_scalar_field(amhc_r8, "amhc_r8"))
  {
    // NZero0, NZero1, NZero2 are enum elements in PIOData.h
    v.resize(3);
    v[0] = amhc_r8[NZero0];
    v[1] = amhc_r8[NZero1];
    v[2] = amhc_r8[NZero2];
    return true;
  }
  else
  {
    v.resize(0);
    return false;
  }
}

std::string PIO_DATA_HDF5::get_eap_version()
{
  int index;
  std::string fieldname = "l_eap_version";
  if (!has_scalar(fieldname.c_str(), index))
  {
    return std::string("");
  }

  std::valarray<std::string> eap_version;
  if (!read_dataset(eap_version, fieldname.c_str(), index))
  {
    return std::string("");
  }
  else
  {
    return eap_version[0];
  }
}

std::string PIO_DATA_HDF5::get_username()
{
  int index;
  std::string fieldname = "hist_usernm";
  if (!has_scalar(fieldname.c_str(), index))
  {
    return std::string("");
  }

  std::valarray<std::string> username;
  if (!read_dataset(username, fieldname.c_str(), index))
  {
    return std::string("");
  }
  else
  {
    return username[username.size() - 1];
  }
}

std::string PIO_DATA_HDF5::get_problemname()
{
  int index;
  std::string fieldname = "hist_prbnm";
  if (!has_scalar(fieldname.c_str(), index))
  {
    return std::string("");
  }

  std::valarray<std::string> problemname;
  if (!read_dataset(problemname, fieldname.c_str(), index))
  {
    return std::string("");
  }
  else
  {
    return problemname[problemname.size() - 1];
  }
}

bool PIO_DATA_HDF5::get_material_names(std::valarray<std::string>& matnames)
{
  // try to read material names from matident
  bool do_fallback = false;
  int index;
  std::string fieldname = "matident";
  if (!has_scalar(fieldname.c_str(), index))
  {
    matnames.resize(0);
    do_fallback = true;
  }
  if (!read_dataset(matnames, fieldname.c_str(), index))
  {
    matnames.resize(0);
    do_fallback = true;
  }

  if (!do_fallback)
  {
    return true;
  }

  // do the fallback plan.
  // the matident field is not present. obtain a material number from
  // the material's matdef field, aka, matdef_1, matdef_2, etc.
  // the material names will be Mat-1-<num1>, Mat-2-<num2>, etc., where
  // <num1> is the first number in matdef_1, <num2> is the first number in
  // matdef_2, etc.
  fieldname = "matdef";
  if (!has_vector(fieldname.c_str()))
  {
    matnames.resize(0);
    return false;
  }

  std::valarray<std::valarray<double>> matdef;
  if (!set_vector_field(matdef, fieldname.c_str()))
  {
    matnames.resize(0);
    return false;
  }

  matnames.resize(matdef.size());
  for (size_t i = 0; i < matdef.size(); i++)
  {
    matnames[i] =
      "Mat-" + std::to_string(i) + "-" + std::to_string(static_cast<int64_t>(matdef[i][0]));
  }

  return true;
}

bool PIO_DATA_HDF5::get_tracer_variable_names(std::valarray<std::string>& varnames)
{
  // read in tracer variable names, stored in tracer_type
  // return false if there are any issues reading in variable names
  // return true if read is good

  int index;
  std::string fieldname = "tracer_type";
  if (!has_scalar(fieldname.c_str(), index))
  {
    varnames.resize(0);
    return false;
  }

  if (!read_dataset(varnames, fieldname.c_str(), index))
  {
    varnames.resize(0);
    return false;
  }

  return true;
}

VTK_ABI_NAMESPACE_END
