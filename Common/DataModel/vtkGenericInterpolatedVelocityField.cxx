/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericInterpolatedVelocityField.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericInterpolatedVelocityField.h"

#include "vtkGenericAttributeCollection.h"
#include "vtkGenericAttribute.h"
#include "vtkGenericDataSet.h"
#include "vtkGenericCellIterator.h"
#include "vtkGenericAdaptorCell.h"
#include "vtkObjectFactory.h"
#include "vtkDataSetAttributes.h" // for vtkDataSetAttributes::VECTORS

#include <vector>

vtkStandardNewMacro(vtkGenericInterpolatedVelocityField);

typedef std::vector< vtkGenericDataSet* > DataSetsTypeBase;
class vtkGenericInterpolatedVelocityFieldDataSetsType: public DataSetsTypeBase {};

vtkGenericInterpolatedVelocityField::vtkGenericInterpolatedVelocityField()
{
  this->NumFuncs = 3; // u, v, w
  this->NumIndepVars = 4; // x, y, z, t
  this->GenCell = 0;
  this->CacheHit = 0;
  this->CacheMiss = 0;
  this->Caching = 1; // Caching on by default

  this->VectorsSelection = 0;

  this->DataSets = new vtkGenericInterpolatedVelocityFieldDataSetsType;
  this->LastDataSet = 0;
}

vtkGenericInterpolatedVelocityField::~vtkGenericInterpolatedVelocityField()
{
  this->NumFuncs = 0;
  this->NumIndepVars = 0;
  if(this->GenCell!=0)
    {
    this->GenCell->Delete();
    }

  this->SetVectorsSelection(0);

  delete this->DataSets;
}

static int tmp_count=0;
// Evaluate u,v,w at x,y,z,t
int vtkGenericInterpolatedVelocityField::FunctionValues(double* x, double* f)
{
  vtkGenericDataSet* ds;
  if(!this->LastDataSet && !this->DataSets->empty())
    {
    ds = (*this->DataSets)[0];
    this->LastDataSet = ds;
    }
  else
    {
    ds = this->LastDataSet;
    }
  int retVal = this->FunctionValues(ds, x, f);
  if (!retVal)
    {
    tmp_count = 0;
    for(DataSetsTypeBase::iterator i = this->DataSets->begin();
        i != this->DataSets->end(); ++i)
      {
      ds = *i;
      if(ds && ds != this->LastDataSet)
        {
        this->ClearLastCell();
        retVal = this->FunctionValues(ds, x, f);
        if (retVal)
          {
          this->LastDataSet = ds;
          return retVal;
          }
        }
      }
    this->ClearLastCell();
    return 0;
    }
  tmp_count++;
  return retVal;
}

const double vtkGenericInterpolatedVelocityField::TOLERANCE_SCALE = 1.0E-8;

