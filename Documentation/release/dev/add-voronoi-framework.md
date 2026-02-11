## Voronoi Framework

A parallel, general framework for computing 2D and 3D Voronoi tessellations
and Delaunay triangulations has been added. In addition, three filters (which
use the framework) are now available. Most of these additions are available
in the new Filters/Meshing/ module.

The framework and filters expect as input a set of explicit generator points
(i.e., vtkPointSet) and optionally, integral point data defining a
segmentation label map that identifies each point as belonging to a
particular region, including regions identified as "outside". This enables
the algorithm to generate concave as well as convex output tessellations.

Extensive, detailed information is available from VTK's Design Documentation.

## Framework

The framework consists of several templated classes that enable users to
customize the information generated during processing. For example,
statistics in the neighborhood of each generator point can be gathered and
(parallel composited) to produce (invariant to the order of thread execution)
global output. Template parameters are used to control the parallel
compositing process, as well as the classification of the spokes (i.e., the
connections between neighboring point generators). Spokes are used to form
topological relationships between Voronoi cells, enabling topological
validity checks, and extraction of mesh features such as boundary faces, or
faces between regions (e.g., form a surface net).

The framework classes consist of:

+ vtkVoronoiCore.h/.txx
+ vtkVoronoiCore2D.h/.txx
+ vtkVoronoiCore3D.h/.txx
+ vtkVoronoiTile.h/.cxx
+ vtkVoronoiHull.h/.cxx


## Filters

The filters provide basic capabilities to generate the Voronoi tessellation in
2D and 3D, as well as unconstrained Delaunay triangulations. Options exist to
produce output in different forms, for example boundary meshes, unstructured
grids, and surface nets can be generated.

The filter classes consist of:

+ vtkVoronoiFlower2D.h/.cxx
+ vtkVoronoiFlower3D.h/.cxx
+ vtkGeneralizedSurfaceNets3D.h/.cxx


## Roadmap

This is an initial implementation of a general purpose meshing framework. We
expect active development to continue: for example meshing segmented volumes,
and supporting constrained triangulations.  This initial implementation will
also grow to better handle important numerical challenges, such as the
degeneracies often found in the Voronoi and Delaunay literature, and improving
the performance of the algorithms.
