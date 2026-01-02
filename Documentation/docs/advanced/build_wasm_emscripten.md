# Building using emscripten for WebAssembly

## Introduction

This page describes how to build and install VTK using [emscripten](https://emscripten.org) on any platform.

```{note}

Guide created using

- CMake 3.31.3
- Ninja 1.12.1
- Emscripten 4.0.20
- NodeJS 24.0.1
- Chrome For Testing 133.0.6943.98
```

## Prerequisites

For this guide, you will need the following:

1. **CMake**: [CMake](http://www.cmake.org/) version 3.29 or higher and Ninja.
  CMake is a tool that makes cross-platform building simple.
  On several systems it will probably be already installed. If it is not,
  please use the following instructions to install it.  There are several
  precompiled binaries available at the [CMake download page](https://cmake.org/download/).
  Add CMake to your PATH environment variable if you downloaded an archive and not an installer.

2. **Emscripten SDK**: [emsdk](https://github.com/emscripten-core/emsdk) and
  any dependencies needed by emsdk.  Emscripten is a complete compiler toolchain
  to WebAssembly, using LLVM, with a special focus on speed, size, and the Web
  platform.

3. **Node.js**: NodeJS will be used as a cross compiling emulator by CMake to run the generated JavaScript
  files

4. **VTK source-code**: You can either click the download button to get an archive containing the latest
  version of VTK-git or clone the repository
  (only recommended if you wish to contribute to VTK). In the last case, see [git/develop.md](../developers_guide/git/develop.md).

5. **Chrome For Testing**: This is optional. It is useful if you intend to run the test suite with `ctest`.
  This is a Chrome flavor that specifically targets web app testing and automation use cases.
  Please download the 133.0.6943.98 version from [vtk.org/files/support](https://vtk.org/files/support)

### Install prerequisites

::::{tab-set}

:::{tab-item} macOS/Linux
```sh
# Download and install fnm:
curl -o- https://fnm.vercel.app/install | bash
# Download and install Node.js
fnm install 24.0.1
fnm use 24.0.1

# Download and install EMSDK
git clone https://github.com/emscripten-core/emsdk.git
./emsdk/emsdk install 4.0.20
export PATH=$PWD/emsdk/upstream/bin:$PWD/emsdk/upstream/emscripten:$PATH
```
:::

:::{tab-item} Windows
```pwsh
# In Powershell
# Download and install fnm:
winget install Schniz.fnm
# Download and install Node.js
fnm install 24.0.1
fnm env --use-on-cd --shell power-shell | Out-String | Invoke-Expression
fnm use 24.0.1

git clone https://github.com/emscripten-core/emsdk.git
.\emsdk\emsdk install 4.0.20
$env:PATH="$PWD\emsdk\upstream\bin;$PWD\emsdk\upstream\emscripten;$env:PATH"
```
:::

::::

## Build project

The following environment variables are assumed:

- `VTK_SOURCE_DIR`: Specifies the path to the root directory of the VTK source code. This variable should point to the location where the VTK source files are located and is used as the input for the build process.
- `VTK_BUILD_DIR`: Defines the directory where the VTK build artifacts will be generated. This is typically a separate directory from the source to allow for out-of-source builds and to keep build files organized.
- `VTK_INSTALL_DIR`: Indicates the target directory where the built VTK libraries and headers will be installed after the build process completes. This allows for easy access and reuse of the compiled VTK components.

### Configure and build

::::{tab-set}

:::{tab-item} wasm32:macOS/Linux
```sh
emcmake cmake \
  -S ${VTK_SOURCE_DIR} \
  -B ${VTK_BUILD_DIR} \
  -G "Ninja" \
  -DCMAKE_BUILD_TYPE=Release \
  "-DBUILD_SHARED_LIBS:BOOL=OFF" \
  "-DVTK_ENABLE_WEBGPU:BOOL=ON"
cmake --build ${VTK_BUILD_DIR}
cmake --install ${VTK_BUILD_DIR} --prefix ${VTK_INSTALL_DIR}
```
:::

:::{tab-item} wasm64:macOS/Linux
```sh
emcmake cmake \
  -S ${VTK_SOURCE_DIR} \
  -B ${VTK_BUILD_DIR} \
  -G "Ninja" \
  -DCMAKE_BUILD_TYPE=Release \
  "-DBUILD_SHARED_LIBS:BOOL=OFF" \
  "-DVTK_ENABLE_WEBGPU:BOOL=ON" \
  "-DVTK_WEBASSEMBLY_64_BIT:BOOL=ON"
cmake --build ${VTK_BUILD_DIR}
cmake --install ${VTK_BUILD_DIR} --prefix ${VTK_INSTALL_DIR}
```
:::

:::{tab-item} wasm32:Windows
```pwsh
# In Powershell
emcmake cmake `
  -S $env:VTK_SOURCE_DIR `
  -B $env:VTK_BUILD_DIR `
  -G "Ninja" `
  -DCMAKE_BUILD_TYPE=Release `
  "-DBUILD_SHARED_LIBS:BOOL=OFF" `
  "-DVTK_ENABLE_WEBGPU:BOOL=ON"
cmake --build $env:VTK_BUILD_DIR
cmake --install $env:VTK_BUILD_DIR --prefix $env:VTK_INSTALL_DIR
```
:::

:::{tab-item} wasm64:Windows
```pwsh
# In Powershell
emcmake cmake `
  -S $env:VTK_SOURCE_DIR `
  -B $env:VTK_BUILD_DIR `
  -G "Ninja" `
  -DCMAKE_BUILD_TYPE=Release `
  "-DBUILD_SHARED_LIBS:BOOL=OFF" `
  "-DVTK_ENABLE_WEBGPU:BOOL=ON" `
  "-DVTK_WEBASSEMBLY_64_BIT:BOOL=ON"
cmake --build $env:VTK_BUILD_DIR
cmake --install $env:VTK_BUILD_DIR --prefix $env:VTK_INSTALL_DIR
```
:::

::::

```{warning}
When copy pasting the commands on windows, please paste them into a Powershell window and not a Command prompt!
```

In order to run the unit tests, please enable testing with `-DVTK_BUILD_TESTING=WANT`. Additionally,
specify the browser that shall be used to run the wasm unit test with `-DVTK_TESTING_WASM_ENGINE:FILEPATH=/path/to/chrome`.

The binaries are now installed and you may use `-DVTK_DIR=/path/to/VTK/installRelease/lib/cmake/vtk-X.Y` to configure VTK wasm applications with CMake.

### Verify installation

If everything went well then it should now be possible to compile and run the one of the C++ examples.
Head over to [Examples/Emscripten/Cxx/Cone/README.md](https://gitlab.kitware.com/vtk/vtk/-/blob/master/Examples/Emscripten/Cxx/Cone/README.md)
and test the simple Cone example.

## Additional WASM Features

### Exception support

By default, emscripten disables exception catching and enables exception throwing because of the overhead in size and speed. The [wasm-exceptions proposal](https://github.com/WebAssembly/exception-handling/blob/master/proposals/exception-handling/Exceptions.md) aims to resolve this issue.
VTK enables exceptions by default with `-fwasm-exceptions` because some internal modules call `longjmp` and trigger C++ exceptions. Additionally, it is convenient since a stack trace can be obtained when a C++ unit test crashes.

### Multithreading support

Multithreading can be enabled in VTK wasm by turning on the CMake setting `VTK_WEBASSEMBLY_THREADS`.
This option simply adds the compile and link flags necessary for emscripten to use WebWorker for a `pthread` and by extension,
`std::thread`. Please refer to [Emscripten/Pthreads](https://emscripten.org/docs/porting/pthreads.html) for details.

You generally want to run your C++ `int main(int, char**)` function in a WebWorker. Doing so keeps the
browser responsive and gives your users a chance to at the very least refresh/close the tab when a long
running VTK algorithm is processing data. You can set this up with the `-sPROXY_TO_PTHREAD=1` linker flag.

If rendering is also part of your main program, please pass `-sPROXY_TO_PTHREAD=1`, `-sOFFSCREENCANVAS_SUPPORT=1`.
These flags will proxy rendering calls to the main browser thread. Since DOM events like mouse, keyboard inputs are
received on the main browser thread, emscripten takes care of queuing the execution of the event callback in the WebWorker
running the VTK application.
You can learn more at [settings_reference/proxy-to-pthread](https://emscripten.org/docs/tools_reference/settings_reference.html#proxy-to-pthread)
and [settings_reference/offscreencanvas-support](https://emscripten.org/docs/tools_reference/settings_reference.html#offscreencanvas-support)

```{tip}
  If you plan to use a custom DOM `id` for the canvases, please also make sure to pass those as a comma separated list.
  Ex: `-sOFFSCREENCANVASES_TO_PTHREAD=#canvas1,#canvas2`
```

### 64-bit support

VTK, by default compiles for the `wasm32-emscripten` architecture. When a 32-bit VTK wasm application
loads and renders very large datasets, it can report out-of-memory errors because the maximum
addressable memory is 4GB. You can overcome this problem by turning on the CMake setting `VTK_WEBASSEMBLY_64_BIT`.
This option compiles VTK for the `wasm64-emscripten` architecture which increases the maximum addressable memory upto 16GB.

In order to execute VTK wasm64 applications, additional flags are required for:
1. chrome/edge: no flag since v133.
2. firefox: no flag since v134.
3. nodejs: `--experimental-wasm-memory64` for node < v24.0.0.


## Testing

In order to run the unit tests, execute the `ctest` command in your build directory.

```{warning}
The `ctest` command will appear to hang if you did not specify the path to a web browser executable in the configure step with VTK_TESTING_WASM_ENGINE.
```

Sometimes, your MR may break a unit test in the wasm32/wasm64 test jobs. In that case, it is extremely helpful to view the full output of the unit tests and even run the test in interactive mode.

### Viewing unit test output

You may run the tests with the verbose flag to view all messages sent to `cout` and `cerr`.

```bash
$ ctest -R FooUnitTest -VV
```

### Running unit tests interactively

In order to run the unit test interactively, you will need to determine the exact program and arguments that `ctest` uses to run your unit test. You can see the test command for a test named `FooUnitTest` by running `ctest -R FooUnitTest -N -V`. Now, reconstruct the test command line and add a `-I` argument at the end. This will keep the browser open because the unit test is in interactive mode. If your test uses data or a valid baseline, you will require the HTTP server. When run using `ctest`, the HTTP server is automatically started before your test runs and shut down after the test completes. On the other hand, if you run the test manually by copying the test command into the console, you will need to run `ctest -R HTTPServerStart` by hand before executing the test. Do not forget to run `ctest -R HTTPServerStop` when you are finished. The wasm test suite is explained in detail at [WebAssemblyTestSuiteArchitecture](../design_documents/WebAssemblyTestSuiteArchitecture.md).

Here's an example:
```pwsh
$ ctest -R TestHardwareSelector -N -V

1: Test command: C:\Users\jaswant.panchumarti\AppData\Local\fnm_multishells\32664_1751993516170\node.exe "C:/dev/vtk/CMake/wasm/server.js" "--directory" "C:/dev/vtk/buildRelease/Testing/Temporary" "--port" "0" "--operation" "start"
1: Working Directory: C:/dev/vtk/buildRelease
  Test   #1: HTTPServerStart

2: Test command: C:\Users\jaswant.panchumarti\AppData\Local\fnm_multishells\32664_1751993516170\node.exe "C:/dev/vtk/CMake/wasm/server.js" "--directory" "C:/dev/vtk/buildRelease/Testing/Temporary" "--port" "0" "--operation" "stop"
2: Working Directory: C:/dev/vtk/buildRelease
  Test   #2: HTTPServerStop

188: Test command: "C:\Program Files\CMake\bin\cmake.exe" "-DEXIT_AFTER_TEST=ON" "-DTESTING_WASM_ENGINE=C:/dev/vtk/.gitlab/chrome/chrome.exe" "-DTESTING_WASM_HTML_TEMPLATE=C:/dev/vtk/CMake/wasm/vtkWasmTest.html.in" "-DTEST_NAME=VTK::RenderingWebGPUCxx-TestHardwareSelector" "-DTEST_OUTPUT_DIR=C:/dev/vtk/buildRelease/Testing/Temporary" "-P" "C:/dev/vtk/CMake/wasm/vtkWasmTestRunner.cmake" "--" "C:/dev/vtk/buildRelease/bin/vtkRenderingWebGPUCxxTests.js" "TestHardwareSelector" "-T" "C:/dev/vtk/buildRelease/Testing/Temporary"
188: Working Directory: C:/dev/vtk/buildRelease/Rendering/WebGPU/Testing/Cxx
188: Environment variables:
188:  VTK_TESTING=1
188:  VTK_TESTING_IMAGE_COMPARE_METHOD=TIGHT_VALID
Labels: VTK::RenderingWebGPU vtkRenderingWebGPU
  Test #188: VTK::RenderingWebGPUCxx-TestHardwareSelector
Total Tests: 3
```

From the result, we can reconstruct the test command and run the unit test interactively.

```pwsh
."C:\Program Files\CMake\bin\cmake.exe" `
  "-DEXIT_AFTER_TEST=OFF" `
  "-DTESTING_WASM_ENGINE=C:/dev/vtk/.gitlab/chrome/chrome.exe" `
  "-DTESTING_WASM_HTML_TEMPLATE=C:/dev/vtk/CMake/wasm/vtkWasmTest.html.in" `
  "-DTEST_NAME=VTK::RenderingWebGPUCxx-TestHardwareSelector" `
  "-DTEST_OUTPUT_DIR=C:/dev/vtk/buildRelease/Testing/Temporary" `
  "-P" `
  "C:/dev/vtk/CMake/wasm/vtkWasmTestRunner.cmake" `
  "--" `
  "C:/dev/vtk/buildRelease/bin/vtkRenderingWebGPUCxxTests.js" `
  "TestHardwareSelector" `
  "-I"
```

Let's breakdown the command. It is important to understand how the command can be customized with additional arguments to the unit test.
The above command runs a CMake script `vtkWasmTestRunner.cmake` with the engine argument pointing to `chrome`. Finally the cmake script
script is given a path to the test executable followed by arguments that will be passed to the unit test.

```{note}
If your test crashes with errors like "Uncaught rejection from [object Promise]: 3829048", "Received exit code 1", "Out of memory", you can
force the test runner to keep the browser open by passing "-DEXIT_AFTER_TEST=OFF". This is also required for asynchronous tests that use JSPI.
```
