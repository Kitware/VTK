/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRConnectivityFilter.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

#include "vtkAMRConnectivityFilter.h"
#include "vtkObjectFactory.h"

#include "vtkAMRBox.h"
#include "vtkMultiProcessController.h"
#include "vtkAMRInterBlockConnectivity.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkAssertUtils.hpp"

//
// Standard methods
//
vtkStandardNewMacro( vtkAMRConnectivityFilter );

vtkAMRConnectivityFilter::vtkAMRConnectivityFilter()
{
  this->RemoteConnectivity = NULL;
  this->LocalConnectivity  = NULL;
  this->Controller         = vtkMultiProcessController::GetGlobalController();
}

//-----------------------------------------------------------------------------
vtkAMRConnectivityFilter::~vtkAMRConnectivityFilter()
{
  this->RemoteConnectivity->Delete();
  this->LocalConnectivity->Delete();
}

//-----------------------------------------------------------------------------
void vtkAMRConnectivityFilter::PrintSelf( std::ostream &os, vtkIndent indent )
{
  if( this->RemoteConnectivity != NULL )
    {
      os << "Remote Connectivity:\n";
      this->RemoteConnectivity->PrintSelf( os, indent );
      os << "\n\n";
    }
  if( this->LocalConnectivity != NULL )
    {
      os << "Local Connectivity:\n";
      this->LocalConnectivity->PrintSelf( os, indent );
    }
}

//-----------------------------------------------------------------------------
void vtkAMRConnectivityFilter::ComputeConnectivity( )
{
  vtkAssertUtils::assertNotNull( this->Controller,__FILE__,__LINE__ );

  if( this->AMRDataSet == NULL )
    {
      vtkErrorMacro( "Cannot compute AMR connectivity on a NULL data-set!" );
    }

  this->RemoteConnectivity = vtkAMRInterBlockConnectivity::New( );
  this->LocalConnectivity  = vtkAMRInterBlockConnectivity::New( );
  vtkAssertUtils::assertNotNull(this->RemoteConnectivity,__FILE__,__LINE__);
  vtkAssertUtils::assertNotNull(this->LocalConnectivity,__FILE__,__LINE__);

  unsigned int level = 0;
  for( ;level < this->AMRDataSet->GetNumberOfLevels(); ++level )
    {
      unsigned int dataIdx = 0;
      for( ;dataIdx < this->AMRDataSet->GetNumberOfDataSets(level); ++dataIdx )
        {
           vtkAMRBox myBox;
           vtkUniformGrid *myGrid =
               this->AMRDataSet->GetDataSet(level,dataIdx,myBox);
           if( myGrid != NULL )
             {
               this->ComputeBlockConnectivity( myBox );
             }

        }// END for all blocks at this level

    } // END for all levels

}

//-----------------------------------------------------------------------------
void vtkAMRConnectivityFilter::ComputeBlockConnectivity(const vtkAMRBox &myBox)
{
  vtkAssertUtils::assertNotNull( this->Controller,__FILE__,__LINE__);
  vtkAssertUtils::assertNotNull( this->AMRDataSet, __FILE__,__LINE__);

  int myRank         = this->Controller->GetLocalProcessId();
  unsigned int level = 0;
  for( ; level < this->AMRDataSet->GetNumberOfLevels(); ++level )
    {
      unsigned int idx = 0;
      for( ; idx < this->AMRDataSet->GetNumberOfDataSets( level ); ++idx )
        {
          vtkAMRBox box;
          vtkUniformGrid *dummyGrid =
              this->AMRDataSet->GetDataSet(level,idx,box);

          // If the blocks are not at the same level or are at the
          // same level but, have a different block ID, the check for
          // collision.
          if( (box.GetLevel() != myBox.GetLevel())   ||
              (box.GetBlockId() != myBox.GetBlockId()   ) )
            {

              if( vtkAMRBox::Collides(box,myBox) )
                {
                  // the boxes collide
                  if( box.GetProcessId() == myRank )
                    {
                      // Add to local connectivity
                      this->LocalConnectivity->InsertConnection(
                          myBox.GetBlockId(),myBox.GetLevel(),
                          box.GetBlockId(),box.GetLevel(),
                          myRank );
                    }
                  else
                    {
                      // Add to remote connectivity
                      this->RemoteConnectivity->InsertConnection(
                          myBox.GetBlockId(),myBox.GetLevel(),
                          box.GetBlockId(),box.GetLevel(),
                          box.GetProcessId() );

                    }

                } // END if the boxes collide

            } // END if different block

        } // END for all blocks at this level
    } // END for all levels


}

