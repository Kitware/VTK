/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCirclePackFrontChainLayoutStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkCirclePackFrontChainLayoutStrategy.h"
#include "vtkAdjacentVertexIterator.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkTree.h"
#include "vtkMath.h"
#include <vector>
#include <limits>
#include <list>
#include <math.h>

vtkStandardNewMacro(vtkCirclePackFrontChainLayoutStrategy);

class vtkCirclePackFrontChainLayoutStrategyImplementation
{

public:
  vtkCirclePackFrontChainLayoutStrategyImplementation();
  ~vtkCirclePackFrontChainLayoutStrategyImplementation();

  void createCirclePacking(vtkTree* tree,
                           vtkDataArray* sizeArray,
                           vtkDataArray* circlesArray,
                           int height,
                           int width);

  void packTreeNodes(vtkIdType treeNode,
                     double originX,
                     double originY,
                     double enclosingCircleRadius,
                     vtkDataArray* circlesArray,
                     vtkDataArray* sizeArray,
                     vtkTree* tree);

  void packBrotherNodes(std::vector<vtkIdType>& packedNodes,
                        double originX,
                        double originY,
                        double enclosingCircleRadius,
                        vtkDataArray* circlesArray,
                        vtkDataArray* sizeArray,
                        vtkTree* tree);

  void findCircleCenter(vtkIdType Ci,
                        vtkIdType Cm,
                        vtkIdType Cn,
                        vtkDataArray* circlesArray);

  void findIntersectingCircle(vtkIdType Ci,
                              bool& CjAfterCn,
                              std::list<vtkIdType>::iterator& Cj,
                              std::list<vtkIdType>::iterator Cm,
                              std::list<vtkIdType>::iterator Cn,
                              vtkDataArray* circlesArray,
                              std::list<vtkIdType>& frontChain);

  bool validCjAfterCn(vtkIdType Ci,
                      std::list<vtkIdType>::iterator Cm,
                      std::list<vtkIdType>::iterator Cn,
                      vtkDataArray* circlesArray,
                      std::list<vtkIdType>& frontChain,
                      int searchPathLength);

  bool validCjBeforeCm(vtkIdType Ci,
                       std::list<vtkIdType>::iterator Cm,
                       std::list<vtkIdType>::iterator Cn,
                       vtkDataArray* circlesArray,
                       std::list<vtkIdType>& frontChain,
                       int searchPathLength);

  void findCm(double originX,
              double originY,
              vtkDataArray* circlesArray,
              std::list<vtkIdType>::iterator& Cm,
              std::list<vtkIdType>& frontChain);

  void findCn(std::list<vtkIdType>::iterator Cm,
              std::list<vtkIdType>::iterator& Cn,
              std::list<vtkIdType>& frontChain);

  bool circlesIntersect(vtkIdType circleOne,
                        vtkIdType circleTwo,
                        vtkDataArray* circlesArray);

  void deleteSection(std::list<vtkIdType>::iterator circleToStartAt,
                     std::list<vtkIdType>::iterator circleToEndAt,
                     std::list<vtkIdType>& frontChain);

  void incrListIteratorWrapAround(std::list<vtkIdType>::iterator& i,
                                  std::list<vtkIdType>& frontChain);

  void decrListIteratorWrapAround(std::list<vtkIdType>::iterator& i,
                                  std::list<vtkIdType>& frontChain);

private:

};

vtkCirclePackFrontChainLayoutStrategyImplementation::vtkCirclePackFrontChainLayoutStrategyImplementation()
{
}

vtkCirclePackFrontChainLayoutStrategyImplementation::~vtkCirclePackFrontChainLayoutStrategyImplementation()
{
}

void vtkCirclePackFrontChainLayoutStrategyImplementation::incrListIteratorWrapAround(std::list<vtkIdType>::iterator& i,
                                                                                     std::list<vtkIdType>& frontChain)
{
  if(i != frontChain.end())
    ++i;
  else if(i == frontChain.end())
    i = frontChain.begin();
}

void vtkCirclePackFrontChainLayoutStrategyImplementation::decrListIteratorWrapAround(std::list<vtkIdType>::iterator& i,
                                                                                     std::list<vtkIdType>& frontChain)
{
  if(i == frontChain.begin())
    i = frontChain.end();
  else if(!frontChain.empty())
    --i;
}

