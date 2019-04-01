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

RTW::Backend *rtwInit(int *argc, const char **argv)
{
    if (!strcmp(argv[0], "optix pathtracer"))
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
        return rtwVisRTXBackend;
#endif
    }
    else
    {
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
        return rtwOSPRayBackend;
#endif
    }

    return nullptr;
}

std::set<RTWBackendType> rtwGetAvailableBackends()
{
    std::set<RTWBackendType> result;
#ifdef VTK_ENABLE_VISRTX
    {
        int ac = 1;
        const char* av[] = { "optix pathtracer\0" };
        if (rtwInit(&ac, av) != nullptr)
            result.insert(RTW_BACKEND_VISRTX);
    }
#endif

#ifdef VTK_ENABLE_OSPRAY
    {
        int ac = 1;
        const char* av[] = { "scivis\0" };
        if (rtwInit(&ac, av) != nullptr)
            result.insert(RTW_BACKEND_OSPRAY);
    }
#endif
    return result;
}
