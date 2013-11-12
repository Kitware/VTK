/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractVolumeMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAbstractVolumeMapper - Abstract class for a volume mapper

// .SECTION Description
// vtkAbstractVolumeMapper is the abstract definition of a volume mapper.
// Specific subclasses deal with different specific types of data input

// .SECTION see also
// vtkVolumeMapper vtkUnstructuredGridVolumeMapper

#ifndef __vtkAbstractVolumeMapper_h
#define __vtkAbstractVolumeMapper_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkAbstractMapper3D.h"

class vtkRenderer;
class vtkVolume;
class vtkWindow;
class vtkDataSet;

class VTKRENDERINGCORE_EXPORT vtkAbstractVolumeMapper : public vtkAbstractMapper3D
{
public:
  vtkTypeMacro(vtkAbstractVolumeMapper,vtkAbstractMapper3D);
  void PrintSelf( ostream& os, vtkIndent indent );

  // Description:
  // Set/Get the input data
  vtkDataSet *GetDataSetInput();
  vtkDataObject *GetDataObjectInput();

  // Description:
  // Return bounding box (array of six doubles) of data expressed as
  // (xmin,xmax, ymin,ymax, zmin,zmax).
  virtual double *GetBounds();
  virtual void GetBounds(double bounds[6])
    { this->vtkAbstractMapper3D::GetBounds(bounds); };

  // Description:
  // Control how the mapper works with scalar point data and cell attribute
  // data.  By default (ScalarModeToDefault), the mapper will use point data,
  // and if no point data is available, then cell data is used. Alternatively
  // you can explicitly set the mapper to use point data
  // (ScalarModeToUsePointData) or cell data (ScalarModeToUseCellData).
  // You can also choose to get the scalars from an array in point field
  // data (ScalarModeToUsePointFieldData) or cell field data
  // (ScalarModeToUseCellFieldData).  If scalars are coming from a field
  // data array, you must call SelectScalarArray.
  vtkSetMacro(ScalarMode,int);
  vtkGetMacro(ScalarMode,int);
  void SetScalarModeToDefault() {
    this->SetScalarMode(VTK_SCALAR_MODE_DEFAULT);};
  void SetScalarModeToUsePointData() {
    this->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_DATA);};
  void SetScalarModeToUseCellData() {
    this->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_DATA);};
  void SetScalarModeToUsePointFieldData() {
    this->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);};
  void SetScalarModeToUseCellFieldData() {
    this->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);};

  // Description:
  // When ScalarMode is set to UsePointFieldData or UseCellFieldData,
  // you can specify which scalar array to use during rendering.
  // The transfer function in the vtkVolumeProperty (attached to the calling
  // vtkVolume) will decide how to convert vectors to colors.
  virtual void SelectScalarArray(int arrayNum);
  virtual void SelectScalarArray(const char* arrayName);

  // Description:
  // Get the array name or number and component to use for rendering.
  virtual char* GetArrayName() { return this->ArrayName; }
  virtual int GetArrayId() { return this->ArrayId; }
  virtual int GetArrayAccessMode() { return this->ArrayAccessMode; }

  // Description:
  // Return the method for obtaining scalar data.
  const char *GetScalarModeAsString();

//BTX
  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  virtual float GetGradientMagnitudeScale() {return 1.0f;};
  virtual float GetGradientMagnitudeBias()  {return 0.0f;};
  virtual float GetGradientMagnitudeScale(int) {return 1.0f;};
  virtual float GetGradientMagnitudeBias(int)  {return 0.0f;};


  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Render the volume
  virtual void Render(vtkRenderer *ren, vtkVolume *vol)=0;

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *) {}

//ETX

protected:
  vtkAbstractVolumeMapper();
  ~vtkAbstractVolumeMapper();

  // see algorithm for more info
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  int         ScalarMode;
  char       *ArrayName;
  int         ArrayId;
  int         ArrayAccessMode;

private:
  vtkAbstractVolumeMapper(const vtkAbstractVolumeMapper&);  // Not implemented.
  void operator=(const vtkAbstractVolumeMapper&);  // Not implemented.
};


#endif


