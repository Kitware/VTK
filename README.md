# Viskores #

Viskores is a toolkit of scientific visualization algorithms for emerging
processor architectures. Viskores supports the fine-grained concurrency for
data analysis and visualization algorithms required to drive extreme scale
computing by providing abstract models for data and execution that can be
applied to a variety of algorithms across many different processor
architectures.

## Current Status

Viskores is a adopting the functionality of [Viskores] as a part of the [High
Performance Software Foundation] (HPSF). As this project becomes more
functional, more details will be added here.

## Technical Charter

See the [technical charter] and [governance] document for the structure of
the project.

## Learning Resources ##

  + A high-level overview is given in the IEEE Vis talk "[Viskores:
    Accelerating the Visualization Toolkit for Massively Threaded
    Architectures][Viskores Overview]."

  + The [Viskores Users Guide] provides extensive documentation. It is broken
    into multiple parts for learning and references at multiple different
    levels.
      + "Part 1: Getting Started" provides the introductory instruction for
        building Viskores and using its high-level features.
      + "Part 2: Using Viskores" covers the core fundamental components of
        Viskores including data model, worklets, and filters.
      + "Part 3: Developing with Viskores" covers how to develop new worklets
        and filters.
      + "Part 4: Advanced Development" covers topics such as new worklet
        types and custom device adapters.

  + A practical [Viskores Tutorial] based in what users want to accomplish with
    Viskores:
      + Building Viskores and using existing Viskores data structures and filters.
      + Algorithm development with Viskores.
      + Writing new Viskores filters.

  + Community discussion takes place on the [Viskores users discussion].

  + Doxygen-generated reference documentation is available for the tip of the
    release branch at [Viskores Doxygen latest]


## Contributing ##

There are many ways to contribute to [Viskores], with varying levels of
effort.

  + Ask a question on the [Viskores users discussion].

  + Submit new or add to discussions of a feature requests or bugs on the
    [Viskores Issue Tracker].

  + Submit a Pull Request to improve [Viskores]
      + See [CONTRIBUTING.md] for detailed instructions on how to create a
        Pull Request.
      + See the [Viskores Coding Conventions] that must be followed for
        contributed code.

  + Submit an Issue or Pull Request for the [Viskores Users Guide]

See the [CONTRIBUTING.md] for more ways to contribute and instructions.

## Dependencies ##

