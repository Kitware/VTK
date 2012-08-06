/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : Id  */
/*  Date : $Date$ */
/*  Version : $Revision$ */
/*                                                                 */
/*  Author:                                                        */
/*     Kenneth Leiter
/*     kenneth.leiter@arl.army.mil                                         */
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
////////////////////////////////
/*

Serializes STL objects to Xdmf

CURRENTLY SUPPORTED:

WRITING:

  STL TYPES:
  vector, deque, list, set, multiset, map, multimap

  DATA TYPES:
  char, short, int, long long, float, double, unsigned char, unsigned short, unsigned int

READING:
  STL TYPES:
  vector, deque, list, set, multiset, map, multimap

  DATA TYPES:
  char, short, int, long long, float, double, unsigned char, unsigned short, unsigned int

TODO:
  Add support for stacks, queues, and priority queues
    (no const_iterator... must find alternative way of getting data)
  Currently user should move data into a supported stl template...

//////////////////////////////*/
#ifndef __XdmfXdmfSTLConverter_txx
#define __XdmfXdmfSTLConverter_txx
#include <XdmfArray.h>
#include <XdmfAttribute.h>
#include <XdmfDOM.h>
#include <XdmfGrid.h>
#include <XdmfMap.h>
#include <XdmfSet.h>

#include <vector>
#include <deque>
#include <list>
#include <set>
#include <sstream>

class XdmfSTLConverter
{

public:
  inline
  XdmfSTLConverter();

  inline
  ~XdmfSTLConverter();

  template<template<typename > class Container, class T>
    void inline
    writeSetToXdmf(Container<T> const& myContainer, XdmfElement * parent, std::string myName,
        std::string heavyName = "Xdmf.h5");

  template<template<typename , typename > class Container, class T, class U>
    void inline
    writeMapToXdmf(Container<T, U> const& myContainer, XdmfElement * parent, std::string myName,
        std::string heavyName = "Xdmf.h5");

  template<template<typename > class Container, class T>
    void inline
    getSetFromXdmf(Container<T> &myContainer, XdmfSet * currSet);

  template<template<typename , typename > class Container, class T, class U>
    void inline
    getMapFromXdmf(Container<T, U> &myContainer, XdmfSet * currSet);

private:
  template<class T>
    void inline
    copyFromXdmfArray(std::vector<T> &myContainer, XdmfArray * myIds);

  template<class T>
    void inline
    copyFromXdmfArray(std::deque<T> &myContainer, XdmfArray * myIds);

  template<class T>
    void inline
    copyFromXdmfArray(std::list<T> &myContainer, XdmfArray * myIds);

  template<class T>
    void inline
    copyFromXdmfArray(std::set<T> &myContainer, XdmfArray * myIds);

  template<class T>
    void inline
    copyFromXdmfArray(std::multiset<T> &myContainer, XdmfArray * myIds);

  template<template<typename > class Container>
    void inline
    generateXdmfArray(XdmfArray * currentArray, const Container<char> &myContainer);

  template<template<typename > class Container>
    void inline
    generateXdmfArray(XdmfArray * currentArray, const Container<short> &myContainer);

  template<template<typename > class Container>
    void inline
    generateXdmfArray(XdmfArray * currentArray, const Container<int> &myContainer);

  template<template<typename > class Container>
    void inline
    generateXdmfArray(XdmfArray * currentArray, const Container<long long> &myContainer);

  template<template<typename > class Container>
    void inline
    generateXdmfArray(XdmfArray * currentArray, const Container<float> &myContainer);

  template<template<typename > class Container>
    void inline
    generateXdmfArray(XdmfArray * currentArray, const Container<double> &myContainer);

  template<template<typename > class Container>
    void inline
    generateXdmfArray(XdmfArray * currentArray, const Container<unsigned char> &myContainer);

  template<template<typename > class Container>
    void inline
    generateXdmfArray(XdmfArray * currentArray, const Container<unsigned short> &myContainer);

  template<template<typename > class Container>
    void inline
    generateXdmfArray(XdmfArray * currentArray, const Container<unsigned int> &myContainer);

  template<template<typename > class Container, class T>
    void inline
    writeArrayValues(XdmfArray * currentArray, const Container<T> &myContainer);

  template<class T>
    T inline
    getArrayValue(XdmfArray * currentArray, int index);
};

XdmfSTLConverter::XdmfSTLConverter()
{

}

XdmfSTLConverter::~XdmfSTLConverter()
{

}

