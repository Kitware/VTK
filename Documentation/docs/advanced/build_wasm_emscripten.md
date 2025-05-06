# Building using emscripten for WebAssembly

## Introduction

This page describes how to build and install VTK using [emscripten](https://emscripten.org) on any platform.

```{note}

Guide created using

- CMake 3.31.3
- Ninja 1.12.1
- Emscripten 4.0.3
- NodeJS 23.8.0
- VTK 9.4.1-1509-g9986a84c8a
- Chrome For Testing 133.0.6943.98
```

## Prerequisites

For this guide, you will need the following:

1. **CMake**: [CMake](http://www.cmake.org/) version 3.12 or higher and a
  working compiler. CMake is a tool that makes cross-platform building simple.
  On several systems it will probably be already installed. If it is not,
  please use the following instructions to install it.  There are several
  precompiled binaries available at the [CMake download page](https://cmake.org/download/).
  Add CMake to your PATH environment variable if you downloaded an archive and not an installer.

2. **Emscripten SDK**: [emsdk](https://github.com/emscripten-core/emsdk) and
   any dependencies needed by emsdk.  Emscripten is a complete compiler toolchain
   to WebAssembly, using LLVM, with a special focus on speed, size, and the Web
   platform.  Please download the SDK from
   [github.com/emscripten-core/emsdk.git](https://github.com/emscripten-core/emsdk). Then,

   - Install 4.0.3 toolchain with `./emsdk install 4.0.3`
   - Activate the toolchain `./emsdk activate 4.0.3`
   - Run `emsdk_env.bat` or `emsdk_env.ps1` (Windows) or `source ./emsdk_env.sh` (Linux and OS X) to set up the environment for the calling terminal.

   For more detailed instructions see  [emsdk/README.md](https://github.com/emscripten-core/emsdk#readme).

3. **Node.js**: Please download the Node.js 23.8.0 binaries from [vtk.org/files/support]
  and ensure that the command `node -v` outputs "v23.8.0" in your console. VTK's CI tests
  work with this version of Node.js, and other versions may not function correctly.

4. **VTK source-code**: If you have these then you can skip the rest of this section and proceed to [Build project](#build-project).
   As VTK-WebAssembly features are frequently updated, you are encouraged to use
   the [git repository](https://gitlab.kitware.com/vtk/vtk) version of VTK.
   This is a more up-to-date version used by developers containing unreleased features.
   You can either click the download button to get an archive containing the latest
   version of VTK-git or clone the repository
   (only recommended if you wish to contribute to VTK). In the last case, see [git/develop.md](../developers_guide/git/develop.md).

   You might also want to download the source code from [VTK Releases](https://vtk.org/download/)
   (look for a zip or tar.gz file with the version number you wish to use) but
   be aware that you will need to disable some more modules to have it build
   and work properly.

5. **Chrome For Testing**: This is optional. It is useful if you intend to run the test suite with `ctest`.
  This is a Chrome flavor that specifically targets web app testing and automation use cases.
  Please download the 133.0.6943.98 version from [vtk.org/files/support](https://vtk.org/files/support)

## Build project

### Build VTK

1. Configure the project with CMake. `emcmake` tells CMake to use the `emscripten` toolchain for cross compilation.

```bash
cd /path/to/VTK/build
$ emcmake cmake \
  -S .. \
  -B . \
  -G "Ninja" \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS:BOOL=OFF \
```

In order to run the unit tests, please enable testing with `-DVTK_BUILD_TESTING=WANT`. Additionally,
specify the browser that shall be used to run the wasm unit test with `-DVTK_TESTING_WASM_ENGINE:FILEPATH=/path/to/chrome`.

2. Compile.

```bash
$ cd /path/to/VTK/build
$ ninja
```

3. Install the project.

```bash
$ cd /path/to/VTK/build
$ ninja install
```

The binaries are now installed and you may use `-DVTK_DIR=/path/to/VTK/install/lib/cmake/vtk-9.4` to configure VTK wasm applications with CMake.

## Verify installation

If everything went well then it should now be possible to compile and run the one of the C++ examples.
Head over to [Examples/Emscripten/Cxx/Cone/README.md](https://gitlab.kitware.com/vtk/vtk/-/blob/master/Examples/Emscripten/Cxx/Cone/README.md)
and test the simple Cone example.

## Exceptions

By default, emscripten disables exception catching and enables exception throwing because of the overhead in size and speed. The [wasm-exceptions proposal](https://github.com/WebAssembly/exception-handling/blob/master/proposals/exception-handling/Exceptions.md) aims to resolve this issue.
In VTK, you can configure exceptions with the CMake setting `VTK_WEBASSEMBLY_EXCEPTIONS` (default `OFF`). Please note that the WASM CI build scripts turn on exceptions for developer convenience, so that a stack trace can be obtained when a C++ unit test crashes due to an uncaught exception or `abort`.

## Multithreading

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

**Tip:**

  If you plan to use a custom DOM `id` for the canvases, please also make sure to pass those as a comma separated list.
  Ex: `-sOFFSCREENCANVASES_TO_PTHREAD=#canvas1,#canvas2`

## 64-bit

VTK, by default compiles for the `wasm32-emscripten` architecture. When a 32-bit VTK wasm application
loads and renders very large datasets, it can report out-of-memory errors because the maximum
addressable memory is 4GB. You can overcome this problem by turning on the CMake setting `VTK_WEBASSEMBLY_64_BIT`.
This option compiles VTK for the `wasm64-emscripten` architecture which increases the maximum addressable memory upto 16GB.

In order to execute VTK wasm64 applications, additional flags are required for:
1. chrome/edge: no flag since v133.
2. firefox: no flag since v134.
3. nodejs: `--experimental-wasm-memory64`.


## Test project

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

In order to run the unit test interactively, you will need to determine the exact program and arguments that `ctest` uses to run your unit test. You can see the test command for a test named `FooUnitTest` by running `ctest -R FooUnitTest -N -VV`. Now, reconstruct the test command line and add a `-I` argument at the end. This will keep the browser open because the unit test is in interactive mode.

Here's an example:
```bash
$ ctest -R TestUserShader2D -N -V

836: Test command: /usr/bin/python3 "/path/to/vtk/Testing/WebAssembly/runner.py" "--engine=/path/to/vtk/.gitlab/chrome/chrome" "--exit" "/path/to/vtk/build/bin/vtkRenderingOpenGL2CxxTests.js" "TestUserShader2D" "-T" "/path/to/vtk/build/Testing/Temporary" "-V" "/path/to/vtk/build/ExternalData/Rendering/OpenGL2/Testing/Data/Baseline/TestUserShader2D.png"
836: Working Directory: /path/to/vtk/build/Rendering/OpenGL2/Testing/Cxx
836: Environment variables:
836:  VTK_TESTING=1
836:  VTK_TESTING_IMAGE_COMPARE_METHOD=TIGHT_VALID
Labels: VTK::RenderingOpenGL2 vtkRenderingOpenGL2
  Test #836: VTK::RenderingOpenGL2Cxx-TestUserShader2D

Total Tests: 1
```

From the result, we can reconstruct the test command and run the unit test interactively.

```bash
/usr/bin/python3 \
"/path/to/vtk/Testing/WebAssembly/runner.py" \
"--engine=/path/to/vtk/.gitlab/chrome/chrome" \
"--exit" \
"/path/to/vtk/build/bin/vtkRenderingOpenGL2CxxTests.js" \
"TestUserShader2D"
"-I"
```

Let's breakdown the command. It is important to understand how the command can be customized with additional arguments to the unit test.
The above command runs a python script `runner.py` with the engine argument pointing to `chrome` and the `--exit` flag to inform the runner that it
should terminate the browser and stop the HTTP server after the unit test `int main(int, char*[])` function returns an exit code. Finally the python
script is given a path to the test executable followed by arguments that will be passed to the unit test. You can explore all arguments of the `runner.py`
script with the `--help` argument.

```{note}
If your test crashes with errors like "Uncaught rejection from [object Promise]: 3829048", "Received exit code 1", "Out of memory", you can
force the test runner to keep the HTTP server active by not providing the `--exit` argument.
```
