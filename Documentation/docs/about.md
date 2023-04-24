# About VTK

## What is VTK ?
The Visualization Toolkit (VTK) is an open-source, freely available software system for 3D computer graphics, modeling, image processing, volume rendering, scientific visualization, and 2D plotting. It supports a wide variety of visualization algorithms and advanced modeling techniques, and it takes advantage of both threaded and distributed memory parallel processing for speed and scalability, respectively.

VTK is designed to be platform agnostic. This means that it runs just about anywhere, including on Linux, Windows, and Mac; on the Web; and on mobile devices.

VTK employs Kitware’s quality software process, which includes CMake, CTest, CDash, and CPack to build, test, and package the system. Combined with a strong distributed developer community, the result is very high-quality, robust code. The core functionality of VTK is written in C++ to maximize efficiency. This functionality is wrapped into other language bindings to expose it to a wider audience. Interoperability with Python is particularly well-refined.

As open source software, VTK is free to use for any purpose. Technically, VTK has a BSD-style license, which imposes minimal restrictions for both open and closed source applications.

For statistics on VTK, please refer to its [Open Hub page](https://www.openhub.net/p/vtk).

### Features

#### Filters
VTK applications manipulate data with filters. Each filter inspects the data it receives and produces derived data. A connected set of filters forms a dataflow network. A configurable network turns raw data into more visually comprehensible formats.

#### Graphics System
VTK adds a rendering abstraction layer over the underlying graphics library (OpenGL for the most part). This higher level simplifies the task of creating compelling visualizations.

#### Data Model
VTK’s core data model has the ability to represent almost any real-world problem related to physical science. The fundamental data structures are particularly well-suited to medical imaging and engineering work that involves finite difference and finite element solutions.

#### Data Interaction
Interaction helps you understand the content, shape and meaning of data. In VTK, 3D widgets, interactors, and interfaces to 2D widget libraries like Qt enable you to add comprehensive user interaction to your programs.

#### 2D Plots and Charts
VTK has a full set of 2D plot and chart types for tabular data. VTK’s picking and selection capabilities help you interactively query your data. In addition, VTK is very interoperable with Python, including Matplotlib.

#### Parallel Processing
VTK has excellent support for scalable distributed-memory parallel processing under MPI. What is more, many VTK filters implement finer-grained parallelism via vtkSMP (for coarse-grained threading) and vtk-m (for fine-grained processing on many-core and GPU architectures).

## License
VTK is distributed under the OSI-approved BSD 3-clause License. See [here](https://gitlab.kitware.com/vtk/vtk/-/blob/master/Copyright.txt) for details.

## How to cite ?

To cite VTK in general, please reference the VTK textbook.

Schroeder, Will; Martin, Ken; Lorensen, Bill (2006), The Visualization Toolkit (4th ed.), Kitware, ISBN 978-1-930934-19-1

To cite a specific filter, check for extra references in the included headers or the [doxygen](https://vtk.org/doc/nightly/html) documentation of the filter.


## History

### Transition from OpenGL to OpenGL2
See [New OpenGL Rendering in VTK](https://www.kitware.com/new-opengl-rendering-in-vtk).

### Rendering Backend in ParaView 5.0
See [Brand-New Rendering Backend in ParaView 5.0](https://www.kitware.com/kitware-unleashes-brand-new-rendering-backend-in-paraview-5-0/).

### Origin
VTK was originally part of the textbook [The Visualization Toolkit An Object-Oriented Approach to 3D Graphics](https://vtk.org/documentation/#textbook). Will Schroeder, Ken Martin, and Bill Lorensen—three graphics and visualization researchers—wrote the book and companion software on their own time, beginning in December 1993, with legal permission from their then-employer, GE R&D. The motivation for the book was to collaborate with other researchers and develop an open framework for creating leading-edge visualization and graphics applications.

VTK grew out of the authors’ experiences at GE, particularly with the LYMB object-oriented graphics system. Other influences included the VISAGE visualization system developed by Schroeder et. al; the Clockworks object-oriented computer animation system developed at Rensselaer Polytechnic Institute; and the Object-Oriented Modeling and Design book, which Bill Lorensen co-authored.

After the core of VTK was written, users and developers around the world began to improve and apply the system to real-world problems. In particular, GE Medical Systems and other GE businesses contributed to the system, and researchers such as Dr. Penny Rheinghans began to teach with the book. Other early advocates include Jim Ahrens at Los Alamos National Laboratory and generous oil and gas supporters.

To address what was becoming a large, active, and world-wide community, Ken and Will—along with Lisa Avila, Charles Law, and Bill Hoffman—left GE in 1998 to found Kitware, Inc. Since that time, hundreds of additional developers have turned VTK into what is now the premier visualization system in the world. Sandia National Laboratories, for example, has been a strong supporter and co-developer, revamping 2D charting and information visualization in VTK.


## Acknowledgments
Many institutions have taken part in the development of VTK. Some of the most fundamental work came from the following:
- [Kitware](https://www.kitware.com)
- [Los Alamos National Lab (LANL)](http://www.lanl.gov)
- [National Library of Medicine (NLM)](http://www.nlm.nih.gov)
- [Department of Energy (DOE) ASC Program](http://www.cio.energy.gov/high-performance-computing.htm)
- [Sandia National Laboratories](http://www.sandia.gov)
- [Army Research Laboratory (ARL)](http://www.arl.army.mil/www/default.htm)

## Commercial Use
We invite commercial entities to use VTK.

VTK is part of Kitware’s collection of commercially supported open-source platforms for software development.

VTK’s License makes Commercial Use Available
* VTK is a free open source software distributed under a [BSD style license](#license).
* The license does not impose restrictions on the use of the software.
* VTK is NOT FDA approved. It is the users responsibility to ensure compliance with applicable rules and regulations.

## Contact Us
It is recommended to post any questions, bug reports, or enhancement requests to the [VTK forum](https://discourse.vtk.org).

Our online issue tracker is available [here](https://gitlab.kitware.com/vtk/vtk/-/issues).

For commercial/confidential consulting, contact [Kitware](https://www.kitware.com/contact/advanced-support/).
