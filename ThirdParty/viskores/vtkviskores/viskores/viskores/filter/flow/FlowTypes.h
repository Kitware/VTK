//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_filter_flow_FlowTypes_h
#define viskores_filter_flow_FlowTypes_h

namespace viskores
{
namespace filter
{
namespace flow
{
enum class IntegrationSolverType
{
  RK4_TYPE = 0,
  EULER_TYPE,
};

enum class VectorFieldType
{
  VELOCITY_FIELD_TYPE = 0,
  ELECTRO_MAGNETIC_FIELD_TYPE,
};

enum FlowResultType
{
  UNKNOWN_TYPE = -1,
  PARTICLE_ADVECT_TYPE,
  STREAMLINE_TYPE,
};

}
}
}

#endif // viskores_filter_flow_FlowTypes_h
