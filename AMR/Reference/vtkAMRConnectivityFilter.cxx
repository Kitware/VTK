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

#include <cassert>

//
// Standard methods
//
vtkStandardNewMacro( vtkAMRConnectivityFilter );

vtkAMRConnectivityFilter::vtkAMRConnectivityFilter()
{
  this->AMRDataSet         = NULL;
  this->RemoteConnectivity = NULL;
  this->LocalConnectivity  = NULL;
  this->Controller         = NULL;
}

//-----------------------------------------------------------------------------
vtkAMRConnectivityFilter::~vtkAMRConnectivityFilter()
{
  this->AMRDataSet         = NULL;
  this->RemoteConnectivity = NULL;
  this->LocalConnectivity  = NULL;
  this->Controller         = NULL;
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

  if( this->AMRDataSet == NULL )
    {
      vtkErrorMacro( "Cannot compute AMR connectivity on a NULL data-set!" );
    }

  this->RemoteConnectivity = vtkAMRInterBlockConnectivity::New( );
  this->LocalConnectivity  = vtkAMRInterBlockConnectivity::New( );

  unsigned int level = 0;
  for( ;level < this->AMRDataSet->GetNumberOfLevels(); ++level )
    {
      unsigned int dataIdx = 0;
      for( ;dataIdx < this->AMRDataSet->GetNumberOfDataSets(level); ++dataIdx )
        {
           vtkAMRBox myBox;
           this->AMRDataSet->GetMetaData( level, dataIdx, myBox );
           assert( "post: metadata level mismatch" &&
                   (myBox.GetLevel()==level) );
           assert( "post: metadata idx mismatch" &&
                   (myBox.GetBlockId()==dataIdx) );
           myBox.WriteBox();
           this->ComputeBlockConnectivity( myBox );

        } // END for all blocks at this level

    } // END for all levels

}

//-----------------------------------------------------------------------------
void vtkAMRConnectivityFilter::ComputeBlockConnectivity( vtkAMRBox &myBox)
{
  assert( "pre: Input AMR dataset is NULL" && (this->AMRDataSet != NULL) );

  int myRank = 0;
  if( this->Controller != NULL )
   myRank = this->Controller->GetLocalProcessId();

  unsigned int level = 0;
  for( ; level < this->AMRDataSet->GetNumberOfLevels(); ++level )
    {
      unsigned int idx = 0;
      for( ; idx < this->AMRDataSet->GetNumberOfDataSets( level ); ++idx )
        {
          vtkAMRBox box;
          this->AMRDataSet->GetMetaData( level, idx, box );

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

