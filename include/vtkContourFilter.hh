/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourFilter.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkContourFilter - generate isosurfaces/isolines from scalar values
// .SECTION Description
// vtkContourFilter is a filter that takes as input any dataset and 
// generates on output isosurfaces and/or isolines. The exact form 
// of the output depends upon the dimensionality of the input data. 
// Data consisting of 3D cells will generate isosurfaces, data 
// consisting of 2D cells will generate isolines, and data with 1D 
// or 0D cells will generate isopoints. Combinations of output type 
// are possible if the input dimension is mixed.
//
// If the input type is volume (e.g., 3D structured point dataset), 
// you may wish to use vtkMarchingCubes. This class is specifically tailored
// for volumes and is therefore much faster.
// .SECTION Caveats
// vtkContourFilter uses variations of marching cubes to generate output
// primitives. The output primitives are disjoint - that is, points may
// be generated that are coincident but distinct. You may want to use
// vtkCleanPolyData to remove the coincident points. Also, the isosurface
// is not generated with surface normals. Use vtkPolyNormals to create them,
// if desired.
// .SECTION See Also
// vtkMarchingCubes vtkSliceCubes vtkDividingCubes

#ifndef __vtkContourFilter_h
#define __vtkContourFilter_h

#include "vtkDataSetToPolyFilter.hh"

#define VTK_MAX_CONTOURS 256

class vtkContourFilter : public vtkDataSetToPolyFilter
{
public:
  vtkContourFilter();
  char *GetClassName() {return "vtkContourFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetValue(int i, float value);
  float GetValue(int i) {return this->Values[i];};

  // Description:
  // Return array of contour values (size of numContours).
  vtkGetVectorMacro(Values,float,VTK_MAX_CONTOURS);

  // Description:
  // Return the number of contour values.
  vtkGetMacro(NumberOfContours,int);

  // Description:
  // Set/Get the computation of normals. Normal computation is failrly expensive
  // in both time and storage. If the output data will be processed by filters
  // that modify topology or geometry, it may be wise to turn Normals and Gradients off.
  vtkSetMacro(ComputeNormals,int);
  vtkGetMacro(ComputeNormals,int);
  vtkBooleanMacro(ComputeNormals,int);

  // Description:
  // Set/Get the computation of gradients. Gradient computation is fairly expensive
  // in both time and storage. Note that if ComputeNormals is on, gradients will
  // have to be calculated, but will not be stored in the output dataset.
  // If the output data will be processed by filters that modify topology or
  // geometry, it may be wise to turn Normals and Gradients off.
  vtkSetMacro(ComputeGradients,int);
  vtkGetMacro(ComputeGradients,int);
  vtkBooleanMacro(ComputeGradients,int);

  // Description:
  // Set/Get the computation of scalars.
  vtkSetMacro(ComputeScalars,int);
  vtkGetMacro(ComputeScalars,int);
  vtkBooleanMacro(ComputeScalars,int);

  void GenerateValues(int numContours, float range[2]);
  void GenerateValues(int numContours, float range1, float range2);

protected:
  void Execute();

  int ComputeNormals;
  int ComputeGradients;
  int ComputeScalars;
  float Values[VTK_MAX_CONTOURS];
  int NumberOfContours;
  float Range[2];

  void StructuredPointsContour(int dim); //special contouring for structured points
};

#endif


