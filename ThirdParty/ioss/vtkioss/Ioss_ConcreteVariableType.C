// Copyright(C) 1999-2020, 2022, 2023, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_ConcreteVariableType.h"
#include "Ioss_VariableType.h"
#include <cassert>
#include <string>

#include "Ioss_CodeTypes.h"

namespace {
  std::string X() { return {"x"}; }
  std::string Y() { return {"y"}; }
  std::string Z() { return {"z"}; }
  std::string Q() { return {"q"}; }
  std::string S() { return {"s"}; }

  std::string XX() { return {"xx"}; }
  std::string YY() { return {"yy"}; }
  std::string ZZ() { return {"zz"}; }
  std::string XY() { return {"xy"}; }
  std::string YZ() { return {"yz"}; }
  std::string ZX() { return {"zx"}; }
  std::string YX() { return {"yx"}; }
  std::string ZY() { return {"zy"}; }
  std::string XZ() { return {"xz"}; }

  std::string invalid() { return {"invalid"}; }
  std::string scalar() { return {"scalar"}; }
  std::string vector_2d() { return {"vector_2d"}; }
  std::string vector_3d() { return {"vector_3d"}; }
  std::string quaternion_2d() { return {"quaternion_2d"}; }
  std::string quaternion_3d() { return {"quaternion_3d"}; }
  std::string full_tensor_36() { return {"full_tensor_36"}; }
  std::string full_tensor_32() { return {"full_tensor_32"}; }
  std::string full_tensor_22() { return {"full_tensor_22"}; }
  std::string full_tensor_16() { return {"full_tensor_16"}; }
  std::string full_tensor_12() { return {"full_tensor_12"}; }
  std::string sym_tensor_33() { return {"sym_tensor_33"}; }
  std::string sym_tensor_31() { return {"sym_tensor_31"}; }
  std::string sym_tensor_21() { return {"sym_tensor_21"}; }
  std::string sym_tensor_13() { return {"sym_tensor_13"}; }
  std::string sym_tensor_11() { return {"sym_tensor_11"}; }
  std::string sym_tensor_10() { return {"sym_tensor_10"}; }
  std::string asym_tensor_03() { return {"asym_tensor_03"}; }
  std::string asym_tensor_02() { return {"asym_tensor_02"}; }
  std::string asym_tensor_01() { return {"asym_tensor_01"}; }
  std::string matrix_22() { return {"matrix_22"}; }
  std::string matrix_33() { return {"matrix_33"}; }
  std::string testonly() { return {"testonly"}; }
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
  Ioss::TestOnly::factory();
}

// ------------------------------------------------------------------------
Ioss::Invalid_Storage::Invalid_Storage() : Ioss::VariableType(invalid(), 0) {}

std::string Ioss::Invalid_Storage::label(int /*which*/, const char /*suffix_sep*/) const
{
  return "";
}

std::string Ioss::Invalid_Storage::label_name(const std::string &base, int /*which*/,
                                              const char /*suffix_sep*/, const char /*suffix_sep*/,
                                              bool /*suffices_uppercase*/) const
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

std::string Ioss::Scalar::label(IOSS_MAYBE_UNUSED int which, const char /*suffix_sep*/) const
{
  assert(which > 0 && which <= component_count());
  return "";
}

std::string Ioss::Scalar::label_name(const std::string &base, int /*which*/,
                                     const char /*suffix_sep*/, const char /*suffix_sep*/,
                                     bool /*suffices_uppercase*/) const
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

Ioss::TestOnly::TestOnly() : Ioss::VariableType(testonly(), 4) {}

std::string Ioss::TestOnly::label(int which, const char /*suffix_sep*/) const
{
  assert(which > 0 && which <= component_count());
  switch (which) {
  case 1: return "H";
  case 2: return "H2";
  case 3: return "H2O";
  case 4: return "O2";
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

void Ioss::TestOnly::factory() { static Ioss::TestOnly registerThis; }