// FOR NON-ASSOCIATIVE SETS
template<template<typename > class Container, class T>
  void
  XdmfSTLConverter::writeSetToXdmf(Container<T> const& myContainer, XdmfElement * parent,
      std::string myName, std::string heavyName)
  {
    XdmfSet * currSet = new XdmfSet();
    currSet->SetSetType(XDMF_SET_TYPE_NODE);
    currSet->SetName(myName.c_str());
    currSet->SetDeleteOnGridDelete(true);

    // Copy Elements from Set to XdmfArray
    std::stringstream name;
    name << heavyName;
    if (heavyName.length() >= 3)
    {
      if (heavyName.substr(heavyName.length() - 3).compare(".h5") == 0)
      {
        name << ":";
      }
    }
    name << "/" << myName;
    currSet->GetIds()->SetHeavyDataSetName(name.str().c_str());
    generateXdmfArray(currSet->GetIds(), myContainer);

    parent->Insert(currSet);
    currSet->Build();
  }

// FOR ASSOCIATIVE SETS (Maps)
// Writing values as set attributes with keys as set dataitems...
template<template<typename , typename > class Container, class T, class U>
  void
  XdmfSTLConverter::writeMapToXdmf(Container<T, U> const& myContainer, XdmfElement * parent,
      std::string myName, std::string heavyName)
  {
    XdmfSet * currSet = new XdmfSet();
    currSet->SetSetType(XDMF_SET_TYPE_NODE);
    currSet->SetName(myName.c_str());
    currSet->SetDeleteOnGridDelete(true);

    // Store Keys and Values in Separate Sets
    std::vector<T> keys(myContainer.size());
    std::vector<U> vals(myContainer.size());
    int index = 0;
    typename Container<T, U>::const_iterator i;

    for (i = myContainer.begin(); i != myContainer.end(); i++)
    {
      keys[index] = i->first;
      vals[index] = i->second;
      index++;
    }

    XdmfAttribute * myAttribute = new XdmfAttribute();
    myAttribute->SetName("Values");
    myAttribute->SetAttributeCenter(XDMF_ATTRIBUTE_CENTER_NODE);
    myAttribute->SetAttributeType(XDMF_ATTRIBUTE_TYPE_SCALAR);
    myAttribute->SetDeleteOnGridDelete(true);

    // Copy Elements from each vector to XdmfArrays
    std::stringstream keyName;
    std::stringstream valName;
    keyName << heavyName;
    valName << heavyName;

    if (heavyName.length() >= 3)
    {
      if (heavyName.substr(heavyName.length() - 3).compare(".h5") == 0)
      {
        keyName << ":";
        valName << ":";
      }
    }
    keyName << "/" << myName << "/Keys";
    valName << "/" << myName << "/Vals";

    currSet->GetIds()->SetHeavyDataSetName(keyName.str().c_str());
    myAttribute->GetValues()->SetHeavyDataSetName(valName.str().c_str());
    generateXdmfArray(currSet->GetIds(), keys);
    generateXdmfArray(myAttribute->GetValues(), vals);

    parent->Insert(currSet);

    currSet->Insert(myAttribute);
    currSet->Build();
  }

template<template<typename > class Container, class T>
  void
  XdmfSTLConverter::getSetFromXdmf(Container<T> & myContainer, XdmfSet * currSet)
  {
    XdmfArray * myIds = currSet->GetIds();
    copyFromXdmfArray(myContainer, myIds);
  }

template<template<typename , typename > class Container, class T, class U>
  void
  XdmfSTLConverter::getMapFromXdmf(Container<T, U> & myContainer, XdmfSet * currSet)
  {
    XdmfArray * myKeys = currSet->GetIds();
    XdmfAttribute * myAttribute = currSet->GetAttribute(0);
    myAttribute->Update();
    XdmfArray * myVals = myAttribute->GetValues(0);
    for (int i = 0; i < myKeys->GetNumberOfElements(); i++)
    {
      myContainer.insert(
          std::pair<T, U>(getArrayValue<T> (myKeys, i), getArrayValue<U> (myVals, i)));
    }
  }

template<class T>
  void
  XdmfSTLConverter::copyFromXdmfArray(std::vector<T> &myContainer, XdmfArray * myIds)
  {
    for (int i = 0; i < myIds->GetNumberOfElements(); i++)
    {
      myContainer.push_back(getArrayValue<T> (myIds, i));
    }
  }

template<class T>
  void
  XdmfSTLConverter::copyFromXdmfArray(std::deque<T> &myContainer, XdmfArray * myIds)
  {
    for (int j = 0; j < myIds->GetNumberOfElements(); j++)
    {
      myContainer.push_back(getArrayValue<T> (myIds, j));
    }
  }

template<class T>
  void
  XdmfSTLConverter::copyFromXdmfArray(std::list<T> &myContainer, XdmfArray * myIds)
  {
    for (int j = 0; j < myIds->GetNumberOfElements(); j++)
    {
      myContainer.push_back(getArrayValue<T> (myIds, j));
    }
  }

template<class T>
  void
  XdmfSTLConverter::copyFromXdmfArray(std::set<T> &myContainer, XdmfArray * myIds)
  {
    for (int j = 0; j < myIds->GetNumberOfElements(); j++)
    {
      myContainer.insert(getArrayValue<T> (myIds, j));
    }
  }

