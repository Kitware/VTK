/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMassProperties.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Abdalmajeid M. Alyassin who developed this class.

Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkWriter - abstract class to write data to file(s)
// .SECTION Description
// vtkMassProperties estimates the volume, the surface area, and the
// normalized shape index of a model.  The algorithm implemented here is
// based on the discrete form of the divergence theorem.  The general
// assumption here is that the model is of closed surface.  For more
// details see the following reference (Alyassin A.M. et al, "Evaluation
// of new algorithms for the interactive measurement of surface area and
// volume", Med Phys 21(6) 1994.).
// NOTE: currently only triangles are processed. Use vtkTriangleFilter
// to convert any strips or polygons to triangles.


#ifndef __vtkMassProperties_h
#define __vtkMassProperties_h

#include "vtkProcessObject.h"
#include "vtkPolyData.h"

class VTK_EXPORT vtkMassProperties : public vtkProcessObject
{
public:
  vtkMassProperties();
  const char *GetClassName() {return "vtkMassProperties";};
  static vtkMassProperties *New() {return new  vtkMassProperties;};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Compute and return the volume.
  double GetVolume() { this->Update();return this->Volume;}

  // Description:
  // Compute and return the volume projected on to each axis aligned plane.
  double GetVolumeX() { this->Update();return this->VolumeX;}
  double GetVolumeY() { this->Update();return this->VolumeY;}
  double GetVolumeZ() { this->Update();return this->VolumeZ;}

  // Description:
  // Compute and return the weighting factors for the maximum unit
  // normal component (MUNC).
  double GetKx() { this->Update();return this->Kx;}
  double GetKy() { this->Update();return this->Ky;}
  double GetKz() { this->Update();return this->Kz;}

  // Description:
  // Compute and return the area.
  double GetSurfaceArea() { this->Update();return this->SurfaceArea;}

  // Description:
  // Compute and return the normalized shape index. This characterizes the
  // deviation of the shape of an object from a sphere. A sphere's NSI
  // is one. This number is always >= 1.0.
  double GetNormalizedShapeIndex() { this->Update();return this->NormalizedShapeIndex;}

  void Execute();
  void Update();

  void SetInput(vtkPolyData *input);
  vtkPolyData *GetInput() {return (vtkPolyData *)this->Input;};

protected:
  double  SurfaceArea;
  double  Volume ;
  double  VolumeX;
  double  VolumeY;
  double  VolumeZ;
  double  Kx;
  double  Ky;
  double  Kz;
  double  NormalizedShapeIndex;
  vtkPolyData *Input;
  vtkTimeStamp ExecuteTime;

};

#endif


