// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <Ioss_ConcreteVariableType.h>
#include <Ioss_VariableType.h>

#include <cassert>
#include <string>

namespace {
  const std::string X() { return std::string("x"); }
  const std::string Y() { return std::string("y"); }
  const std::string Z() { return std::string("z"); }
  const std::string Q() { return std::string("q"); }
  const std::string S() { return std::string("s"); }

  const std::string XX() { return std::string("xx"); }
  const std::string YY() { return std::string("yy"); }
  const std::string ZZ() { return std::string("zz"); }
  const std::string XY() { return std::string("xy"); }
  const std::string YZ() { return std::string("yz"); }
  const std::string ZX() { return std::string("zx"); }
  const std::string YX() { return std::string("yx"); }
  const std::string ZY() { return std::string("zy"); }
  const std::string XZ() { return std::string("xz"); }

  const std::string invalid() { return std::string("invalid"); }
  const std::string scalar() { return std::string("scalar"); }
  const std::string vector_2d() { return std::string("vector_2d"); }
  const std::string vector_3d() { return std::string("vector_3d"); }
  const std::string quaternion_2d() { return std::string("quaternion_2d"); }
  const std::string quaternion_3d() { return std::string("quaternion_3d"); }
  const std::string full_tensor_36() { return std::string("full_tensor_36"); }
  const std::string full_tensor_32() { return std::string("full_tensor_32"); }
  const std::string full_tensor_22() { return std::string("full_tensor_22"); }
  const std::string full_tensor_16() { return std::string("full_tensor_16"); }
  const std::string full_tensor_12() { return std::string("full_tensor_12"); }
  const std::string sym_tensor_33() { return std::string("sym_tensor_33"); }
  const std::string sym_tensor_31() { return std::string("sym_tensor_31"); }
  const std::string sym_tensor_21() { return std::string("sym_tensor_21"); }
  const std::string sym_tensor_13() { return std::string("sym_tensor_13"); }
  const std::string sym_tensor_11() { return std::string("sym_tensor_11"); }
  const std::string sym_tensor_10() { return std::string("sym_tensor_10"); }
  const std::string asym_tensor_03() { return std::string("asym_tensor_03"); }
  const std::string asym_tensor_02() { return std::string("asym_tensor_02"); }
  const std::string asym_tensor_01() { return std::string("asym_tensor_01"); }
  const std::string matrix_22() { return std::string("matrix_22"); }
  const std::string matrix_33() { return std::string("matrix_33"); }
} // namespace

Ioss::StorageInitializer::StorageInitializer()
{
  // List all storage types here with a call to their factory method.
  // This is Used to get the linker to pull in all needed libraries.
  Ioss::Invalid_Storage::factory();
  Ioss::Scalar::factory();
  Ioss::Vector_2D::factory();
  Ioss::Vector_3D::factory();
  Ioss::Quaternion_2D::factory();
  Ioss::Quaternion_3D::factory();
  Ioss::Full_Tensor_36::factory();
  Ioss::Full_Tensor_32::factory();
  Ioss::Full_Tensor_22::factory();
  Ioss::Full_Tensor_16::factory();
  Ioss::Full_Tensor_12::factory();
  Ioss::Sym_Tensor_33::factory();
  Ioss::Sym_Tensor_31::factory();
  Ioss::Sym_Tensor_21::factory();
  Ioss::Sym_Tensor_13::factory();
  Ioss::Sym_Tensor_11::factory();
  Ioss::Sym_Tensor_10::factory();
  Ioss::Asym_Tensor_03::factory();
  Ioss::Asym_Tensor_02::factory();
  Ioss::Asym_Tensor_01::factory();
  Ioss::Matrix_22::factory();
  Ioss::Matrix_33::factory();
}

// ------------------------------------------------------------------------
Ioss::Invalid_Storage::Invalid_Storage() : Ioss::VariableType(invalid(), 0) {}

std::string Ioss::Invalid_Storage::label(int /*which*/, const char /*suffix_sep*/) const
{
  return "";
}

std::string Ioss::Invalid_Storage::label_name(const std::string &base, int /*which*/,
                                              const char /*suffix_sep*/) const
{
  return base;
}

