/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageGridSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageGridSource
 * @brief   Create an image of a grid.
 *
 * vtkImageGridSource produces an image of a grid.  The
 * default output type is double.
*/

#ifndef vtkImageGridSource_h
#define vtkImageGridSource_h

#include "vtkImagingSourcesModule.h" // For export macro
#include "vtkImageAlgorithm.h"

class VTKIMAGINGSOURCES_EXPORT vtkImageGridSource : public vtkImageAlgorithm
{
public:
  static vtkImageGridSource *New();
  vtkTypeMacro(vtkImageGridSource,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Set/Get the grid spacing in pixel units.  Default (10,10,0).
   * A value of zero means no grid.
   */
  vtkSetVector3Macro(GridSpacing,int);
  vtkGetVector3Macro(GridSpacing,int);
  //@}

  //@{
  /**
   * Set/Get the grid origin, in ijk integer values.  Default (0,0,0).
   */
  vtkSetVector3Macro(GridOrigin,int);
  vtkGetVector3Macro(GridOrigin,int);
  //@}

  //@{
  /**
   * Set the grey level of the lines. Default 1.0.
   */
  vtkSetMacro(LineValue,double);
  vtkGetMacro(LineValue,double);
  //@}

  //@{
  /**
   * Set the grey level of the fill. Default 0.0.
   */
  vtkSetMacro(FillValue,double);
  vtkGetMacro(FillValue,double);
  //@}

  //@{
  /**
   * Set/Get the data type of pixels in the imported data.
   * As a convenience, the OutputScalarType is set to the same value.
   */
  vtkSetMacro(DataScalarType,int);
  void SetDataScalarTypeToDouble(){this->SetDataScalarType(VTK_DOUBLE);}
  void SetDataScalarTypeToInt(){this->SetDataScalarType(VTK_INT);}
  void SetDataScalarTypeToShort(){this->SetDataScalarType(VTK_SHORT);}
  void SetDataScalarTypeToUnsignedShort()
    {this->SetDataScalarType(VTK_UNSIGNED_SHORT);}
  void SetDataScalarTypeToUnsignedChar()
    {this->SetDataScalarType(VTK_UNSIGNED_CHAR);}
  vtkGetMacro(DataScalarType, int);
  const char *GetDataScalarTypeAsString() {
    return vtkImageScalarTypeNameMacro(this->DataScalarType); }
  //@}

  //@{
  /**
   * Set/Get the extent of the whole output image,
   * Default: (0,255,0,255,0,0)
   */
  vtkSetVector6Macro(DataExtent,int);
  vtkGetVector6Macro(DataExtent,int);
  //@}

  //@{
  /**
   * Set/Get the pixel spacing.
   */
  vtkSetVector3Macro(DataSpacing,double);
  vtkGetVector3Macro(DataSpacing,double);
  //@}

  //@{
  /**
   * Set/Get the origin of the data.
   */
  vtkSetVector3Macro(DataOrigin,double);
  vtkGetVector3Macro(DataOrigin,double);
  //@}

protected:
  vtkImageGridSource();
  ~vtkImageGridSource() {}

  int GridSpacing[3];
  int GridOrigin[3];

  double LineValue;
  double FillValue;

  int DataScalarType;

  int DataExtent[6];
  double DataSpacing[3];
  double DataOrigin[3];

  virtual int RequestInformation (vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);
  virtual void ExecuteDataWithInformation(vtkDataObject *data, vtkInformation* outInfo);

private:
  vtkImageGridSource(const vtkImageGridSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageGridSource&) VTK_DELETE_FUNCTION;
};


#endif