void vtkCirclePackFrontChainLayoutStrategyImplementation::createCirclePacking(vtkTree* tree,
                                                                              vtkDataArray* sizeArray,
                                                                              vtkDataArray* circlesArray,
                                                                              int height,
                                                                              int width)
{
  double enclosingCircleRadius = height/2.0;
  if(width < height)
    {
    enclosingCircleRadius = width/2.0;
    }

  this->packTreeNodes(tree->GetRoot(),
                      width/2.0,
                      height/2.0,
                      enclosingCircleRadius,
                      circlesArray,
                      sizeArray,
                      tree);

}

void vtkCirclePackFrontChainLayoutStrategyImplementation::packTreeNodes(vtkIdType treeNode,
                                                                        double originX,
                                                                        double originY,
                                                                        double enclosingCircleRadius,
                                                                        vtkDataArray* circlesArray,
                                                                        vtkDataArray* sizeArray,
                                                                        vtkTree* tree)
{

  if(tree->IsLeaf(treeNode))
    {
    return;
    }
  else
    {
    if(tree->GetRoot() == treeNode)
      {
      double circle[3];
      circle[0] = originX;
      circle[1] = originY;
      circle[2] = enclosingCircleRadius;
      circlesArray->SetTuple(treeNode,
                             circle);
      }
    std::vector<vtkIdType> childNodesPackList;
    for(int i=0;i<tree->GetNumberOfChildren(treeNode);i++)
      {
      childNodesPackList.push_back(tree->GetChild(treeNode,i));
      }
    this->packBrotherNodes(childNodesPackList,
                           originX,
                           originY,
                           enclosingCircleRadius,
                           circlesArray,
                           sizeArray,
                           tree);
    }
}