// ------------------------------------------------------------------------

Ioss::Scalar::Scalar() : Ioss::VariableType(scalar(), 1)
{
  // Sierra uses 'REAL' as a variable storage type
  Ioss::VariableType::alias("scalar", "real");
  // Sierra also uses 'INTEGER' as a variable storage type
  Ioss::VariableType::alias("scalar", "integer");
  Ioss::VariableType::alias("scalar", "unsigned integer");
}

std::string Ioss::Scalar::label(int which, const char /*suffix_sep*/) const
{
  assert(which > 0 && which <= component_count());
  switch (which) {
  case 1: return "";
  default: return "";
  }
}

std::string Ioss::Scalar::label_name(const std::string &base, int /*which*/,
                                     const char /*suffix_sep*/) const
{
  return base;
}

// ------------------------------------------------------------------------

Ioss::Vector_2D::Vector_2D() : Ioss::VariableType(vector_2d(), 2)
{
  Ioss::VariableType::alias("vector_2d", "pair");
}

std::string Ioss::Vector_2D::label(int which, const char /*suffix_sep*/) const
{
  assert(which > 0 && which <= component_count());
  switch (which) {
  case 1: return X();
  case 2: return Y();
  default: return "";
  }
}

// ------------------------------------------------------------------------

Ioss::Vector_3D::Vector_3D() : Ioss::VariableType(vector_3d(), 3) {}

std::string Ioss::Vector_3D::label(int which, const char /*suffix_sep*/) const
{
  assert(which > 0 && which <= component_count());
  switch (which) {
  case 1: return X();
  case 2: return Y();
  case 3: return Z();
  default: return "";
  }
}

// ------------------------------------------------------------------------

Ioss::Quaternion_2D::Quaternion_2D() : Ioss::VariableType(quaternion_2d(), 2) {}

std::string Ioss::Quaternion_2D::label(int which, const char /*suffix_sep*/) const
{
  assert(which > 0 && which <= component_count());
  switch (which) {
  case 1: return S();
  case 2: return Q();
  default: return "";
  }
}

// ------------------------------------------------------------------------

Ioss::Quaternion_3D::Quaternion_3D() : Ioss::VariableType(quaternion_3d(), 4) {}

std::string Ioss::Quaternion_3D::label(int which, const char /*suffix_sep*/) const
{
  assert(which > 0 && which <= component_count());
  switch (which) {
  case 1: return X();
  case 2: return Y();
  case 3: return Z();
  case 4: return Q();
  default: return "";
  }
}

// ------------------------------------------------------------------------

Ioss::Full_Tensor_36::Full_Tensor_36() : Ioss::VariableType(full_tensor_36(), 9) {}

std::string Ioss::Full_Tensor_36::label(int which, const char /*suffix_sep*/) const
{
  assert(which > 0 && which <= component_count());
  switch (which) {
  case 1: return XX();
  case 2: return YY();
  case 3: return ZZ();
  case 4: return XY();
  case 5: return YZ();
  case 6: return ZX();
  case 7: return YX();
  case 8: return ZY();
  case 9: return XZ();
  default: return "";
  }
}

// ------------------------------------------------------------------------

Ioss::Full_Tensor_32::Full_Tensor_32() : Ioss::VariableType(full_tensor_32(), 5) {}

std::string Ioss::Full_Tensor_32::label(int which, const char /*suffix_sep*/) const
{
  assert(which > 0 && which <= component_count());
  switch (which) {
  case 1: return XX();
  case 2: return YY();
  case 3: return ZZ();
  case 4: return XY();
  case 5: return YX();
  default: return "";
  }
}

// ------------------------------------------------------------------------

Ioss::Full_Tensor_22::Full_Tensor_22() : Ioss::VariableType(full_tensor_22(), 4) {}

std::string Ioss::Full_Tensor_22::label(int which, const char /*suffix_sep*/) const
{
  assert(which > 0 && which <= component_count());
  switch (which) {
  case 1: return XX();
  case 2: return YY();
  case 3: return XY();
  case 4: return YX();
  default: return "";
  }
}

// ------------------------------------------------------------------------

