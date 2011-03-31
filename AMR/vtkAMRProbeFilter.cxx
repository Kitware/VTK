/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRProbeFilter.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

#include <cassert>
#include <list>
#include <algorithm>

#include "vtkInformation.h"
#include "vtkInformationVector.h"

#include "vtkAMRProbeFilter.h"
#include "vtkObjectFactory.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkAMRBox.h"

vtkStandardNewMacro(vtkAMRProbeFilter);

//------------------------------------------------------------------------------
vtkAMRProbeFilter::vtkAMRProbeFilter()
{
  this->SetNumberOfInputPorts( 2 );
  this->SetNumberOfOutputPorts( 1 );
}

//------------------------------------------------------------------------------
vtkAMRProbeFilter::~vtkAMRProbeFilter()
{
  // TODO Auto-generated destructor stub
}

//------------------------------------------------------------------------------
void vtkAMRProbeFilter::PrintSelf(std::ostream &oss, vtkIndent indent )
{
  this->Superclass::PrintSelf( oss, indent );
}

//------------------------------------------------------------------------------
void vtkAMRProbeFilter::SetAMRDataSet( vtkHierarchicalBoxDataSet *amrds )
{
  assert( "pre: input AMR dataset is NULL" && (amrds != NULL) );
  this->SetInput(0,amrds);
}

//------------------------------------------------------------------------------
void vtkAMRProbeFilter::SetProbePoints( vtkPointSet *probes )
{
  assert( "pre: input probe points are NULL" && (probes != NULL) );
  this->SetInput(1,probes);
}

//------------------------------------------------------------------------------
void vtkAMRProbeFilter::ProbeAMR(
    vtkPointSet *probes, vtkHierarchicalBoxDataSet *amrds,
    vtkMultiBlockDataSet *mbds )
{
  // Sanity Check!
  assert( "pre: input AMR dataset is NULL" && (amrds != NULL) );
  assert( "pre: input probe pointset is NULL" && (probes != NULL) );
  assert( "pre: multiblock output is NULL" && (mbds != NULL) );

  std::list< vtkIdType > probeIds;
  unsigned int pnt = 0;
  for( ; pnt < probes->GetNumberOfPoints(); ++pnt )
      probeIds.push_front( pnt );

  // Loop through all levels from highest to lowest and while there
  // are still probe points.
  unsigned int level=amrds->GetNumberOfLevels()-1;
  for( ; !probeIds.empty() && (level >= 0); --level )
    {

      std::list< vtkIdType >::iterator probeIter = probeIds.begin();
      for( ; probeIter != probeIds.end(); ++probeIter )
        {
          vtkIdType idx = *probeIter;
          double coords[3];
          probes->GetPoint( idx, coords );

          unsigned int dataIdx = 0;
          for( ; dataIdx < amrds->GetNumberOfDataSets(level); ++dataIdx )
            {
              vtkAMRBox box;
              amrds->GetMetaData( level, dataIdx, box );
              if( box.HasPoint(coords[0],coords[1],coords[2]) )
                {
                  // TODO: implement this
                }

            } // END for all data
        } // END for all probes

    } // END for all levels

}

//------------------------------------------------------------------------------
int vtkAMRProbeFilter::RequestData(
    vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector )
{

  vtkHierarchicalBoxDataSet *amrds=
   vtkHierarchicalBoxDataSet::SafeDownCast(
    inputVector[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));
  assert( "pre: input AMR dataset is NULL" && (amrds != NULL) );

  vtkPointSet *probes=
   vtkPointSet::SafeDownCast(
    inputVector[1]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));
  assert( "pre: input probe pointset is NULL" && (probes != NULL) );

  vtkMultiBlockDataSet *mbds=
   vtkMultiBlockDataSet::SafeDownCast(
    outputVector->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));
  assert( "pre: multiblock output is NULL" && (mbds != NULL) );

  this->ProbeAMR( probes, amrds, mbds );

  return 1;
}

//------------------------------------------------------------------------------
int vtkAMRProbeFilter::FillInputPortInformation(int port, vtkInformation *info)
{
  int status = 0;
  switch( port )
    {
      case 0:
        info->Set(
         vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHierarchicalBoxDataSet");
        status = 1;
        break;
      case 1:
        info->Set(
         vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
        status = 1;
        break;
      default:
        vtkErrorMacro( "Called FillInputPortInformation with invalid port!" );
        status = 0;
    }
  return( status );
}

//------------------------------------------------------------------------------
int vtkAMRProbeFilter::FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(),"vtkMultiBlockDataSet");
  return 1;
}
