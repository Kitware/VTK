/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAxes.hh
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
// .NAME vtkAxes - create an x-y-z axes
// .SECTION Description
// vtkAxes creates three lines that form an x-y-z axes. The origin of the 
// axes is user specified (0,0,0 is default), and the size is specified 
// with a scale factor. Three scalar values are generated for the three 
// lines and can be used (via color map) to indicate a particular 
// coordinate axis.

#ifndef __vtkAxes_h
#define __vtkAxes_h

#include "vtkPolySource.hh"

class vtkAxes : public vtkPolySource 
{
public:
  vtkAxes();
  char *GetClassName() {return "vtkAxes";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the origin of the axes.
  vtkSetVector3Macro(Origin,float);
  vtkGetVectorMacro(Origin,float,3);

  // Description:
  // Set the scale factor of the axes. Used to control size.
  vtkSetMacro(ScaleFactor,float);
  vtkGetMacro(ScaleFactor,float);

protected:
  void Execute();

  float Origin[3];
  float ScaleFactor;
};

#endif