Ioss::Full_Tensor_16::Full_Tensor_16() : Ioss::VariableType(full_tensor_16(), 7) {}

std::string Ioss::Full_Tensor_16::label(int which, const char /*suffix_sep*/) const
{
  assert(which > 0 && which <= component_count());
  switch (which) {
  case 1: return XX();
  case 2: return XY();
  case 3: return YZ();
  case 4: return ZX();
  case 5: return YX();
  case 6: return ZY();
  case 7: return XZ();
  default: return "";
  }
}

// ------------------------------------------------------------------------

Ioss::Full_Tensor_12::Full_Tensor_12() : Ioss::VariableType(full_tensor_12(), 3) {}

std::string Ioss::Full_Tensor_12::label(int which, const char /*suffix_sep*/) const
{
  assert(which > 0 && which <= component_count());
  switch (which) {
  case 1: return XX();
  case 2: return XY();
  case 3: return YX();
  default: return "";
  }
}

// ------------------------------------------------------------------------

Ioss::Sym_Tensor_33::Sym_Tensor_33() : Ioss::VariableType(sym_tensor_33(), 6) {}

std::string Ioss::Sym_Tensor_33::label(int which, const char /*suffix_sep*/) const
{
  assert(which > 0 && which <= component_count());
  switch (which) {
  case 1: return XX();
  case 2: return YY();
  case 3: return ZZ();
  case 4: return XY();
  case 5: return YZ();
  case 6: return ZX();
  default: return "";
  }
}

// ------------------------------------------------------------------------

Ioss::Sym_Tensor_31::Sym_Tensor_31() : Ioss::VariableType(sym_tensor_31(), 4) {}

std::string Ioss::Sym_Tensor_31::label(int which, const char /*suffix_sep*/) const
{
  assert(which > 0 && which <= component_count());
  switch (which) {
  case 1: return XX();
  case 2: return YY();
  case 3: return ZZ();
  case 4: return XY();
  default: return "";
  }
}

// ------------------------------------------------------------------------

Ioss::Sym_Tensor_21::Sym_Tensor_21() : Ioss::VariableType(sym_tensor_21(), 3) {}

std::string Ioss::Sym_Tensor_21::label(int which, const char /*suffix_sep*/) const
{
  assert(which > 0 && which <= component_count());
  switch (which) {
  case 1: return XX();
  case 2: return YY();
  case 3: return XY();
  default: return "";
  }
}

// ------------------------------------------------------------------------

Ioss::Sym_Tensor_13::Sym_Tensor_13() : Ioss::VariableType(sym_tensor_13(), 4) {}

std::string Ioss::Sym_Tensor_13::label(int which, const char /*suffix_sep*/) const
{
  assert(which > 0 && which <= component_count());
  switch (which) {
  case 1: return XX();
  case 2: return XY();
  case 3: return YZ();
  case 4: return ZX();
  default: return "";
  }
}

// ------------------------------------------------------------------------

Ioss::Sym_Tensor_11::Sym_Tensor_11() : Ioss::VariableType(sym_tensor_11(), 2) {}

std::string Ioss::Sym_Tensor_11::label(int which, const char /*suffix_sep*/) const
{
  assert(which > 0 && which <= component_count());
  switch (which) {
  case 1: return XX();
  case 2: return XY();
  default: return "";
  }
}

// ------------------------------------------------------------------------

Ioss::Sym_Tensor_10::Sym_Tensor_10() : Ioss::VariableType(sym_tensor_10(), 1) {}

std::string Ioss::Sym_Tensor_10::label(int which, const char /*suffix_sep*/) const
{
  assert(which > 0 && which <= component_count());
  switch (which) {
  case 1: return XX();
  default: return "";
  }
}

// ------------------------------------------------------------------------

Ioss::Asym_Tensor_03::Asym_Tensor_03() : Ioss::VariableType(asym_tensor_03(), 3) {}

std::string Ioss::Asym_Tensor_03::label(int which, const char /*suffix_sep*/) const
{
  assert(which > 0 && which <= component_count());
  switch (which) {
  case 1: return XY();
  case 2: return YZ();
  case 3: return ZX();
  default: return "";
  }
}

