/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpTo.h
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
// .NAME vtkWarpTo - deform geometry by warping towards a point
// .SECTION Description
// vtkWarpTo is a filter that modifies point coordinates by moving the
// points towards a user specified position.

#ifndef __vtkWarpTo_h
#define __vtkWarpTo_h

#include "vtkPointSetToPointSetFilter.h"

class VTK_EXPORT vtkWarpTo : public vtkPointSetToPointSetFilter
{
public:
  vtkWarpTo() {this->ScaleFactor = 0.5; this->Absolute = 0;
	       this->Position[0] = this->Position[1] = this->Position[2] = 0.0;};
  char *GetClassName() {return "vtkWarpTo";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the value to scale displacement.
  vtkSetMacro(ScaleFactor,float);
  vtkGetMacro(ScaleFactor,float);

  // Description:
  // Set/Get the position to warp towards.
  vtkGetVectorMacro(Position,float,3);
  vtkSetVector3Macro(Position,float);

  // Description:
  // Set/Get the Absolute ivar. Turning Absolute on causes scale factor
  // of the new position to be one unit away from Position.
  vtkSetMacro(Absolute,int);
  vtkGetMacro(Absolute,int);
  vtkBooleanMacro(Absolute,int);
  
protected:
  void Execute();
  float ScaleFactor;
  float Position[3];
  int   Absolute;
};

#endif


