/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPoints.h
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
// .NAME vtkPoints - abstract interface to 3D points
// .SECTION Description
// vtkPoints provides an abstract interface to 3D points. The data model
// for vtkPoints is an array of x-y-z triplets accessible by point id.
// The subclasses of vtkPoints are concrete data types (float, int, etc.) 
// that implement the interface of vtkPoints. 

// .SECTION See Also
// vtkFloatPoints vtkIntPoints

#ifndef __vtkPoints_h
#define __vtkPoints_h

#include "vtkReferenceCount.h"

class vtkFloatPoints;
class vtkIdList;

class VTK_EXPORT vtkPoints : public vtkReferenceCount 
{
public:
  vtkPoints();
  const char *GetClassName() {return "vtkPoints";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a copy of this object.
  virtual vtkPoints *MakeObject(int sze, int ext=1000) = 0;

  // Description:
  // Return data type. One of "bit", "char", "short", "int", "float", or
  // "double".
  virtual char *GetDataType() = 0;

  // Description:
  // Return number of points in list.
  virtual int GetNumberOfPoints() = 0;

  // Description:
  // Return a pointer to a float array x[3] for a specified point id.
  virtual float *GetPoint(int id) = 0;

  // Description:
  // Copy point coordinates into user provided array x[3] for specified
  // point id.
  virtual void GetPoint(int id, float x[3]);

  // Description:
  // Specify the number of points for this object to hold. Does an
  // allocation as well as setting the MaxId ivar. Used in conjunction with
  // SetPoint() method for fast insertion.
  virtual void SetNumberOfPoints(int number) = 0;

  // Description:
  // Insert point into object. No range checking performed (fast!).
  // Make sure you use SetNumberOfPoints() to allocate memory prior
  // to using SetPoint().
  virtual void SetPoint(int id, float x[3]) = 0;

  // Description:
  // Insert point into object. Range checking performed and memory
  // allocated as necessary.
  virtual void InsertPoint(int id, float x[3]) = 0;
  void InsertPoint(int id, float x, float y, float z);

  // Description:
  // Insert point into next available slot. Returns point id of slot.
  virtual int InsertNextPoint(float x[3]) = 0;
  int InsertNextPoint(float x, float y, float z);

  // Description:
  // Reclaim any extra memory.
  virtual void Squeeze() = 0; // reclaim memory

  // Description:
  // Get the point coordinates for the point ids specified.
  virtual void GetPoints(vtkIdList& ptId, vtkFloatPoints& fp);

  virtual void ComputeBounds();
  float *GetBounds();
  void GetBounds(float bounds[6]);

protected:
  float Bounds[6];
  vtkTimeStamp ComputeTime; // Time at which bounds computed

};

// These include files are placed here so that if Points.h is included 
// all other classes necessary for compilation are also included. 
#include "vtkIdList.h"
#include "vtkFloatPoints.h"

#endif
