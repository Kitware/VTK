/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridTestRayIntegrator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUnstructuredGridTestRayIntegrator.h"

#include "vtkObjectFactory.h"
#include "vtkDoubleArray.h"
#include "vtkVolumeProperty.h"
#include "vtkColorTransferFunction.h"
#include "vtkPiecewiseFunction.h"

vtkCxxRevisionMacro(vtkUnstructuredGridTestRayIntegrator, "1.2");
vtkStandardNewMacro(vtkUnstructuredGridTestRayIntegrator);

template<class T>
static void TemplateIntegrateColor(
                        vtkIdType numIntersections,
                        int numComponents,
                        const double *intersectionLengths,
                        const T *nearIntersections,
                        const T *farIntersections,
                        vtkUnstructuredGridTestRayIntegrator *self,
                        float color[4]);

//----------------------------------------------------------------------------

vtkUnstructuredGridTestRayIntegrator::vtkUnstructuredGridTestRayIntegrator()
{
  this->ColorTable                   = NULL;
  this->ColorTableSize               = NULL;
  this->ColorTableShift              = NULL;
  this->ColorTableScale              = NULL;
  
  this->SavedRGBFunction             = NULL;
  this->SavedGrayFunction            = NULL;
  this->SavedScalarOpacityFunction   = NULL;
  this->SavedColorChannels           = NULL;
  this->SavedScalarOpacityDistance   = NULL;
  this->SavedSampleDistance          = 0.0;
  this->SavedNumberOfComponents      = 0;
  this->SavedParametersScalars       = NULL;
}

vtkUnstructuredGridTestRayIntegrator::~vtkUnstructuredGridTestRayIntegrator()
{
  this->SetNumberOfComponents(0);
}

void vtkUnstructuredGridTestRayIntegrator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-----------------------------------------------------------------------------

void vtkUnstructuredGridTestRayIntegrator::SetNumberOfComponents(int num)
{
  if ( num == this->SavedNumberOfComponents )
    {
    return;
    }
  
  int i;
  for ( i = 0; i < this->SavedNumberOfComponents; i++ )
    {
    delete [] this->ColorTable[i];
    }
  
  delete [] this->ColorTable;
  delete [] this->ColorTableSize;
  delete [] this->ColorTableShift;
  delete [] this->ColorTableScale;
  
  delete [] this->SavedRGBFunction;
  delete [] this->SavedGrayFunction;
  delete [] this->SavedScalarOpacityFunction;
  delete [] this->SavedColorChannels;
  delete [] this->SavedScalarOpacityDistance;
  
  this->ColorTable                   = NULL;
  this->ColorTableSize               = NULL;
  this->ColorTableShift              = NULL;
  this->ColorTableScale              = NULL;
  
  this->SavedRGBFunction             = NULL;
  this->SavedGrayFunction            = NULL;
  this->SavedScalarOpacityFunction   = NULL;
  this->SavedColorChannels           = NULL;
  this->SavedScalarOpacityDistance   = NULL;
  this->SavedParametersScalars       = NULL;
   
  this->SavedNumberOfComponents      = num;
  
  if ( num > 0 )
    {
    this->ColorTable      = new double *[num];
    this->ColorTableSize  = new    int  [num];
    this->ColorTableShift = new double  [num];
    this->ColorTableScale = new double  [num];
    
    this->SavedRGBFunction             = new vtkColorTransferFunction *[num];
    this->SavedGrayFunction            = new     vtkPiecewiseFunction *[num];
    this->SavedScalarOpacityFunction   = new     vtkPiecewiseFunction *[num];

    this->SavedColorChannels         = new    int [num];
    this->SavedScalarOpacityDistance = new double [num];
    
    for ( i = 0; i < num; i++ )
      {
      this->ColorTable[i]      = NULL;
      this->ColorTableSize[i]  = 0;
      this->ColorTableShift[i] = 0.0;
      this->ColorTableScale[i] = 1.0;
      
      this->SavedRGBFunction[i]            = NULL;
      this->SavedGrayFunction[i]           = NULL;
      this->SavedScalarOpacityFunction[i]  = NULL;
      this->SavedColorChannels[i]          = 0;
      this->SavedScalarOpacityDistance[i]  = 0.0;
      }
    }
}

