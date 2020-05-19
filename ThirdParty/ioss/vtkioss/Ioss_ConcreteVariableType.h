// Copyright(C) 1999-2017 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//
//     * Neither the name of NTESS nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef IOSS_Ioss_ConcreteVariableType_h
#define IOSS_Ioss_ConcreteVariableType_h

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
    std::string label(int which, char suffix_sep = '_') const override;
    std::string label_name(const std::string &base, int /*which*/, char suffix_sep) const override;
    int         suffix_count() const override { return 0; }
    static void factory();

  protected:
    Invalid_Storage();

  private:
    Invalid_Storage(const Invalid_Storage &);
  };

  class Scalar : public VariableType
  {
  public:
    std::string label(int which, char suffix_sep = '_') const override;
    std::string label_name(const std::string &base, int /*which*/, char suffix_sep) const override;
    int         suffix_count() const override { return 0; }
    static void factory();

  protected:
    Scalar();
    Scalar(const Scalar &) = delete;
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
#endif
