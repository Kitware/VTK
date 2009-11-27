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

vtkCxxRevisionMacro(vtkPlot, "1.1");
vtkCxxSetObjectMacro(vtkPlot, Selection, vtkIdTypeArray);

//-----------------------------------------------------------------------------
vtkPlot::vtkPlot()
{
  this->r = 0;
  this->g = 0;
  this->b = 0;
  this->a = 0;
  this->Width = 1.0;
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
}

//-----------------------------------------------------------------------------
void vtkPlot::SetColor(unsigned char r, unsigned char g, unsigned char b,
                       unsigned char a)
{
  this->r = r;
  this->g = g;
  this->b = b;
  this->a = a;
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
  this->Data->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS,
                                     xColumn);
  this->Data->SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS,
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
  os << indent << "Color: " << this->r << ", " << this->g
     << ", " << this->b << ", " << this->a << endl;
  os << indent << "Width: " << this->Width << endl;
}