//-----------------------------------------------------------------------------


// Update the color table to store the mapping from scalar value to
// color/opacity.  Although the volume property supports the notion of
// non-independent components, this mapper only supports independent
// components (where each component specified an independent property, not
// a single property such as a 3 component dataset representing color)
void vtkUnstructuredGridTestRayIntegrator::UpdateColorTable(
                                                    vtkVolumeProperty *property,
                                                    vtkDataArray *scalars)
{
  int needToUpdate = 0;

  // Set the number of components. If this is different than previous, it will
  // reset all the arrays to the right size (filled with null).
  int components = scalars->GetNumberOfComponents();
 this->SetNumberOfComponents(components);
  
  // Has the data itself changed?
  if ( scalars != this->SavedParametersScalars ||
       scalars->GetMTime() > this->SavedParametersMTime.GetMTime() )
    {
    needToUpdate = 1;
    }
 
  vtkColorTransferFunction **rgbFunc               = new vtkColorTransferFunction *[components];
  vtkPiecewiseFunction     **grayFunc              = new     vtkPiecewiseFunction *[components];
  vtkPiecewiseFunction     **scalarOpacityFunc     = new     vtkPiecewiseFunction *[components];
  int                       *colorChannels         = new                      int  [components];
  float                     *scalarOpacityDistance = new                    float  [components];
  
  int c;
     
 for ( c = 0; c < components; c++ )
    {
    rgbFunc[c]               = property->GetRGBTransferFunction(c);
    grayFunc[c]              = property->GetGrayTransferFunction(c);
    scalarOpacityFunc[c]     = property->GetScalarOpacity(c);
    colorChannels[c]         = property->GetColorChannels(c);
    scalarOpacityDistance[c] = property->GetScalarOpacityUnitDistance(c);
    
    // Has the number of color channels changed?
    if ( this->SavedColorChannels[c] != colorChannels[c] )
      {
      needToUpdate = 1;
      }

    // Has the color transfer function changed in some way,
    // and we are using it?
    if ( colorChannels[c] == 3 )
      {
      if ( this->SavedRGBFunction[c] != rgbFunc[c] ||
           this->SavedParametersMTime.GetMTime() < rgbFunc[c]->GetMTime() )
        {
        needToUpdate = 1;
        }
      }

    // Has the gray transfer function changed in some way,
    // and we are using it?
    if ( colorChannels[c] == 1 )
      {
      if ( this->SavedGrayFunction[c] != grayFunc[c] ||
           this->SavedParametersMTime.GetMTime() < grayFunc[c]->GetMTime() )
        {
        needToUpdate = 1;
        }
      }
  
    // Has the scalar opacity transfer function changed in some way?
    if ( this->SavedScalarOpacityFunction[c] != scalarOpacityFunc[c] ||
         this->SavedParametersMTime.GetMTime() < scalarOpacityFunc[c]->GetMTime() )
      {
      needToUpdate = 1;
      }

    // Has the distance over which the scalar opacity function is defined changed?
    if ( this->SavedScalarOpacityDistance[c] != scalarOpacityDistance[c] )
      {
      needToUpdate = 1;
      }
    }
  
  // If we have not found any need to update, free memory and return now
  if ( !needToUpdate )
    {
    delete [] rgbFunc;
    delete [] grayFunc;
    delete [] scalarOpacityFunc;
    delete [] colorChannels;
    delete [] scalarOpacityDistance;
    return;
    }

  for ( c = 0; c < components; c++ )
    {
    this->SavedRGBFunction[c]             = rgbFunc[c];
    this->SavedGrayFunction[c]            = grayFunc[c];
    this->SavedScalarOpacityFunction[c]   = scalarOpacityFunc[c];
    this->SavedColorChannels[c]           = colorChannels[c];
    this->SavedScalarOpacityDistance[c]   = scalarOpacityDistance[c];
    }
  
  this->SavedParametersScalars       = scalars;
  
  this->SavedParametersMTime.Modified();

  int scalarType = scalars->GetDataType();
  
  int i;
  float *tmpArray = new float[3*65536];
  
  // Find the scalar range
  double *scalarRange = new double [2*components];
  for ( c = 0; c < components; c++ )
    {
    scalars->GetRange((scalarRange+2*c), c);
    
    // Is the difference between max and min less than 65536? If so, and if
    // the data is not of float or double type, use a simple offset mapping.
    // If the difference between max and min is 65536 or greater, or the data
    // is of type float or double, we must use an offset / scaling mapping.
    // In this case, the array size will be 65536 - we need to figure out the 
    // offset and scale factor.
    float offset;
    float scale;
    
    int arraySizeNeeded;
    
    if ( scalarType == VTK_FLOAT ||
         scalarType == VTK_DOUBLE ||
         scalarRange[c*2+1] - scalarRange[c*2] > 65535 )
      {
      arraySizeNeeded = 65536;
      offset          = -scalarRange[2*c];
      scale           = 65535.0 / (scalarRange[2*c+1] - scalarRange[2*c]);
      }
    else
      {
      arraySizeNeeded = static_cast<int>(scalarRange[2*c+1] - scalarRange[2*c]) + 1;
      offset          = -scalarRange[2*c]; 
      scale           = 1.0;
      }
    
    if ( this->ColorTableSize[c] != arraySizeNeeded )
      {
      delete [] this->ColorTable[c];
      this->ColorTable[c] = new double [4*arraySizeNeeded];
      }
    
    this->ColorTableSize[c]   = arraySizeNeeded;
    this->ColorTableShift[c]  = offset;
    this->ColorTableScale[c]  = scale;  
    }
  
  for ( c = 0; c < components; c++ )
    {
    // Sample the transfer functions between the min and max.
    if ( colorChannels[c] == 1 )
      {
      float *tmpArray2 = new float[65536];
      grayFunc[c]->GetTable( scalarRange[c*2], scalarRange[c*2+1], 
                             this->ColorTableSize[c], tmpArray2 );
      for ( int index = 0; index < this->ColorTableSize[c]; index++ )
        {
        tmpArray[3*index  ] = tmpArray2[index];
        tmpArray[3*index+1] = tmpArray2[index];
        tmpArray[3*index+2] = tmpArray2[index];
        }
      delete [] tmpArray2;
      }
    else
      {
      rgbFunc[c]->GetTable( scalarRange[c*2], scalarRange[c*2+1], 
                            this->ColorTableSize[c], tmpArray );
      }
    // Add color to the color table in double format
    for ( i = 0; i < this->ColorTableSize[c]; i++ )
      {
      this->ColorTable[c][4*i  ] = (double)(tmpArray[3*i  ]);
      this->ColorTable[c][4*i+1] = (double)(tmpArray[3*i+1]);
      this->ColorTable[c][4*i+2] = (double)(tmpArray[3*i+2]);
      }

    // Get the scalar opacity table.
    // No need to correct the opacity here since we do not have a uniform spacing -
    // we'll need to correct it as we sample along the ray (slow, but necessary)
    scalarOpacityFunc[c]->GetTable( scalarRange[c*2], scalarRange[c*2+1], 
                                    this->ColorTableSize[c], tmpArray );
    

    // Add opacity to color table in double format
    // Multiply by component weight
    double weight =  property->GetComponentWeight(c);
    for ( i = 0; i < this->ColorTableSize[c]; i++ )
      {
      this->ColorTable[c][4*i+3] = (double)(tmpArray[i]) * weight;
      }    
    }
  
  // Clean up the temporary arrays we created
  delete [] tmpArray;
  delete [] rgbFunc;
  delete [] grayFunc;
  delete [] scalarOpacityFunc;
  delete [] colorChannels;
  delete [] scalarOpacityDistance;
  delete [] scalarRange;
}

