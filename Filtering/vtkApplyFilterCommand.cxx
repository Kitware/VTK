/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkApplyFilterCommand.cxx
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

#include "vtkApplyFilterCommand.h"

#include "vtkDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSource.h"

#include "vtkDataSetToDataSetFilter.h"
#include "vtkDataSetToImageFilter.h"
#include "vtkDataSetToPolyDataFilter.h"
#include "vtkDataSetToStructuredGridFilter.h"
#include "vtkDataSetToStructuredPointsFilter.h"
#include "vtkDataSetToUnstructuredGridFilter.h"

#include "vtkApplyFilterCommandInternal.h"

vtkCxxRevisionMacro(vtkApplyFilterCommand, "1.1");

vtkCxxSetObjectMacro(vtkApplyFilterCommand,
                     Filter, 
                     vtkSource);

//----------------------------------------------------------------
vtkApplyFilterCommand::vtkApplyFilterCommand() 
{ 
  this->Internal = new vtkApplyFilterCommandInternal;

  vtkApplyFilterCommandInternal::FilterTypesVector dsfilters;
  dsfilters.push_back("vtkDataSetToDataSetFilter");
  dsfilters.push_back("vtkDataSetToImageFilter");
  dsfilters.push_back("vtkDataSetToPolyDataFilter");
  dsfilters.push_back("vtkDataSetToStructuredGridFilter");
  dsfilters.push_back("vtkDataSetToStructuredPointsFilter");
  dsfilters.push_back("vtkDataSetToUnstructuredGridFilter");
  this->Internal->FilterTypes["vtkDataSet"] = dsfilters;

  this->Filter = 0;
}

//----------------------------------------------------------------
vtkApplyFilterCommand::~vtkApplyFilterCommand() 
{ 
  delete this->Internal;

  this->SetFilter(0);
}

//----------------------------------------------------------------
void vtkApplyFilterCommand::SetFilterInput(vtkSource* source,
                                           vtkDataObject* input)
{
  vtkDataSet* dsinput = vtkDataSet::SafeDownCast(input);
  if (dsinput)
    {
    vtkDataSetToDataSetFilter* dsToDs = 
      vtkDataSetToDataSetFilter::SafeDownCast(source);
    if (dsToDs)
      {
      dsToDs->SetInput(dsinput);
      return;
      }
    vtkDataSetToImageFilter* dsToIf =
      vtkDataSetToImageFilter::SafeDownCast(source);
    if (dsToIf)
      {
      dsToIf->SetInput(dsinput);
      return;
      }
    vtkDataSetToPolyDataFilter* dsToPf =
      vtkDataSetToPolyDataFilter::SafeDownCast(source);
    if (dsToPf)
      {
      dsToPf->SetInput(dsinput);
      return;
      }
    vtkDataSetToStructuredGridFilter* dsToSg =
      vtkDataSetToStructuredGridFilter::SafeDownCast(source);
    if (dsToSg)
      {
      dsToSg->SetInput(dsinput);
      return;
      }
    vtkDataSetToStructuredPointsFilter* dsToSp =
      vtkDataSetToStructuredPointsFilter::SafeDownCast(source);
    if (dsToSp)
      {
      dsToSp->SetInput(dsinput);
      return;
      }
    vtkDataSetToUnstructuredGridFilter* dsToUg =
      vtkDataSetToUnstructuredGridFilter::SafeDownCast(source);
    if (dsToUg)
      {
      dsToUg->SetInput(dsinput);
      return;
      }
    }
}

//----------------------------------------------------------------
int vtkApplyFilterCommand::CheckFilterInputMatch(vtkDataObject* inp)
{
  vtkApplyFilterCommandInternal::FilterTypesMap::iterator cur;
  for(cur = this->Internal->FilterTypes.begin();
      cur != this->Internal->FilterTypes.end();
      cur++)
    {
    if ( inp->IsA( (*cur).first.c_str() ) )
      {
      vtkApplyFilterCommandInternal::FilterTypesVector::iterator idx;
      for (idx = (*cur).second.begin();
           idx != (*cur).second.end();
           idx++)
        {
        if ( this->Filter->IsA( (*idx).c_str() ) )
          {
          return 1;
          }
        }
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkApplyFilterCommand::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Filter: ";
  if (this->Filter)
    {
    os << endl;
    this->Filter->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}
