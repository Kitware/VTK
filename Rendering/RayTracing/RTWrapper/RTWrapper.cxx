#include "Backend.h"

#ifdef VTK_ENABLE_OSPRAY
#include "OSPRay/OSPRayBackend.h"
#endif
#ifdef VTK_ENABLE_VISRTX
#include "VisRTX/VisRTXBackend.h"
#endif

#include "RTWrapper.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <map>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <set>

#ifdef VTK_ENABLE_VISRTX
RTW::VisRTXBackend* rtwVisRTXBackend = nullptr;
#endif

#ifdef VTK_ENABLE_OSPRAY
RTW::OSPRayBackend* rtwOSPRayBackend = nullptr;
#endif

void rtwInit(int *argc, const char **argv)
{
#ifdef VTK_ENABLE_VISRTX
  if (!rtwVisRTXBackend)
  {
    rtwVisRTXBackend = new RTW::VisRTXBackend();
    if (rtwVisRTXBackend->Init(argc, argv) != RTW_NO_ERROR)
    {
      std::cerr << "WARNING: Failed to initialize RTW VisRTX backend.\n";
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
    if (rtwOSPRayBackend->Init(argc, argv) != RTW_NO_ERROR)
    {
      std::cerr << "WARNING: Failed to initialize RTW OSPRay backend.\n";
      rtwOSPRayBackend->Shutdown();
      delete rtwOSPRayBackend;
      rtwOSPRayBackend = nullptr;
    }
  }
#endif
}

RTW::Backend *rtwSwitch(int *argc, const char **argv)
{
    if (!strcmp(argv[0], "optix pathtracer"))
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
#endif
#ifdef VTK_ENABLE_OSPRAY
  if (rtwOSPRayBackend)
  {
    rtwOSPRayBackend->Shutdown();
  }
#endif
}

std::set<RTWBackendType> rtwGetAvailableBackends()
{
    rtwInit(nullptr, nullptr);
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