//-----------------------------------------------------------------------------

void vtkUnstructuredGridTestRayIntegrator::Initialize(
                                                  vtkVolumeProperty *property,
                                                  vtkDataArray *scalars)
{
  this->UpdateColorTable(property, scalars);

  this->ScalarOpacityUnitDistance = property->GetScalarOpacityUnitDistance();
}

//-----------------------------------------------------------------------------

template<class T>
static void TemplateIntegrateColor(
                        vtkIdType numIntersections,
                        int numComponents,
                        const double *intersectionLengths,
                        const T *nearIntersections,
                        const T *farIntersections,
                        vtkUnstructuredGridTestRayIntegrator *self,
                        float color[4])
{
  double **colorTable      = self->GetColorTable();
  double  *colorTableShift = self->GetColorTableShift();
  double  *colorTableScale = self->GetColorTableScale();

  double inverseUnitDistance = 1.0 / self->GetScalarOpacityUnitDistance();

  for (vtkIdType i = 0; i < numIntersections; i++)
    {
    float newColor1[4] = {0.0,0.0,0.0,0.0};
    float newColor2[4] = {0.0,0.0,0.0,0.0};

    float remainingOpacity = 1.0 - color[3];
    double factor = (intersectionLengths[i]/2.0) * inverseUnitDistance;

    int c;
    for (c = 0; c < numComponents; c++)
      {
      double v = nearIntersections[i*numComponents + c];
      
      double *tmpptr = 
        colorTable[c] +
        4 * (unsigned short)((v+colorTableShift[c])*colorTableScale[c]);

      float opacity =  1.0-pow((double)(1.0-(*(tmpptr+3))),factor);

      float w =  remainingOpacity * opacity;
      newColor1[0] += w * *(tmpptr);
      newColor1[1] += w * *(tmpptr+1);
      newColor1[2] += w * *(tmpptr+2);
      newColor1[3] += opacity;
      }

    remainingOpacity *= (1.0 - newColor1[3]);

    for (c = 0; c < numComponents; c++)
      {
      double v = farIntersections[i*numComponents + c];
      
      double *tmpptr = 
        colorTable[c] +
        4 * (unsigned short)((v+colorTableShift[c])*colorTableScale[c]);

      float opacity =  1.0-pow((double)(1.0-(*(tmpptr+3))),factor);
      float w =  remainingOpacity * opacity;
      newColor2[0] += w * *(tmpptr);
      newColor2[1] += w * *(tmpptr+1);
      newColor2[2] += w * *(tmpptr+2);
      newColor2[3] += opacity;
    }

    color[0] += newColor1[0] + newColor2[0];
    color[1] += newColor1[1] + newColor2[1];
    color[2] += newColor1[2] + newColor2[2];
    color[3] = 1.0 - remainingOpacity * (1.0 - newColor2[3]);
    }
}

//-----------------------------------------------------------------------------

void vtkUnstructuredGridTestRayIntegrator::Integrate(
                                            vtkDoubleArray *intersectionLengths,
                                            vtkDataArray *nearIntersections,
                                            vtkDataArray *farIntersections,
                                            float color[4])
{
  int type = nearIntersections->GetDataType();

  if (farIntersections->GetDataType() != type)
    {
    vtkErrorMacro("Near and far intersection types do not match.");
    return;
    }

  switch (type)
    {
    vtkTemplateMacro(TemplateIntegrateColor
                     (intersectionLengths->GetNumberOfTuples(),
                      nearIntersections->GetNumberOfComponents(),
                      intersectionLengths->GetPointer(0),
                      (const VTK_TT *)nearIntersections->GetVoidPointer(0),
                      (const VTK_TT *)farIntersections->GetVoidPointer(0),
                      this, color));
    }
}
