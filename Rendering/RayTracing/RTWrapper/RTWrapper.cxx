#include "Backend.h"

#ifdef VTK_ENABLE_OSPRAY
#include "OSPRay/OSPRayBackend.h"
#endif
#ifdef VTK_ENABLE_VISRTX
#include "VisRTX/VisRTXBackend.h"
#endif

#include "RTWrapper.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

#ifdef VTK_ENABLE_VISRTX
RTW::VisRTXBackend* rtwVisRTXBackend = nullptr;
#endif

#ifdef VTK_ENABLE_OSPRAY
RTW::OSPRayBackend* rtwOSPRayBackend = nullptr;
#endif

void rtwInit()
{
#ifdef VTK_ENABLE_VISRTX
  if (!rtwVisRTXBackend)
  {
    rtwVisRTXBackend = new RTW::VisRTXBackend();
    if (rtwVisRTXBackend->Init() != RTW_NO_ERROR)
    {
      // std::cerr << "WARNING: Failed to initialize RTW VisRTX backend.\n";
      rtwVisRTXBackend->Shutdown();
      delete rtwVisRTXBackend;
      rtwVisRTXBackend = nullptr;
    }
  }
#endif
#ifdef VTK_ENABLE_OSPRAY
  if (!rtwOSPRayBackend)
  {
    rtwOSPRayBackend = new RTW::OSPRayBackend();
    if (rtwOSPRayBackend->Init() != RTW_NO_ERROR)
    {
      // std::cerr << "WARNING: Failed to initialize RTW OSPRay backend.\n";
      rtwOSPRayBackend->Shutdown();
      delete rtwOSPRayBackend;
      rtwOSPRayBackend = nullptr;
    }
  }
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
