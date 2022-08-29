#pragma once

#if XR_MSFT_holographic_remoting
#define FOR_EACH_HAR_EXPERIMENTAL_EXTENSION_FUNCTION(_)                                            \
  _(xrRemotingSetContextPropertiesMSFT)                                                            \
  _(xrRemotingConnectMSFT)                                                                         \
  _(xrRemotingListenMSFT)                                                                          \
  _(xrRemotingDisconnectMSFT)                                                                      \
  _(xrRemotingGetConnectionStateMSFT)                                                              \
  _(xrRemotingSetSecureConnectionClientCallbacksMSFT)                                              \
  _(xrRemotingSetSecureConnectionServerCallbacksMSFT)                                              \
  _(xrCreateRemotingDataChannelMSFT)                                                               \
  _(xrDestroyRemotingDataChannelMSFT)                                                              \
  _(xrGetRemotingDataChannelStateMSFT)                                                             \
  _(xrSendRemotingDataMSFT)                                                                        \
  _(xrRetrieveRemotingDataMSFT)
#else
#define FOR_EACH_HAR_EXPERIMENTAL_EXTENSION_FUNCTION(_)
#endif

#define FOR_EACH_EXTENSION_FUNCTION(_) FOR_EACH_HAR_EXPERIMENTAL_EXTENSION_FUNCTION(_)

#define GET_INSTANCE_PROC_ADDRESS(name)                                                            \
  (void)xrGetInstanceProcAddr(                                                                     \
    instance, #name, reinterpret_cast<PFN_xrVoidFunction*>(const_cast<PFN_##name*>(&name)));
#define DEFINE_PROC_MEMBER(name) PFN_##name name{ nullptr };

namespace xr
{
struct ConnectionExtensionDispatchTable
{
  FOR_EACH_EXTENSION_FUNCTION(DEFINE_PROC_MEMBER);

  ConnectionExtensionDispatchTable() = default;
  void PopulateDispatchTable(XrInstance instance)
  {
    FOR_EACH_EXTENSION_FUNCTION(GET_INSTANCE_PROC_ADDRESS);
  }
};
} // namespace xr

#undef DEFINE_PROC_MEMBER
#undef GET_INSTANCE_PROC_ADDRESS
#undef FOR_EACH_EXTENSION_FUNCTION
