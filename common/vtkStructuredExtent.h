/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredExtent.h
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
// .NAME vtkStructuredExtent - Min/Max for each of the three axes.
// .SECTION Description
// Note:  This object is under development an might change in the future.
// vtkStructuredExtent contains information to specify update extents of 
// structured data sets like vtkImageData.

#ifndef __vtkStructuredExtent_h
#define __vtkStructuredExtent_h

#include "vtkExtent.h"

class VTK_EXPORT vtkStructuredExtent : public vtkExtent
{
public:
  vtkStructuredExtent();
  static vtkStructuredExtent *New() {return new vtkStructuredExtent;};
  const char *GetClassName() {return "vtkStructuredExtent";}
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Copy information from one extent into a similar type event.
  // Subclasses over ride the correct type.
  void Copy(vtkStructuredExtent *in);

  // Description:
  // Access to the extent.  Note: I do not like the name "Extent" for this ivar
  // and will probably change it in the future.
  vtkSetVector6Macro(Extent, int);
  vtkGetVector6Macro(Extent, int);
  
protected:
  
  // This is the way the extent was specified before these objects.
  int Extent[6];
};


#endif
