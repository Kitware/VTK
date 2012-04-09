/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBlueObeliskDataParser.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBlueObeliskDataParser - Fill a vtkBlueObeliskData
// container with data from the BODR XML dataset.
//
// .SECTION Description
// The Blue Obelisk Data Repository is a free, open repository of
// chemical information. This class extracts the BODR information into
// vtk arrays, which are stored in a vtkBlueObeliskData object.
//
// \warning The vtkBlueObeliskDataParser class should never need to be
// used directly. For convenient access to the BODR data, use
// vtkPeriodicTable. For access to the raw arrays produced by this
// parser, see the vtkBlueObeliskData class. A static
// vtkBlueObeliskData object is accessible via
// vtkPeriodicTable::GetBlueObeliskData().
//
// .SECTION See Also
// vtkPeriodicTable vtkBlueObeliskData

#ifndef __vtkBlueObeliskDataParser_h
#define __vtkBlueObeliskDataParser_h

#include "vtkXMLParser.h"

#include <vtkSmartPointer.h> // For vtkSmartPointer

class vtkAbstractArray;
class vtkBlueObeliskData;
class vtkFloatArray;
class vtkStdString;
class vtkStringArray;
class vtkUnsignedShortArray;

class VTK_CHEMISTRY_EXPORT vtkBlueObeliskDataParser : public vtkXMLParser
{
 public:
  vtkTypeMacro(vtkBlueObeliskDataParser, vtkXMLParser);
  static vtkBlueObeliskDataParser * New();

  // Description:
  // Set the target vtkBlueObeliskData object that this parser will
  // populate
  virtual void SetTarget(vtkBlueObeliskData *bodr);

  // Description:
  // Start parsing
  virtual int Parse();

  // Description:
  // These are only implemented to prevent compiler warnings about hidden
  // virtual overloads. This function simply call Parse(); the arguments are
  // ignored.
  virtual int Parse(const char *);
  virtual int Parse(const char *, unsigned int);

protected:
  vtkBlueObeliskDataParser();
  ~vtkBlueObeliskDataParser();

  void StartElement(const char *name, const char **attr);
  void EndElement(const char *name);

  void CharacterDataHandler(const char *data, int length);

  void SetCurrentValue(const char *data, int length);
  void SetCurrentValue(const char *data);

  vtkBlueObeliskData *Target;

  bool IsProcessingAtom;
  void NewAtomStarted(const char **attr);
  void NewAtomFinished();

  bool IsProcessingValue;
  void NewValueStarted(const char **attr);
  void NewValueFinished();

  enum AtomValueType {
    None = 0,
    AtomicNumber,
    Symbol,
    Name,
    PeriodicTableBlock,
    ElectronicConfiguration,
    Family,
    Mass,
    ExactMass,
    IonizationEnergy,
    ElectronAffinity,
    PaulingElectronegativity,
    CovalentRadius,
    VDWRadius,
    DefaultColor,
    BoilingPoint,
    MeltingPoint,
    Period,
    Group
  } CurrentValueType;

  int CurrentAtomicNumber;
  vtkStdString *CurrentSymbol;
  vtkStdString *CurrentName;
  vtkStdString *CurrentPeriodicTableBlock;
  vtkStdString *CurrentElectronicConfiguration;
  vtkStdString *CurrentFamily;
  float CurrentMass;
  float CurrentExactMass;
  float CurrentIonizationEnergy;
  float CurrentElectronAffinity;
  float CurrentPaulingElectronegativity;
  float CurrentCovalentRadius;
  float CurrentVDWRadius;
  float CurrentDefaultColor[3];
  float CurrentBoilingPoint;
  float CurrentMeltingPoint;
  unsigned int CurrentPeriod;
  unsigned int CurrentGroup;

private:
  // Not implemented
  vtkBlueObeliskDataParser(const vtkBlueObeliskDataParser&);
  void operator=(const vtkBlueObeliskDataParser&);

  // Description:
  // Resize array if needed and set the entry at ind to val.
  static void ResizeArrayIfNeeded(vtkAbstractArray *arr, vtkIdType ind);
  static void ResizeAndSetValue(vtkStdString *val,
                                vtkStringArray *arr,
                                vtkIdType ind);
  static void ResizeAndSetValue(float val,
                                vtkFloatArray *arr,
                                vtkIdType ind);
  static void ResizeAndSetValue(unsigned short val,
                                vtkUnsignedShortArray *arr,
                                vtkIdType ind);

  // Description:
  // Parse types from const char *
  static int parseInt(const char *);
  static float parseFloat(const char *);
  static void parseFloat3(const char * str, float[3]);
  static unsigned short parseUnsignedShort(const char *);

  // Description:
  // Convert a string to lower case. This will modify the input string
  // and return the input pointer.
  static vtkStdString * ToLower(vtkStdString *);
};

#endif