void vtkCirclePackFrontChainLayoutStrategyImplementation::packBrotherNodes(std::vector<vtkIdType>& packedNodes,
                                                                           double originX,
                                                                           double originY,
                                                                           double enclosingCircleRadius,
                                                                           vtkDataArray* circlesArray,
                                                                           vtkDataArray* sizeArray,
                                                                           vtkTree* tree)
{

  if(!packedNodes.size())
    {
    return;
    }

  std::list<vtkIdType> frontChain;
  double circle[3]; // circle[0] is x,
                    // circle[1] is y,
                    // and circle[2] is the radius

  // Handle only one node
  if(packedNodes.size() == 1)
    {
    frontChain.push_back(packedNodes[0]);
    circle[0] = 0.0;
    circle[1] = 0.0;
    circle[2] = sizeArray->GetTuple1(packedNodes[0]);
    circlesArray->SetTuple(packedNodes[0],
                           circle);
    }
  // Handle two nodes, align circles along x-axis
  else if(packedNodes.size() == 2)
    {
    frontChain.push_back(packedNodes[0]);
    circle[0] = 0.0 - sizeArray->GetTuple1(packedNodes[0]);
    circle[1] = 0.0;
    circle[2] = sizeArray->GetTuple1(packedNodes[0]);
    circlesArray->SetTuple(packedNodes[0],
                           circle);

    frontChain.push_back(packedNodes[1]);
    circle[0] = 0.0 + sizeArray->GetTuple1(packedNodes[1]);
    circle[1] = 0.0;
    circle[2] = sizeArray->GetTuple1(packedNodes[1]);
    circlesArray->SetTuple(packedNodes[1],
                           circle);
    }
  else
    {
    // Base case, find initial front-chain for first three nodes
    double frad = sizeArray->GetTuple1(packedNodes[0]);
    double srad = sizeArray->GetTuple1(packedNodes[1]);
    double trad = sizeArray->GetTuple1(packedNodes[2]);

    // Initially align first two circles along x-axis
    frontChain.push_back(packedNodes[0]);
    circle[0] = 0.0 - frad;
    circle[1] = 0.0;
    circle[2] = frad;
    circlesArray->SetTuple(packedNodes[0],
                           circle);

    frontChain.push_back(packedNodes[1]);
    circle[0] = 0.0 + srad;
    circle[1] = 0.0;
    circle[2] = srad;
    circlesArray->SetTuple(packedNodes[1],
                           circle);

    circle[0] = 0.0;
    circle[1] = 0.0;
    circle[2] = trad;
    circlesArray->SetTuple(packedNodes[2],
                           circle);

    this->findCircleCenter(packedNodes[2],
                           packedNodes[0],
                           packedNodes[1],
                           circlesArray);
    frontChain.insert(--(frontChain.end()),packedNodes[2]);

    // Adjust the three circle centers so that they are centered around the origin point.
    // First, find the radius of the interior Soddy circle.  This is the circle that is
    // tangent to all three circles in the interior space defined by the circles.
    // Simple formula, just google for Soddy circle.  We take the positive solution, which is
    // the interior Soddy circle.
    double r1 = frad;
    double r2 = srad;
    double r3 = trad;
    double soddyRad = (r1*r2*r3)/((r2*r3) + (r1*r2) + (r1*r3) + 2.0*sqrt(r1*r2*r3*(r1 + r2 + r3)));
    // Use the Law of Cosines again to find the angle between the first circle center and the center
    // of the Soddy circle.
    double angle = acos((-pow(srad+soddyRad,2) + pow(frad+soddyRad,2) + pow(frad+srad,2))/(2.0*(frad+soddyRad)*(frad+srad)));
    // Find x and y adjustments
    double yAdjust = (frad+soddyRad)*sin(angle);
    double xAdjust = (frad+soddyRad)*cos(angle);
    double c0[3];
    double c1[3];
    double c2[3];
    circlesArray->GetTuple(packedNodes[0],
                           c0);
    circlesArray->GetTuple(packedNodes[1],
                           c1);
    circlesArray->GetTuple(packedNodes[2],
                           c2);
    c0[1] -= yAdjust;
    c1[1] -= yAdjust;
    c2[1] -= yAdjust;
    if(xAdjust > frad)
      {
      c0[0] -= (xAdjust-frad);
      c1[0] -= (xAdjust-frad);
      c2[0] -= (xAdjust-frad);
      }
    else
      {
      c0[0] += (frad - xAdjust);
      c1[0] += (frad - xAdjust);
      c2[0] += (frad - xAdjust);
      }
    circlesArray->SetTuple(packedNodes[0],
                           c0);
    circlesArray->SetTuple(packedNodes[1],
                           c1);
    circlesArray->SetTuple(packedNodes[2],
                           c2);


    // Iterate over the remaining brother nodes to find circle packing
    std::list<vtkIdType>::iterator Cm;
    std::list<vtkIdType>::iterator Cn;
    this->findCm(0.0,
                 0.0,
                 circlesArray,
                 Cm,
                 frontChain);
    this->findCn(Cm,
                 Cn,
                 frontChain);

    for(int i = 3;i < (int) packedNodes.size();i++)
      {
      circle[0] = 0.0;
      circle[1] = 0.0;
      circle[2] = sizeArray->GetTuple1(packedNodes[i]);
      circlesArray->SetTuple(packedNodes[i],
                             circle);

      std::list<vtkIdType>::iterator Cj;
      bool CjAfterCn;
      this->findIntersectingCircle(packedNodes[i],
                                   CjAfterCn,
                                   Cj,
                                   Cm,
                                   Cn,
                                   circlesArray,
                                   frontChain);

      while(Cj != frontChain.end())
        {
        // We have an intersection, so handle one of the two cases.
        // Cj is greater than Cn on the front chain
        if(CjAfterCn)
          {
          this->deleteSection(Cm,
                              Cj,
                              frontChain);
          Cn = Cj;
          }
        // Cj is less than Cm on the front chain
        else
          {
          this->deleteSection(Cj,
                              Cn,
                              frontChain);
          Cm = Cj;
          }

        this->findIntersectingCircle(packedNodes[i],
                                     CjAfterCn,
                                     Cj,
                                     Cm,
                                     Cn,
                                     circlesArray,
                                     frontChain);

        }

      // First case, there is no intersection so just insert Ci between Cm and Cn
      std::list<vtkIdType>::iterator ti = Cm;
      incrListIteratorWrapAround(ti, frontChain);
      frontChain.insert(ti,packedNodes[i]);
      this->findCn(Cm,
                   Cn,
                   frontChain);

      }

    }

  // Scale circle layout to fit within the enclosing circle radius
  double Xfc = 0.0;
  double Yfc = 0.0;
  std::list<vtkIdType>::iterator it;
  for(it = frontChain.begin();it != frontChain.end();++it)
    {
    circlesArray->GetTuple(*it,
                            circle);
    Xfc += circle[0];
    Yfc += circle[1];
    }

  Xfc = Xfc/((double) frontChain.size());
  Yfc = Yfc/((double) frontChain.size());

  double layoutRadius = 0;
  for(it = frontChain.begin();it != frontChain.end();++it)
    {
    circlesArray->GetTuple(*it,
                           circle);
    double distance = sqrt(pow(circle[0] - Xfc,2) +
                           pow(circle[1] - Yfc,2)) +
                           circle[2];
    if(distance > layoutRadius)
      {
      layoutRadius = distance;
      }
    }

  double scaleFactor = enclosingCircleRadius/layoutRadius;
  if (layoutRadius == 0) scaleFactor = 1;
  // Scale and translate each circle
  for(int i = 0;i < (int) packedNodes.size();i++)
    {
    circlesArray->GetTuple(packedNodes[i],
                           circle);
    circle[0] = (circle[0] - Xfc)*scaleFactor + originX;
    circle[1] = (circle[1] - Yfc)*scaleFactor + originY;
    circle[2] = circle[2]*scaleFactor;
    circlesArray->SetTuple(packedNodes[i],
                           circle);

    }

  // Now that each circle at this level is positioned and scaled,
  // find the layout for the children of each circle and add the
  // result to the packing list.
  for(int i = 0;i < (int) packedNodes.size();i++)
    {
    circlesArray->GetTuple(packedNodes[i], circle);
    this->packTreeNodes(packedNodes[i],
                        circle[0],
                        circle[1],
                        circle[2],
                        circlesArray,
                        sizeArray,
                        tree);
    }

}

