// Copyright(C) 1999-2021, 2023 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_CoordinateFrame.h"
#include "Ioss_Utils.h"
#include "vtk_fmt.h"
#include VTK_FMT(fmt/ostream.h)

namespace Ioss {
  CoordinateFrame::CoordinateFrame(int64_t my_id, char my_tag, const double *point_list)
      : id_(my_id), tag_(my_tag)
  {
    pointList_.reserve(9);
    for (int i = 0; i < 9; i++) {
      pointList_.push_back(point_list[i]);
    }
  }

  int64_t CoordinateFrame::id() const { return id_; }
  char    CoordinateFrame::tag() const { return tag_; }

  const double *CoordinateFrame::coordinates() const { return Data(pointList_); }
  const double *CoordinateFrame::origin() const { return &pointList_[0]; }
  const double *CoordinateFrame::axis_3_point() const { return &pointList_[3]; }
  const double *CoordinateFrame::plane_1_3_point() const { return &pointList_[6]; }

  bool Ioss::CoordinateFrame::equal_(const Ioss::CoordinateFrame &rhs, bool quiet) const
  {
    if (this->id_ != rhs.id_) {
      if (!quiet) {
        fmt::print(Ioss::OUTPUT(), "CoordinateFrame : ID mismatch ({} vs. {})\n", this->id_,
                   rhs.id_);
      }
      return false;
    }

    if (this->pointList_ != rhs.pointList_) {
      if (!quiet) {
        fmt::print(Ioss::OUTPUT(), "CoordinateFrame : Point list mismatch ([ ");
        for (const auto &point : this->pointList_) {
          fmt::print(Ioss::OUTPUT(), "{} ", point);
        }
        fmt::print(Ioss::OUTPUT(), "] vs [");
        for (const auto &point : rhs.pointList_) {
          fmt::print(Ioss::OUTPUT(), "{} ", point);
        }
        fmt::print(Ioss::OUTPUT(), "])\n");
      }
      return false;
    }

    if (this->id_ != rhs.id_) {
      if (!quiet) {
        fmt::print(Ioss::OUTPUT(), "CoordinateFrame : TAG mismatch ({} vs. {})\n", this->tag_,
                   rhs.tag_);
      }
      return false;
    }

    return true;
  }
  bool Ioss::CoordinateFrame::operator==(const Ioss::CoordinateFrame &rhs) const
  {
    return equal_(rhs, true);
  }

  bool Ioss::CoordinateFrame::operator!=(const Ioss::CoordinateFrame &rhs) const
  {
    return !(*this == rhs);
  }

  bool Ioss::CoordinateFrame::equal(const Ioss::CoordinateFrame &rhs) const
  {
    return equal_(rhs, false);
  }

} // namespace Ioss
