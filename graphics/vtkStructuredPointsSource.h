/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsSource.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
// .NAME vtkStructuredPointsSource - Abstract class whose subclasses generates structured Points data
// .SECTION Description
// vtkStructuredPointsSource is an abstract class whose subclasses generate
// structured Points data.

// .SECTION See Also
// vtkStructuredPointsReader vtkPLOT3DReader

#ifndef __vtkStructuredPointsSource_h
#define __vtkStructuredPointsSource_h

#include "vtkSource.h"
#include "vtkStructuredPoints.h"

class VTK_EXPORT vtkStructuredPointsSource : public vtkSource
{
public:
  static vtkStructuredPointsSource *New() {return new vtkStructuredPointsSource;}
  const char *GetClassName() {return "vtkStructuredPointsSource";}

  // Description:
  // Set/Get the output of this source.
  void SetOutput(vtkStructuredPoints *output);
  vtkStructuredPoints *GetOutput();
  
protected:
  vtkStructuredPointsSource();
  ~vtkStructuredPointsSource() {};
  // Used by streaming: The extent of the output being processed
  // by the execute method. Set in the ComputeInputUpdateExtent method.
  int ExecuteExtent[6];

};

#endif