// ------------------------------------------------------------------------

Ioss::Asym_Tensor_02::Asym_Tensor_02() : Ioss::VariableType(asym_tensor_02(), 2) {}

std::string Ioss::Asym_Tensor_02::label(int which, const char /*suffix_sep*/) const
{
  assert(which > 0 && which <= component_count());
  switch (which) {
  case 1: return XY();
  case 2: return YZ();
  default: return "";
  }
}

// ------------------------------------------------------------------------

Ioss::Asym_Tensor_01::Asym_Tensor_01() : Ioss::VariableType(asym_tensor_01(), 1) {}

std::string Ioss::Asym_Tensor_01::label(int which, const char /*suffix_sep*/) const
{
  assert(which > 0 && which <= component_count());
  switch (which) {
  case 1: return XY();
  default: return "";
  }
}

// ------------------------------------------------------------------------

Ioss::Matrix_22::Matrix_22() : Ioss::VariableType(matrix_22(), 4) {}

std::string Ioss::Matrix_22::label(int which, const char /*suffix_sep*/) const
{
  assert(which > 0 && which <= component_count());
  switch (which) {
  case 1: return XX();
  case 2: return XY();
  case 3: return YX();
  case 4: return YY();
  default: return "";
  }
}

// ------------------------------------------------------------------------

Ioss::Matrix_33::Matrix_33() : Ioss::VariableType(matrix_33(), 9) {}

std::string Ioss::Matrix_33::label(int which, const char /*suffix_sep*/) const
{
  assert(which > 0 && which <= component_count());
  switch (which) {
  case 1: return XX();
  case 2: return XY();
  case 3: return XZ();
  case 4: return YX();
  case 5: return YY();
  case 6: return YZ();
  case 7: return ZX();
  case 8: return ZY();
  case 9: return ZZ();
  default: return "";
  }
}

// Define all factories here:
void Ioss::Invalid_Storage::factory() { static Ioss::Invalid_Storage registerThis; }

void Ioss::Scalar::factory() { static Ioss::Scalar registerThis; }

void Ioss::Vector_2D::factory() { static Ioss::Vector_2D registerThis; }

void Ioss::Vector_3D::factory() { static Ioss::Vector_3D registerThis; }

void Ioss::Quaternion_2D::factory() { static Ioss::Quaternion_2D registerThis; }

void Ioss::Quaternion_3D::factory() { static Ioss::Quaternion_3D registerThis; }

void Ioss::Full_Tensor_36::factory() { static Ioss::Full_Tensor_36 registerThis; }

void Ioss::Full_Tensor_32::factory() { static Ioss::Full_Tensor_32 registerThis; }

void Ioss::Full_Tensor_22::factory() { static Ioss::Full_Tensor_22 registerThis; }

void Ioss::Full_Tensor_16::factory() { static Ioss::Full_Tensor_16 registerThis; }

void Ioss::Full_Tensor_12::factory() { static Ioss::Full_Tensor_12 registerThis; }

void Ioss::Sym_Tensor_33::factory() { static Ioss::Sym_Tensor_33 registerThis; }

void Ioss::Sym_Tensor_31::factory() { static Ioss::Sym_Tensor_31 registerThis; }

void Ioss::Sym_Tensor_21::factory() { static Ioss::Sym_Tensor_21 registerThis; }

void Ioss::Sym_Tensor_13::factory() { static Ioss::Sym_Tensor_13 registerThis; }

void Ioss::Sym_Tensor_11::factory() { static Ioss::Sym_Tensor_11 registerThis; }

void Ioss::Sym_Tensor_10::factory() { static Ioss::Sym_Tensor_10 registerThis; }

void Ioss::Asym_Tensor_03::factory() { static Ioss::Asym_Tensor_03 registerThis; }

void Ioss::Asym_Tensor_02::factory() { static Ioss::Asym_Tensor_02 registerThis; }

void Ioss::Asym_Tensor_01::factory() { static Ioss::Asym_Tensor_01 registerThis; }

void Ioss::Matrix_22::factory() { static Ioss::Matrix_22 registerThis; }

void Ioss::Matrix_33::factory() { static Ioss::Matrix_33 registerThis; }
