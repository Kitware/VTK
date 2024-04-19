// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2021, Triad National Security, LLC
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-LANL-Triad-USGov
/**
 *
 * @class PIO_DATA_HDF5
 * @brief   class for reading PIO (Parallel Input Output) files in HDF5 format
 *
 * This class reads in dump files generated from xRage, a LANL physics code.
 * The PIO (Parallel Input Output) library is used to create the dump files,
 * and the dump files are written in HDF5 format.
 *
 * This class represents one dump file.
 *
 * The HDF5 format has all datasets in the root group. Each dataset represents
 * a field in the pio format. Dataset names are in the form of [name]..[index].
 *
 * For scalar fields, index will be either 0 or -1, with -1 indicating that
 * the field was a derived field requested in the input deck.
 *
 * For vector fields, each component of the vector is separated into its own
 * dataset, with the index value beginning at 1, and incremented for each
 * component. For example, the vector field cell_center is composed of the
 * datasets "cell_center..1", "cell_center..2", and "cell_center..3".
 *
 * @par Thanks:
 * Developed by Boonthanome Nouanesengsy at Los Alamos National Laboratory
 */

#if !defined(_PIODATAHDF5_H)
#define _PIODATAHDF5_H

#include <PIOData.h>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string.h>
#include <string>
#include <valarray>
#include <vtk_hdf5.h>

VTK_ABI_NAMESPACE_BEGIN

class PIO_DATA_HDF5 : public PIO_DATA
{
public:
  PIO_DATA_HDF5(const char* piofile);
  ~PIO_DATA_HDF5() override;

  bool set_scalar_field(std::valarray<int>&, const char*) override;
  bool set_scalar_field(std::valarray<int64_t>&, const char*) override;
  bool set_scalar_field(std::valarray<double>&, const char*) override;
  bool set_vector_field(std::valarray<std::valarray<double>>&, const char*) override;
  bool read_dataset(std::valarray<double>&, const char*, int index);
  bool read_dataset(std::valarray<std::string>&, const char*, int index);

  bool has_scalar(const char*);
  bool has_scalar(const char*, int&);
  bool has_vector(const char*);
  bool has_field(const char*) override;

  bool good_read() override;
  int get_pio_num() const override;
  int get_num_components(const char*) const override;
  int get_num_materials() const override;
  int64_t get_num_cells() override;
  int get_cycle() override;
  double get_simtime() override;
  int get_dimension() override;
  bool get_gridsize(std::valarray<int>&) override;
  bool get_gridscale(std::valarray<double>&) override;
  bool get_gridorigin(std::valarray<double>&) override;
  std::string get_eap_version() override;
  std::string get_username() override;
  std::string get_problemname() override;
  bool get_material_names(std::valarray<std::string>&) override;
  bool get_tracer_variable_names(std::valarray<std::string>&) override;
  PIO_FIELD* get_pio_field() const override;

  bool reconstruct_chunk_field(int64_t numcell, std::valarray<double>& va, const char* prefix,
    const char* var, int materialId) override;

  // cell_active is a mask array to determine which cells are active.
  // since we could be using cell_active multiple times,
  // only load cell_active array once, and keep it loaded
  std::valarray<int> cell_active;
  bool cell_active_loaded;

private:
  bool is_good_read;
  hid_t hdf5_file_id;
  PIO_FIELD* pio_field;

}; // End class PIO_DATA_HDF5

VTK_ABI_NAMESPACE_END
#endif //! defined(_PIODATAHDF5_H)
