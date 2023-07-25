// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <catalyst.h>
#include <catalyst.hpp>
#include <catalyst_stub.h>

#include "catalyst_impl_vtk.h"

//-----------------------------------------------------------------------------
enum catalyst_status catalyst_initialize_vtk(const conduit_node* params)
{
  return catalyst_stub_initialize(params);
}

//-----------------------------------------------------------------------------
enum catalyst_status catalyst_execute_vtk(const conduit_node* params)
{
  return catalyst_stub_execute(params);
}

//-----------------------------------------------------------------------------
enum catalyst_status catalyst_finalize_vtk(const conduit_node* params)
{
  return catalyst_stub_finalize(params);
}

//-----------------------------------------------------------------------------
enum catalyst_status catalyst_about_vtk(conduit_node* params)
{
  catalyst_status status = catalyst_stub_about(params);
  conduit_node_set_path_char8_str(params, "catalyst/implementation", "vtk");
  return status;
}

//-----------------------------------------------------------------------------
enum catalyst_status catalyst_results_vtk(conduit_node* params)
{
  return catalyst_stub_results(params);
}
