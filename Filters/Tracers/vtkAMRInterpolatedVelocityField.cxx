#include "vtkAMRInterpolatedVelocityField.h"
#include "vtkObjectFactory.h"
#include "vtkUniformGrid.h"
#include "vtkOverlappingAMR.h"
#include <assert.h>
//----------------------------------------------------------------------------
namespace
{

  bool Inside(double q[3], const vtkAMRBox& box)
  {
    double gbounds[6];
    box.GetBounds(gbounds);
    if ((q[0] < gbounds[0]) || (q[0] > gbounds[1]) ||
        (q[1] < gbounds[2]) || (q[1] > gbounds[3]) ||
        (q[2] < gbounds[4]) || (q[2] > gbounds[5]))
      {
      return false;
      }
    else
      {
      return true;
      }
  }

  bool FindInLevel(double q[3], vtkOverlappingAMR *amrds, int level, unsigned int& gridId)
  {
    vtkAMRBox box;
    for(unsigned int i = 0; i < amrds->GetNumberOfDataSets(level); i++ )
      {
      amrds->GetMetaData(level,i,box);
      bool inside = Inside(q,box);
      if(inside)
        {
        gridId = i;
        return true;
        }
      }
    return false;
  }


};

bool vtkAMRInterpolatedVelocityField::FindGrid(double q[3],vtkOverlappingAMR *amrds,
                                               unsigned int& level, unsigned int& gridId)
{
  if (!FindInLevel(q, amrds, 0,gridId))
    {
    return false;
    }

  unsigned int maxLevels = amrds->GetNumberOfLevels();
  for(level=0; level<maxLevels;level++)
    {
    unsigned int *children = amrds->GetChildren(level, gridId);
    if (children == NULL)
      {
      break;
      }
    int n = children[0];
    int i;
    for (i = 1; i <= n; i++)
      {
      vtkAMRBox box;
      amrds->GetMetaData(level+1,children[i],box);
      if(Inside(q,box))
        {
        gridId = children[i];
        break;
        }
      }
    if(i>n)
      {
      break;
      }
    }
  return true;
}
vtkStandardNewMacro(vtkAMRInterpolatedVelocityField);


vtkAMRInterpolatedVelocityField::vtkAMRInterpolatedVelocityField():
  LastAMRBox(3)
{
  this->Weights = new double[8];
  this->AmrDataSet = NULL;
  this->LastAMRBox.Invalidate();
}

vtkAMRInterpolatedVelocityField::~vtkAMRInterpolatedVelocityField()
{
  delete [] this->Weights;
  this->Weights = NULL;
}

void vtkAMRInterpolatedVelocityField::PrintSelf( ostream & os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}


void vtkAMRInterpolatedVelocityField::SetAMRData(vtkOverlappingAMR* amrds)
{
  this->AmrDataSet = amrds;
}

int vtkAMRInterpolatedVelocityField::FunctionValues( double * x, double * f )
{
  if(this->LastDataSet &&
     this->FunctionValues(this->LastDataSet,x,f))
    {
    return 1;
    }

  //Either we do not know which data set it is, or existing LastDataSet does not contain x
  //In any case, set LastDataSet to NULL and try to find a new one
  this->LastDataSet = NULL;
  this->LastAMRBox.Invalidate();
  this->LastCellId  = -1;

  unsigned int level, gridId;
  if(!FindGrid(x,this->AmrDataSet,level,gridId))
    {
    return 0;
    }

  vtkDataSet* ds = this->AmrDataSet->GetDataSet(level,gridId,this->LastAMRBox);
  assert(!this->LastAMRBox.IsInvalid());

  if(!ds)
    {
    return 0;
    }
  if(!this->FunctionValues(ds,x,f))
    {
    return 0;
    }

  this->LastDataSet = ds;
  return 1;
}

bool vtkAMRInterpolatedVelocityField::SetLastDataSet(int level, int id)
{
  this->LastDataSet = this->AmrDataSet->GetDataSet(level,id,this->LastAMRBox);
  return this->LastDataSet!=NULL;
}


void vtkAMRInterpolatedVelocityField::SetLastCellId(vtkIdType, int )
{
  vtkWarningMacro("Calling SetLastCellId has no effect")
}
