/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkColorScalars.h
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
// .NAME vtkColorScalars - abstract class represents scalar data in color specification
// .SECTION Description
// vtkColorScalars is an abstract class whose subclasses represent scalar
// data using a color specification such as rgb, grayscale, rgba, hsv, etc. 
//
// In order to be a vtkScalars subclass, vtkColorScalars must be able to 
// return a single value given a point id. By default, this operation is 
// performed by computing luminance (or equivalent) as the single value. 
// Concrete subclasses of vtkColorScalars may have additional methods to 
// convert multi-dimensional color information into a single scalar value.
// .SECTION Caveats
// Derived classes of vtkColorScalars treat colors differently. All derived 
// classes will return a rgba (red-green-blue-alpha transparency) array in
// response to "GetColor()" methods. However, when setting colors, the rgba
// data is converted to internal form. For example, a vtkAGraymap converts
// rgba into a luminance value and stores that.
// .SECTION See Also
// vtkAGraymap vtkAPixmap vtkBitmap vtkGraymap vtkPixmap 

#ifndef __vtkColorScalars_h
#define __vtkColorScalars_h

#include "vtkScalars.h"

class vtkAPixmap;

class VTK_EXPORT vtkColorScalars : public vtkScalars 
{
public:
  vtkColorScalars() {};
  char *GetClassName() {return "vtkColorScalars";};

  // vtkScalars interface
  char *GetScalarType() {return "ColorScalar";};
  char *GetDataType() {return "unsigned char";};
  float GetScalar(int i);
  void SetNumberOfScalars(int number) {this->SetNumberOfColors(number);};
  void SetScalar(int i, float s);
  void InsertScalar(int i, float s);
  int InsertNextScalar(float s);

  // Description:
  // Get pointer to array of data starting at data position "id".
  virtual unsigned char *GetPtr(const int id) = 0;

  // abstract interface for vtkColorScalars
  // Description:
  // Return number of colors (same as number of scalars).
  int GetNumberOfColors() {return this->GetNumberOfScalars();};  

  // Description:
  // Return an unsigned char rgba for a particular point id. No matter
  // what internal representation of color, derived class must convert it
  // to rgba.
  virtual unsigned char *GetColor(int id) = 0;

  // Description:
  // Copy color components into user provided array rgba[4] for specified
  // point id. No matter what internal representation of color, derived 
  // class must convert it to rgba form.
  virtual void GetColor(int id, unsigned char rgba[4]) = 0;

  // Description:
  // Specify the number of colors for this object to hold. Does an
  // allocation as well as setting the MaxId ivar. Used in conjunction with
  // SetColor() method for fast insertion.
  virtual void SetNumberOfColors(int number) = 0;

  // Description:
  // Insert color into object. No range checking performed (fast!).
  // Make sure you use SetNumberOfColors() to allocate memory prior
  // to using SetColor().
  virtual void SetColor(int id, unsigned char rgba[4]) = 0;

  // Description:
  // Insert color into object. Range checking performed and memory
  // allocated as necessary.
  virtual void InsertColor(int id, unsigned char rgba[4]) = 0;
  void InsertColor(int id, float R, float G, float B, float A);

  // Description:
  // Insert color into next available slot. Returns point id of slot.
  virtual int InsertNextColor(unsigned char rgba[4]) = 0;
  int InsertNextColor(float R, float G, float B, float A);

  // Description:
  // Allocate space for color data.
  virtual int Allocate(const int sz, const int ext=1000) = 0;

  void GetColors(vtkIdList& ptId, vtkAPixmap& ap);

  void GetComponentRange(unsigned char range[8]);
  unsigned char *GetComponentRange();
};

// These include files are placed here so that if CoScalar.h is included 
// all other classes necessary for compilation are also included. 
#include "vtkIdList.h"
#include "vtkAPixmap.h"

#endif
