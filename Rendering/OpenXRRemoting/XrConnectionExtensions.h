/*=========================================================================

  Program:   Visualization Toolkit
  Module:    XrConnectionExtensions.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//*********************************************************
//    Copyright (c) Microsoft. All rights reserved.
//
//    Apache 2.0 License
//
//    You may obtain a copy of the License at
//    http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
//    implied. See the License for the specific language governing
//    permissions and limitations under the License.
//
//*********************************************************
/**
 * @file   XrConnectionExtensions.h
 *
 * @brief  Load OpenXR extensions required for remote connection.
 *
 * Provides the ConnectionExtensionDispatchTable struct to load remoting
 * extensions at runtime for the current XrInstance.
 * The OpenXR header corresponding to the expected connection strategy must be
 * included prior to including this header to provide the required definitions.
 * For instance, including <openxr_msft_holographic_remoting.h> defines
 * XR_MSFT_holographic_remoting, which enables the OpenXR Remoting extension in
 * this file.
 *
 * File adapted from:
 * https://github.com/microsoft/MixedReality-HolographicRemoting-Samples/blob/f6b55479646bda3bffea58bb3e9c9d9c5e0ab177/remote_openxr/desktop/XrUtility/XrExtensions.h
 *
 * @sa
 * vtkOpenXRManagerRemoteConnection XrExtensions.h XrGraphicsExtensions.h
 */

#ifndef XrConnectionExtensions_h
#define XrConnectionExtensions_h

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
VTK_ABI_NAMESPACE_BEGIN
struct ConnectionExtensionDispatchTable
{
  FOR_EACH_EXTENSION_FUNCTION(DEFINE_PROC_MEMBER);

  ConnectionExtensionDispatchTable() = default;
  void PopulateDispatchTable(XrInstance instance)
  {
    FOR_EACH_EXTENSION_FUNCTION(GET_INSTANCE_PROC_ADDRESS);
  }
};
VTK_ABI_NAMESPACE_END
} // namespace xr

#undef DEFINE_PROC_MEMBER
#undef GET_INSTANCE_PROC_ADDRESS
#undef FOR_EACH_EXTENSION_FUNCTION

#endif
