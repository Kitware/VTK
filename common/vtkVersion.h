/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVersion.h
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
// .NAME vtkVersion - Versioning class for vtk
// .SECTION Description
// Holds methods for defining/determining the current vtk version
// (major, minor, build).

// .SECTION Caveats
// This file will change frequently to update the VTKSourceVersion which
// timestamps a particular source release.


#ifndef __vtkVersion_h
#define __vtkVersion_h


#include "vtkObject.h"

#define VTK_VERSION "2.2.2"
#define VTK_MAJOR_VERSION 2
#define VTK_MINOR_VERSION 2
#define VTK_BUILD_VERSION 2
#define VTK_SOURCE_VERSION "vtk version " VTK_VERSION ", vtk source $Revision: 1.61 $, $Date: 1999-01-13 03:04:16 $ (GMT)"


class VTK_EXPORT vtkVersion : public vtkObject {
 public:
  static vtkVersion *New() {return new vtkVersion;};
  const char *GetClassName() {return "vtkVersion";};

  // Description: 
  // Return the version of vtk this object is a part of.
  // A variety of methods are included. GetVTKSourceVersion returns a string
  // with an identifier which timestamps a particular source tree. 
  static const char *GetVTKVersion() { return VTK_VERSION; };
  static int GetVTKMajorVersion() { return VTK_MAJOR_VERSION; };
  static int GetVTKMinorVersion() { return VTK_MINOR_VERSION; };
  static int GetVTKBuildVersion() { return VTK_BUILD_VERSION; };
  static const char *GetVTKSourceVersion() { return VTK_SOURCE_VERSION; };
  
 protected:

};

#endif 
