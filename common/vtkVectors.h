/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectors.h
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
// .NAME vtkVectors - abstract interface to 3D vectors
// .SECTION Description
// vtkVectors provides an abstract interface to 3D vectors. The data model
// for vtkVectors is an array of vx-vy-vz triplets accessible by point id.
// The subclasses of vtkVectors are concrete data types (float, int, etc.)
// that implement the interface of vtkVectors.

#ifndef __vtkVectors_h
#define __vtkVectors_h

#include "vtkRefCount.hh"

class vtkIdList;
class vtkFloatVectors;

class vtkVectors : public vtkRefCount 
{
public:
  vtkVectors();
  char *GetClassName() {return "vtkVectors";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a copy of this object.
  virtual vtkVectors *MakeObject(int sze, int ext=1000) = 0;

  // Description:
  // Return data type. One of "bit", "unsigned char", "short", "int", "float", or
  // "double".
  virtual char *GetDataType() = 0;

  // Description:
  // Return number of vectors in array.
  virtual int GetNumberOfVectors() = 0;

  // Description:
  // Return a pointer to a float vector v[3] for a specific point id.
  virtual float *GetVector(int id) = 0;

  // Description:
  // Copy vector components into user provided array v[3] for specified
  // point id.
  virtual void GetVector(int id, float v[3]);

  // Description:
  // Insert vector into object. No range checking performed (fast!).
  virtual void SetVector(int id, float v[3]) = 0;

  // Description:
  // Insert vector into object. Range checking performed and memory
  // allocated as necessary.
  virtual void InsertVector(int id, float v[3]) = 0;
  void InsertVector(int id, float vx, float vy, float vz);

  // Description:
  // Insert vector into next available slot. Returns point id of slot.
  virtual int InsertNextVector(float v[3]) = 0;
  int InsertNextVector(float vx, float vy, float vz);

  // Description:
  // Reclaim any extra memory.
  virtual void Squeeze() = 0;

  void GetVectors(vtkIdList& ptId, vtkFloatVectors& fp);
  virtual void ComputeMaxNorm();
  float GetMaxNorm();

protected:
  float MaxNorm;
  vtkTimeStamp ComputeTime; // Time at which MaxNorm computed
};

// These include files are placed here so that if Vectors.hh is included 
// all other classes necessary for compilation are also included. 
#include "vtkIdList.hh"
#include "vtkFloatVectors.hh"

#endif
