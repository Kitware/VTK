/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : Id  */
/*  Date : $Date$ */
/*  Version : $Revision$ */
/*                                                                 */
/*  Author:                                                        */
/*     Kenneth Leiter                                              */
/*     kenneth.leiter@arl.army.mil                                   */
/*     US Army Research Laboratory                                 */
/*     Aberdeen Proving Ground, MD                                 */
/*                                                                 */
/*     Copyright @ 2009 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/

#include <string>
#include <iostream>

#ifndef XDMFFORTRAN_H_
#define XDMFFORTRAN_H_

using std::cerr;
using std::cout;
using std::cin;
using std::endl;

#if defined(WIN32) && !defined(XDMFSTATIC)

// Windows and DLL configuration
#if defined(XdmfUtils_EXPORTS)
  #define XDMF_UTILS_DLL __declspec(dllexport)
#else
  #define XDMF_UTILS_DLL __declspec(dllimport)
#endif

#else

// Linux or static configuration
#define XDMF_UTILS_DLL

#endif

class XDMF_UTILS_DLL XdmfFortran
{
public:
  XdmfFortran(char * outputName);
  ~XdmfFortran();
  void SetTime(double * t);
  void AddCollection(char * collectionType);
  void CloseCollection();
  void SetGridTopology(char * topologyType, int * numberOfElements, XdmfInt32 * conns);
  void SetGridTopologyFromShape(char * topologyType, char * shape, XdmfInt32 * conns);
  void SetGridGeometry(char * geometryType, char * numberType, int * numberOfPoints,
       XdmfPointer * points);
  void AddGridAttribute(char * attributeName, char * numberType, char * attributeCenter,
       char * attributeType, int * numberOfPoints, XdmfPointer * data);
  void AddGridAttributeFromShape(char * attributeName, char * numberType, char * attributeCenter,
       char * attributeType, char * shape, char * units, XdmfPointer * data);
  void AddGridInformation(char * informationName, char * value);
  void AddCollectionAttribute(char * attributeName, char * numberType, char * attributeCenter,
       char * attributeType, int * numberOfPoints, XdmfPointer * data);
  void AddCollectionInformation(char * informationName, char * value);
  void AddArray(char * name, char * numberType, int * numberOfValues, XdmfPointer * data);
  void ReadFile(char * filePath);
  void ReadGrid(char * gridName);
  void ReadGridAtIndex(int * gridIndex);
  void GetNumberOfPoints(XdmfInt32 * toReturn);
  void GetNumberOfGrids(XdmfInt32 * toReturn);
  void ReadPointValues(char * numberType, XdmfInt32 * startIndex, XdmfPointer * arrayToFill,
       XdmfInt32 * numberOfValues, XdmfInt32 * arrayStride, XdmfInt32 * valuesStride);
  void GetNumberOfAttributeValues(char * attributeName, XdmfInt32 * toReturn);
  void ReadAttributeValues(char * attributeName, char * numberType, XdmfInt32 * startIndex,
       XdmfPointer * arrayToFill, XdmfInt32 * numberOfValues, XdmfInt32 * arrayStride,
       XdmfInt32 * valuesStride);
  void ReadInformationValue(char * informationName, char * valueToReturn);
  void GetTime(XdmfFloat64 * toReturn);
  void WriteGrid(char * gridName);
  void WriteToFile();
  void Serialize();
  void GetDOM(char * charPointer);
private:
  void Destroy();
  void ReadFilePriv(XdmfXmlNode currElement);
  void ReadGridPriv(char * gridName, XdmfXmlNode currElement);
  void ReadGridPriv(XdmfConstString gridPath);
  void WriteToXdmfArray(XdmfArray * array, XdmfPointer * data);
  void ReadFromXdmfArray(XdmfArray * array, char * numberType, XdmfInt32 * startIndex,
       XdmfPointer * arrayToFill, XdmfInt32 * numberOfValues, XdmfInt32 * arrayStride,
       XdmfInt32 * valuesStride);
  XdmfDOM * myDOM;
  XdmfRoot * myRoot;
  XdmfDomain * myDomain;
  XdmfTopology * myTopology;
  XdmfGeometry * myGeometry;
  std::stack<XdmfGrid*> myCollections;
  std::vector<XdmfAttribute*> myAttributes;
  std::vector<XdmfInformation*> myInformations;
  std::map<const char*, int> myGridNames;
  std::vector<std::string> myGridPaths;
  std::string myName;
  double currentTime;
};

#endif /* XDMFFORTRAN_H_ */
