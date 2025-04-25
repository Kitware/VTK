# Cross-compiling for Mobile devices

```{tip}
For complete build instructions see [here](../build_instructions/build.md).
```

VTK supports mobile devices in its build. These are triggered by a top-level
flag which then exposes some settings for a cross-compiled VTK that is
controlled from the top-level build.

## IOS

iOS builds may be enabled by setting the `VTK_IOS_BUILD` option. The following
settings than affect the iOS build:

  * `IOS_SIMULATOR_ARCHITECTURES`
  * `IOS_DEVICE_ARCHITECTURES`
  * `IOS_DEPLOYMENT_TARGET`
  * `IOS_EMBED_BITCODE`

## Android

Android builds may be enabled by setting the `VTK_ANDROID_BUILD` option.
Android build use a set of defined option in `vtkAndroid.cmake` but user options
can also be added by setting `VTK_ANDROID_USER_OPTIONS`.

The following settings affect the Android build:

  * `ANDROID_NDK`
  * `ANDROID_NATIVE_API_LEVEL`
  * `ANDROID_ARCH_ABI`

A typical CMake configuration line would be:
`cmake -S /vtk-src -B /vtk-build  -DANDROID_ARCH_ABI=arm64-v8a -DANDROID_NATIVE_API_LEVEL=34 -DANDROID_NDK=/path/to/ndk -DCMAKE_BUILD_TYPE=Release -DVTK_ANDROID_BUILD=ON -DVTK_ANDROID_USER_OPTIONS="-DVTK_MODULE_ENABLE_VTK_IOInfovis=NO;-DVTK_MODULE_ENABLE_VTK_TestingRendering=NO"`
