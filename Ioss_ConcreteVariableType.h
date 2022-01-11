// Copyright(C) 1999-2020, 2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "vtk_ioss_mangle.h"

#include <Ioss_VariableType.h> // for VariableType
#include <string>              // for string

namespace Ioss {
  class StorageInitializer
  {
  public:
    StorageInitializer();
    // Assignment operator
    // Copy constructor
  };

#define MAKE_CLASS(X)                                                                              \
  class X : public VariableType                                                                    \
  {                                                                                                \
  public:                                                                                          \
    std::string label(int which, const char suffix_sep = '_') const override;                      \
    static void factory();                                                                         \
                                                                                                   \
  protected:                                                                                       \
    X();                                                                                           \
                                                                                                   \
  private:                                                                                         \
    X(const X &);                                                                                  \
  }

  class Invalid_Storage : public VariableType
  {
  public:
    Invalid_Storage(const Invalid_Storage &) = delete;
    std::string label(int which, const char suffix_sep = '_') const override;
    std::string label_name(const std::string &base, int /*which*/, char suffix_sep,
                           bool suffices_uppercase) const override;
    int         suffix_count() const override { return 0; }
    static void factory();

  protected:
    Invalid_Storage();
  };

  class Scalar : public VariableType
  {
  public:
    Scalar(const Scalar &) = delete;
    std::string label(int which, const char suffix_sep = '_') const override;
    std::string label_name(const std::string &base, int /*which*/, char suffix_sep,
                           bool suffices_uppercase) const override;
    int         suffix_count() const override { return 0; }
    static void factory();

  protected:
    Scalar();
  };

  MAKE_CLASS(Vector_2D);
  MAKE_CLASS(Vector_3D);
  MAKE_CLASS(Quaternion_2D);
  MAKE_CLASS(Quaternion_3D);
  MAKE_CLASS(Full_Tensor_36);
  MAKE_CLASS(Full_Tensor_32);
  MAKE_CLASS(Full_Tensor_22);
  MAKE_CLASS(Full_Tensor_16);
  MAKE_CLASS(Full_Tensor_12);
  MAKE_CLASS(Sym_Tensor_33);
  MAKE_CLASS(Sym_Tensor_31);
  MAKE_CLASS(Sym_Tensor_21);
  MAKE_CLASS(Sym_Tensor_13);
  MAKE_CLASS(Sym_Tensor_11);
  MAKE_CLASS(Sym_Tensor_10);
  MAKE_CLASS(Asym_Tensor_03);
  MAKE_CLASS(Asym_Tensor_02);
  MAKE_CLASS(Asym_Tensor_01);
  MAKE_CLASS(Matrix_22);
  MAKE_CLASS(Matrix_33);
} // namespace Ioss