bool vtkCirclePackFrontChainLayoutStrategyImplementation::validCjAfterCn(vtkIdType Ci,
                                                                         std::list<vtkIdType>::iterator Cm,
                                                                         std::list<vtkIdType>::iterator Cn,
                                                                         vtkDataArray* circlesArray,
                                                                         std::list<vtkIdType>& frontChain,
                                                                         int searchPathLength)
{
  this->findCircleCenter(Ci,
                         *Cm,
                         *Cn,
                         circlesArray);

  int CnSearchPathLength = 0;
  while(CnSearchPathLength < searchPathLength)
    {
    CnSearchPathLength++;
    decrListIteratorWrapAround(Cn, frontChain);
    if(Cn == frontChain.end())
      {
      decrListIteratorWrapAround(Cn, frontChain);
      }
    if(this->circlesIntersect(Ci,
                              *Cn,
                              circlesArray))
      {
      return false;
      }
    }

  return true;
}

bool vtkCirclePackFrontChainLayoutStrategyImplementation::validCjBeforeCm(vtkIdType Ci,
                                                                          std::list<vtkIdType>::iterator Cm,
                                                                          std::list<vtkIdType>::iterator Cn,
                                                                          vtkDataArray* circlesArray,
                                                                          std::list<vtkIdType>& frontChain,
                                                                          int searchPathLength)
{
  this->findCircleCenter(Ci,
                         *Cm,
                         *Cn,
                         circlesArray);

  int CmSearchPathLength = 0;
  while(CmSearchPathLength < searchPathLength)
    {
    CmSearchPathLength++;
    incrListIteratorWrapAround(Cm, frontChain);
    if(Cm == frontChain.end())
      {
      incrListIteratorWrapAround(Cm, frontChain);
      }
    if(this->circlesIntersect(Ci,
                              *Cm,
                              circlesArray))
      {
      return false;
      }
    }
  return true;
}

