/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PlaneSrc.hh
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
// .NAME vtkPlaneSource - create an array of quadrilaterals located in the plane
// .SECTION Description
// vtkPlaneSource creates an m x n array of quadrilaterals arranged as a
// regular tiling in the plane. The plane is centered at the origin, and 
// orthogonal to the global z-axis.  The resolution of the plane can be
// specified in both the x and y directions (i.e., specify m and n, 
// respectively).

#ifndef __vtkPlaneSource_h
#define __vtkPlaneSource_h

#include "PolySrc.hh"

class vtkPlaneSource : public vtkPolySource 
{
public:
  vtkPlaneSource() : XRes(1), YRes(1) {};
  vtkPlaneSource(const int xR, const int yR) {XRes=xR; YRes=yR;};
  void PrintSelf(ostream& os, vtkIndent indent);
  char *GetClassName() {return "vtkPlaneSource";};

  void SetResolution(const int xR, const int yR);
  void GetResolution(int& xR,int& yR) {xR=this->XRes; yR=this->YRes;};

protected:
  void Execute();
  int XRes;
  int YRes;
};

#endif


