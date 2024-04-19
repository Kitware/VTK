// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "Backend.h"

#ifdef VTK_ENABLE_OSPRAY
#include "OSPRay/OSPRayBackend.h"
#endif
#ifdef VTK_ENABLE_VISRTX
#include "VisRTX/VisRTXBackend.h"
#endif

#include "RTWrapper.h"
#include "vtksys/SystemTools.hxx"
#include "vtkLogger.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

#ifdef VTK_ENABLE_VISRTX
VTK_ABI_NAMESPACE_BEGIN
RTW::VisRTXBackend* rtwVisRTXBackend = nullptr;
VTK_ABI_NAMESPACE_END
#endif

#ifdef VTK_ENABLE_OSPRAY
VTK_ABI_NAMESPACE_BEGIN
RTW::OSPRayBackend* rtwOSPRayBackend = nullptr;
VTK_ABI_NAMESPACE_END
#endif

VTK_ABI_NAMESPACE_BEGIN
void rtwInit()
{
#ifdef VTK_ENABLE_VISRTX
  const bool enableVisRTX = vtksys::SystemTools::GetEnv("VTK_DISABLE_VISRTX") == nullptr;

  if (!rtwVisRTXBackend && enableVisRTX)
  {
    vtkLogF(TRACE, "VisRTX/OptiX backend enabled, attempting to initialize backend");
    rtwVisRTXBackend = new RTW::VisRTXBackend();
    if (rtwVisRTXBackend->Init() != RTW_NO_ERROR)
    {
      vtkLogF(TRACE, "VisRTX/OptiX backend initialization failed, retrying initialization");
      rtwVisRTXBackend = new RTW::VisRTXBackend();
      if (rtwVisRTXBackend->Init() != RTW_NO_ERROR)
      {
        vtkLogF(TRACE, "VisRTX/OptiX backend initialization failed, terminating initialization");
        rtwVisRTXBackend->Shutdown();
        delete rtwVisRTXBackend;
        rtwVisRTXBackend = nullptr;
      }
    }
  }
  else if (!enableVisRTX)
  {
    vtkLogF(TRACE, "VisRTX/OptiX backend skipped due to env variable VTK_DISABLE_VISRTX");
  }
#else
  vtkLogF(TRACE, "VisRTX/OptiX backend disabled via CMake configuration for this build");
#endif
#ifdef VTK_ENABLE_OSPRAY
  const bool enableOSPRAY = vtksys::SystemTools::GetEnv("VTK_DISABLE_OSPRAY") == nullptr;

  if (!rtwOSPRayBackend && enableOSPRAY)
  {
    vtkLogF(TRACE, "OSPRay backend enabled, attempting to initialize backend");
    rtwOSPRayBackend = new RTW::OSPRayBackend();
    if (rtwOSPRayBackend->Init() != RTW_NO_ERROR)
    {
      vtkLogF(TRACE, "OSPRay backend initialization failed, retrying initialization");
      rtwOSPRayBackend = new RTW::OSPRayBackend();
      if (rtwOSPRayBackend->Init() != RTW_NO_ERROR)
      {
        vtkLogF(TRACE, "OSPRay backend initialization failed, terminating initialization");
        rtwOSPRayBackend->Shutdown();
        delete rtwOSPRayBackend;
        rtwOSPRayBackend = nullptr;
      }
    }
  }
  else if (!enableOSPRAY)
  {
    vtkLogF(TRACE, "OSPRay backend skipped due to env variable VTK_DISABLE_OSPRAY");
  }
#else
  vtkLogF(TRACE, "OSPRay backend disabled via CMake configuration for this build");
#endif
}

RTW::Backend *rtwSwitch(const char *name)
{
    if (!strcmp(name, "optix pathtracer"))
    {
#ifdef VTK_ENABLE_VISRTX
        return rtwVisRTXBackend;
#endif
    }
    else
    {
#ifdef VTK_ENABLE_OSPRAY
        return rtwOSPRayBackend;
#endif
    }
    return nullptr;
}

void rtwShutdown()
{
#ifdef VTK_ENABLE_VISRTX
    if (rtwVisRTXBackend)
    {
        rtwVisRTXBackend->Shutdown();
    }
    rtwVisRTXBackend=nullptr;
#endif
#ifdef VTK_ENABLE_OSPRAY
    if (rtwOSPRayBackend)
    {
        rtwOSPRayBackend->Shutdown();
    }
    rtwOSPRayBackend=nullptr;
#endif
}

std::set<RTWBackendType> rtwGetAvailableBackends()
{
    rtwInit();
    std::set<RTWBackendType> result;
#ifdef VTK_ENABLE_VISRTX
    if (rtwVisRTXBackend)
        result.insert(RTW_BACKEND_VISRTX);
#endif

#ifdef VTK_ENABLE_OSPRAY
    if (rtwOSPRayBackend)
        result.insert(RTW_BACKEND_OSPRAY);
#endif
    return result;
}
VTK_ABI_NAMESPACE_END
