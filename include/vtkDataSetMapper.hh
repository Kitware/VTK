/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetMapper.hh
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
// .NAME vtkDataSetMapper - map vtkDataSet and derived classes to graphics primitives
// .SECTION Description
// vtkDataSetMapper is a mapper to map data sets (i.e., vtkDataSet and 
// all derived classes) to graphics primitives. The mapping procedure
// is as follows: all 0D, 1D, and 2D cells are converted into points,
// lines, and polygons/triangle strips and then mapped to the graphics 
// system. The 2D faces of 3D cells are mapped only if they are used by 
// only one cell, i.e., on the boundary of the data set.

#ifndef __vtkDataSetMapper_h
#define __vtkDataSetMapper_h

#include "vtkGeometryFilter.hh"
#include "vtkPolyMapper.hh"
#include "vtkRenderer.hh"

class vtkDataSetMapper : public vtkMapper 
{
public:
  vtkDataSetMapper();
  ~vtkDataSetMapper();
  char *GetClassName() {return "vtkDataSetMapper";};
  void PrintSelf(ostream& os, vtkIndent indent);
  void Render(vtkRenderer *ren, vtkActor *act);
  float *GetBounds();

  // Description:
  // Specify the input data to map.
  void SetInput(vtkDataSet *in);
  void SetInput(vtkDataSet& in) {this->SetInput(&in);};

protected:
  vtkGeometryFilter *GeometryExtractor;
  vtkPolyMapper *PolyMapper;
};

#endif


