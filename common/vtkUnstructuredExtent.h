/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredExtent.h
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
// .NAME vtkUnstructuredExtent - Specified in "borg" notation: Piece x of n.
// .SECTION Description
// Note:  This object is under development an might change in the future.
// vtkUnstructuredExtent contains information used to specify 
// a piece of unstructured data like vtkPolyData.  The notation is:
// piece x of N where x is in the range [0, n-1].


#ifndef __vtkUnstructuredExtent_h
#define __vtkUnstructuredExtent_h

#include "vtkExtent.h"

class VTK_EXPORT vtkUnstructuredExtent : public vtkExtent
{
public:
  static vtkUnstructuredExtent *New();

  const char *GetClassName() {return "vtkUnstructuredExtent";}
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Copy information from one extent into another.
  // This tries to be smart if the types are different.
  void Copy(vtkExtent *in);

  // Description:
  // Access to the extent.  ext[0] is piece x, ext[1] is Number of pieces.
  // I do not like the name "Extent" for this ivar.  I will probably
  // change it soon.
  vtkSetVector2Macro(Extent, int);
  vtkGetVector2Macro(Extent, int);  
  int GetPiece() { return this->Extent[0]; }
  int GetNumberOfPieces() { return this->Extent[1]; }
  
  // Description:
  // Serialization provided for the multi-process ports.
  void ReadSelf(istream& is);
  void WriteSelf(ostream& os);

protected:
  vtkUnstructuredExtent();
  ~vtkUnstructuredExtent() {};
  vtkUnstructuredExtent(const vtkUnstructuredExtent&) {};
  void operator=(const vtkUnstructuredExtent&) {};
  
  // This is the way the extent was specified before these objects.
  int Extent[2];
};


#endif
