/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkLODActor.h
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
// .NAME vtkLODActor - an actor that supports multiple levels of detail
// .SECTION Description
// vtkLODActor is an actor that stores multiple Levels of Detail and can
// automatically switch between them. It selects which level of detail
// to use based on how much time it has been allocated to render. 
// Currently a very simple method of TotalTime/NumberOfActors is used.
// In the future this should be modified to dynamically allocate the
// rendering time between different actors based on their needs.
// There are currently three levels of detail. The top level is just
// the normal data.  The lowest level of detail is a simple bounding
// box outline of the actor. The middle level of detail is a point
// cloud of a fixed number of points that have been randomly sampled
// from the Mappers input data.  Point attributes are copied over to
// the point cloud.  These two lower levels of detail are accomplished by
// creating instances of a vtkOutlineFilter, vtkGlyph3D, and vtkPointSource.

// .SECTION see also
// vtkActor vtkRenderer

#ifndef __vtkLODActor_h
#define __vtkLODActor_h

#include "vtkActor.h"
#include "vtkMaskPoints.h"
#include "vtkOutlineFilter.h"
#include "vtkPolyMapper.h"
#include "vtkGlyph3D.h"
#include "vtkPointSource.h"

class VTK_EXPORT vtkLODActor : public vtkActor
{
 public:
  vtkLODActor();
  ~vtkLODActor();
  static vtkLODActor *New() {return new vtkLODActor;};
  char *GetClassName() {return "vtkLODActor";};
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Render(vtkRenderer *ren);

  // Description:
  // Set/Get the number of random points for the point cloud.
  vtkGetMacro(NumberOfCloudPoints,int);
  vtkSetMacro(NumberOfCloudPoints,int);

protected:
  vtkPointSource      PointSource;
  vtkGlyph3D          Glyph3D;
  vtkPolyMapper       *MediumMapper;
  vtkPolyMapper       *LowMapper;
  vtkMaskPoints       MaskPoints;
  vtkOutlineFilter    OutlineFilter;
  vtkTimeStamp        BuildTime;
  float               Size;
  int                 NumberOfCloudPoints;
  float               Timings[3];
  vtkActor           *Device;
};

#endif