Viskores Requires:

  + C++14 Compiler. Viskores has been confirmed to work with the following
      + GCC 5.4+
      + Clang 5.0+
      + XCode 5.0+
      + MSVC 2015+
      + Intel 17.0.4+
  + [CMake](http://www.cmake.org/download/)
      + CMake 3.15+
      + CMake 3.24+ (for ROCM+THRUST support)

Optional dependencies are:

  + Kokkos Device Adapter
      + [Kokkos](https://kokkos.github.io/) 3.7+
      + CXX env variable or CMAKE_CXX_COMPILER should be set to
        hipcc when using Kokkos device adapter with HIP (ROCM>=6).
  + CUDA Device Adapter
      + [Cuda Toolkit 9.2, >= 10.2](https://developer.nvidia.com/cuda-toolkit)
      + Note CUDA >= 10.2 is required on Windows
  + TBB Device Adapter
      + [TBB](https://www.threadingbuildingblocks.org/)
  + OpenMP Device Adapter
      + Requires a compiler that supports OpenMP >= 4.0.
  + OpenGL Rendering
      + The rendering module contains multiple rendering implementations
        including standalone rendering code. The rendering module also
        includes (optionally built) OpenGL rendering classes.
      + The OpenGL rendering classes require that you have a extension
        binding library and one rendering library. A windowing library is
        not needed except for some optional tests.
  + Extension Binding
      + [GLEW](http://glew.sourceforge.net/)
  + On Screen Rendering
      + OpenGL Driver
      + Mesa Driver
  + On Screen Rendering Tests
      + [GLFW](http://www.glfw.org/)
      + [GLUT](http://freeglut.sourceforge.net/)
  + Headless Rendering
      + [OS Mesa](https://www.mesa3d.org/osmesa.html)
      + EGL Driver

Viskores has been tested on the following configurations:c
  + On Linux
      + GCC 5.4.0, 5.4, 6.5, 7.4, 8.2, 9.2; Clang 5, 8; Intel 17.0.4; 19.0.0
      + CMake 3.12, 3.13, 3.16, 3.17
      + CUDA 9.2, 10.2, 11.0, 11.1
      + TBB 4.4 U2, 2017 U7
  + On Windows
      + Visual Studio 2015, 2017
      + CMake 3.12, 3.17
      + CUDA 10.2
      + TBB 2017 U3, 2018 U2
  + On MacOS
      + AppleClang 9.1
      + CMake 3.12
      + TBB 2018


## Building ##

Viskores supports all major platforms (Windows, Linux, OSX), and uses CMake
to generate all the build rules for the project. The Viskores source code is
available from the [Viskores download page] or by directly cloning the [Viskores
git repository].

The basic procedure for building Viskores is to unpack the source, create a
build directory, run CMake in that build directory (pointing to the source)
and then build. Here are some example *nix commands for the process
(individual commands may vary).

```sh
$ tar xvzf ~/Downloads/viskores-v2.0.0.tar.gz
$ mkdir viskores-build
$ cd viskores-build
$ cmake-gui ../viskores-v2.0.0
$ cmake --build -j .              # Runs make (or other build program)
```

A more detailed description of building Viskores is available in the [Viskores
Users Guide].


## Example ##

The Viskores source distribution includes a number of examples. The goal of the
Viskores examples is to illustrate specific Viskores concepts in a consistent and
simple format. However, these examples cover only a small portion of the
capabilities of Viskores.

Below is a simple example of using Viskores to create a simple data set and use Viskores's rendering
engine to render an image and write that image to a file. It then computes an isosurface on the
input data set and renders this output data set in a separate image file:

```cpp
#include <viskores/cont/Initialize.h>
#include <viskores/source/Tangle.h>

#include <viskores/rendering/Actor.h>
#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/MapperRayTracer.h>
#include <viskores/rendering/MapperVolume.h>
#include <viskores/rendering/MapperWireframer.h>
#include <viskores/rendering/Scene.h>
#include <viskores/rendering/View3D.h>

#include <viskores/filter/contour/Contour.h>

using viskores::rendering::CanvasRayTracer;
using viskores::rendering::MapperRayTracer;
using viskores::rendering::MapperVolume;
using viskores::rendering::MapperWireframer;

int main(int argc, char* argv[])
{
  viskores::cont::Initialize(argc, argv, viskores::cont::InitializeOptions::Strict);

  auto tangle = viskores::source::Tangle(viskores::Id3{ 50, 50, 50 });
  viskores::cont::DataSet tangleData = tangle.Execute();
  std::string fieldName = "tangle";

  // Set up a camera for rendering the input data
  viskores::rendering::Camera camera;
  camera.SetLookAt(viskores::Vec3f_32(0.5, 0.5, 0.5));
  camera.SetViewUp(viskores::make_Vec(0.f, 1.f, 0.f));
  camera.SetClippingRange(1.f, 10.f);
  camera.SetFieldOfView(60.f);
  camera.SetPosition(viskores::Vec3f_32(1.5, 1.5, 1.5));
  viskores::cont::ColorTable colorTable("inferno");

  // Background color:
  viskores::rendering::Color bg(0.2f, 0.2f, 0.2f, 1.0f);
  viskores::rendering::Actor actor(tangleData.GetCellSet(),
                               tangleData.GetCoordinateSystem(),
                               tangleData.GetField(fieldName),
                               colorTable);
  viskores::rendering::Scene scene;
  scene.AddActor(actor);
  // 2048x2048 pixels in the canvas:
  CanvasRayTracer canvas(2048, 2048);
  // Create a view and use it to render the input data using OS Mesa

  viskores::rendering::View3D view(scene, MapperVolume(), canvas, camera, bg);
  view.Paint();
  view.SaveAs("volume.png");

  // Compute an isosurface:
  viskores::filter::contour::Contour filter;
  // [min, max] of the tangle field is [-0.887, 24.46]:
  filter.SetIsoValue(3.0);
  filter.SetActiveField(fieldName);
  viskores::cont::DataSet isoData = filter.Execute(tangleData);
  // Render a separate image with the output isosurface
  viskores::rendering::Actor isoActor(
    isoData.GetCellSet(), isoData.GetCoordinateSystem(), isoData.GetField(fieldName), colorTable);
  // By default, the actor will automatically scale the scalar range of the color table to match
  // that of the data. However, we are coloring by the scalar that we just extracted a contour
  // from, so we want the scalar range to match that of the previous image.
  isoActor.SetScalarRange(actor.GetScalarRange());
  viskores::rendering::Scene isoScene;
  isoScene.AddActor(isoActor);

  // Wireframe surface:
  viskores::rendering::View3D isoView(isoScene, MapperWireframer(), canvas, camera, bg);
  isoView.Paint();
  isoView.SaveAs("isosurface_wireframer.png");

  // Smooth surface:
  viskores::rendering::View3D solidView(isoScene, MapperRayTracer(), canvas, camera, bg);
  solidView.Paint();
  solidView.SaveAs("isosurface_raytracer.png");

  return 0;
}
```

A minimal CMakeLists.txt such as the following one can be used to build this
example.

```CMake
cmake_minimum_required(VERSION 3.12...3.15 FATAL_ERROR)
project(ViskoresDemo CXX)

#Find the Viskores package
find_package(Viskores REQUIRED QUIET)

if(TARGET viskores::rendering)
  add_executable(Demo Demo.cxx)
  target_link_libraries(Demo PRIVATE viskores::filter viskores::rendering viskores::source)
endif()
```

## License ##

Viskores is distributed under the OSI-approved BSD 3-clause License.
See [LICENSE.txt](LICENSE.txt) for details.


[Viskores]:                             https://github.com/Viskores/viskores
[Viskores Coding Conventions]:          docs/CodingConventions.md
[Viskores Doxygen latest]:              https://viskores.github.io/viskores-doxygen/
[Viskores download page]:               https://github.com/Viskores/viskores/releases
[Viskores git repository]:              https://github.com/Viskores/viskores
[Viskores Issue Tracker]:               https://github.com/Viskores/viskores/issues
[Viskores Overview]:                    http://m.vtk.org/images/2/29/ViskoresVis2016.pptx
[Viskores Users Guide]:                 https://viskores.readthedocs.io/en/latest/
[Viskores users discussion]:            https://github.com/Viskores/viskores/discussions
[Viskores Tutorial]:                    tutorial/README.md
[CONTRIBUTING.md]:                      CONTRIBUTING.md
[High Performance Software Foundation]: https://hpsf.io/
[technical charter]:                    docs/technical-charter.pdf
[governance]:                           GOVERNANCE.md
