/*=========================================================================

  Program:   Visualization Library
  Module:    TubeF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "TubeF.hh"
#include "AppendP.hh"
#include "RibbonF.hh"

vlTubeFilter::vlTubeFilter()
{
  this->Radius = 0.5;
  this->VaryRadius = 1;
  this->NumberOfSides = 3;
  this->Rotation = 0.0;
}

void vlTubeFilter::Execute()
{
  int i;
  int numSides=this->NumberOfSides;
  vlRibbonFilter **ribbons;
  vlAppendPolyData *appendF;
  vlPoints *inPoints;
  vlCellArray *inLines;
  vlPointData *pd;
  vlPolyData *input=(vlPolyData *)this->Input;

  vlDebugMacro(<<"Creating tube");
  this->Initialize();

  inPoints = input->GetPoints();
  inLines =   input->GetLines();
  pd = input->GetPointData();

  ribbons = new vlRibbonFilter * [numSides];

  if ( numSides == 0 ) //just copy lines through
    {
    this->PointData = *pd;
    this->SetPoints(inPoints);
    this->SetLines(inLines);
    }

  else if ( numSides < 3 ) //create ribbons along line
    {
    appendF = new vlAppendPolyData;
    for (i=0; i < numSides; i++)
      {
      ribbons[i] = new vlRibbonFilter();
      ribbons[i]->SetInput(input);
      ribbons[i]->SetRadius(0.0);
      ribbons[i]->SetAngle(i*90.0);
      appendF->AddInput(ribbons[i]);
      }
    }

  else //create offset ribbons that form a tube
    {
    for (i=0; i < numSides; i++)
      {
      ribbons[i] = new vlRibbonFilter();
      ribbons[i]->SetInput(input);
      ribbons[i]->SetRadius(this->Radius);
      ribbons[i]->SetAngle(this->Rotation + i*(360.0/numSides));
      appendF->AddInput(ribbons[i]);
      }
    }

  appendF->Update(); //cause all ribbons to generate
  this->PointData = *(appendF->GetPointData());
  this->SetPoints(appendF->GetPoints());
  this->SetStrips(appendF->GetStrips());

  delete appendF; //side effect deletes all ribbon filters
  delete [] ribbons;
}

void vlTubeFilter::PrintSelf(ostream& os, vlIndent indent)
{
  vlPolyToPolyFilter::PrintSelf(os,indent);

  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Vary Radius: " << (this->VaryRadius ? "On\n" : "Off\n");
  os << indent << "Number Of Sides: " << this->NumberOfSides << "\n";
  os << indent << "Rotation: " << this->Rotation << "\n";
}

