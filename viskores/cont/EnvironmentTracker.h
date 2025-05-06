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
#ifndef viskores_cont_EnvironmentTracker_h
#define viskores_cont_EnvironmentTracker_h

#include <viskores/Types.h>
#include <viskores/cont/viskores_cont_export.h>
#include <viskores/internal/ExportMacros.h>

#include <viskores/thirdparty/diy/diy.h>

namespace viskores
{
namespace cont
{

/// \brief Maintain MPI controller, if any, for distributed operation.
///
/// `EnvironmentTracker` is a class that provides static API to track the global
/// MPI controller to use for operating in a distributed environment.
class VISKORES_CONT_EXPORT EnvironmentTracker
{
public:
  VISKORES_CONT
  static void SetCommunicator(const viskoresdiy::mpi::communicator& comm);

  VISKORES_CONT
  static const viskoresdiy::mpi::communicator& GetCommunicator();
};
}
}


#endif // viskores_cont_EnvironmentTracker_h
