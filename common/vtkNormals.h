/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNormals.h
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
// .NAME vtkNormals - abstract interface to 3D normals
// .SECTION Description
// vtkNormals provides an abstract interface to 3D normals. The data model
// for vtkNormals is an array of nx-ny-nz triplets accessible by point id.
// (Each normal is assumed normalized |n| = 1.) The subclasses of 
// vtkNormals are concrete data types (float, int, etc.) that implement 
// the interface of vtkNormals. 

#ifndef __vtkNormals_h
#define __vtkNormals_h

#include "vtkReferenceCount.h"

class vtkIdList;
class vtkFloatNormals;

class VTK_EXPORT vtkNormals : public vtkReferenceCount 
{
public:
  vtkNormals() {};
  virtual ~vtkNormals() {};
  char *GetClassName() {return "vtkNormals";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a copy of this object.
  virtual vtkNormals *MakeObject(int sze, int ext=1000) = 0;

  // Description:
  // Return data type. One of "bit", "unsigned char", "short", "int", "float", or
  // "double".
  virtual char *GetDataType() = 0;

  // Description:
  // Return number of normals in array.
  virtual int GetNumberOfNormals() = 0;

  // Description:
  // Return a float normal n[3] for a particular point id.
  virtual float *GetNormal(int id) = 0;

  // Description:
  // Copy normal components into user provided array n[3] for specified
  // point id.
  virtual void GetNormal(int id, float n[3]);

  // Description:
  // Specify the number of normals for this object to hold. Does an
  // allocation as well as setting the MaxId ivar. Used in conjunction with
  // SetNormal() method for fast insertion.
  virtual void SetNumberOfNormals(int number) = 0;

  // Description:
  // Insert normal into object. No range checking performed (fast!).
  // Make sure you use SetNumberOfNormals() to allocate memory prior
  // to using SetNormal().
  virtual void SetNormal(int id, float n[3]) = 0;

  // Description:
  // Insert normal into object. Range checking performed and memory
  // allocated as necessary.
  virtual void InsertNormal(int id, float n[3]) = 0;
  void InsertNormal(int id, float nx, float ny, float nz);

  // Description:
  // Insert normal into next available slot. Returns point id of slot.
  virtual int InsertNextNormal(float n[3]) = 0;
  int InsertNextNormal(float nx, float ny, float nz);

  // Description:
  // Reclaim any extra memory.
  virtual void Squeeze() = 0;

  void GetNormals(vtkIdList& ptId, vtkFloatNormals& fp);
};

// These include files are placed here so that if Normals.h is included 
// all other classes necessary for compilation are also included. 
#include "vtkIdList.h"
#include "vtkFloatNormals.h"

#endif
