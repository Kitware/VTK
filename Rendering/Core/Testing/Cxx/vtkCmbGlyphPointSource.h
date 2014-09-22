/*=========================================================================

Copyright (c) 1998-2012 Kitware Inc. 28 Corporate Drive,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced,
distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO
PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
=========================================================================*/
// .NAME vtkCmbGlyphPointSource - Represents a set of points that will be used for Glyphing
// .SECTION Description
// The input Source data is shallow copied to the output

#ifndef __vtkCmbGlyphPointSource_h
#define __vtkCmbGlyphPointSource_h

//#include "vtkCMBFilteringModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"
#include "vtkSmartPointer.h"
//#include "cmbSystemConfig.h"

class vtkPoints;
class vtkUnsignedCharArray;
class vtkDoubleArray;
class vtkBitArray;
class vtkIntArray;
class vtkCellArray;
class vtkTransform;

class vtkCmbGlyphPointSource : public vtkPolyDataAlgorithm
{
public:
  static vtkCmbGlyphPointSource *New();
  vtkTypeMacro(vtkCmbGlyphPointSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Insert the next point into the object
  vtkIdType InsertNextPoint(double x, double y, double z);
  vtkIdType InsertNextPoint(double p[3])
  { return this->InsertNextPoint(p[0], p[1], p[2]);}

  // Description:
  // Insert the next point and its properties into the object
  vtkIdType InsertNextPoint(double x, double y, double z,
                            double r, double g, double b, double a,
                            double sx, double sy, double sz,
                            double ox, double oy, double oz,
                            int vis);

  void SetScale(vtkIdType index, double sx, double sy, double sz);
  void SetOrientation(vtkIdType index, double ox, double oy, double oz);
  void ApplyTransform(double *orinetationDelta, double *positionDelta,
                      double *scaleDelta);
  void ApplyTransform(vtkIdType index, double *orinetationDelta,
                      double *positionDelta, double *scaleDelta);
  double *GetBounds(vtkIdType index);
  void SetVisibility(vtkIdType index, int flag);
  void SetGlyphType(vtkIdType index, int type);
  void SetColor(vtkIdType index, double r, double g, double b, double a);
  void UnsetColor(vtkIdType index);
  void ResetColorsToDefault();
  vtkIdType GetNumberOfPoints()
  {
    return this->Source->GetNumberOfPoints();
  }
  void SetPoint(vtkIdType index, double x, double y, double z);
  void GetPoint(vtkIdType index, double *p);
  double *GetPoint(vtkIdType index)
  {
    this->GetPoint(index, this->TempData);
    return this->TempData;
  }
  void GetScale(vtkIdType index, double *s);
  double *GetScale(vtkIdType index)
  {
    this->GetScale(index, this->TempData);
    return this->TempData;
  }

  void GetOrientation(vtkIdType index, double *o);
  double *GetOrientation(vtkIdType index)
  {
    this->GetOrientation(index, this->TempData);
    return this->TempData;
  }
  int GetVisibility(vtkIdType index);

  void GetColor(vtkIdType index, double *color);
  double *GetColor(vtkIdType index)
  {
    this->GetColor(index, this->TempData);
    return this->TempData;
  }

  void SetDefaultColor(double r, double g, double b, double a);
  const double *GetDefaultColor() const
  {
    return this->DefaultColor;
  }
  void SetGlyphSourceBounds(double bounds[6])
  {
    for(int i=0; i<6; i++)this->GlyphSourceBounds[i]=bounds[i];
  }
  const double *GetGlyphSourceBounds() const
  {
    return this->GlyphSourceBounds;
  }

  // Load the point information from a file
  void ReadFromFile(const char *);

  // Write the point information to a file
  void WriteToFile(const char *);

protected:
  vtkCmbGlyphPointSource();
  ~vtkCmbGlyphPointSource();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  vtkSmartPointer<vtkPolyData>Source;
  vtkSmartPointer<vtkPoints> Points;
  vtkSmartPointer<vtkUnsignedCharArray>Color;
  vtkSmartPointer<vtkBitArray>Visibility;
  vtkSmartPointer<vtkBitArray>SelectionMask;
  vtkSmartPointer<vtkDoubleArray>Scaling;
  vtkSmartPointer<vtkDoubleArray>Orientation;
  vtkSmartPointer<vtkCellArray>CellIds;
  vtkSmartPointer<vtkTransform>Transform;
  vtkSmartPointer<vtkIntArray>GlyphType;
  double TempData[6];
  double DefaultColor[4];
  double GlyphSourceBounds[6];
private:
  vtkCmbGlyphPointSource(const vtkCmbGlyphPointSource&);  // Not implemented.
  void operator=(const vtkCmbGlyphPointSource&);  // Not implemented.
};

#endif
