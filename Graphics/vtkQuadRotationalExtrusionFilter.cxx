/*=========================================================================

Program:   Visualization Toolkit
Module:    $RCSfile: vtkQuadRotationalExtrusionFilter.cxx,v $

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkQuadRotationalExtrusionFilter.h"

#include "vtkCellArray.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPolyData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkMultiBlockDataSet.h"

vtkStandardNewMacro(vtkQuadRotationalExtrusionFilter);

// ----------------------------------------------------------------------
// Create object with capping on, angle of 360 degrees, resolution = 12, and
// no translation along z-axis.
// vector (0,0,1), and point (0,0,0).
vtkQuadRotationalExtrusionFilter::vtkQuadRotationalExtrusionFilter()
{
  this->Axis = 2;
  this->Capping = 1;
  this->DefaultAngle = 360.0;
  this->DeltaRadius = 0.0;
  this->Translation = 0.0;
  this->Resolution = 12; // 30 degree increments
}

// ----------------------------------------------------------------------
int vtkQuadRotationalExtrusionFilter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");

  return 1;
}

// ----------------------------------------------------------------------
int vtkQuadRotationalExtrusionFilter::RequestData(
                                                  vtkInformation *vtkNotUsed(request),
                                                  vtkInformationVector **inputVector,
                                                  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get composite input
  vtkCompositeDataSet * compositeInput = vtkCompositeDataSet::SafeDownCast(
                                                                           inInfo->Get(vtkDataObject::DATA_OBJECT()) );

  // get typed output
  vtkMultiBlockDataSet * compositeOutput = vtkMultiBlockDataSet::SafeDownCast(
                                                                              outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if(compositeInput==0 || compositeOutput==0)
    {
    vtkErrorMacro(<<"Invalid algorithm connection\n");
    return 0;
    }

  vtksys_stl::map<int,vtkDataSet*> outputBlocks;

  vtkDebugMacro(<<"input="<<compositeInput->GetClassName()<<"\n");
  //   if( compositeInput != 0 )
  //   {
  //      compositeInput->PrintSelf( std::cout, vtkIndent(1) );
  //   }

  // alocate composite iterator
  vtkCompositeDataIterator * inputIterator = vtkCompositeDataIterator::New();
  inputIterator->SetDataSet ( compositeInput );
  inputIterator->VisitOnlyLeavesOn();
  inputIterator->SkipEmptyNodesOn();
  inputIterator->InitTraversal();
  inputIterator->GoToFirstItem();

  while (inputIterator->IsDoneWithTraversal() == 0)
    {
    // get the input and ouptut
    vtkPolyData *input = vtkPolyData::SafeDownCast( inputIterator->GetCurrentDataObject() );
    vtkPolyData *output = vtkPolyData::New();
    int blockId = inputIterator->GetCurrentFlatIndex();
    inputIterator->GoToNextItem();

    vtkIdType numPts, numCells;
    numPts = input->GetNumberOfPoints();
    numCells = input->GetNumberOfCells();

    if (numPts > 0 && numCells > 0)
      {
      // recuperation de l'angle pour chaque block, ou l'angle par defaut
      double blockAngle = this->GetDefaultAngle();
      vtkDebugMacro(<<"DefaultAngle="<<blockAngle<<"\n");

      vtksys_stl::map<vtkIdType,double>::iterator amit = this->PerBlockAngles.find( blockId );
      if( amit != this->PerBlockAngles.end() )
        {
        vtkDebugMacro(<<"Found angle "<<amit->second<<" for block "<<blockId<<"\n");
        blockAngle = amit->second;
        }

      vtkDebugMacro(<<"process block "<<blockId<<", angle="<<blockAngle<<", resolution="<<this->Resolution<<", obj="<<output<<"\n");

      vtkPointData *pd=input->GetPointData();
      vtkCellData *cd=input->GetCellData();
      vtkPolyData *mesh;
      vtkPoints *inPts;
      vtkCellArray *inVerts, *inLines, *inPolys, *inStrips;
      int numEdges;
      vtkIdType *pts = 0;
      vtkIdType npts = 0;
      vtkIdType cellId, ptId, ncells;
      double x[3], newX[3], radius, angleIncr, radIncr, transIncr;
      double psi, theta;
      vtkPoints *newPts;
      vtkCellArray *newLines=NULL, *newPolys, *newStrips=NULL;
      vtkCell *edge;
      vtkIdList *cellIds;
      int i, j, k;
      vtkIdType p1, p2;
      vtkPointData *outPD=output->GetPointData();
      vtkCellData *outCD=output->GetCellData();
      double tempd;
      int abort=0;

      // Initialize / check input
      //
      vtkDebugMacro(<<"Rotationally extruding data");


      // Build cell data structure.
      //
      mesh = vtkPolyData::New();
      inPts = input->GetPoints();
      inVerts = input->GetVerts();
      inLines = input->GetLines();
      inPolys = input->GetPolys();
      inStrips = input->GetStrips();
      mesh->SetPoints(inPts);
      mesh->SetVerts(inVerts);
      mesh->SetLines(inLines);
      mesh->SetPolys(inPolys);
      mesh->SetStrips(inStrips);
      if ( inPolys || inStrips )
        {
        mesh->BuildLinks();
        }

      // Allocate memory for output. We don't copy normals because surface geometry
      // is modified.
      //
      outPD->CopyNormalsOff();
      outPD->CopyAllocate(pd,(this->Resolution+1)*numPts);
      newPts = vtkPoints::New();
      newPts->Allocate((this->Resolution+1)*numPts);
      if ( (ncells=inVerts->GetNumberOfCells()) > 0 ) 
        {
        newLines = vtkCellArray::New();
        newLines->Allocate(newLines->EstimateSize(ncells,this->Resolution+1));
        }
      // arbitrary initial allocation size
      ncells = inLines->GetNumberOfCells() + inPolys->GetNumberOfCells()/10 +
        inStrips->GetNumberOfCells()/10;
      ncells = (ncells < 100 ? 100 : ncells);
      newPolys = vtkCellArray::New();
      newPolys->Allocate(newPolys->EstimateSize(ncells,2*(this->Resolution+1)));
      outCD->CopyNormalsOff();
      outCD->CopyAllocate(cd,ncells);

      // copy points
      for (ptId=0; ptId < numPts; ptId++) //base level
        {
        newPts->InsertPoint(ptId,inPts->GetPoint(ptId));
        outPD->CopyData(pd,ptId,ptId);
        }
      this->UpdateProgress(0.1);

      radIncr = this->DeltaRadius / this->Resolution;
      transIncr = this->Translation / this->Resolution;
      angleIncr = vtkMath::RadiansFromDegrees(blockAngle) / this->Resolution;
      // rotation around z-axis
      if (Axis == 2)
        {
        for ( i = 1; i <= this->Resolution; i++ )
          {
          this->UpdateProgress(0.1 + 0.5*(i-1)/this->Resolution);
          for (ptId=0; ptId < numPts; ptId++)
            {
            inPts->GetPoint(ptId, x);
            //convert to cylindrical
            radius = sqrt(x[0]*x[0] + x[1]*x[1]);
            if (radius > 0.0)
              {
              tempd = (double)x[0]/radius;
              if (tempd < -1.0)
                {
                tempd = -1.0;
                }
              if (tempd > 1.0)
                {
                tempd = 1.0;
                }
              theta = acos(tempd);
              tempd = (double)x[1]/radius;
              if (tempd < -1.0)
                {
                tempd = -1.0;
                }
              if (tempd > 1.0)
                {
                tempd = 1.0;
                }
              if ( (psi=asin(tempd)) < 0.0 ) 
                {
                if ( theta < (vtkMath::Pi()/2.0) )
                  {
                  theta = 2.0*vtkMath::Pi() + psi;
                  }
                else
                  {
                  theta = vtkMath::Pi() - psi;
                  }
                }
              
              //increment angle
              radius += i*radIncr;
              newX[0] = radius * cos (i*angleIncr + theta);
              newX[1] = radius * sin (i*angleIncr + theta);
              newX[2] = x[2] + i * transIncr;
              }
            else // radius is zero
              {
              newX[0] = 0.0;
              newX[1] = 0.0;
              newX[2] = x[2] + i * transIncr;
              }
            newPts->InsertPoint(ptId+i*numPts,newX);
            outPD->CopyData(pd,ptId,ptId+i*numPts);
            }
          }
        }
      // rotation  around y-axis
      else if (Axis == 1)
        {
        for ( i = 1; i <= this->Resolution; i++ )
          {
          this->UpdateProgress(0.1 + 0.5*(i-1)/this->Resolution);
          for (ptId=0; ptId < numPts; ptId++)
            {
            inPts->GetPoint(ptId, x);
            //convert to cylindrical
            radius = sqrt(x[0]*x[0] + x[2]*x[2]);
            if (radius > 0.0)
              {
              tempd = (double)x[0]/radius;
              if (tempd < -1.0)
                {
                tempd = -1.0;
                }
              if (tempd > 1.0)
                {
                tempd = 1.0;
                }
              theta = acos(tempd);
              tempd = (double)x[2]/radius;
              if (tempd < -1.0)
                {
                tempd = -1.0;
                }
              if (tempd > 1.0)
                {
                tempd = 1.0;
                }
              if ( (psi=asin(tempd)) < 0.0 ) 
                {
                if ( theta < (vtkMath::Pi()/2.0) )
                  {
                  theta = 2.0*vtkMath::Pi() + psi;
                  }
                else
                  {
                  theta = vtkMath::Pi() - psi;
                  }
                }
              
              //increment angle
              radius += i*radIncr;
              newX[0] = radius * cos (i*angleIncr + theta);
              newX[1] = x[1] + i * transIncr;
              newX[2] = radius * sin (i*angleIncr + theta);
              }
            else // radius is zero
              {
              newX[0] = 0.0;
              newX[1] = x[1] + i * transIncr;
              newX[2] = 0.0;
              }
            newPts->InsertPoint(ptId+i*numPts,newX);
            outPD->CopyData(pd,ptId,ptId+i*numPts);
            }
          }
        }
      // rotation around x-axis
      else
        {
        for ( i = 1; i <= this->Resolution; i++ )
          {
          this->UpdateProgress(0.1 + 0.5*(i-1)/this->Resolution);
          for (ptId=0; ptId < numPts; ptId++)
            {
            inPts->GetPoint(ptId, x);
            //convert to cylindrical
            radius = sqrt(x[1]*x[1] + x[2]*x[2]);
            if (radius > 0.0)
              {
              tempd = (double)x[1]/radius;
              if (tempd < -1.0)
                {
                tempd = -1.0;
                }
              if (tempd > 1.0)
                {
                tempd = 1.0;
                }
              theta = acos(tempd);
              tempd = (double)x[2]/radius;
              if (tempd < -1.0)
                {
                tempd = -1.0;
                }
              if (tempd > 1.0)
                {
                tempd = 1.0;
                }
              if ( (psi=asin(tempd)) < 0.0 ) 
                {
                if ( theta < (vtkMath::Pi()/2.0) )
                  {
                  theta = 2.0*vtkMath::Pi() + psi;
                  }
                else
                  {
                  theta = vtkMath::Pi() - psi;
                  }
                }
              
              //increment angle
              radius += i*radIncr;
              newX[0] = x[0] + i * transIncr;
              newX[1] = radius * cos (i*angleIncr + theta);
              newX[2] = radius * sin (i*angleIncr + theta);
              }
            else // radius is zero
              {
              newX[0] = x[0] + i * transIncr;
              newX[1] = 0.0;
              newX[2] = 0.0;
              }
            newPts->InsertPoint(ptId+i*numPts,newX);
            outPD->CopyData(pd,ptId,ptId+i*numPts);
            }
          }
        }


      // To insure that cell attributes are in consistent order with the
      // cellId's, we process the verts, lines, polys and strips in order.
      vtkIdType newCellId=0;
      int type;
      if ( newLines ) // there are verts which produce lines
        {
        for ( cellId=0; cellId < numCells && !abort; cellId++)
          {
          type = mesh->GetCellType(cellId);
          if ( type == VTK_VERTEX || type == VTK_POLY_VERTEX )
            {
            mesh->GetCellPoints(cellId,npts,pts);
            for (i=0; i<npts; i++)
              {
              ptId = pts[i];
              newLines->InsertNextCell(this->Resolution+1);
              for ( j=0; j<=this->Resolution; j++ )
                {
                newLines->InsertCellPoint(ptId + j*numPts);
                }
              outCD->CopyData(cd,cellId,newCellId++);
              }
            }//if a vertex or polyVertex
          }//for all cells
        }//if there are verts generating lines
      this->UpdateProgress (0.25);
      abort = this->GetAbortExecute();
  
      // If capping is on, copy 2D cells to output (plus create cap). Notice
      // that polygons are done first, then strips.
      //
      if ( this->Capping && (blockAngle != 360.0 || this->DeltaRadius != 0.0 
                             || this->Translation != 0.0) )
        {
        if ( inPolys->GetNumberOfCells() > 0 )
          {
          //newPolys = vtkCellArray::New();
          //newPolys->Allocate(inPolys->GetSize());

          for ( cellId=0; cellId < numCells && !abort; cellId++ )
            {
            type = mesh->GetCellType(cellId);
            if ( type == VTK_TRIANGLE || type == VTK_QUAD || type == VTK_POLYGON )
              {
              mesh->GetCellPoints(cellId, npts, pts);
              newPolys->InsertNextCell(npts,pts);
              outCD->CopyData(cd,cellId,newCellId++);
              newPolys->InsertNextCell(npts);
              for (i=0; i < npts; i++)
                {
                newPolys->InsertCellPoint(pts[i] + this->Resolution*numPts);
                }
              outCD->CopyData(cd,cellId,newCellId++);
              }
            }
          }

        if ( inStrips->GetNumberOfCells() > 0 )
          {
          newStrips = vtkCellArray::New();
          newStrips->Allocate(inStrips->GetSize());
       
          for ( cellId=0; cellId < numCells && !abort; cellId++ )
            {
            type = mesh->GetCellType(cellId);
            if ( type == VTK_TRIANGLE_STRIP )
              {
              mesh->GetCellPoints(cellId, npts, pts);
              newStrips->InsertNextCell(npts,pts);
              outCD->CopyData(cd,cellId,newCellId++);
              newStrips->InsertNextCell(npts);
              for (i=0; i < npts; i++)
                {
                newStrips->InsertCellPoint(pts[i] + this->Resolution*numPts);
                }
              outCD->CopyData(cd,cellId,newCellId++);
              }
            }
          }
        }//if capping
      this->UpdateProgress (0.5);
      abort = this->GetAbortExecute();

      // Now process lines, polys and/or strips to produce strips
      //
      if ( inLines->GetNumberOfCells() || inPolys->GetNumberOfCells() ||
           inStrips->GetNumberOfCells() )
        {
        cellIds = vtkIdList::New();
        cellIds->Allocate(VTK_CELL_SIZE);
        vtkGenericCell *cell = vtkGenericCell::New();

        for ( cellId=0; cellId < numCells && !abort; cellId++)
          {
          type = mesh->GetCellType(cellId);
          if ( type == VTK_LINE || type == VTK_POLY_LINE )
            {
            mesh->GetCellPoints(cellId,npts,pts);
            for (i=0; i<(npts-1); i++)
              {
              p1 = pts[i];
              p2 = pts[i+1];
              for (k=0; k<this->Resolution; k++)
                {
                newPolys->InsertNextCell(4);
                newPolys->InsertCellPoint(p1 + k*numPts);
                newPolys->InsertCellPoint(p2 + k*numPts);
                newPolys->InsertCellPoint(p2 + (k+1)*numPts);
                newPolys->InsertCellPoint(p1 + (k+1)*numPts);
                outCD->CopyData(cd,cellId,newCellId++);
                }
              }
            }//if a line
       
          else if ( type == VTK_TRIANGLE || type == VTK_QUAD || 
                    type == VTK_POLYGON || type == VTK_TRIANGLE_STRIP ) 
            {// create strips from boundary edges
            mesh->GetCell(cellId,cell);
            numEdges = cell->GetNumberOfEdges();
            for (i=0; i<numEdges; i++)
              {
              edge = cell->GetEdge(i);
              for (j=0; j<(edge->GetNumberOfPoints()-1); j++)
                {
                p1 = edge->PointIds->GetId(j);
                p2 = edge->PointIds->GetId(j+1);
                mesh->GetCellEdgeNeighbors(cellId, p1, p2, cellIds);
                
                if ( cellIds->GetNumberOfIds() < 1 ) //generate strip
                  {
                  for (k=0; k<this->Resolution; k++)
                    {
                    newPolys->InsertNextCell(4);
                    newPolys->InsertCellPoint(p1 + k*numPts);
                    newPolys->InsertCellPoint(p2 + k*numPts);
                    newPolys->InsertCellPoint(p2 + (k+1)*numPts);
                    newPolys->InsertCellPoint(p1 + (k+1)*numPts);
                    outCD->CopyData(cd,cellId,newCellId++);
                    }
                  } //if boundary edge
                } //for each sub-edge
              } //for each edge
            } //for each polygon or triangle strip
          }//for all cells
    
        cellIds->Delete();
        cell->Delete();
        } //if strips are being generated
      this->UpdateProgress (1.00);

      // Update ourselves and release memory
      //
      output->SetPoints(newPts);
      newPts->Delete();
      mesh->Delete();

      if ( newLines ) 
        {
        output->SetLines(newLines);
        newLines->Delete();
        }
  
      if ( newPolys ) 
        {
        output->SetPolys(newPolys);
        newPolys->Delete();
        }
  
      if ( newStrips ) 
        {
        output->SetStrips(newStrips);
        newStrips->Delete();
        }

      output->Squeeze();

      if( blockId == -1 ) blockId = outputBlocks.size();
      outputBlocks[ blockId ] = output;

      } /* if numPts>0 && numCells>0 */

    } /* Iterate over input blocks */
  inputIterator->Delete();

   
  // build final composite output. also tagging blocks with their associated Id
  compositeOutput->SetNumberOfBlocks( outputBlocks.size() );
  int blockIndex=0;
  for(vtksys_stl::map<int,vtkDataSet*>::iterator it=outputBlocks.begin(); it!=outputBlocks.end(); ++it, ++blockIndex)
    {
    if( it->second->GetNumberOfCells() > 0 )
      {
      compositeOutput->SetBlock( blockIndex , it->second );
      it->second->Delete();
      }
    }

  return 1;
}