void vtkCirclePackFrontChainLayoutStrategyImplementation::findIntersectingCircle(vtkIdType Ci,
                                                                                 bool& CjAfterCn,
                                                                                 std::list<vtkIdType>::iterator& Cj,
                                                                                 std::list<vtkIdType>::iterator Cm,
                                                                                 std::list<vtkIdType>::iterator Cn,
                                                                                 vtkDataArray* circlesArray,
                                                                                 std::list<vtkIdType>& frontChain)
{
  // Start from Cn and look for the first intersection Cj until we have searched half
  // of the frontChain.  Do the same for Cm.  Compare the Cm and Cn search lengths and
  // take the shortest, if any intersecting Cj was found.
  std::list<vtkIdType>::iterator CjfromCn = frontChain.end();
  std::list<vtkIdType>::iterator CjfromCm = frontChain.end();
  std::list<vtkIdType>::iterator lCm = Cm;
  std::list<vtkIdType>::iterator lCn = Cn;
  int fcl = (int) frontChain.size();
  int searchPathLength = (int) ceil(((double) fcl - 2.0)/2.0);

  this->findCircleCenter(Ci,
                         *Cm,
                         *Cn,
                         circlesArray);

  int CnSearchPathLength = 0;
  while(CnSearchPathLength < searchPathLength)
    {
    CnSearchPathLength++;
    incrListIteratorWrapAround(lCn, frontChain);
    if(lCn == frontChain.end())
      incrListIteratorWrapAround(lCn, frontChain);

    if(this->circlesIntersect(Ci,
                              *lCn,
                              circlesArray))
      {
      CjfromCn = lCn;
      break;
      }
    }

  // There is an intersection.  Check
  // to see if the front chain is clear
  // to be deleted from Cm to Cj.
  if(CjfromCn != frontChain.end())
    {
    Cj = CjfromCn;
    if(this->validCjAfterCn(Ci,
                            Cm,
                            lCn,
                            circlesArray,
                            frontChain,
                            CnSearchPathLength))
      {
      CjAfterCn = true;
      }
    else
      {
      CjAfterCn = false;
      }
    return;
    }

  int CmSearchPathLength = 0;
  while(CmSearchPathLength < searchPathLength)
   {
   CmSearchPathLength++;
   decrListIteratorWrapAround(lCm, frontChain);
   if(lCm == frontChain.end())
     decrListIteratorWrapAround(lCm, frontChain);

   if(this->circlesIntersect(Ci,
                             *lCm,
                             circlesArray))
     {
     CjfromCm = lCm;
     break;
     }
    }

  // There is an intersection.  Check
  // to see if the front chain is clear
  // to be deleted from Cj to Cn.
  if(CjfromCm != frontChain.end())
    {
    Cj = CjfromCm;
    if(this->validCjBeforeCm(Ci,
                             lCm,
                             Cn,
                             circlesArray,
                             frontChain,
                             CmSearchPathLength))
      {
      CjAfterCn = false;
      }
    else
      {
      CjAfterCn = true;
      }
    return;
    }

  // No intersection found
  Cj = frontChain.end();
  CjAfterCn = false;
  return;
}

void vtkCirclePackFrontChainLayoutStrategyImplementation::findCircleCenter(vtkIdType Ci,
                                                                           vtkIdType Cm,
                                                                           vtkIdType Cn,
                                                                           vtkDataArray* circlesArray)
{

  double circle[3];
  circlesArray->GetTuple(Cm,circle);
  double rCm = circle[2];
  double xCm = circle[0];
  double yCm = circle[1];

  circlesArray->GetTuple(Cn,circle);
  double rCn = circle[2];
  double xCn = circle[0];
  double yCn = circle[1];

  circlesArray->GetTuple(Ci,circle);
  double rCi = circle[2];
  double xCi = circle[0];
  double yCi = circle[1];

  // Find the angle as measured from the x-axis to the line segment between rCm and rCn, with the origin
  // at rCm.
  double xc = xCn - xCm;
  double yc = yCn - yCm;
  double xAxisAngle = atan2(yc,xc);
  // Find positive rotation angle
  if (xAxisAngle < 0)
    {
    xAxisAngle = vtkMath::Pi() + (vtkMath::Pi() + xAxisAngle);
    }

  // Find the distance between the centers of Cm and Cn
  double CmCnDistance = sqrt(pow(xCn - xCm,2) + pow(yCn - yCm,2));

  // Find an interior angle of the triangle defined by all three circle centers using the Law of Cosines
  double angle = acos((-pow(rCn+rCi,2) + pow(rCm+rCi,2) + pow(CmCnDistance,2))/(2.0*(rCm+rCi)*(CmCnDistance)));

  // Find the center of the third triangle by using the interior angle and the length of the triangle side
  xCi = (rCm+rCi)*cos(angle);
  yCi = (rCm+rCi)*sin(angle);

  // Rotate the center of Ci from the x-axis by xAxisAngle
  double x = xCi;
  double y = yCi;
  xCi = x*cos(xAxisAngle) - y*sin(xAxisAngle);
  yCi = x*sin(xAxisAngle) + y*cos(xAxisAngle);
  xCi += xCm;
  yCi += yCm;

  circlesArray->GetTuple(Ci,circle);
  circle[0] = xCi;
  circle[1] = yCi;
  circlesArray->SetTuple(Ci,circle);

}