template<class T>
  void
  XdmfSTLConverter::copyFromXdmfArray(std::multiset<T> & myContainer, XdmfArray * myIds)
  {
    for (int j = 0; j < myIds->GetNumberOfElements(); j++)
    {
      myContainer.insert(getArrayValue<T> (myIds, j));
    }
  }

template<template<typename > class Container>
  void
  XdmfSTLConverter::generateXdmfArray(XdmfArray * currentArray, const Container<char> & myContainer)
  {
    currentArray->SetNumberType(XDMF_INT8_TYPE);
    writeArrayValues(currentArray, myContainer);
  }

template<template<typename > class Container>
  void
  XdmfSTLConverter::generateXdmfArray(XdmfArray * currentArray,
      const Container<short> & myContainer)
  {
    currentArray->SetNumberType(XDMF_INT16_TYPE);
    writeArrayValues(currentArray, myContainer);
  }

template<template<typename > class Container>
  void
  XdmfSTLConverter::generateXdmfArray(XdmfArray * currentArray, const Container<int> & myContainer)
  {
    currentArray->SetNumberType(XDMF_INT32_TYPE);
    writeArrayValues(currentArray, myContainer);
  }

template<template<typename > class Container>
  void
  XdmfSTLConverter::generateXdmfArray(XdmfArray * currentArray,
      const Container<long long> & myContainer)
  {
    currentArray->SetNumberType(XDMF_INT64_TYPE);
    writeArrayValues(currentArray, myContainer);
  }

template<template<typename > class Container>
  void
  XdmfSTLConverter::generateXdmfArray(XdmfArray * currentArray,
      const Container<float> & myContainer)
  {
    currentArray->SetNumberType(XDMF_FLOAT32_TYPE);
    writeArrayValues(currentArray, myContainer);
  }

template<template<typename > class Container>
  void
  XdmfSTLConverter::generateXdmfArray(XdmfArray * currentArray,
      const Container<double> & myContainer)
  {
    currentArray->SetNumberType(XDMF_FLOAT64_TYPE);
    writeArrayValues(currentArray, myContainer);
  }

template<template<typename > class Container>
  void
  XdmfSTLConverter::generateXdmfArray(XdmfArray * currentArray,
      const Container<unsigned char> & myContainer)
  {
    currentArray->SetNumberType(XDMF_UINT8_TYPE);
    writeArrayValues(currentArray, myContainer);
  }

template<template<typename > class Container>
  void
  XdmfSTLConverter::generateXdmfArray(XdmfArray * currentArray,
      const Container<unsigned short> & myContainer)
  {
    currentArray->SetNumberType(XDMF_UINT16_TYPE);
    writeArrayValues(currentArray, myContainer);
  }

template<template<typename > class Container>
  void
  XdmfSTLConverter::generateXdmfArray(XdmfArray * currentArray,
      const Container<unsigned int> & myContainer)
  {
    currentArray->SetNumberType(XDMF_UINT32_TYPE);
    writeArrayValues(currentArray, myContainer);
  }

template<template<typename > class Container, class T>
  void
  XdmfSTLConverter::writeArrayValues(XdmfArray * currentArray, const Container<T> & myContainer)
  {
    currentArray->SetNumberOfElements(myContainer.size());
    int index = 0;
    typename Container<T>::const_iterator i;
    for (i = myContainer.begin(); i != myContainer.end(); i++)
    {
      currentArray->SetValue(index, *i);
      index++;
    }
  }

template<class T>
  T
  XdmfSTLConverter::getArrayValue(XdmfArray * currentArray, int index)
  {
    switch (currentArray->GetNumberType())
      {
    case XDMF_INT8_TYPE:
      return (T) currentArray->GetValueAsInt8(index);
    case XDMF_INT16_TYPE:
      return (T) currentArray->GetValueAsInt16(index);
    case XDMF_INT32_TYPE:
      return (T) currentArray->GetValueAsInt32(index);
    case XDMF_INT64_TYPE:
      return (T) currentArray->GetValueAsInt64(index);
    case XDMF_FLOAT32_TYPE:
      return (T) currentArray->GetValueAsFloat32(index);
    case XDMF_FLOAT64_TYPE:
      return (T) currentArray->GetValueAsFloat64(index);
    case XDMF_UINT8_TYPE:
      return (T) currentArray->GetValueAsInt8(index);
    case XDMF_UINT16_TYPE:
      return (T) currentArray->GetValueAsInt16(index);
    case XDMF_UINT32_TYPE:
      return (T) currentArray->GetValueAsInt32(index);
    default:
      return (T) currentArray->GetValueAsFloat64(index);
      }
  }
#endif // __XdmfXdmfSTLConverter_txx
