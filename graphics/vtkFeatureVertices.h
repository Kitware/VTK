/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFeatureVertices.h
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
// .NAME vtkFeatureVertices - extract boundary, non-manifold, and/or sharp vertices from polygonal data (operates on line primitives)
// .SECTION Description
// vtkFeatureVertices is a filter to extract special types of vertices from
// input polygonal data. In particular, the filter operates on the line
// primitives in the polygonal data. The vertex types are: 1) boundary 
// (used by one line) or a vertex cell type; 2) non-manifold (used by three 
// or more lines); or 3) feature edges (vertices used by two lines 
// and whose orientation angle > FeatureAngle). The orientation angle is 
// computed from the dot product between the two lines. These vertices may 
// be extracted in any combination. Vertices may also be "colored" (i.e., 
// scalar values assigned) based on vertex type.
// .SECTION Caveats
// This filter operates only on line primitives in polygonal data. Some data
// may require pre-processing with vtkCleanPolyData to merge coincident points.
// Otherwise points may be flagged as boundary. (This is true when running
// vtkFeatureEdges and then vtkFeatureVertices.)
// .SECTION See Also
// vtkFeatureEdges

#ifndef __vtkFeatureVertices_h
#define __vtkFeatureVertices_h

#include "vtkPolyToPolyFilter.hh"

class vtkFeatureVertices : public vtkPolyToPolyFilter
{
public:
  vtkFeatureVertices();
  char *GetClassName() {return "vtkFeatureVertices";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Turn on/off the extraction of boundary vertices.
  vtkSetMacro(BoundaryVertices,int);
  vtkGetMacro(BoundaryVertices,int);
  vtkBooleanMacro(BoundaryVertices,int);

  // Description:
  // Turn on/off the extraction of feature vertices.
  vtkSetMacro(FeatureVertices,int);
  vtkGetMacro(FeatureVertices,int);
  vtkBooleanMacro(FeatureVertices,int);

  // Description:
  // Specify the feature angle for extracting feature vertices.
  vtkSetClampMacro(FeatureAngle,float,0.0,180.0);
  vtkGetMacro(FeatureAngle,float);

  // Description:
  // Turn on/off the extraction of non-manifold vertices.
  vtkSetMacro(NonManifoldVertices,int);
  vtkGetMacro(NonManifoldVertices,int);
  vtkBooleanMacro(NonManifoldVertices,int);

  // Description:
  // Turn on/off the coloring of vertices by type.
  vtkSetMacro(Coloring,int);
  vtkGetMacro(Coloring,int);
  vtkBooleanMacro(Coloring,int);

protected:
  // Usual data generation method
  void Execute();

  float FeatureAngle;
  int BoundaryVertices;
  int FeatureVertices;
  int NonManifoldVertices;
  int Coloring;
};

#endif


