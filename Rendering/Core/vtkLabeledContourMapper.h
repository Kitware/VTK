/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLabeledContourMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLabeledContourMapper - Draw labeled isolines.
// .SECTION Description
// Draw isolines with 3D inline labels.
//
// The lines in the input polydata will be drawn with labels displaying the
// scalar value.
//
// For this mapper to function properly, stenciling must be enabled in the
// render window (it is disabled by default). Otherwise the lines will be
// drawn through the labels.

#ifndef __vtkLabeledContourMapper_h
#define __vtkLabeledContourMapper_h

#include "vtkRenderingCoreModule.h" // For export macro

#include "vtkMapper.h"
#include "vtkNew.h" // For vtkNew
#include "vtkSmartPointer.h" // For vtkSmartPointer

class vtkTextActor3D;
class vtkTextProperty;
class vtkTextPropertyCollection;
class vtkPolyData;
class vtkPolyDataMapper;

class VTKRENDERINGCORE_EXPORT vtkLabeledContourMapper : public vtkMapper
{
public:
  static vtkLabeledContourMapper *New();
  vtkTypeMacro(vtkLabeledContourMapper, vtkMapper)
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Render(vtkRenderer *ren, vtkActor *act);

  // Description:
  // Specify the input data to map.
  void SetInputData(vtkPolyData *in);
  vtkPolyData *GetInput();

  // Description:
  // Return bounding box (array of six doubles) of data expressed as
  // (xmin,xmax, ymin,ymax, zmin,zmax).
  virtual double *GetBounds();
  virtual void GetBounds(double bounds[6]);

  // Description:
  // The text property used to label the lines. Note that both vertical and
  // horizontal justifications will be reset to "Centered" prior to rendering.
  // @note This is a convenience method that clears TextProperties and inserts
  // the argument as the only property in the collection.
  // @sa SetTextProperties
  virtual void SetTextProperty(vtkTextProperty *tprop);

  // Description:
  // The text properties used to label the lines. Note that both vertical and
  // horizontal justifications will be reset to "Centered" prior to rendering.
  // The collection is iterated through as the labels are generated, such that
  // the first line (cell) in the dataset is labeled using the first text
  // property in the collection, the second line is labeled with the second
  // property, and so on. If the number of cells exceeds the number of
  // properties, the property collection is repeated.
  // @sa SetTextProperty
  virtual void SetTextProperties(vtkTextPropertyCollection *coll);
  virtual vtkTextPropertyCollection *GetTextProperties();

  // Description:
  // If true, labels will be placed and drawn during rendering. Otherwise,
  // only the mapper returned by GetPolyDataMapper() will be rendered.
  // The default is to draw labels.
  vtkSetMacro(LabelVisibility, bool)
  vtkGetMacro(LabelVisibility, bool)
  vtkBooleanMacro(LabelVisibility, bool)

  // Description:
  // The polydata mapper used to render the contours.
  vtkGetNewMacro(PolyDataMapper, vtkPolyDataMapper)

protected:
  vtkLabeledContourMapper();
  ~vtkLabeledContourMapper();

  virtual void ComputeBounds();

  virtual int FillInputPortInformation(int, vtkInformation*);

  void Reset();

  bool CheckInputs(vtkRenderer *ren);
  bool CheckRebuild(vtkRenderer *ren, vtkActor *act);
  bool PrepareRender(vtkRenderer *ren, vtkActor *act);
  bool PlaceLabels();
  bool ResolveLabels();
  bool CreateLabels();
  bool BuildStencilQuads();
  virtual bool ApplyStencil(vtkRenderer *ren, vtkActor *act);
  bool RenderPolyData(vtkRenderer *ren, vtkActor *act);
  virtual bool RemoveStencil();
  bool RenderLabels(vtkRenderer *ren, vtkActor *act);

  bool AllocateTextActors(vtkIdType num);
  bool FreeTextActors();

  vtkTextProperty* GetTextPropertyForCellId(vtkIdType cellId) const;

  bool LabelVisibility;
  vtkIdType NumberOfTextActors;
  vtkIdType NumberOfUsedTextActors;
  vtkTextActor3D **TextActors;

  vtkNew<vtkPolyDataMapper> PolyDataMapper;
  vtkSmartPointer<vtkTextPropertyCollection> TextProperties;

  float *StencilQuads;
  vtkIdType StencilQuadsSize;
  unsigned int *StencilQuadIndices;
  vtkIdType StencilQuadIndicesSize;
  void FreeStencilQuads();

  vtkTimeStamp BuildTime;

private:
  vtkLabeledContourMapper(const vtkLabeledContourMapper&);  // Not implemented.
  void operator=(const vtkLabeledContourMapper&);  // Not implemented.

  struct Private;
  Private *Internal;
};

#endif
