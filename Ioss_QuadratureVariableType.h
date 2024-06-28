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
  struct QuadraturePoint
  {
    double xi;
    double eta;
    double zeta;
    double weight;
  };

  class IOSS_EXPORT QuadratureVariableType : public VariableType
  {
  public:
    //  'which' is 1-based
    IOSS_NODISCARD std::string label(int which, const char /* suffix_sep */) const override
    {
      assert(which > 0 && which <= component_count());
      if (component_count() == 1) {
        return "";
      }
      return VariableType::numeric_label(which, component_count(), name());
    }

    QuadratureVariableType(const std::string                        &my_name,
                           const std::vector<Ioss::QuadraturePoint> &quad_points, bool delete_me)
        : Ioss::VariableType(Ioss::Utils::lowercase(my_name), quad_points.size(), delete_me),
          m_quadratureType_(my_name), m_quadrature_(quad_points)
    {
    }

    QuadratureVariableType(const QuadratureVariableType &) = delete;

    IOSS_NODISCARD VariableType::Type type() const override { return Type::QUADRATURE; }
    IOSS_NODISCARD std::string type_string() const override { return "Quadrature"; }

    IOSS_NODISCARD std::vector<Ioss::QuadraturePoint> get_quadrature() const
    {
      return m_quadrature_;
    }
    IOSS_NODISCARD Ioss::QuadraturePoint get_quadrature_component(int which) const
    {
      assert(which > 0 && which <= component_count());
      return m_quadrature_[which - 1];
    }

    void print() const override final;

  private:
    std::string                        m_quadratureType_{};
    std::vector<Ioss::QuadraturePoint> m_quadrature_{};
  };
} // namespace Ioss

#if 0
typedef struct ex_quadrature
{
  char    name[EX_MAX_NAME + 1];
  int     cardinality; /* Number of quadrature points */
  int     dimension;   /* 1,2,3 -- spatial dimension of points */
  double *xi;          /* xi  (x) coordinate of points; dimension = cardinality  or NULL */
  double *
      eta; /* eta (y) coordinate of points; dimension = cardinality if dimension = 2 or 3 or NULL */
  double
      *zeta; /* zeta (z) coordinate of points; dimension = cardinality if dimension == 3. or NULL */
  double *weight; /* weights for each point; dimension = cardinality or NULL */
} ex_quadrature;
#endif
