/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtLabelSurface.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkQtLabelSurface - draw text labels at dataset points
// .SECTION Description
// vtkQtLabelSurface is an image algorithm that creates an image 
// containing labels given a point set, labels and the renderer using
// cairo to generate the image data.
// 
// The format with which the label is drawn is specified using a
// printf style format string. The font attributes of the text can
// be set through the vtkTextProperty associated to this mapper. 
//
// .SECTION Caveats
// Use this filter in combination with vtkSelectVisiblePoints if you want
// to label only points that are visible. If you want to label cells rather
// than points, use the filter vtkCellCenters to generate points at the
// center of the cells. Also, you can use the class vtkIdFilter to
// generate ids as scalars or field data, which can then be labeled.

// .SECTION See Also
// vtkMapper2D vtkActor2D vtkTextMapper vtkTextProperty vtkSelectVisiblePoints 
// vtkIdFilter vtkCellCenters

#ifndef __vtkQtLabelSurface_h
#define __vtkQtLabelSurface_h

#include "vtkImageAlgorithm.h"
#include "QVTKWin32Header.h"

class vtkDataObject;
class vtkDataSet;
class vtkTextProperty;
class vtkRenderer;
class QPainter;

class QVTK_EXPORT vtkQtLabelSurface : public vtkImageAlgorithm
{
public:
  // Description:
  // Instantiate object with %%-#6.3g label format. By default, point ids
  // are labeled.
  static vtkQtLabelSurface *New();

  vtkTypeRevisionMacro(vtkQtLabelSurface,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get the component number to label if the data to print has
  // more than one component. For example, all the components of
  // scalars, vectors, normals, etc. are labeled by default
  // (LabeledComponent=(-1)). However, if this ivar is nonnegative,
  // then only the one component specified is labeled.
  vtkSetMacro(LabeledComponent,int);
  vtkGetMacro(LabeledComponent,int);

  // Description:
  // Set/Get the field data array to label. This instance variable is
  // only applicable if field data is labeled.  This will clear
  // FieldDataName when set.
  void SetFieldDataArray(int arrayIndex);
  vtkGetMacro(FieldDataArray,int);

  // Description:
  // Set/Get the name of the field data array to label.  This instance
  // variable is only applicable if field data is labeled.  This will
  // override FieldDataArray when set.
  void SetFieldDataName(const char *arrayName);
  vtkGetStringMacro(FieldDataName);

  // Description:
  // Set/Get the name of the text rotation array.
  vtkSetStringMacro(TextRotationArrayName);
  vtkGetStringMacro(TextRotationArrayName);

  // Description:
  // Set the input dataset to the mapper. This mapper handles any type of data.
  virtual void SetInput(vtkDataObject*);

  // Description:
  // Use GetInputDataObject() to get the input data object for composite
  // datasets.
  vtkDataSet *GetInput();

  // Description:
  // Set/Get the text property.
  virtual void SetLabelTextProperty(vtkTextProperty *p);
  vtkGetObjectMacro(LabelTextProperty,vtkTextProperty);

  void SetRenderer(vtkRenderer* ren)
    {
      if (this->Renderer != ren)
        {
        this->Renderer = ren;
        this->Modified();
        }
    }
  vtkRenderer* GetRenderer() { return this->Renderer; }

  virtual int RequestData( vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  int RequestInformation ( vtkInformation * vtkNotUsed(request),
                           vtkInformationVector ** vtkNotUsed( inputVector ),
                           vtkInformationVector *outputVector);

protected:
  vtkQtLabelSurface();
  ~vtkQtLabelSurface();

  vtkDataSet *Input;
  vtkTextProperty *LabelTextProperty;

  int   LabeledComponent;
  int   FieldDataArray;
  char* FieldDataName;
  char* TextRotationArrayName;

  vtkRenderer *Renderer;

  vtkTimeStamp BuildTime;

  int NumberOfLabels;
  int NumberOfLabelsAllocated;
  double* LabelPositions;
  int DataExtent[6];

  virtual int FillInputPortInformation(int, vtkInformation*);
  virtual int FillOutputPortInformation(int port, vtkInformation* info);

  void AllocateLabels(int numLabels);
  void BuildLabels( QPainter* painter );
  void BuildLabelsInternal( vtkDataSet*, QPainter* painter );
private:
  vtkQtLabelSurface(const vtkQtLabelSurface&);  // Not implemented.
  void operator=(const vtkQtLabelSurface&);  // Not implemented.
};

#endif

