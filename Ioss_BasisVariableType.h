/*
 * Copyright(C) 2024 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
#pragma once

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

#include "Ioss_CodeTypes.h"
#include <string>

#include "Ioss_VariableType.h"

namespace Ioss {
  struct BasisComponent
  {
    int    subc_dim;
    int    subc_ordinal;
    int    subc_dof_ordinal;
    int    subc_num_dof;
    double xi;
    double eta;
    double zeta;
  };

  struct Basis
  {
    size_t                      size() const { return basies.size(); }
    std::vector<BasisComponent> basies;
  };

  class IOSS_EXPORT BasisVariableType : public VariableType
  {
  public:
    //  'which' is 1-based
    IOSS_NODISCARD std::string label(int which, const char /* suffix_sep */) const override
    {
      assert(which > 0 && which <= component_count());
      if (component_count() == 1) {
        return "";
      }
      return VariableType::numeric_label(which - 1, component_count(), name());
    }

    BasisVariableType(const std::string &my_name, const Ioss::Basis &basis, bool delete_me)
        : Ioss::VariableType(my_name, basis.size(), delete_me), m_basis_type_(my_name),
          m_basis_(basis)
    {
    }

    BasisVariableType(const BasisVariableType &) = delete;

    IOSS_NODISCARD VariableType::Type type() const override { return Type::BASIS; }
    IOSS_NODISCARD std::string type_string() const override { return "Basis"; }

    IOSS_NODISCARD const Ioss::Basis &get_basis() const { return m_basis_; }
    IOSS_NODISCARD const Ioss::BasisComponent &get_basis_component(int which) const
    {
      assert(which > 0 && which <= component_count());
      return m_basis_.basies[which - 1];
    }

    void print() const override final;

  private:
    std::string m_basis_type_{};
    Ioss::Basis m_basis_{};
  };
} // namespace Ioss

#if 0
typedef struct ex_basis
{
  /*
   * subc_dim: dimension of the subcell associated with the specified DoF ordinal
   *      -- 0 node, 1 edge, 2 face, 3 volume [Range: 0..3]
   * subc_ordinal: ordinal of the subcell relative to its parent cell
   *      -- 0..n for each ordinal with the same subc dim [Range: <= DoF ordinal]
   * subc_dof_ordinal: ordinal of the DoF relative to the subcell
   * subc_num_dof: cardinality of the DoF set associated with this subcell.
   * xi, eta, mu (ξ, η, ζ): Parametric coordinate location of the DoF
   *      -- (Only first ndim values are valid)
   */
  char    name[EX_MAX_NAME + 1];
  int     cardinality; /* number of `basis` points == dimension of non-null subc_*, xi, eta, mu */
  int    *subc_dim;
  int    *subc_ordinal;
  int    *subc_dof_ordinal;
  int    *subc_num_dof;
  double *xi;
  double *eta;
  double *zeta;
} ex_basis;
#endif
