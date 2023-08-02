# VTK::RenderingOpenXRRemoting
## VTK - OpenXR Holographic Remoting

Holographic remoting consists in a player application running on the XR device, and a VTK-based remote application running on a standard Windows machine.<br/>
The remote application receives camera information and rendering resources from the player. It renders the VTK scene before streaming back the resulting texture to the player application.<br/>
This way we avoid the need to build VTK for Universal Windows Platform (UWP), and we can also keep using VTK's OpenGL-based rendering pipeline.<br/>
Still, DirectX must be used to fill the texture to be streamed back to the Hololens. This is possible by creating a texture shared by both a DirectX and an OpenGL context, thanks to the NV_DX_interop extension available on almost every recent GPU.

At this time holographic remoting is supported only for the Microsoft HoloLens 2 virtual reality headset.

### Player application

- Download the Microsoft MixedReality HolographicRemoting [samples](https://github.com/microsoft/MixedReality-HolographicRemoting-Samples) and follow the instruction to build the player application.<br/>
  ⚠️ The version number in the branch name **must** match the version of the `Microsoft.Holographic.Remoting.OpenXr` package used below by the remote application.

- Follow the [instructions](https://docs.microsoft.com/en-us/windows/mixed-reality/develop/advanced-concepts/using-visual-studio?tabs=hl2#enabling-developer-mode) to deploy the player application to the Hololens 2.
  Alternatively, if you don't have access to a device, you can use the [Hololens emulator](https://docs.microsoft.com/en-us/windows/mixed-reality/develop/advanced-concepts/using-the-hololens-emulator#hololens-2-emulator-overview).

- When the player is deployed, you should see the following message: `Waiting for connection on XX.XX.XX.XX` where *XX.XX.XX.XX* describes the IP address the remote application should connect to.

### Remote application

- Enable the CMake option `VTK_MODULE_ENABLE_VTK_RenderingOpenXRRemoting` when building VTK.

- Set the `OpenXRRemoting_BIN_DIR` and `OpenXRRemoting_INCLUDE_DIR` to provide the path to the OpenXR Remoting headers and binary directory.<br/>
  The `Microsoft.Holographic.Remoting.OpenXr` Nuget package that provides this dependency is available [here](https://www.nuget.org/packages/Microsoft.Holographic.Remoting.OpenXr) on nuget.org.<br/>
  ⚠️ The version of the `Microsoft.Holographic.Remoting.OpenXr` package must match the branch name of the player application above.

- When successfully built, run the TestOpenXRRemotingInitialization test by sending the IP displayed in the player application as argument:<br/>
  `vtkRenderingOpenXRRemotingCxxTests.exe "TestOpenXRRemotingInitialization" -playerIP XX.XX.XX.XX`<br/>
  Alternatively, the `VTK_PLAYER_IP` environment variable can be used to specify the IP address to connect to.<br/>
  ⚠️ Make sure to provide the content of the OpenXR Remoting binary directory in the system PATH or next to the executable before running the program.

- To use this feature in your own application, use the OpenXR and OpenXRRemoting dedicated rendering stack: `vtkOpenXRRenderer`, `vtkOpenXRRemotingRenderWindow`, `vtkOpenXRRenderWindowInteractor` and `vtkOpenXRCamera`.<br/>
  The address of the player application to connect to must be set using `vtkOpenXRRemotingRenderWindow::SetRemotingIPAddress("XX.XX.XX.XX")` before starting the interactor.<br/>
  See the TestOpenXRRemotingInitialization test for a complete example.

### Troubleshooting:

> The OpenXR runtime fails to create and initialize the XrInstance.

To make sure that the player and remote application are compatible, the version of the Microsoft.Holographic.Remoting.OpenXr package must match the version number of the player application branch name.

> The remote application exits with the following output:
  WARN| Failed to initialize connection strategy.
  ERR| vtkOpenXRRemotingRenderWindow: Failed to initialize OpenXRManager

The remote application could not find the RemotingXR.json or the Microsoft.Holographic.AppRemoting.OpenXr.dll. Make sure to provide the content of the OpenXR Remoting binary directory in the system PATH or next to the executable.

> When running in the Hololens emulator, the connection fails with the following error displayed in the player: "Transport connection was closed due to the requested video format not being supported"

If you have both an Intel and NVidia GPU in your laptop, try disabling the NVidia GPU temporarily under "Display Adaptors" in the "Device Manager".

> When building the player application from VisualStudio, the Hololens emulator does not appear in the list of machine to deploy to.

Add a new x64 solution platform within VisualStudio and switch the current platform from ARM64 to x64. When building, if you now get the error `module machine type 'x64' conflicts with target machine type 'ARM64'`, then edit the project file to remove all occurrence of `/machine:ARM64`.

## Additional Notes

See [VTK `OpenXR` documentation](../OpenXR/README.md) for information on virtual reality rendering with OpenGL.
