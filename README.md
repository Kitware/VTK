![nanoflann](https://raw.githubusercontent.com/jlblancoc/nanoflann/master/doc/logo.png)

# nanoflann
[![CI Linux](https://github.com/jlblancoc/nanoflann/actions/workflows/ci-linux.yml/badge.svg)](https://github.com/jlblancoc/nanoflann/actions/workflows/ci-linux.yml)
[![CI Check clang-format](https://github.com/jlblancoc/nanoflann/actions/workflows/check-clang-format.yml/badge.svg)](https://github.com/jlblancoc/nanoflann/actions/workflows/check-clang-format.yml)
[![CircleCI](https://circleci.com/gh/jlblancoc/nanoflann/tree/master.svg?style=svg)](https://circleci.com/gh/jlblancoc/nanoflann/tree/master)
[![Windows build status](https://ci.appveyor.com/api/projects/status/h8k1apfogxyqhskd/branch/master?svg=true)](https://ci.appveyor.com/project/jlblancoc/nanoflann/branch/master)
[![codecov](https://codecov.io/gh/jlblancoc/nanoflann/graph/badge.svg?token=yOKq5lpizE)](https://codecov.io/gh/jlblancoc/nanoflann)


| Distro | Build dev | Build releases | Stable version |
| ---    | ---       | ---            | ---            |
| ROS 2 Humble (u22.04) | [![Build Status](https://build.ros2.org/job/Hdev__nanoflann_vendor__ubuntu_jammy_amd64/badge/icon)](https://build.ros2.org/job/Hdev__nanoflann_vendor__ubuntu_jammy_amd64/) | [![Build Status](https://build.ros2.org/job/Hbin_uJ64__nanoflann_vendor__ubuntu_jammy_amd64__binary/badge/icon)](https://build.ros2.org/job/Hbin_uJ64__nanoflann_vendor__ubuntu_jammy_amd64__binary/)  <br/> [![Build Status](https://build.ros2.org/job/Hbin_ujv8_uJv8__nanoflann_vendor__ubuntu_jammy_arm64__binary/badge/icon)](https://build.ros2.org/job/Hbin_ujv8_uJv8__nanoflann_vendor__ubuntu_jammy_arm64__binary/) | [![Version](https://img.shields.io/ros/v/humble/nanoflann_vendor)](https://index.ros.org/?search_packages=true&pkgs=nanoflann_vendor) |
| ROS 2 Jazzy (u24.04) | [![Build Status](https://build.ros2.org/job/Jdev__nanoflann_vendor__ubuntu_noble_amd64/badge/icon)](https://build.ros2.org/job/Jdev__nanoflann_vendor__ubuntu_noble_amd64/) | [![Build Status](https://build.ros2.org/job/Jbin_uN64__nanoflann_vendor__ubuntu_noble_amd64__binary/badge/icon)](https://build.ros2.org/job/Jbin_uN64__nanoflann_vendor__ubuntu_noble_amd64__binary/) <br/> [![Build Status](https://build.ros2.org/job/Jbin_unv8_uNv8__nanoflann_vendor__ubuntu_noble_arm64__binary/badge/icon)](https://build.ros2.org/job/Jbin_unv8_uNv8__nanoflann_vendor__ubuntu_noble_arm64__binary/) | [![Version](https://img.shields.io/ros/v/jazzy/nanoflann_vendor)](https://index.ros.org/?search_packages=true&pkgs=nanoflann_vendor) |
| ROS 2 Kilted (u24.04) | [![Build Status](https://build.ros2.org/job/Kdev__nanoflann_vendor__ubuntu_noble_amd64/badge/icon)](https://build.ros2.org/job/Kdev__nanoflann_vendor__ubuntu_noble_amd64/) | [![Build Status](https://build.ros2.org/job/Kbin_uN64__nanoflann_vendor__ubuntu_noble_amd64__binary/badge/icon)](https://build.ros2.org/job/Kbin_uN64__nanoflann_vendor__ubuntu_noble_amd64__binary/) <br/> [![Build Status](https://build.ros2.org/job/Kbin_unv8_uNv8__nanoflann_vendor__ubuntu_noble_arm64__binary/badge/icon)](https://build.ros2.org/job/Kbin_unv8_uNv8__nanoflann_vendor__ubuntu_noble_arm64__binary/) | [![Version](https://img.shields.io/ros/v/kilted/nanoflann_vendor)](https://index.ros.org/?search_packages=true&pkgs=nanoflann_vendor) |
| ROS 2 Lyrical (u26.04) | [![Build Status](https://build.ros2.org/job/Ldev__nanoflann_vendor__ubuntu_resolute_amd64/badge/icon)](https://build.ros2.org/job/Ldev__nanoflann_vendor__ubuntu_resolute_amd64/) | [![Build Status](https://build.ros2.org/job/Lbin_uR64__nanoflann_vendor__ubuntu_resolute_amd64__binary/badge/icon)](https://build.ros2.org/job/Lbin_uR64__nanoflann_vendor__ubuntu_resolute_amd64__binary/) <br/> [![Build Status](https://build.ros2.org/job/Lbin_armv8_uRv8__nanoflann_vendor__ubuntu_resolute_arm64__binary/badge/icon)](https://build.ros2.org/job/Lbin_armv8_uRv8__nanoflann_vendor__ubuntu_resolute_arm64__binary/) | [![Version](https://img.shields.io/ros/v/lyrical/nanoflann_vendor)](https://index.ros.org/?search_packages=true&pkgs=nanoflann_vendor) |
| ROS 2 Rolling (u26.04) | [![Build Status](https://build.ros2.org/job/Rdev__nanoflann_vendor__ubuntu_resolute_amd64/badge/icon)](https://build.ros2.org/job/Rdev__nanoflann_vendor__ubuntu_resolute_amd64/) | [![Build Status](https://build.ros2.org/job/Rbin_uR64__nanoflann_vendor__ubuntu_resolute_amd64__binary/badge/icon)](https://build.ros2.org/job/Rbin_uR64__nanoflann_vendor__ubuntu_resolute_amd64__binary/) <br/> [![Build Status](https://build.ros2.org/job/Rbin_unv8_uRv8__nanoflann_vendor__ubuntu_resolute_arm64__binary/badge/icon)](https://build.ros2.org/job/Rbin_unv8_uRv8__nanoflann_vendor__ubuntu_resolute_arm64__binary/) |  [![Version](https://img.shields.io/ros/v/rolling/nanoflann_vendor)](https://index.ros.org/?search_packages=true&pkgs=nanoflann_vendor) |

(Binary build badges are for amd64 and arm64, respectively)

## 1. About

*nanoflann* is a **C++11 [header-only](http://en.wikipedia.org/wiki/Header-only) library** for building KD-Trees of datasets with different topologies: R<sup>2</sup>, R<sup>3</sup> (point clouds), SO(2) and SO(3) (2D and 3D rotation groups). No support for approximate NN is provided. *nanoflann* does not require compiling or installing. You just need to `#include <nanoflann.hpp>` in your code.

This library is a *fork* of the [flann library](https://github.com/flann-lib/flann) by Marius Muja and David G. Lowe, and born as a child project of [MRPT](https://www.mrpt.org/). Following the original license terms, *nanoflann* is distributed under the BSD license. Please, for bugs use the issues button or fork and open a pull request.

Cite as:
```
@misc{blanco2014nanoflann,
  title        = {nanoflann: a {C}++ header-only fork of {FLANN}, a library for Nearest Neighbor ({NN}) with KD-trees},
  author       = {Blanco, Jose Luis and Rai, Pranjal Kumar},
  howpublished = {\url{https://github.com/jlblancoc/nanoflann}},
  year         = {2014}
}
```

See the release [CHANGELOG](CHANGELOG.rst) for a list of project changes.

### 1.1. Obtaining the code

* Easiest way: clone this GIT repository and take the `include/nanoflann.hpp` file for use where you need it.
* Debian or Ubuntu ([21.04 or newer](https://launchpad.net/ubuntu/+source/nanoflann)) users can install it simply with:
  ```bash 
  $ sudo apt install libnanoflann-dev
  ```
* macOS users can install `nanoflann` with [Homebrew](https://brew.sh) with:
  ```shell
  $ brew install brewsci/science/nanoflann
  ```
  or
  ```shell
  $ brew tap brewsci/science
  $ brew install nanoflann
  ```
  MacPorts users can use:
  ```shell
  $ sudo port install nanoflann
  ```
* Linux users can also install it with [Linuxbrew](https://docs.brew.sh/Homebrew-on-Linux) with: `brew install homebrew/science/nanoflann`
* List of [**stable releases**](https://github.com/jlblancoc/nanoflann/releases). Check out the [CHANGELOG](https://github.com/jlblancoc/nanoflann/blob/master/CHANGELOG.md)

Although nanoflann itself doesn't have to be compiled, you can build some examples and tests with:

```shell
$ sudo apt-get install build-essential cmake libgtest-dev libeigen3-dev
$ mkdir build && cd build && cmake ..
$ make && make test
```

### 1.2. C++ API reference

  * Browse the [Doxygen documentation](https://jlblancoc.github.io/nanoflann/).

  * **Important note:** If L2 norms are used, notice that search radius and all passed and returned distances are actually *squared distances*.

### 1.3. Code examples

  * KD-tree look-up with `knnSearch()` and `radiusSearch()`: [pointcloud_kdd_radius.cpp](https://github.com/jlblancoc/nanoflann/blob/master/examples/pointcloud_kdd_radius.cpp)
  * KD-tree look-up on a point cloud dataset: [pointcloud_example.cpp](https://github.com/jlblancoc/nanoflann/blob/master/examples/pointcloud_example.cpp)
  * KD-tree look-up on a dynamic point cloud dataset: [dynamic_pointcloud_example.cpp](https://github.com/jlblancoc/nanoflann/blob/master/examples/dynamic_pointcloud_example.cpp)
  * KD-tree look-up on a rotation group (SO2): [SO2_example.cpp](https://github.com/jlblancoc/nanoflann/blob/master/examples/SO2_adaptor_example.cpp)
  * KD-tree look-up on a rotation group (SO3): [SO3_example.cpp](https://github.com/jlblancoc/nanoflann/blob/master/examples/SO3_adaptor_example.cpp)
  * KD-tree look-up on a point cloud dataset with an external adaptor class: [pointcloud_adaptor_example.cpp](https://github.com/jlblancoc/nanoflann/blob/master/examples/pointcloud_adaptor_example.cpp)
  * KD-tree look-up directly on an `Eigen::Matrix<>`: [matrix_example.cpp](https://github.com/jlblancoc/nanoflann/blob/master/examples/matrix_example.cpp)
  * KD-tree look-up directly on `std::vector<std::vector<T> >` or `std::vector<Eigen::VectorXd>`: [vector_of_vectors_example.cpp](https://github.com/jlblancoc/nanoflann/blob/master/examples/vector_of_vectors_example.cpp)
  * Example with a `Makefile` for usage through `pkg-config` (for example, after doing a "make install" or after installing from Ubuntu repositories): [example_with_pkgconfig/](https://github.com/jlblancoc/nanoflann/blob/master/examples/example_with_pkgconfig/)
  * Example of how to build an index and save it to disk for later usage: [saveload_example.cpp](https://github.com/jlblancoc/nanoflann/blob/master/examples/saveload_example.cpp)
  * GUI examples (requires `mrpt-gui`, e.g. `sudo apt install libmrpt-gui-dev`):
    - [nanoflann_gui_example_R3](https://github.com/jlblancoc/nanoflann/blob/master/examples/examples_gui/nanoflann_gui_example_R3/nanoflann_gui_example_R3.cpp)

![nanoflann-demo-1](https://user-images.githubusercontent.com/5497818/201550433-d561c5a9-4e87-453d-9cf8-8202d7876235.gif)



### 1.4. Why a fork?

  * **Execution time efficiency**:
    * The power of the original `flann` library comes from the possibility of choosing between different ANN algorithms. The cost of this flexibility is the declaration of pure virtual methods which (in some circumstances) impose [run-time penalties](http://www.cs.cmu.edu/~gilpin/c%2B%2B/performance.html#virtualfunctions). In `nanoflann` all those virtual methods have been replaced by a combination of the [Curiously Recurring Template Pattern](http://en.wikipedia.org/wiki/Curiously_recurring_template_pattern) (CRTP) and inlined methods, which are much faster.
    * For `radiusSearch()`, there is no need to make a call to determine the number of points within the radius and then call it again to get the data. By using STL containers for the output data, containers are automatically resized.
    * Users can (optionally) set the problem dimensionality at compile-time via a template argument, thus allowing the compiler to fully unroll loops.
    * `nanoflann` allows users to provide a precomputed bounding box of the data, if available, to avoid recomputation.
    * Indices of data points have been converted from `int` to `size_t`, which removes a limit when handling very large data sets.

  * **Memory efficiency**: Instead of making a copy of the entire dataset into a custom `flann`-like matrix before building a KD-tree index, `nanoflann` allows direct access to your data via an **adaptor interface** which must be implemented in your class.

Refer to the examples below or to the C++ API of [nanoflann::KDTreeSingleIndexAdaptor<>](https://jlblancoc.github.io/nanoflann/classnanoflann_1_1KDTreeSingleIndexAdaptor.html) for more info.


### 1.5. What can *nanoflann* do?

  * Building KD-trees with a single index (no randomized KD-trees, no approximate searches).
  * Fast, thread-safe querying for closest neighbors on KD-trees. The entry points are:
    * [nanoflann::KDTreeSingleIndexAdaptor<>](https://jlblancoc.github.io/nanoflann/classnanoflann_1_1KDTreeSingleIndexAdaptor.html)`::knnSearch()`
      * Finds the `num_closest` nearest neighbors to `query_point[0:dim-1]`. Their indices are stored inside the result object. See an [example usage code](https://github.com/jlblancoc/nanoflann/blob/master/examples/pointcloud_kdd_radius.cpp#L119).
    * [nanoflann::KDTreeSingleIndexAdaptor<>](https://jlblancoc.github.io/nanoflann/classnanoflann_1_1KDTreeSingleIndexAdaptor.html)`::radiusSearch()`
      * Finds all the neighbors to `query_point[0:dim-1]` within a maximum radius. The output is given as a vector of pairs, of which the first element is a point index and the second the corresponding distance. See an [example usage code](https://github.com/jlblancoc/nanoflann/blob/master/examples/pointcloud_kdd_radius.cpp#L141).
    * [nanoflann::KDTreeSingleIndexAdaptor<>](https://jlblancoc.github.io/nanoflann/classnanoflann_1_1KDTreeSingleIndexAdaptor.html)`::radiusSearchCustomCallback()`
    * Can be used to receive a callback for each point found in range. This may be more efficient in some situations instead of building a huge vector of pairs with the results.
    * [nanoflann::KDTreeSingleIndexAdaptor<>](https://jlblancoc.github.io/nanoflann/classnanoflann_1_1KDTreeSingleIndexAdaptor.html)`::findWithinBox()` [New in 1.8.0]: Optimized search within a given axis-aligned bound box.
  * Working with 2D and 3D point clouds or N-dimensional data sets.
  * Working with integral element types, including unsigned ones. Since
    `_DistanceType` defaults to the element type and must be **signed**, an
    unsigned element type requires passing it explicitly, wide enough for the
    distances of the actual coordinate range, e.g.
    `nanoflann::L2_Simple_Adaptor<uint8_t, MyCloud, int32_t>`. To use it
    through the `nanoflann::metric_*` tags, define your own tag:
    ```cpp
    struct my_metric_L2 : public nanoflann::Metric
    {
        template <class T, class DataSource, typename IndexType = size_t>
        struct traits
        {
            using distance_t = nanoflann::L2_Simple_Adaptor<T, DataSource, int32_t, IndexType>;
        };
    };
    ```
  * Working directly with `Eigen::Matrix<>` classes (matrices and vectors-of-vectors).
  * Working with dynamic point clouds without a need to rebuild entire kd-tree index. Two options:
    * `nanoflann::KDTreeSingleIndexDynamicAdaptor<>`: the Bentley–Saxe "logarithmic forest" of static sub-trees.
    * `nanoflann::KDTreeSingleIndexIncrementalAdaptor<>` [New]: a single self-balancing tree, recommended for sliding-window LiDAR-style maps. Supports incremental `addPoints`, lazy `removePoint`, and axis-aligned box trimming (`removeBox` / `removeOutsideBox`) with bounded memory under churn. See the design & benchmark report in [nanoflann-benchmark/incrementalTests](https://github.com/MRPT/nanoflann-benchmark/tree/master/incrementalTests/REPORT.md).
    * `nanoflann::KDTreeSingleIndexIncrementalAdaptorMT<>` [New]: the same index with the large rebalancing rebuilds offloaded to a background thread, to bound the foreground update-latency tail (see the [threading analysis](https://github.com/MRPT/nanoflann-benchmark/tree/master/incrementalTests/async_rebalance.md)). Disabled under `NANOFLANN_NO_THREADS`.
  * Working with the distance metrics:
    * `R^N`: Euclidean spaces:
      * `L1` (Manhattan)
      * `L2` (**squared** Euclidean norm, favoring SSE2 optimization).
      * `L2_Simple` (**squared** Euclidean norm, for low-dimensionality data sets like point clouds).
    * `SO(2)`: 2D rotational group
      * `metric_SO2`: Absolute angular diference.
    * `SO(3)`: 3D rotational group (better suppport to be provided in future releases)
      * `metric_SO3`: Inner product between quaternions.
  * Saves and load the built indices to disk.
  * GUI based support for benchmarking multiple kd-tree libraries namely nanoflann, flann, fastann and libkdtree.

### 1.6. What can't *nanoflann* do?

  * Use other distance metrics apart from L1, L2, SO2 and SO3.
  * Support for SE(3) groups.
  * Only the C++ interface exists: there is no support for C, MATLAB or Python.
  * There is no automatic algorithm configuration (as described in the original Muja & Lowe's paper).

### 1.7. Use in your project via CMake

You can directly drop the `nanoflann.hpp` file in your project. Alternatively,
the CMake standard method is also available:

  * Build and "install" nanoflann. Set `CMAKE_INSTALL_PREFIX` to a proper path
  and then execute `make install` (Linux, OSX) or build the `INSTALL`
  target (Visual Studio).
  * Then, add something like this to the CMake script of your project:

```cmake
# Find nanoflannConfig.cmake:
find_package(nanoflann)

add_executable(my_project test.cpp)

# Make sure the include path is used:
target_link_libraries(my_project nanoflann::nanoflann)
```

### 1.8. Package Managers

#### Using `conan`

You can install pre-built binaries for `nanoflann` or build it from source using [Conan](https://conan.io/). Use the following command to install latest version:

```sh
$ conan install --requires="nanoflann/[*]" --build=missing
```

For detailed instructions on how to use Conan, please refer to the [Conan documentation](https://docs.conan.io/2/).

The `nanoflann` Conan recipe is kept up to date by Conan maintainers and community contributors.
If the version is out of date, please [create an issue or pull request](https://github.com/conan-io/conan-center-index) on the ConanCenterIndex repository.

#### Using `vcpkg`

You can download and install nanoflann using the [vcpkg](https://github.com/Microsoft/vcpkg) dependency manager:

```sh
$ git clone https://github.com/Microsoft/vcpkg.git
$ cd vcpkg
$ ./bootstrap-vcpkg.sh
$ ./vcpkg integrate install
$ ./vcpkg install nanoflann
```

The nanoflann port in vcpkg is kept up to date by Microsoft team members and community contributors. If the version is out of date, please [create an issue or pull request](https://github.com/Microsoft/vcpkg) on the vcpkg repository.


### 1.9. Compile time definitions

  * `NANOFLANN_FIRST_MATCH`: If defined and two points have the same distance, the one with the lowest-index will be returned first. Otherwise there is no particular order.
  * `NANOFLANN_NO_THREADS`: If defined, multithreading capabilities will be disabled, so that the library can be used without linking with pthreads. If one tries to use multiple threads, an exception will be thrown.
  * `NANOFLANN_NODE_ALIGNMENT`: The memory alignment, in bytes, used for KD-tree nodes. Defaults to `16`.

### 1.10. Thread safety

  * **Index build**: pass `n_thread_build > 1` in `KDTreeSingleIndexAdaptorParams` to parallelize the build via `std::async` (unless `NANOFLANN_NO_THREADS` is defined).
  * **Queries**: `findNeighbors()`, `knnSearch()`, `radiusSearch()` and `rknnSearch()` are `const` and safe to call concurrently from multiple threads on the same index, as long as no thread is concurrently building or modifying it.
  * Building an index from multiple threads (or mixing queries with an in-progress build) is not supported.

------

## 2. Any help choosing the KD-tree parameters?

### 2.1. `KDTreeSingleIndexAdaptorParams::leaf_max_size`

A KD-tree is... well, a tree :-). And as such it has a root node, a set of intermediary nodes and finally, "leaf" nodes which are those without children.

Points (or, properly, point indices) are only stored in leaf nodes. Each leaf contains a list of which points fall within its range.

While building the tree, nodes are recursively divided until the number of points inside is equal or below some threshold. **That is `leaf_max_size`**. While doing queries, the  "tree algorithm" ends by selecting leaf nodes, then performing linear search (one-by-one) for the closest point to the query within all those in the leaf.

So, `leaf_max_size` must be set as a **tradeoff**:
  * Large values mean that the tree will be built faster (since the tree will be smaller), but each query will be slower (since the linear search in the leaf is to be done over more points).
  * Small values will build the tree much slower (there will be many tree nodes), but queries will be faster... up to some point, since the "tree-part" of the search (logarithmic complexity) still has a significant cost.

What number to select really depends on the application and even on the size of the processor cache memory, so ideally you should do some benchmarking for maximizing efficiency.

But to help choosing a good value as a rule of thumb, I provide the following two benchmarks. Each graph represents the tree build (horizontal) and query (vertical) times for different `leaf_max_size` values between 1 and 10K (as 95% uncertainty ellipses, deformed due to the logarithmic scale).

  * A 100K point cloud, uniformly distributed (each point has (x,y,z) `float` coordinates):

![perf5_1e5pts_time_vs_maxleaf](https://raw.githubusercontent.com/jlblancoc/nanoflann/master/doc/perf5_1e5pts_time_vs_maxleaf.png)

  * A ~150K point cloud from a real dataset (`scan_071_points.dat` from the [Freiburg Campus 360 dataset](http://ais.informatik.uni-freiburg.de/projects/datasets/fr360/), each point has (x,y,z) `float` coordinates):

![perf5_1e5pts_time_vs_maxleaf_real_dataset](https://raw.githubusercontent.com/jlblancoc/nanoflann/master/doc/perf5_1e5pts_time_vs_maxleaf_real_dataset.png)

So, it seems that a `leaf_max_size` **between 10 and 50** would be optimum in applications where the cost of queries dominates (e.g. [ICP](http://en.wikipedia.org/wiki/Iterative_closest_point])). At present, its default value is 10.


### 2.2. `KDTreeSingleIndexAdaptorParams::checks`

This parameter is really ignored in `nanoflann`, but was kept for backward compatibility with the original FLANN interface. Just ignore it.

### 2.3. `KDTreeSingleIndexAdaptorParams::n_thread_build`

This parameter determines the maximum number of threads that can be called concurrently during the construction of the KD tree. The default value is 1. When the parameter is set to 0, `nanoflann` automatically determines the number of threads to use.

See [this pull request](https://github.com/jlblancoc/nanoflann/pull/236) for some benchmarking showing that using the maximum number of threads is not always the most efficient approach. Do benchmarking on your data!

-----

## 3. Performance

### 3.1. `nanoflann`: faster and less memory usage

Refer to the "Why a fork?" section above for the main optimization ideas behind `nanoflann`.

Notice that there are no explicit SSE2/SSE3 optimizations in `nanoflann`, but the intensive usage of `inline` and templates in practice turns into automatically SSE-optimized code generated by the compiler.


### 3.2. Benchmark: original `flann` vs `nanoflann`

The most time-consuming part of many point cloud algorithms (like ICP) is querying a KD-Tree for nearest neighbors. This operation is therefore the most time critical.

`nanoflann` provides a ~50% time saving with respect to the original `flann` implementation (times in this chart are in microseconds for each query):

![perf3_query](https://raw.githubusercontent.com/jlblancoc/nanoflann/master/doc/perf3_query.small.png)

Although most of the gain comes from the queries (due to the large number of them in any typical operation with point clouds), there is also some time saved while building the KD-tree index, due to the templatized-code but also for the avoidance of duplicating the data in an auxiliary matrix (times in the next chart are in milliseconds):

![perf4_time_saved](https://raw.githubusercontent.com/jlblancoc/nanoflann/master/doc/perf4_time_saved.small.png)

These performance tests are only representative of our testing. If you want to repeat them, read the instructions in [perf-tests](https://github.com/jlblancoc/nanoflann/tree/master/perf-tests)


----

## 4. Releasing a new version (maintainers only)

The version number is duplicated in `include/nanoflann.hpp` (two macros),
`package.xml`, the `CHANGELOG.rst` header and the git tag. `scripts/release.py`
keeps all of them in sync:

```bash
scripts/release.py check                   # verify all sources agree (also run in CI)
scripts/release.py prepare --bump minor    # or: --version 1.12.0, and --dry-run
```

`prepare` runs `catkin_generate_changelog`, pauses for you to edit the
`Forthcoming` section, then calls `catkin_prepare_release` and folds the header
macro bump into the same release commit and tag. Nothing is pushed: the script
prints the remaining `git push` and `bloom-release` commands.

Pushing the tag triggers the `Create GitHub release` workflow, which publishes a
GitHub release with the notes taken from that version's `CHANGELOG.rst` entry
(`scripts/release.py notes` renders it as Markdown).

----

## 5. Other KD-tree projects

  * [FLANN](http://www.cs.ubc.ca/research/flann/) - Marius Muja and David G. Lowe (University of British Columbia).
  * [FASTANN](http://www.robots.ox.ac.uk/~vgg/software/fastann/) - James Philbin (VGG, University of Oxford).
  * [ANN](http://www.cs.umd.edu/~mount/ANN/) - David M. Mount and Sunil Arya (University of Maryland).
  * [libkdtree++](https://packages.debian.org/source/sid/libkdtree++) - Martin F. Krafft & others.

<br>

*Note: The project logo is due to [CedarSeed](http://www.iconarchive.com/show/patisserie-icons-by-cedarseed/Flan-icon.html)*

**Contributors**

<a href="https://github.com/jlblancoc/nanoflann/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=jlblancoc/nanoflann" />
</a>

