// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2021, Triad National Security, LLC
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-LANL-Triad-USGov
/**
 *
 * @class PIO_DATA
 * @brief   class for reading PIO (Parallel Input Output) data files
 *
 * This class reads in dump files generated from xRage, a LANL physics code.
 * The PIO (Parallel Input Output) library is used to create the dump files.
 *
 * @par Thanks:
 * Developed by Patricia Fasel at Los Alamos National Laboratory
 */

#if !defined(_PIODATA_H)
#define _PIODATA_H

#include "vtkABINamespace.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string.h>
#include <string>
#include <valarray>

VTK_ABI_NAMESPACE_BEGIN
struct Cstring_less
{
  bool operator()(const char* p, const char* q) const { return strcmp(p, q) < 0; };
};

// Class Declarations
class PIO_FIELD
{
public:
  char* pio_name;
  int index; // index = 0 is scalar, index = 1 is vector, index = -1 is request from input deck
  int64_t length;
  int64_t position;
  int64_t chksum;
  size_t cdata_len;
  bool read_field_data;
  friend class PIO_DATA;
  friend class PIO_DATA_PIO;
  friend class PIO_DATA_HDF5;

  double* data;
  char* cdata;
}; // End class PIO_FIELD

// Typedefs for the mapping between the names of the PIO blocks in the PIO file and
// the PIO_FIELD's used to store the data values in the PIO blocks.
typedef std::multimap<const char*, PIO_FIELD*, Cstring_less> VAR_MAP;
typedef VAR_MAP::iterator VMI;
typedef VAR_MAP::const_iterator CVMI;
typedef std::pair<VMI, VMI> VMP;
typedef std::pair<CVMI, CVMI> CVMP;

class PIO_DATA
{
public:
  PIO_DATA();
  virtual ~PIO_DATA() = 0;

  // stuff needed for interface, used in PIOAdaptor
  virtual bool good_read() = 0;
  virtual bool set_scalar_field(std::valarray<int>&, const char*) = 0;
  virtual bool set_scalar_field(std::valarray<int64_t>&, const char*) = 0;
  virtual bool set_scalar_field(std::valarray<double>&, const char*) = 0;
  virtual bool set_vector_field(std::valarray<std::valarray<double>>&, const char*) = 0;
  virtual int get_pio_num() const = 0;
  virtual PIO_FIELD* get_pio_field() const = 0;
  virtual int get_num_components(const char*) const = 0;
  virtual int get_num_materials() const = 0;
  virtual int64_t get_num_cells() = 0;
  virtual int get_cycle() = 0;
  virtual double get_simtime() = 0;
  virtual int get_dimension() = 0;
  virtual bool get_gridsize(std::valarray<int>&) = 0;
  virtual bool get_gridscale(std::valarray<double>&) = 0;
  virtual bool get_gridorigin(std::valarray<double>&) = 0;
  virtual std::string get_eap_version() = 0;
  virtual std::string get_username() = 0;
  virtual std::string get_problemname() = 0;
  virtual bool get_material_names(std::valarray<std::string>&) = 0;
  virtual bool get_tracer_variable_names(std::valarray<std::string>&) = 0;

  virtual bool has_field(const char*) = 0; // true if field exists
  virtual bool reconstruct_chunk_field(int64_t numcell, std::valarray<double>& va,
    const char* prefix, const char* var, int materialId) = 0;

  VAR_MAP VarMMap; // Multimap from pio_name to a PIO_FIELD class

}; // End class PIO_DATA

// Locations of various data items from the input arrays, amhc_i, amhc_r8,
// amch_l, and controller_r8
enum
{
  Ntime = 0,    // time = controller_r8[Ntime];
  Nnumdim = 42, // numdim = amhc_i[Nnumdim]
  Nmesh0 = 16,  // N[0] = amhc_i[Nmesh0]
  Nmesh1 = 17,  // N[1] = amhc_i[Nmesh1]
  Nmesh2 = 29,  // N[2] = amhc_i[Nmesh2]
  Nd0 = 21,     // d[0] = amhc_r8[Nd0]
  Nd1 = 22,     // d[1] = amhc_r8[Nd1]
  Nd2 = 38,     // d[2] = amhc_r8[Nd2]
  NZero0 = 19,  // Zero[0] = amhc_r8[NZero0]
  NZero1 = 20,  // Zero[1] = amhc_r8[NZero1]
  NZero2 = 35,  // Zero[2] = amhc_r8[NZero2]
  Ncylin = 1,   // cylindrically (axisymmetric) symmetric
                // geometry if amhc_l[Ncylin]!=0
  Nsphere = 8   // spherically symmetirc geometry if
                // amhc_l[Nsphere]!=0
};

VTK_ABI_NAMESPACE_END
#endif //! defined(_PIODATA_H)
