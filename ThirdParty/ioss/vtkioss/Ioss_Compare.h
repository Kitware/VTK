// Copyright(C) 1999-2020 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#ifndef IOSS_Ioss_Compare_h
#define IOSS_Ioss_Compare_h

namespace Ioss {
  class Region;
  struct MeshCopyOptions;
} // namespace Ioss

namespace Ioss {
  /* \brief Methods to compare databases.
   */
  class Compare
  {
  public:
    Compare()  = default;
    ~Compare() = default;

    // Compare the mesh in 'input_region_1' to 'input_region_2'.  Behavior can be controlled
    // via options in 'options'
    static bool compare_database(Ioss::Region &input_region_1, Ioss::Region &input_region_2,
                                 const Ioss::MeshCopyOptions &options);
  };
} // namespace Ioss
#endif
