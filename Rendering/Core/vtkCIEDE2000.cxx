/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCIEDE2000.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================
The MIT License (MIT)

Copyright (c) 2015 Greg Fiumara

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
=========================================================================*/

#include "vtkCIEDE2000.h"

#include <algorithm> // std::min, std::max
#include <array>
#include <deque>
#include <limits>
#include <set>
#include <utility> // std::pair, std::make_pair
#include <vtkMath.h>

namespace CIEDE2000
{

//----------------------------------------------------------------------------
static const int COLORSPACE_SIZE_X = 17;
static const int COLORSPACE_SIZE_Y = 17;
static const int COLORSPACE_SIZE_Z = 17;

static const int NEIGHBORHOOD_SIZE_X = 1;
static const int NEIGHBORHOOD_SIZE_Y = 1;
static const int NEIGHBORHOOD_SIZE_Z = 1;

typedef int PositionComponent;
typedef std::array<PositionComponent, 3> Position;
typedef double Distance;

//----------------------------------------------------------------------------
inline static void getPosition(const double rgb[3], Position& pos)
{
  pos[0] = static_cast<PositionComponent>(rgb[0] * (COLORSPACE_SIZE_X - 1) + 0.5);
  pos[1] = static_cast<PositionComponent>(rgb[1] * (COLORSPACE_SIZE_Y - 1) + 0.5);
  pos[2] = static_cast<PositionComponent>(rgb[2] * (COLORSPACE_SIZE_Z - 1) + 0.5);
}

//----------------------------------------------------------------------------
inline static void getRGBColor(const Position& pos, double rgb[3])
{
  rgb[0] = pos[0] / static_cast<double>(COLORSPACE_SIZE_X - 1);
  rgb[1] = pos[1] / static_cast<double>(COLORSPACE_SIZE_Y - 1);
  rgb[2] = pos[2] / static_cast<double>(COLORSPACE_SIZE_Z - 1);
}

//----------------------------------------------------------------------------
void MapColor(double rgb[3])
{
  Position pos;
  getPosition(rgb, pos);
  getRGBColor(pos, rgb);
}

//----------------------------------------------------------------------------
inline static void getLabColor(const Position& pos, double _lab[3])
{
  double rgb[3];
  getRGBColor(pos, rgb);

  vtkMath::RGBToLab(rgb, _lab);
}

//----------------------------------------------------------------------------
inline static int getIndex(const Position& pos)
{
  return pos[0] + COLORSPACE_SIZE_X * (pos[1] + COLORSPACE_SIZE_Y * pos[2]);
}

//----------------------------------------------------------------------------
double GetCIEDeltaE2000(const double lab1[3], const double lab2[3])
{
  // The three constants used in the CIEDE2000 measure
  static const double k_L = 1.0;
  static const double k_C = 1.0;
  static const double k_H = 1.0;

  // Calculate and return Delta E

  double C1 = std::sqrt((lab1[1] * lab1[1]) + (lab1[2] * lab1[2]));
  double C2 = std::sqrt((lab2[1] * lab2[1]) + (lab2[2] * lab2[2]));

  double barC = 0.5 * (C1 + C2);

  double G =
    0.5 * (1.0 - std::sqrt(std::pow(barC, 7.0) / (std::pow(barC, 7.0) + std::pow(25.0, 7.0))));

  double a1Prime = (1.0 + G) * lab1[1];
  double a2Prime = (1.0 + G) * lab2[1];

  double CPrime1 = std::sqrt((a1Prime * a1Prime) + (lab1[2] * lab1[2]));
  double CPrime2 = std::sqrt((a2Prime * a2Prime) + (lab2[2] * lab2[2]));

  double hPrime1;
  if ((lab1[2] == 0.0) && (a1Prime == 0.0))
  {
    hPrime1 = 0.0;
  }
  else
  {
    hPrime1 = std::atan2(lab1[2], a1Prime);
    if (hPrime1 < 0.0)
    {
      hPrime1 += 2.0 * vtkMath::Pi();
    }
  }

  double hPrime2;
  if ((lab2[2] == 0.0) && (a2Prime == 0.0))
  {
    hPrime2 = 0.0;
  }
  else
  {
    hPrime2 = std::atan2(lab2[2], a2Prime);
    if (hPrime2 < 0.0)
    {
      hPrime2 += 2.0 * vtkMath::Pi();
    }
  }

  double deltaLPrime = lab2[0] - lab1[0];

  double deltaCPrime = CPrime2 - CPrime1;

  double CPrimeProduct = CPrime1 * CPrime2;

  double deltahPrime;
  if (CPrimeProduct == 0.0)
  {
    deltahPrime = 0.0;
  }
  else
  {
    deltahPrime = hPrime2 - hPrime1;

    if (deltahPrime < -vtkMath::Pi())
    {
      deltahPrime += 2.0 * vtkMath::Pi();
    }
    else if (deltahPrime > vtkMath::Pi())
    {
      deltahPrime -= 2.0 * vtkMath::Pi();
    }
  }

  double deltaHPrime = 2.0 * std::sqrt(CPrimeProduct) * std::sin(0.5 * deltahPrime);

  double barLPrime = 0.5 * (lab1[0] + lab2[0]);

  double barCPrime = 0.5 * (CPrime1 + CPrime2);

  double hPrimeSum = hPrime1 + hPrime2;

  double barhPrime;
  if (CPrime1 * CPrime2 == 0.0)
  {
    barhPrime = hPrimeSum;
  }
  else
  {
    if (std::fabs(hPrime1 - hPrime2) <= vtkMath::Pi())
    {
      barhPrime = 0.5 * hPrimeSum;
    }
    else
    {
      if (hPrimeSum < 2.0 * vtkMath::Pi())
      {
        barhPrime = 0.5 * (hPrimeSum + 2.0 * vtkMath::Pi());
      }
      else
      {
        barhPrime = 0.5 * (hPrimeSum - 2.0 * vtkMath::Pi());
      }
    }
  }

  double T = 1.0 - 0.17 * std::cos(barhPrime - (vtkMath::Pi() * 30.0 / 180.0)) +
    0.24 * std::cos(2.0 * barhPrime) +
    0.32 * std::cos(3.0 * barhPrime + (vtkMath::Pi() * 6.0 / 180.0)) -
    0.20 * std::cos(4.0 * barhPrime - (vtkMath::Pi() * 63.0 / 180.0));

  double deltaTheta = (vtkMath::Pi() * 30.0 / 180.0) *
    std::exp(-std::pow(
      (barhPrime - (vtkMath::Pi() * 275.0 / 180.0)) / (vtkMath::Pi() * 25.0 / 180.0), 2.0));

  double R_C =
    2.0 * std::sqrt(std::pow(barCPrime, 7.0) / (std::pow(barCPrime, 7.0) + std::pow(25.0, 7.0)));

  double S_L =
    1.0 + (0.015 * pow(barLPrime - 50.0, 2.0) / std::sqrt(20.0 + std::pow(barLPrime - 50.0, 2.0)));

  double S_C = 1.0 + (0.045 * barCPrime);

  double S_H = 1.0 + (0.015 * barCPrime * T);

  double R_T = -std::sin(2.0 * deltaTheta) * R_C;

  double deltaE = std::sqrt(std::pow(deltaLPrime / (k_L * S_L), 2.0) +
    std::pow(deltaCPrime / (k_C * S_C), 2.0) + std::pow(deltaHPrime / (k_H * S_H), 2.0) +
    R_T * (deltaCPrime / (k_C * S_C)) * (deltaHPrime / (k_H * S_H)));

  return deltaE;
}

//----------------------------------------------------------------------------
double CorrectedDistance(std::vector<Node>& path)
{
  double distance = 0.0;

  for (std::size_t i = 1; i < path.size(); ++i)
  {
    double currentLabColor[3];
    vtkMath::RGBToLab(path.at(i).rgb, currentLabColor);

    double previousLabColor[3];
    vtkMath::RGBToLab(path.at(i - 1).rgb, previousLabColor);

    distance += GetCIEDeltaE2000(currentLabColor, previousLabColor);
    path.at(i).distance = distance;
  }

  return distance;
}

//----------------------------------------------------------------------------
double GetColorPath(
  const double rgb1[3], const double rgb2[3], std::vector<Node>& path, bool forceExactSupportColors)
{
  Position pos1, pos2;
  getPosition(rgb1, pos1);
  getPosition(rgb2, pos2);

  // Use Dijkstra's algorithm backwards to calculate the shortest distances from
  // the second color

  std::deque<Distance> distances(COLORSPACE_SIZE_X * COLORSPACE_SIZE_Y * COLORSPACE_SIZE_Z,
    std::numeric_limits<Distance>::infinity());
  std::deque<Position> predecessors(COLORSPACE_SIZE_X * COLORSPACE_SIZE_Y * COLORSPACE_SIZE_Z);

  // Use a set as the priority queue so we can update an entry in the queue by
  // deleting the old entry and re-inserting the new entry.
  // The set is sorted first by the distance from the seed node, so that the
  // first entry always is the node that can be reached shortest.
  std::set<std::pair<Distance, Position> > front;

  // Start backwards and use the second color as seed
  distances[getIndex(pos2)] = static_cast<Distance>(0);
  front.insert(std::make_pair(static_cast<Distance>(0), pos2));

  while (!front.empty())
  {
    Distance currentDist = front.begin()->first;
    Position currentPos = front.begin()->second;

    front.erase(front.begin());

    double currentLabColor[3];
    getLabColor(currentPos, currentLabColor);

    int minNeighborPosX = std::max(static_cast<int>(currentPos[0]) - NEIGHBORHOOD_SIZE_X, 0);
    int minNeighborPosY = std::max(static_cast<int>(currentPos[1]) - NEIGHBORHOOD_SIZE_Y, 0);
    int minNeighborPosZ = std::max(static_cast<int>(currentPos[2]) - NEIGHBORHOOD_SIZE_Z, 0);

    int maxNeighborPosX =
      std::min(static_cast<int>(currentPos[0]) + NEIGHBORHOOD_SIZE_X, COLORSPACE_SIZE_X - 1);
    int maxNeighborPosY =
      std::min(static_cast<int>(currentPos[1]) + NEIGHBORHOOD_SIZE_Y, COLORSPACE_SIZE_Y - 1);
    int maxNeighborPosZ =
      std::min(static_cast<int>(currentPos[2]) + NEIGHBORHOOD_SIZE_Z, COLORSPACE_SIZE_Z - 1);

    for (int neighborPosZ = minNeighborPosZ; neighborPosZ <= maxNeighborPosZ; ++neighborPosZ)
    {
      for (int neighborPosY = minNeighborPosY; neighborPosY <= maxNeighborPosY; ++neighborPosY)
      {
        for (int neighborPosX = minNeighborPosX; neighborPosX <= maxNeighborPosX; ++neighborPosX)
        {
          Position neighborPos;
          neighborPos[0] = neighborPosX;
          neighborPos[1] = neighborPosY;
          neighborPos[2] = neighborPosZ;

          if (neighborPos == currentPos)
          {
            continue;
          }

          double neighborLabColor[3];
          getLabColor(neighborPos, neighborLabColor);

          Distance deltaE =
            static_cast<Distance>(GetCIEDeltaE2000(currentLabColor, neighborLabColor));

          int neighborIdx = getIndex(neighborPos);

          Distance oldNeighborDist = distances[neighborIdx];
          Distance newNeighborDist = currentDist + deltaE;

          if (newNeighborDist < oldNeighborDist)
          {
            front.erase(std::make_pair(oldNeighborDist, neighborPos));

            distances[neighborIdx] = newNeighborDist;
            predecessors[neighborIdx] = currentPos;
            front.insert(std::make_pair(newNeighborDist, neighborPos));
          }
        }
      }
    }
  }

  // We started backwards from the second color, so the overall length of the
  // path is the distance value at the position of the first color
  Distance pathDistance = distances[getIndex(pos1)];

  // Start the path from the first color and follow each node's predecessor
  // until the second color is reached.
  // Since each node was reached shortest from its predecessor, this results in
  // a shortest path from the first to the second color.

  path.clear();

  Position currentPos = pos1;

  while (true)
  {
    int currentIdx = getIndex(currentPos);

    Node node;
    getRGBColor(currentPos, node.rgb);

    // The shortest distance from the first color to the node is the overall
    // shortest distance
    // from the first to the second color minus the shortest distance from the
    // second color to the node.
    node.distance = pathDistance - distances[currentIdx];

    path.push_back(node);

    if (currentPos == pos2)
    {
      break;
    }

    currentPos = predecessors[currentIdx];
  }

  // Force the first and the last node's color to be exact
  if (forceExactSupportColors)
  {
    for (int i = 0; i < 3; ++i)
    {
      path.front().rgb[i] = rgb1[i];
      path.back().rgb[i] = rgb2[i];
    }

    // Return the corrected overall length of the path. Necessary if forcing the
    return CorrectedDistance(path);
  }

  // Return the overall length of the path
  return pathDistance;
}
//----------------------------------------------------------------------------

} // namespace CIEDE2000
