// Copyright(C) 1999-2021, 2023, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_Field.h"        // for Field, etc
#include "Ioss_VariableType.h" // for VariableType
#include "transform/Iotr_MinMax.h"
#include <cmath>   // for fabs
#include <cstddef> // for size_t
#include <stdint.h>
#include <string> // for operator==, string

#include "Ioss_Transform.h" // for Factory, Transform
#include "Ioss_TransformFactory.h"

namespace Iotr {

  const MinMax_Factory *MinMax_Factory::factory()
  {
    static MinMax_Factory registerThis;
    return &registerThis;
  }

  MinMax_Factory::MinMax_Factory() : Ioss::TransformFactory("generic_minmax")
  {
    Ioss::TransformFactory::alias("generic_minmax", "minimum");
    Ioss::TransformFactory::alias("generic_minmax", "maximum");
    Ioss::TransformFactory::alias("generic_minmax", "absolute_minimum");
    Ioss::TransformFactory::alias("generic_minmax", "absolute_maximum");
  }

  Ioss::Transform *MinMax_Factory::make(const std::string &type) const { return new MinMax(type); }

  MinMax::MinMax(const std::string &type)
  {
    if (type == "minimum") {
      doMin = true;
      doAbs = false;
    }
    else if (type == "maximum") {
      doMin = false;
      doAbs = false;
    }
    else if (type == "absolute_minimum") {
      doMin = true;
      doAbs = true;
    }
    else if (type == "absolute_maximum") {
      doMin = false;
      doAbs = true;
    }
    else {
      doMin = false;
      doAbs = false;
    }
  }

  const Ioss::VariableType *MinMax::output_storage(const Ioss::VariableType *in) const
  {
    // Only operates on scalars...
    static const Ioss::VariableType *sca = Ioss::VariableType::factory("scalar");
    if (in == sca) {
      return sca;
    }
    return nullptr;
  }

  size_t MinMax::output_count(size_t /* in */) const
  {
    // Returns a single value...
    return 1;
  }

  bool MinMax::internal_execute(const Ioss::Field &field, void *data)
  {
    size_t count      = field.transformed_count();
    size_t components = field.transformed_storage()->component_count();
    size_t n          = count * components;
    if (field.get_type() == Ioss::Field::REAL) {
      auto  *rdata = static_cast<double *>(data);
      double value;
      if (doMin) {
        if (doAbs) {
          value = *std::min_element(&rdata[0], &rdata[n], [](double p1, double p2) {
            return std::fabs(p1) < std::fabs(p2);
          });
        }
        else {
          value = *std::min_element(&rdata[0], &rdata[n]);
        }
      }
      else { // doMax
        if (doAbs) {
          value = *std::max_element(&rdata[0], &rdata[n], [](double p1, double p2) {
            return std::fabs(p1) < std::fabs(p2);
          });
        }
        else {
          value = *std::max_element(&rdata[0], &rdata[n]);
        }
      }
      rdata[0] = value;
    }
    else if (field.get_type() == Ioss::Field::INTEGER) {
      int *idata = static_cast<int *>(data);
      int  value;
      if (doMin) {
        if (doAbs) {
          value = *std::min_element(&idata[0], &idata[n],
                                    [](int p1, int p2) { return std::fabs(p1) < std::fabs(p2); });
        }
        else {
          value = *std::min_element(&idata[0], &idata[n]);
        }
      }
      else { // doMax
        if (doAbs) {
          value = *std::max_element(&idata[0], &idata[n],
                                    [](int p1, int p2) { return std::fabs(p1) < std::fabs(p2); });
        }
        else {
          value = *std::max_element(&idata[0], &idata[n]);
        }
      }
      idata[0] = value;
    }
    else if (field.get_type() == Ioss::Field::INT64) {
      auto   *idata = static_cast<int64_t *>(data);
      int64_t value;
      if (doMin) {
        if (doAbs) {
          value = *std::min_element(&idata[0], &idata[n], [](int64_t p1, int64_t p2) {
            return std::fabs(p1) < std::fabs(p2);
          });
        }
        else {
          value = *std::min_element(&idata[0], &idata[n]);
        }
      }
      else { // doMax
        if (doAbs) {
          value = *std::max_element(&idata[0], &idata[n], [](int64_t p1, int64_t p2) {
            return std::fabs(p1) < std::fabs(p2);
          });
        }
        else {
          value = *std::max_element(&idata[0], &idata[n]);
        }
      }
      idata[0] = value;
    }
    return true;
  }
} // namespace Iotr