// ----------------------------------------------------------------------
void vtkQuadRotationalExtrusionFilter::RemoveAllPerBlockAngles()
{
  vtkDebugMacro(<<"RemoveAllPerBlockAngles\n");
  this->PerBlockAngles.clear();
  this->Modified();
}

// ----------------------------------------------------------------------
void vtkQuadRotationalExtrusionFilter::AddPerBlockAngle(vtkIdType blockId, double angle)
{
  vtkDebugMacro(<<"PerBlockAngles["<<blockId<<"]="<<angle<<"\n");
  this->PerBlockAngles[ blockId ] = angle;
  this->Modified();
}

// ----------------------------------------------------------------------
void vtkQuadRotationalExtrusionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Resolution: " << this->Resolution << "\n";
  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
  os << indent << "DefaultAngle: " << this->DefaultAngle << "\n";
  os << indent << "Translation: " << this->Translation << "\n";
  os << indent << "Delta Radius: " << this->DeltaRadius << "\n";
  os << indent << "PerBlockAngles:\n";
  for(vtksys_stl::map<vtkIdType,double>::iterator it=this->PerBlockAngles.begin();it!=this->PerBlockAngles.end();++it)
    {
    os << indent.GetNextIndent() << "Block #" << it->first << " -> " << it->second << "°\n";
     
    }
}
