/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalars.h
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
// .NAME vtkScalars - represent and manipulate scalar data
// .SECTION Description
// vtkScalars provides an interface to scalar data.  The data model for
// vtkScalars is an array accessible by point or cell id.  Scalar data is
// represented by a subclass of vtkDataArray, and may be any any type of
// data, although generic operations on scalar data is performed using
// floating point values.
//
// Scalars typically provide a single value per point. However, there are
// types of scalars that have multiple values or components. (For example,
// scalars that represent (RGB) color information, etc.) In this case,
// the ActiveComponent instance variable is used to indicate which
// component value is to be used for scalar data.
//
// Because of the close relationship between scalars and colors, scalars 
// also maintain an internal lookup table. If provided, this table is used 
// to map scalars into colors, rather than the lookup table that the vtkMapper
// objects are associated with.
// .SECTION See Also
// vtkDataArray vtkAttributeData vtkPointData vtkCellData

#ifndef __vtkScalars_h
#define __vtkScalars_h

#include "vtkAttributeData.h"

#define VTK_COLOR_MODE_DEFAULT 0
#define VTK_COLOR_MODE_MAP_SCALARS 1
#define VTK_COLOR_MODE_LUMINANCE 2

class vtkIdList;
class vtkScalars;
class vtkLookupTable;
class vtkUnsignedCharArray;

class VTK_EXPORT vtkScalars : public vtkAttributeData
{
public:
  vtkScalars(int dataType=VTK_FLOAT, int numComp=1);
  ~vtkScalars();
  static vtkScalars *New(int dataType=VTK_FLOAT, int numComp=1) 
    {return new vtkScalars(dataType,numComp);};
  const char *GetClassName() {return "vtkScalars";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // overload vtkAttributeData API
  void SetData(vtkDataArray *);
  vtkAttributeData *MakeObject();

  // generic access to scalar data
  int GetNumberOfScalars();
  float GetScalar(int id);
  void SetNumberOfScalars(int number);
  void SetScalar(int id, float s);
  void InsertScalar(int id, float s);
  int InsertNextScalar(float s);

  // Description:
  // Specify/Get the number of components in scalar data.
  void SetNumberOfComponents(int num);
  int GetNumberOfComponents() {return this->Data->GetNumberOfComponents();}

  // Description:
  // Set/Get the active scalar component. This ivar specifies which
  // value (or component) to use with multivalued scalars. Currently,
  // a scalar can have at most four components (assumed RGBA).
  vtkSetClampMacro(ActiveComponent,int,0,3);
  vtkGetMacro(ActiveComponent,int);

  // Special computational methods.
  void ComputeRange();
  float *GetRange();
  void GetRange(float range[2]);
  void GetDataTypeRange(float range[2]);
  float GetDataTypeMin();
  float GetDataTypeMax();
  
  // Description:
  // Create default lookup table. Generally used to create one when none
  // is available.
  virtual void CreateDefaultLookupTable();

  void SetLookupTable(vtkLookupTable *lut);
  vtkGetObjectMacro(LookupTable,vtkLookupTable);

  // Get a list of scalars for ids listed.
  void GetScalars(vtkIdList& ptIds, vtkScalars& fv);

  // Description:
  // Get the scalar values for the range of points ids specified 
  // (i.e., p1->p2 inclusive). You must insure that the vtkScalars has 
  // been previously allocated with enough space to hold the data.
  void GetScalars(int p1, int p2, vtkScalars& fs);
  
  // Description:
  // Initialize the traversal of the scalar data to generate colors 
  // (typically used during the rendering process). The method takes
  // an alpha opacity value and returns a flag indicating whether alpha
  // blending will occur. Also takes a lookup table used to map the 
  // scalar data. The color mode parameter controls how the scalar data 
  // is mapped to colors (see vtkMapper::ColorMode methods for a definition).
  int InitColorTraversal(float alpha, vtkLookupTable *lut, 
			 int colorMode=VTK_COLOR_MODE_DEFAULT);
  
  // Description:
  // Get the color value at a particular id. Returns a pointer to a 4-byte
  // array of rgba. Make sure you call InitColorTraversal() before
  // invoking this method.
  unsigned char *GetColor(int id) {return (this->*(CurrentColorFunction))(id);};

protected:
  float Range[8];
  vtkTimeStamp ComputeTime;
  vtkLookupTable *LookupTable;
  int ActiveComponent; //for multiple component scalars, the current component

  // following stuff is used for converting scalars to colors
  float CurrentAlpha;
  vtkLookupTable *CurrentLookupTable;
  //BTX
  unsigned char *(vtkScalars::*CurrentColorFunction)(int id);
  unsigned char *PassRGBA(int id);
  unsigned char *PassRGB(int id);
  unsigned char *PassIA(int id);
  unsigned char *PassI(int id);
  unsigned char *CompositeRGBA(int id);
  unsigned char *CompositeIA(int id);
  unsigned char *CompositeMapThroughLookupTable(int id);  
  unsigned char *MapThroughLookupTable(int id);  
  unsigned char *Luminance(int id);  
  vtkUnsignedCharArray *Colors;
  unsigned char RGBA[4];
  //ETX
};

// Description:
// Create a copy of this object.
inline vtkAttributeData *vtkScalars::MakeObject()
{
  return new vtkScalars(this->GetDataType(),this->GetNumberOfComponents());
}

// Description:
// Specify the number of components in the scalar.
inline void vtkScalars::SetNumberOfComponents(int num)
{
  num = (num < 1 ? 1 : (num > 4 ? 4 : num));
  this->Data->SetNumberOfComponents(num);
}

// Description:
// Specify the number of scalars for this object to hold. Make sure
// that you set the number of components in texture first.
inline void vtkScalars::SetNumberOfScalars(int number)
{
  this->Data->SetNumberOfTuples(number);
}

// Description:
// Return number of scalars in the array.
inline int vtkScalars::GetNumberOfScalars()
{
  return this->Data->GetNumberOfTuples();
}

// Description:
// Return the scalar value as a float for a specific id.
inline float vtkScalars::GetScalar(int id)
{
  return this->Data->GetComponent(id,this->ActiveComponent);
}

// Description:
// Insert Scalar into object. No range checking performed (fast!).
// Make sure you use SetNumberOfScalars() to allocate memory prior
// to using SetScalar().
inline void vtkScalars::SetScalar(int id, float s)
{
  this->Data->SetComponent(id,this->ActiveComponent,s);
}

// Description:
// Insert Scalar into object. Range checking performed and memory
// allocated as necessary.
inline void vtkScalars::InsertScalar(int id, float s)
{
  this->Data->InsertComponent(id,this->ActiveComponent,s);
}

// Description:
// Insert Scalar at end of array and return its location (id) in the array.
inline int vtkScalars::InsertNextScalar(float s)
{
  int id=this->Data->GetMaxId()+1;
  this->Data->InsertComponent(id,this->ActiveComponent,s);
  return this->Data->GetNumberOfTuples();
}

// These include files are placed here so that if Scalars.h is included 
// all other classes necessary for compilation are also included. 
#include "vtkIdList.h"

#endif
