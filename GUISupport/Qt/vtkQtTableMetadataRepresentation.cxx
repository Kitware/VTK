/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtTableMetadataRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkQtTableMetadataRepresentation.h"
#include "vtkQtTableModelAdapter.h"
#include "vtkQtItemView.h"

#include <vtkDoubleArray.h>
#include <vtkObjectFactory.h>

#include <QColor>
#include <QModelIndex>
#include <QPixmap>
#include <QIcon>

#include <assert.h>

vtkCxxRevisionMacro(vtkQtTableMetadataRepresentation, "1.2");
vtkStandardNewMacro(vtkQtTableMetadataRepresentation);

// ----------------------------------------------------------------------

vtkQtTableMetadataRepresentation::vtkQtTableMetadataRepresentation()
{
  // nothing to do -- all handled in the superclass
}

// ----------------------------------------------------------------------

vtkQtTableMetadataRepresentation::~vtkQtTableMetadataRepresentation()
{
  // nothing to do -- all handled in the superclass
}

// ----------------------------------------------------------------------

void
vtkQtTableMetadataRepresentation::SetupInputConnections()
{
  this->Superclass::SetupInputConnections();

  // The superclass took care of creating colors for each series.
  // Since the model is populated at this point, go through and put
  // them in.
  QColor c;
  for (int i = 0; i < this->SeriesColors->GetNumberOfTuples(); ++i)
    {
    double tuple[4];
    this->SeriesColors->GetTuple(i, tuple);
    c.setRgbF(tuple[0], tuple[1], tuple[2], tuple[3]);
    QPixmap block(10, 10);
    block.fill(c);
    QModelIndex index = this->ModelAdapter->index(i, 0);
    this->ModelAdapter->setData(index, QVariant(QIcon(block)),
                                Qt::DecorationRole);
    }
}

// ----------------------------------------------------------------------

void
vtkQtTableMetadataRepresentation::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------

bool
vtkQtTableMetadataRepresentation::AddToView(vtkView *v)
{
  vtkQtItemView *itemView = 
    vtkQtItemView::SafeDownCast(v);

  if (!itemView)
    {
    vtkErrorMacro(<<"vtkQtTableMetadataRepresentation can only be added to vtkItemView or subclasses.  You tried to add it to an instance of "
                  << v->GetClassName());
    return false;
    }

  itemView->SetItemModelAdapter(this->ModelAdapter);
  // Since the model is already populated that's all we really need to do.
  return true;
}

// ----------------------------------------------------------------------

bool
vtkQtTableMetadataRepresentation::RemoveFromView(vtkView *v)
{
  vtkQtItemView *itemView = 
    vtkQtItemView::SafeDownCast(v);

  if (itemView)
    {
    itemView->SetItemModelAdapter(NULL);
    }
  return true;
}

// ----------------------------------------------------------------------

void
vtkQtTableMetadataRepresentation::SetModelType()
{
  this->ModelAdapter->SetViewType(vtkQtAbstractModelAdapter::METADATA_VIEW);
}
