/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyMapperDevice.hh
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
// .NAME vtkPolyMapperDevice - abstract interface for geometric data
// .SECTION Description
// vtkPolyMapperDevice is an abstract specification for objects that 
// interface to the polygonal based rendering libraries. Subclasses of
// vtkPolyMapperDevice interface indirectly to a renderer during its two
// pass rendering process. In the first pass (Build()), the 
// vtkPolyMapperDevice object is asked to build its data from its input 
// polygonal data. In the next pass (Draw()), the object is asked to load
// its data into the graphics pipeline. Typically the user will never 
// encounter this object or its subclasses. It is used to interface
// the Mappers to the underlying graphics library. 

// .SECTION see also
// vtkPolyMapper

#ifndef __vtkPolyMapperDevice_hh
#define __vtkPolyMapperDevice_hh

#include "vtkObject.hh"

class vtkPolyData;
class vtkColorScalars;
class vtkRenderer;
class vtkActor;

class vtkPolyMapperDevice : public vtkObject
{
 public:
  vtkPolyMapperDevice();
  char *GetClassName() {return "vtkPolyMapperDevice";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Build appropriate graphical data representation for the
  // particular library.
  virtual void Build(vtkPolyData *data, vtkColorScalars *c) = 0;

  // Description:
  // Load data into a specific graphics library.
  virtual void Draw(vtkRenderer *ren, vtkActor *a) = 0;

 protected:
  vtkPolyData *Data;
  vtkColorScalars *Colors;
};

#endif