void vtkCirclePackFrontChainLayoutStrategyImplementation::findCm(double originX,
                                                                 double originY,
                                                                 vtkDataArray* circlesArray,
                                                                 std::list<vtkIdType>::iterator& Cm,
                                                                 std::list<vtkIdType>& frontChain)
{
  std::list<vtkIdType>::iterator it = frontChain.begin();
  Cm = it;
  double circle[3];
  double minDistance = 0.0;
  if(!frontChain.empty())
    {
    circlesArray->GetTuple(*it,circle);
    minDistance = pow(circle[0] - originX,2) + pow(circle[1] - originY,2);
    ++it;
    }

  for(; it != frontChain.end(); ++it)
    {
    circlesArray->GetTuple(*it,circle);
    double distanceSq = pow(circle[0] - originX,2) + pow(circle[1] - originY,2);

    if(distanceSq < minDistance)
      {
      Cm = it;
      minDistance = distanceSq;
      }
    }
}

void vtkCirclePackFrontChainLayoutStrategyImplementation::findCn(std::list<vtkIdType>::iterator Cm,
                                                                 std::list<vtkIdType>::iterator& Cn,
                                                                 std::list<vtkIdType>& frontChain)
{
  incrListIteratorWrapAround(Cm, frontChain);
  if(Cm == frontChain.end())
    {
    Cn = frontChain.begin();
    }
  else
    {
    Cn = Cm;
    }
}

bool vtkCirclePackFrontChainLayoutStrategyImplementation::circlesIntersect(vtkIdType circleOne,
                                                                           vtkIdType circleTwo,
                                                                           vtkDataArray* circlesArray)
{
  double c1[3];
  double c2[3];
  circlesArray->GetTuple(circleOne,c1);
  circlesArray->GetTuple(circleTwo,c2);

  double distanceSq = pow(c1[0]- c2[0],2) + pow(c1[1] - c2[1],2);

  if(distanceSq > pow(c1[2] + c2[2], 2))
    {
    return false;
    }
  else
    {
    return true;
    }
}

// Delete all circles out of fronChain from circleToStartAt to circleToEndAt, not including circleToStartAt and circleToEndAt.
// Insert all deleted elements into circlePacking list.
void vtkCirclePackFrontChainLayoutStrategyImplementation::deleteSection(std::list<vtkIdType>::iterator circleToStartAt,
                                                                        std::list<vtkIdType>::iterator circleToEndAt,
                                                                        std::list<vtkIdType>& frontChain)
{
  incrListIteratorWrapAround(circleToStartAt, frontChain);
  while ( (circleToStartAt != frontChain.end())&&(circleToStartAt != circleToEndAt) )
    {
    circleToStartAt = frontChain.erase(circleToStartAt);
    }

  if(circleToStartAt != circleToEndAt)
    {
    circleToStartAt = frontChain.begin();
    while ( (circleToStartAt != frontChain.end())&&(circleToStartAt != circleToEndAt) )
      {
      circleToStartAt = frontChain.erase(circleToStartAt);
      }
    }
}

vtkCirclePackFrontChainLayoutStrategy::vtkCirclePackFrontChainLayoutStrategy()
{
  this->Width = 1;
  this->Height = 1;
  this->pimpl = new vtkCirclePackFrontChainLayoutStrategyImplementation;
}

vtkCirclePackFrontChainLayoutStrategy::~vtkCirclePackFrontChainLayoutStrategy()
{
  delete this->pimpl;
  this->pimpl=0;
}

void vtkCirclePackFrontChainLayoutStrategy::Layout(vtkTree *inputTree,
                                                   vtkDataArray *areaArray,
                                                   vtkDataArray* sizeArray)
{

    this->pimpl->createCirclePacking(inputTree,
                                     sizeArray,
                                     areaArray,
                                     this->Height,
                                     this->Width);

}

void vtkCirclePackFrontChainLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Width: " << this->Width << endl;
  os << indent << "Height: " << this->Height << endl;
}
