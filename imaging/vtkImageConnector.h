/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageConnector.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

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
// .NAME vtkImageConnector - Create a binary image of a sphere.
// .SECTION Description
// vtkImageConnector is a helper class for connectivity filters.  
// It is not meant to be used directly.
// It implements a stack and breadth first search necessary for
// some connectivity filters.  Filtered axes sets the dimensionality 
// of the neighbor comparison, and
// cannot be more than three dimensions.  
// As implemented, only voxels which share faces are considered
// neighbors. 


#ifndef __vtkImageConnector_h
#define __vtkImageConnector_h

#include "vtkObject.h"
class vtkImageRegion;



//
// Special classes for manipulating data
//
//BTX - begin tcl exclude
//
// For the breadth first search
class vtkImageConnectorSeed { //;prevent man page generation
public:
  void *Pointer;
  int  Index[3];
  vtkImageConnectorSeed *Next;
};
//ETX - end tcl exclude
//


class VTK_EXPORT vtkImageConnector : public vtkObject
{
public:
  vtkImageConnector();
  ~vtkImageConnector();
  static vtkImageConnector *New() {return new vtkImageConnector;};
  const char *GetClassName() {return "vtkImageConnector";};
  
  vtkImageConnectorSeed *NewSeed(int index[3], void *ptr);
  void AddSeed(vtkImageConnectorSeed *seed);
  void AddSeedToEnd(vtkImageConnectorSeed *seed);
  void RemoveAllSeeds();

  // Description:
  // Values used by the MarkRegion method
  vtkSetMacro(ConnectedValue, unsigned char); 
  vtkGetMacro(ConnectedValue, unsigned char);
  vtkSetMacro(UnconnectedValue, unsigned char);
  vtkGetMacro(UnconnectedValue, unsigned char);

  void MarkRegion(vtkImageRegion *region, int dimensionality);

private:
  unsigned char ConnectedValue;
  unsigned char UnconnectedValue;

  vtkImageConnectorSeed *PopSeed();

  vtkImageConnectorSeed *Seeds;
  vtkImageConnectorSeed *LastSeed;
};



#endif

  
