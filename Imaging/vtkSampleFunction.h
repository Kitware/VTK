/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSampleFunction.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSampleFunction - sample an implicit function over a structured point set
// .SECTION Description
// vtkSampleFunction is a source object that evaluates an implicit function
// and normals at each point in a vtkStructuredPoints. The user can specify
// the sample dimensions and location in space to perform the sampling. To
// create closed surfaces (in conjunction with the vtkContourFilter), capping
// can be turned on to set a particular value on the boundaries of the sample
// space.

// .SECTION See Also
// vtkImplicitModeller

#ifndef __vtkSampleFunction_h
#define __vtkSampleFunction_h

#include "vtkImageSource.h"
#include "vtkImplicitFunction.h"

class VTK_IMAGING_EXPORT vtkSampleFunction : public vtkImageSource
{
public:
  vtkTypeRevisionMacro(vtkSampleFunction,vtkImageSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with ModelBounds=(-1,1,-1,1,-1,1), SampleDimensions=(50,50,50),
  // Capping turned off, and normal generation on.
  static vtkSampleFunction *New();

  // Description:
  // Specify the implicit function to use to generate data.
  vtkSetObjectMacro(ImplicitFunction,vtkImplicitFunction);
  vtkGetObjectMacro(ImplicitFunction,vtkImplicitFunction);

  // Description:
  // Set what type of scalar data this source should generate.
  vtkSetMacro(OutputScalarType,int);
  vtkGetMacro(OutputScalarType,int);
  void SetOutputScalarTypeToDouble()
    {this->SetOutputScalarType(VTK_DOUBLE);}
  void SetOutputScalarTypeToFloat()
    {this->SetOutputScalarType(VTK_FLOAT);}
  void SetOutputScalarTypeToLong()
    {this->SetOutputScalarType(VTK_LONG);}
  void SetOutputScalarTypeToUnsignedLong()
    {this->SetOutputScalarType(VTK_UNSIGNED_LONG);};
  void SetOutputScalarTypeToInt()
    {this->SetOutputScalarType(VTK_INT);}
  void SetOutputScalarTypeToUnsignedInt()
    {this->SetOutputScalarType(VTK_UNSIGNED_INT);}
  void SetOutputScalarTypeToShort()
    {this->SetOutputScalarType(VTK_SHORT);}
  void SetOutputScalarTypeToUnsignedShort()
    {this->SetOutputScalarType(VTK_UNSIGNED_SHORT);}
  void SetOutputScalarTypeToChar()
    {this->SetOutputScalarType(VTK_CHAR);}
  void SetOutputScalarTypeToUnsignedChar()
    {this->SetOutputScalarType(VTK_UNSIGNED_CHAR);}

  // Description:
  // Control the type of the scalars object by explicitly providing a scalar
  // object.  THIS IS DEPRECATED, although it still works!!! Please use
  // SetOutputScalarType instead.
  virtual void SetScalars(vtkDataArray *da)
    {
      if (da)
        {
        this->SetOutputScalarType(da->GetDataType());
        }
    }    

  // Description:
  // Specify the dimensions of the data on which to sample.
  void SetSampleDimensions(int i, int j, int k);

  // Description:
  // Specify the dimensions of the data on which to sample.
  void SetSampleDimensions(int dim[3]);
  vtkGetVectorMacro(SampleDimensions,int,3);

  // Description:
  // Specify the region in space over which the sampling occurs.
  vtkSetVector6Macro(ModelBounds,float);
  vtkGetVectorMacro(ModelBounds,float,6);

  // Description:
  // Turn on/off capping. If capping is on, then the outer boundaries of the
  // structured point set are set to cap value. This can be used to insure
  // surfaces are closed.
  vtkSetMacro(Capping,int);
  vtkGetMacro(Capping,int);
  vtkBooleanMacro(Capping,int);
  
  // Description:
  // Set the cap value.
  vtkSetMacro(CapValue,float);
  vtkGetMacro(CapValue,float);

  // Description:
  // Turn on/off the computation of normals.
  vtkSetMacro(ComputeNormals,int);
  vtkGetMacro(ComputeNormals,int);
  vtkBooleanMacro(ComputeNormals,int);

  // Description:
  // Return the MTime also considering the implicit function.
  unsigned long GetMTime();

protected:
  vtkSampleFunction();
  ~vtkSampleFunction();

  void ExecuteData(vtkDataObject *);
  void ExecuteInformation();
  void Cap(vtkDataArray *s);

  int OutputScalarType;
  int SampleDimensions[3];
  float ModelBounds[6];
  int Capping;
  float CapValue;
  vtkImplicitFunction *ImplicitFunction;
  int ComputeNormals;
private:
  vtkSampleFunction(const vtkSampleFunction&);  // Not implemented.
  void operator=(const vtkSampleFunction&);  // Not implemented.
};

#endif


