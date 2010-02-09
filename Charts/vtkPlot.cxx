/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlot.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPlot.h"

#include "vtkTable.h"
#include "vtkDataObject.h"
#include "vtkIdTypeArray.h"
#include "vtkContextMapper2D.h"
#include "vtkObjectFactory.h"

#include "vtkStdString.h"

vtkCxxRevisionMacro(vtkPlot, "1.6");
vtkCxxSetObjectMacro(vtkPlot, Selection, vtkIdTypeArray);

//-----------------------------------------------------------------------------
vtkPlot::vtkPlot()
{
  this->Color[0] = 0;
  this->Color[1] = 0;
  this->Color[2] = 0;
  this->Color[3] = 255;
  this->Width = 2.0;
  this->Label = NULL;
  this->UseIndexForXSeries = false;
  this->Data = vtkContextMapper2D::New();
  this->Selection = NULL;
}

//-----------------------------------------------------------------------------
vtkPlot::~vtkPlot()
{
  if (this->Data)
    {
    this->Data->Delete();
    this->Data = NULL;
    }
  this->SetLabel(NULL);
}

//-----------------------------------------------------------------------------
void vtkPlot::SetColor(unsigned char r, unsigned char g, unsigned char b,
                       unsigned char a)
{
  this->Color[0] = r;
  this->Color[1] = g;
  this->Color[2] = b;
  this->Color[3] = a;
}

//-----------------------------------------------------------------------------
void vtkPlot::SetColor(double r, double g, double b)
{
  this->Color[0] = static_cast<unsigned char>(r*255.0);
  this->Color[1] = static_cast<unsigned char>(g*255.0);
  this->Color[2] = static_cast<unsigned char>(b*255.0);
}

//-----------------------------------------------------------------------------
void vtkPlot::GetColor(double rgb[3])
{
  for (int i = 0; i < 3; ++i)
    {
    rgb[i] = double(this->Color[0]) / 255.0;
    }
}

//-----------------------------------------------------------------------------
void vtkPlot::SetInput(vtkTable *table)
{
  this->Data->SetInput(table);
}

//-----------------------------------------------------------------------------
void vtkPlot::SetInput(vtkTable *table, const char *xColumn,
                       const char *yColumn)
{
  if (!xColumn || !yColumn)
    {
    vtkErrorMacro(<< "Called with null arguments for X or Y column.")
    }
  vtkDebugMacro(<< "Setting input, X column = \"" << vtkstd::string(xColumn)
                << "\", " << "Y column = \"" << vtkstd::string(yColumn) << "\"");

  this->Data->SetInput(table);
  this->Data->SetInputArrayToProcess(0, 0, 0,
                                     vtkDataObject::FIELD_ASSOCIATION_ROWS,
                                     xColumn);
  this->Data->SetInputArrayToProcess(1, 0, 0,
                                     vtkDataObject::FIELD_ASSOCIATION_ROWS,
                                     yColumn);
}

//-----------------------------------------------------------------------------
void vtkPlot::SetInput(vtkTable *table, vtkIdType xColumn,
                       vtkIdType yColumn)
{
  this->SetInput(table,
                 table->GetColumnName(xColumn),
                 table->GetColumnName(yColumn));
}

//-----------------------------------------------------------------------------
void vtkPlot::SetInputArray(int index, const char *name)
{
  this->Data->SetInputArrayToProcess(index, 0, 0,
                                     vtkDataObject::FIELD_ASSOCIATION_ROWS,
                                     name);
}

//-----------------------------------------------------------------------------
void vtkPlot::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  // Print out our color and width
  os << indent << "Color: " << this->Color[0] << ", " << this->Color[1]
     << ", " << this->Color[2] << ", " << this->Color[3] << endl;
  os << indent << "Width: " << this->Width << endl;
}
