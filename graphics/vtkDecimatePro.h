/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDecimatePro.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkDecimatePro - reduce the number of triangles in a triangle mesh (and generate progressive meshes)
// .SECTION Description
// vtkDecimatePro is a filter to reduce the number of triangles in a triangle 
// mesh, forming a good approximation to the original geometry. The input to 
// vtkDecimatePro is a vtkPolyData object, and only triangles are treated. If 
// you desire to decimate polygonal meshes, first triangulate the polygons
// with vtkTriangleFilter object.
// 
// The implementation of vtkDecimatePro is similar to vtkDecimate,
// with three major differences. First, this algorithm does not
// necessarily preserve the topology of the mesh. Second, it is
// guaranteed to give the a mesh reduction factor specified by the
// user (if Splitting is on). Third, it is set up generate progressive
// meshes, that is a stream of operations that can be easily transmitted and
// incrementally updated (see Hugues Hoppe's Siggraph '96 paper on
// progressive meshes).
// 
// The algorithm proceeds as follows. Each vertex in the mesh is classified and
// inserted into a priority queue. The priority is based on the error to
// delete the vertex and retriangulate the hole. Vertices that cannot be
// deleted or triangulated at this point are skipped. Then, each vertex in
// the priority queue is processed (i.e., deleted followed by hole
// triangulation). This continues until the priority queue is empty. Next,
// all vertices are processed, and the mesh is split into separate
// pieces. The vertices are then again reinserted into the priority queue,
// but this time as they are processed, vertices that cannot be triangulated
// are split (possibly recursively). This continues until the requested
// reduction level is achieved.
// 
// To use this object, at a minimum you need to specify the ivar Reduction. The
// algorithm is guaranteed to generate a reduced mesh at this level, unless
// the ivar Splitting is turned off. (Splitting prevents the separation of the mesh
// into separate pieces, so topological contraints may prevent the algorithm
// from realizing the requested reduction.) You may also wish to adjust the
// FeatureAngle and SplitAngle ivars, since these can impact the quality of
// the final mesh.

// .SECTION Caveats
// Do not use this class if you need to preserve the topology of the
// mesh. Even with splitting turned off, holes may be closed or non-manifold
// attachments may be formed. Use vtkDecimate instead.

// .SECTION See Also
// vtkDecimate vtkProgressiveMeshReader

#ifndef __vtkDecimatePro_h
#define __vtkDecimatePro_h

// This file is included to grab structure definitions and include files
#include "vtkDecimate.h"

class vtkDecimatePro : public vtkPolyToPolyFilter
{
public:
  vtkDecimatePro();
  char *GetClassName() {return "vtkDecimatePro";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the desired reduction as a fraction of the original number of triangles.
  // The algorithm is guaranteed to give this reduction factor (or greater)
  // if the Splitting ivar is turned on.
  vtkSetClampMacro(Reduction,float,0.0,1.0);
  vtkGetMacro(Reduction,float);

  // Description:
  // Specify the mesh feature angle. This angle is used to define what
  // an edge is (i.e., if the surface normal between two adjacent triangles
  // is >= FeatureAngle, an edge exists).
  vtkSetClampMacro(FeatureAngle,float,0.0,180.0);
  vtkGetMacro(FeatureAngle,float);

  // Description:
  // Turn on/off the splitting of the mesh at corners, along edges, at
  // non-manifold points, or anywhere else a split is required. Turning 
  // splitting off will better preserve the original topology of the
  // mesh, but you may not obtain the requested reduction.
  vtkSetMacro(Splitting,int);
  vtkGetMacro(Splitting,int);
  vtkBooleanMacro(Splitting,int);

  // Description:
  // Specify the mesh split angle. This angle is used to control the splitting
  // of the mesh. A split line exists when the surface normals between
  // two edge connected triangles are >= SplitAngle.
  vtkSetClampMacro(SplitAngle,float,0.0,180.0);
  vtkGetMacro(SplitAngle,float);

  // Description:
  // Force the algorithm to defer splitting the mesh as long as possible.
  vtkSetMacro(DeferSplitting,int);
  vtkGetMacro(DeferSplitting,int);
  vtkBooleanMacro(DeferSplitting,int);

  // Description:
  // If the number of triangles connected to a vertex exceeds "Degree", then 
  // the vertex will be split. (NOTE: the complexity of the triangulation algorithm 
  // is proportional to Degree^2. Setting degree small can improve the
  // performance of the algorithm.)
  vtkSetClampMacro(Degree,int,25,VTK_CELL_SIZE);
  vtkGetMacro(Degree,int);
  
  // Description:
  // Get the number of "operations" used to reduce the data to the 
  // requested reduction value. An operation is something like an edge
  // collapse or vertex split that modifies the mesh. The number of
  // operations is valid only after the filter has executed.
  vtkGetMacro(NumberOfOperations,int);

  // Description:
  // Specify the inflection point ratio. An inflection point occurs
  // when the ratio of reduction error between two iterations is >=
  // InflectionPointRatio.
  vtkSetClampMacro(InflectionPointRatio,float,1.001,VTK_LARGE_FLOAT);
  vtkGetMacro(InflectionPointRatio,float);

  // Get a list of inflection points. These are integer values 
  // 0 < v <= NumberOfOperations corresponding to operation number. These
  // methods return a valid result only after the filter has executed.
  int GetNumberOfInflectionPoints();
  void GetInflectionPoints(int *inflectionPoints);
  int *GetInflectionPoints();

  // These are special methods that enable you to get or write output
  // without changing modified time. This means that you can execute the
  // filter at a certain reduction level (say 90%), and then grab output
  // at reduction levels <= 90% without re-execution.
  // ****Commented out temporarily
  //***void GetOutput(vtkPolyData &pd, float reduction); //copy data into pd
  //***void WriteProgressiveMesh(char *filename); //write data

  void ProcessDeferredSplits(int numPops);

protected:
  void Execute();

  float Reduction; 
  float FeatureAngle;
  float SplitAngle;
  int Splitting;
  int DeferSplitting;
  int Degree;
  int NumberOfOperations;
  float InflectionPointRatio;
  vtkIntArray InflectionPoints;
};

#endif


