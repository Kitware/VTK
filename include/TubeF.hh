/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TubeF.hh
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
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.


=========================================================================*/
// .NAME vtkTubeFilter - filter that generates tubes around lines
// .SECTION Description
// vtkTubeFilter is a filter that generates a tube around each input line. 
// The tubes are made up of triangle strips and rotate around the tube with
// the rotation of the line normals. (If no normals are present, they are
// computed automatically). The radius of the tube can be set to vary with 
// scalar value. If the scalar value is speed (i.e., magnitude of velocity),
// the variation of the tube radius is such that it preserves mass flux in
// incompressible flow. The number of sides for the tube can also be 
// specified.
// .SECTION Caveats
// The number of tube sides must be greater than 3. If you wish to use fewer
// sides (i.e., a ribbon), use vtkRibbonFilter.
//    The input line must not have duplicate points, or normals at points that
// are parallel to the incoming/outgoing line segments. (Duplicate points
// can be removed with vtkCleanPolyData).

#ifndef __vtkTubeFilter_h
#define __vtkTubeFilter_h

#include "P2PF.hh"

class vtkTubeFilter : public vtkPolyToPolyFilter
{
public:
  vtkTubeFilter();
  ~vtkTubeFilter() {};
  char *GetClassName() {return "vtkTubeFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the minimum tube radius (minimum because the tube radius may vary).
  vtkSetClampMacro(Radius,float,0.0,LARGE_FLOAT);
  vtkGetMacro(Radius,float);

  // Description:
  // Turn on/off the variation of tube radius with scalar value.
  vtkSetMacro(VaryRadius,int);
  vtkGetMacro(VaryRadius,int);
  vtkBooleanMacro(VaryRadius,int);

  // Description:
  // Set the number of sides for the tube. At a minimum, number of sides is 3.
  vtkSetClampMacro(NumberOfSides,int,3,LARGE_INTEGER);
  vtkGetMacro(NumberOfSides,int);

  // Description:
  // Set the maximum tube radius in terms of a multiple of the minimum radius.
  vtkSetMacro(RadiusFactor,float);
  vtkGetMacro(RadiusFactor,float);

protected:
  // Usual data generation method
  void Execute();

  float Radius; //minimum radius of tube
  int VaryRadius; //controls whether radius varies with scalar data
  int NumberOfSides; //number of sides to create tube
  float RadiusFactor; //maxium allowablew radius
};

#endif