// Evaluate u,v,w at x,y,z,t
int vtkGenericInterpolatedVelocityField::FunctionValues(
  vtkGenericDataSet* dataset,
  double* x,
  double* f)
{
  int i, subId;
  vtkGenericAttribute *vectors=0;
  double dist2;
  int ret;
  int attrib;

  for(i=0; i<3; i++)
    {
    f[i] = 0;
    }

  // See if a dataset has been specified and if there are input vectors
  int validState=dataset!=0;
  if(validState)
    {
    if(this->VectorsSelection!=0)
      {
      attrib=dataset->GetAttributes()->FindAttribute(this->VectorsSelection);
      validState=attrib>=0;
      if(validState)
        {
        vectors=dataset->GetAttributes()->GetAttribute(attrib);
        validState=(vectors->GetType()==vtkDataSetAttributes::VECTORS)||(vectors->GetCentering()==vtkPointCentered);
        }
      }
    else
      {
       // Find the first attribute, point centered and with vector type.
        attrib=0;
        validState=0;
        int c=dataset->GetAttributes()->GetNumberOfAttributes();
        while(attrib<c&&!validState)
          {
          validState=(dataset->GetAttributes()->GetAttribute(attrib)->GetType()==vtkDataSetAttributes::VECTORS)&&(dataset->GetAttributes()->GetAttribute(attrib)->GetCentering()==vtkPointCentered);
          ++attrib;
          }
        if(validState)
          {
          vectors=dataset->GetAttributes()->GetAttribute(attrib-1);
          }
      }
    }

  if (!validState)
    {
    vtkErrorMacro(<<"Can't evaluate dataset!");
    return 0;
    }

  double tol2 =
    dataset->GetLength() * vtkGenericInterpolatedVelocityField::TOLERANCE_SCALE;

  int found = 0;

  if (this->Caching)
    {
    // See if the point is in the cached cell
    if (this->GenCell==0 || this->GenCell->IsAtEnd() ||
        !(ret=this->GenCell->GetCell()->EvaluatePosition(x, 0, subId,
                                                         this->LastPCoords,
                                                         dist2))
        || ret == -1)
      {
      // if not, find and get it
      if (this->GenCell!=0 && !this->GenCell->IsAtEnd())
        {
        this->CacheMiss++;
        found=dataset->FindCell(x,this->GenCell,tol2,subId,
                                this->LastPCoords);
        }
      }
    else
      {
      this->CacheHit++;
      found = 1;
      }
    }

  if (!found)
    {
    // if the cell is not found, do a global search (ignore initial
    // cell if there is one)
    if(this->GenCell==0)
      {
      this->GenCell=dataset->NewCellIterator();
      }
    found=dataset->FindCell(x,this->GenCell,tol2,subId,this->LastPCoords);
    if(!found)
      {
      return 0;
      }
    }

  this->GenCell->GetCell()->InterpolateTuple(vectors,this->LastPCoords,f);

  return 1;
}

//-----------------------------------------------------------------------------
void vtkGenericInterpolatedVelocityField::AddDataSet(vtkGenericDataSet* dataset)
{
  if (!dataset)
    {
    return;
    }

  this->DataSets->push_back(dataset);
}

//-----------------------------------------------------------------------------
// Description:
// Set the last cell id to -1 so that the next search does not
// start from the previous cell
void vtkGenericInterpolatedVelocityField::ClearLastCell()
{
  if(this->GenCell!=0)
    {
    if(!this->GenCell->IsAtEnd())
      {
      this->GenCell->Next();
      }
    }
}
//-----------------------------------------------------------------------------
// Description:
// Return the cell cached from last evaluation.
vtkGenericAdaptorCell *vtkGenericInterpolatedVelocityField::GetLastCell()
{
  vtkGenericAdaptorCell *result;
  if(this->GenCell!=0 && !this->GenCell->IsAtEnd())
    {
    result=this->GenCell->GetCell();
    }
  else
    {
    result=0;
    }
  return result;
}

//-----------------------------------------------------------------------------
int vtkGenericInterpolatedVelocityField::GetLastLocalCoordinates(double pcoords[3])
{
  int j;

  // If last cell is valid, fill p with the local coordinates
  // and return true
  if (this->GenCell!=0 && !this->GenCell->IsAtEnd())
    {
    for (j=0; j < 3; j++)
      {
      pcoords[j] = this->LastPCoords[j];
      }
    return 1;
    }
  // otherwise, return false
  else
    {
    return 0;
    }
}

void vtkGenericInterpolatedVelocityField::CopyParameters(
  vtkGenericInterpolatedVelocityField* from)
{
  this->Caching = from->Caching;
}

void vtkGenericInterpolatedVelocityField::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if ( this->VectorsSelection )
    {
    os << indent << "VectorsSelection: " << this->VectorsSelection << endl;
    }
  else
    {
    os << indent << "VectorsSelection: (none)" << endl;
    }
  if ( this->GenCell )
    {
    os << indent << "Last cell: " << this->GenCell << endl;
    }
  else
    {
    os << indent << "Last cell: (none)" << endl;
    }
  os << indent << "Cache hit: " << this->CacheHit << endl;
  os << indent << "Cache miss: " << this->CacheMiss << endl;
  os << indent << "Caching: ";
  if ( this->Caching )
    {
    os << "on." << endl;
    }
  else
    {
    os << "off." << endl;
    }

  os << indent << "VectorsSelection: "
     << (this->VectorsSelection?this->VectorsSelection:"(none)") << endl;
  os << indent << "LastDataSet : "
     << this->LastDataSet << endl;

}
