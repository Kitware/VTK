/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObject.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkDataObject.h"
#include "vtkSource.h"
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkDataObject* vtkDataObject::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkDataObject");
  if(ret)
    {
    return (vtkDataObject*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkDataObject;
}




// Initialize static member that controls global data release 
// after use by filter
static int vtkDataObjectGlobalReleaseDataFlag = 0;

//----------------------------------------------------------------------------
vtkDataObject::vtkDataObject()
{
  this->Source = NULL;

  // We have to assume that if a user is creating the data on their own,
  // then they will fill it with valid data.
  this->DataReleased = 0;

  this->ReleaseDataFlag = 0;
  this->FieldData = vtkFieldData::New();

  // The extent is uninitialized
  this->WholeExtent[0] = this->WholeExtent[2] = this->WholeExtent[4] =  0;
  this->WholeExtent[1] = this->WholeExtent[3] = this->WholeExtent[5] = -1;

  this->Extent[0] = this->Extent[2] = this->Extent[4] =  0;
  this->Extent[1] = this->Extent[3] = this->Extent[5] = -1;

  this->UpdateExtent[0] = this->UpdateExtent[2] = this->UpdateExtent[4] =  0;
  this->UpdateExtent[1] = this->UpdateExtent[3] = this->UpdateExtent[5] = -1;

  // If we used pieces instead of 3D extent, then assume this object was
  // created by the user and this is piece 0 of 1 pieces.
  this->Piece          =  0;
  this->NumberOfPieces =  1;

  this->UpdatePiece          =   0;
  this->UpdateNumberOfPieces =   1;

  this->MaximumNumberOfPieces = 1;

  this->PipelineMTime = 0;
}

//----------------------------------------------------------------------------
vtkDataObject::~vtkDataObject()
{
  this->FieldData->Delete();
}


//----------------------------------------------------------------------------
// Determine the modified time of this object
unsigned long int vtkDataObject::GetMTime()
{
  unsigned long result;

  result = vtkObject::GetMTime();
  if ( this->FieldData )
    {
    unsigned long mtime = this->FieldData->GetMTime();
    result = ( mtime > result ? mtime : result);
    }
  
  return result;
}

//----------------------------------------------------------------------------
void vtkDataObject::Initialize()
{
//
// We don't modify ourselves because the "ReleaseData" methods depend upon
// no modification when initialized.
//
  this->FieldData->Initialize();
}

//----------------------------------------------------------------------------
void vtkDataObject::SetGlobalReleaseDataFlag(int val)
{
  if (val == vtkDataObjectGlobalReleaseDataFlag)
    {
    return;
    }
  vtkDataObjectGlobalReleaseDataFlag = val;
}

//----------------------------------------------------------------------------

void vtkDataObject::DataHasBeenGenerated()
{
  this->DataReleased = 0;
  this->UpdateTime.Modified();
}

//----------------------------------------------------------------------------
int vtkDataObject::GetGlobalReleaseDataFlag()
{
  return vtkDataObjectGlobalReleaseDataFlag;
}

//----------------------------------------------------------------------------
void vtkDataObject::ReleaseData()
{
  this->Initialize();
  this->DataReleased = 1;
}

//----------------------------------------------------------------------------
int vtkDataObject::ShouldIReleaseData()
{
  if ( vtkDataObjectGlobalReleaseDataFlag || this->ReleaseDataFlag )
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::Update()
{
  this->UpdateInformation();
  this->PropagateUpdateExtent();
  this->TriggerAsynchronousUpdate();
  this->UpdateData();
}

//----------------------------------------------------------------------------
void vtkDataObject::UpdateInformation()
{
  if (this->Source)
    {
    this->Source->UpdateInformation();
    }

  // Now we should know what our whole extent is. If our update extent
  // was not set yet, (or has been set to something invalid - with no 
  // data in it ) then set it to the whole extent.
  switch ( this->GetExtentType() )
    {
    case VTK_PIECES_EXTENT:
      if ( this->UpdatePiece == -1 && this->UpdateNumberOfPieces == 0 )
	{
	this->SetUpdateExtentToWholeExtent();
	}
      break;
      
    case VTK_3D_EXTENT:
      if ( this->UpdateExtent[1] < this->UpdateExtent[0] ||
	   this->UpdateExtent[3] < this->UpdateExtent[2] ||
	   this->UpdateExtent[5] < this->UpdateExtent[4] ) 
	{
	this->SetUpdateExtentToWholeExtent();
	}
      break;
    }
}

//----------------------------------------------------------------------------

void vtkDataObject::PropagateUpdateExtent()
{
  // Release data if update extent does not lie within extent
  this->ModifyExtentForUpdateExtent();

  // If we need to update due to PipelineMTime, or the fact that our
  // data was released, then propagate the update extent to the source 
  // if there is one.
  if ( this->UpdateTime < this->PipelineMTime || this->DataReleased )
    {
    if (this->Source)
      {
      this->Source->PropagateUpdateExtent(this);
      }
    }

  // Check that the update extent lies within the whole extent
  if ( ! this->VerifyUpdateExtent() )
    {
    // invalid update piece - this should not occur!
    return;
    }

  // Release data if update extent does not lie within extent
  // We have to do it again because the source may have modified our
  // UpdateExtent during propagation.
  this->ModifyExtentForUpdateExtent();
}

//----------------------------------------------------------------------------

void vtkDataObject::TriggerAsynchronousUpdate()
{
  // If we need to update due to PipelineMTime, or the fact that our
  // data was released, then propagate the trigger to the source
  // if there is one.
  if ( this->UpdateTime < this->PipelineMTime || this->DataReleased )
    {
    if (this->Source)
      {
      this->Source->TriggerAsynchronousUpdate();
      }
    }
}

//----------------------------------------------------------------------------

void vtkDataObject::UpdateData()
{
  // If we need to update due to PipelineMTime, or the fact that our
  // data was released, then propagate the UpdateData to the source
  // if there is one.
  if ( this->UpdateTime < this->PipelineMTime || this->DataReleased )
    {
    if (this->Source)
      {
      this->Source->UpdateData(this);
      } 
    } 
}

//----------------------------------------------------------------------------

unsigned long vtkDataObject::GetEstimatedPipelineMemorySize()
{
  unsigned long sizes[3];
  unsigned long memorySize = 0;

  if (this->Source)
    {
    this->Source->ComputeEstimatedPipelineMemorySize( this, sizes );
    memorySize = sizes[2];
    } 

  return memorySize;
} 

//----------------------------------------------------------------------------

void vtkDataObject::ComputeEstimatedPipelineMemorySize(unsigned long sizes[3])
{
  if (this->Source)
    {
    this->Source->ComputeEstimatedPipelineMemorySize( this, sizes );
    } 
}

//----------------------------------------------------------------------------

unsigned long vtkDataObject::GetEstimatedMemorySize()
{
  // This should be implemented in a subclass. If not, default to
  // estimating that no memory is used.
  return 0;
}

//----------------------------------------------------------------------------

void vtkDataObject::SetUpdateExtent( int x1, int x2, 
				     int y1, int y2, 
				     int z1, int z2 )
{
  this->UpdateExtent[0] = x1;
  this->UpdateExtent[1] = x2;
  this->UpdateExtent[2] = y1;
  this->UpdateExtent[3] = y2;
  this->UpdateExtent[4] = z1;
  this->UpdateExtent[5] = z2;
}

//----------------------------------------------------------------------------

void vtkDataObject::SetUpdateExtent( int ext[6] )
{
  memcpy( this->UpdateExtent, ext, 6*sizeof(int) );
}

//----------------------------------------------------------------------------

void vtkDataObject::SetSource(vtkSource *arg)
{
  vtkDebugMacro( << this->GetClassName() << " (" 
                 << this << "): setting Source to " << arg ); 

  if (this->Source != arg) 
    {
    vtkSource *tmp = this->Source;
    this->Source = arg; 
    if (this->Source != NULL) 
      { 
      this->Source->Register(this); 
      } 
    if (tmp != NULL) 
      { 
      tmp->UnRegister(this); 
      }
    this->Modified(); 
    } 
}


//----------------------------------------------------------------------------
void vtkDataObject::UnRegister(vtkObject *o)
{
  // detect the circular loop source <-> data
  // If we have two references and one of them is my data
  // and I am not being unregistered by my data, break the loop.
  if (this->ReferenceCount == 2 && this->Source != NULL &&
      o != this->Source && this->Source->InRegisterLoop(this))
    {
    this->SetSource(NULL);
    }
  
  this->vtkObject::UnRegister(o);
}

//----------------------------------------------------------------------------
unsigned long vtkDataObject::GetUpdateTime()
{
  return this->UpdateTime.GetMTime();
}

//----------------------------------------------------------------------------
void vtkDataObject::SetUpdateExtentToWholeExtent()
{
  switch ( this->GetExtentType() )
    {
    // Our update extent will be the first piece of one piece (the whole thing)
    case VTK_PIECES_EXTENT:
      this->UpdateNumberOfPieces  = 1;
      this->UpdatePiece           = 0;
      break;

    // Our update extent will be the whole extent
    case VTK_3D_EXTENT:
      memcpy( this->UpdateExtent, this->WholeExtent, 6*sizeof(int) );
      break;

    // We should never have this case occur
    default:
      vtkErrorMacro( << "Internal error - invalid extent type!" );
      break;
    }
}

//----------------------------------------------------------------------------

int vtkDataObject::VerifyUpdateExtent()
{
  int retval = 1;

  switch ( this->GetExtentType() )
    {
    // Are we asking for more pieces than we can get?
    case VTK_PIECES_EXTENT:
      if ( this->UpdateNumberOfPieces > this->MaximumNumberOfPieces )
	{
	vtkErrorMacro( << "Cannot break object into " <<
	               this->UpdateNumberOfPieces << ". The limit is " <<
	               this->MaximumNumberOfPieces );
	retval = 0;
	}

      if ( this->UpdatePiece >= this->UpdateNumberOfPieces ||
	   this->UpdatePiece < 0 )
	{
	  vtkErrorMacro( << "Invalid update piece " << this->UpdatePiece
	                 << ". Must be between 0 and " 
	                 << this->UpdateNumberOfPieces - 1);
	retval = 0;
	}
      break;

    // Is our update extent within the whole extent?
    case VTK_3D_EXTENT:
      if ( this->UpdateExtent[0] < this->WholeExtent[0] ||
	   this->UpdateExtent[1] > this->WholeExtent[1] ||
	   this->UpdateExtent[2] < this->WholeExtent[2] ||
	   this->UpdateExtent[3] > this->WholeExtent[3] ||
	   this->UpdateExtent[4] < this->WholeExtent[4] ||
	   this->UpdateExtent[5] > this->WholeExtent[5] )
	{
	vtkErrorMacro( << "Update extent does not lie within whole extent" );
	retval = 0;
	}
      break;

    // We should never have this case occur
    default:
      vtkErrorMacro( << "Internal error - invalid extent type!" );
      break;
    }

  return retval;
}

//----------------------------------------------------------------------------

void vtkDataObject::ModifyExtentForUpdateExtent()
{
  switch ( this->GetExtentType() )
    {
    // Release data if the piece and number of pieces does not match the
    // update piece and update number of pieces
    case VTK_PIECES_EXTENT:
      if ( this->UpdatePiece != this->Piece ||
	   this->UpdateNumberOfPieces != this->NumberOfPieces )
	{
	this->ReleaseData();
	this->Piece = this->UpdatePiece;
	this->NumberOfPieces = this->UpdateNumberOfPieces;
	}
      break;

    case VTK_3D_EXTENT:
      if ( this->UpdateExtent[0] < this->Extent[0] ||
	   this->UpdateExtent[1] > this->Extent[1] ||
	   this->UpdateExtent[2] < this->Extent[2] ||
	   this->UpdateExtent[3] > this->Extent[3] ||
	   this->UpdateExtent[4] < this->Extent[4] ||
	   this->UpdateExtent[5] > this->Extent[5] )
	{
	this->ReleaseData();
	memcpy( this->Extent, this->UpdateExtent, 6*sizeof(int) );
	}
      break;

    // We should never have this case occur
    default:
      vtkErrorMacro( << "Internal error - invalid extent type!" );
      break;
    }
}

//----------------------------------------------------------------------------

unsigned long vtkDataObject::GetActualMemorySize()
{
  return this->FieldData->GetActualMemorySize();
}

//----------------------------------------------------------------------------

void vtkDataObject::CopyInformation( vtkDataObject *data )
{
  if ( this->GetExtentType() == VTK_3D_EXTENT &&
       data->GetExtentType() == VTK_3D_EXTENT )
    {
    memcpy( this->WholeExtent, data->GetWholeExtent(), 6*sizeof(int) );
    }
  else if ( this->GetExtentType() == VTK_PIECES_EXTENT &&
	    data->GetExtentType() == VTK_PIECES_EXTENT )
    {
    this->MaximumNumberOfPieces = data->GetMaximumNumberOfPieces();
    }  
}

//----------------------------------------------------------------------------

void vtkDataObject::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  if ( this->Source )
    {
    os << indent << "Source: " << this->Source << "\n";
    }
  else
    {
    os << indent << "Source: (none)\n";
    }

  os << indent << "Release Data: " 
     << (this->ReleaseDataFlag ? "On\n" : "Off\n");

  os << indent << "Data Released: " 
     << (this->DataReleased ? "True\n" : "False\n");
  
  os << indent << "Global Release Data: " 
     << (vtkDataObjectGlobalReleaseDataFlag ? "On\n" : "Off\n");

  os << indent << "UpdateTime: " << this->UpdateTime << endl;
  
  os << indent << "Field Data:\n";
  this->FieldData->PrintSelf(os,indent.GetNextIndent());
}
