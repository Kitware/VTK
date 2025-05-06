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

#include <viskores/cont/ErrorBadAllocation.h>
#include <viskores/cont/ErrorBadType.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/cont/ErrorUserAbort.h>
#include <viskores/cont/RuntimeDeviceTracker.h>

namespace viskores
{
namespace cont
{
namespace detail
{

VISKORES_CONT_EXPORT void HandleTryExecuteException(viskores::cont::DeviceAdapterId deviceId,
                                                    viskores::cont::RuntimeDeviceTracker& tracker,
                                                    const std::string& functorName)
{
  try
  {
    //re-throw the last exception
    throw;
  }
  catch (viskores::cont::ErrorBadAllocation& e)
  {
    VISKORES_LOG_TRYEXECUTE_DISABLE(
      "Bad allocation (" << e.GetMessage() << ")", functorName, deviceId);
    //currently we only consider OOM errors worth disabling a device for
    //than we fallback to another device
    tracker.ReportAllocationFailure(deviceId, e);
  }
  catch (viskores::cont::ErrorBadDevice& e)
  {
    VISKORES_LOG_TRYEXECUTE_DISABLE("Bad device (" << e.GetMessage() << ")", functorName, deviceId);
    tracker.ReportBadDeviceFailure(deviceId, e);
  }
  catch (viskores::cont::ErrorBadType& e)
  {
    //should bad type errors should stop the execution, instead of
    //deferring to another device adapter?
    VISKORES_LOG_TRYEXECUTE_FAIL("ErrorBadType (" << e.GetMessage() << ")", functorName, deviceId);
  }
  catch (viskores::cont::ErrorBadValue& e)
  {
    // Should bad values be deferred to another device? Seems unlikely they will succeed.
    // Re-throw instead.
    VISKORES_LOG_TRYEXECUTE_FAIL("ErrorBadValue (" << e.GetMessage() << ")", functorName, deviceId);
    throw;
  }
  catch (viskores::cont::ErrorUserAbort& e)
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                   e.GetMessage() << " Aborting: " << functorName << ", on device "
                                  << deviceId.GetName());
    throw;
  }
  catch (viskores::cont::Error& e)
  {
    VISKORES_LOG_TRYEXECUTE_FAIL(e.GetMessage(), functorName, deviceId);
    if (e.GetIsDeviceIndependent())
    {
      // re-throw the exception as it's a device-independent exception.
      throw;
    }
  }
  catch (std::exception& e)
  {
    VISKORES_LOG_TRYEXECUTE_FAIL(e.what(), functorName, deviceId);
  }
  catch (...)
  {
    VISKORES_LOG_TRYEXECUTE_FAIL("Unknown exception", functorName, deviceId);
    std::cerr << "unknown exception caught" << std::endl;
  }
}
}
}
}
