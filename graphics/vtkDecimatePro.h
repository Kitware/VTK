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
// user. Third, it is set up generate progressive meshes, that is a
// stream of operations that can be easily transmitted and
// incrementally updated (see Hugues Hoppe's Siggraph '96 paper on
// progressive meshes).
// 

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
  // Specify the desired reduction in the total number of polygons. The
  // algorithm is guaranteed to give this reduction factor (or greater).
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
  // splitting off will preserve the original topology of the mesh, but you
  // may not obtain the request reduction.
  vtkSetMacro(Splitting,int);
  vtkGetMacro(Splitting,int);
  vtkBooleanMacro(Splitting,int);

  // Description:
  // Force the algorithm to defer splitting the mesh as long as possible.
  vtkSetMacro(DeferSplitting,int);
  vtkGetMacro(DeferSplitting,int);
  vtkBooleanMacro(DeferSplitting,int);

  // Description:
  // Get the number of "operations" used to reduce the data to the 
  // requested reduction value. An operation is something like an edge
  // collapse or vertex split that modifies the mesh. The number of
  // operations is valid only after the filter has executed.
  // after the filter has executed.
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
  void GetOutput(vtkPolyData &pd, float reduction); //copy data into pd
  void WriteProgressiveMesh(char *filename); //write data
  void ProcessDeferredSplits(int numPts, int numPops);

protected:
  void Execute();

  float Reduction; 
  float FeatureAngle;
  int Splitting;
  int DeferSplitting;
  int NumberOfOperations;
  float InflectionPointRatio;
  vtkIntArray InflectionPoints;
};

#endif


