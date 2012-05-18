/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMoleculeMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMoleculeMapper - Mapper that draws vtkMolecule objects
//
// .SECTION Description
// vtkMoleculeMapper uses glyphs (display lists) to quickly render a
// molecule.

#ifndef __vtkMoleculeMapper_h
#define __vtkMoleculeMapper_h

#include "vtkDomainsChemistryModule.h" // For export macro
#include "vtkMapper.h"
#include "vtkNew.h" // For vtkNew

class vtkActor;
class vtkGlyph3DMapper;
class vtkIdTypeArray;
class vtkMolecule;
class vtkPeriodicTable;
class vtkPolyData;
class vtkRenderer;
class vtkSelection;
class vtkSphereSource;
class vtkTrivialProducer;

class VTKDOMAINSCHEMISTRY_EXPORT vtkMoleculeMapper : public vtkMapper
{
public:
  static vtkMoleculeMapper *New();
  vtkTypeMacro(vtkMoleculeMapper,vtkMapper);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the input vtkMolecule.
  void SetInputData(vtkMolecule *in);
  vtkMolecule *GetInput();

  // Description:
  // Set ivars to default ball-and-stick settings. This is equivalent
  // to the following:
  //   - SetRenderAtoms( true )
  //   - SetRenderBonds( true )
  //   - SetAtomicRadiusType( VDWRadius )
  //   - SetAtomicRadiusScaleFactor( 0.3 )
  //   - SetBondColorMode( DiscreteByAtom )
  //   - SetUseMultiCylindersForBonds( true )
  //   - SetBondRadius( 0.075 )
  void UseBallAndStickSettings();

  // Description:
  // Set ivars to default van der Waals spheres settings. This is
  // equivalent to the following:
  //   - SetRenderAtoms( true )
  //   - SetRenderBonds( true )
  //   - SetAtomicRadiusType( VDWRadius )
  //   - SetAtomicRadiusScaleFactor( 1.0 )
  //   - SetBondColorMode( DiscreteByAtom )
  //   - SetUseMultiCylindersForBonds( true )
  //   - SetBondRadius( 0.075 )
  void UseVDWSpheresSettings();

  // Description:
  // Set ivars to default liquorice stick settings. This is
  // equivalent to the following:
  //   - SetRenderAtoms( true )
  //   - SetRenderBonds( true )
  //   - SetAtomicRadiusType( UnitRadius )
  //   - SetAtomicRadiusScaleFactor( 0.1 )
  //   - SetBondColorMode( DiscreteByAtom )
  //   - SetUseMultiCylindersForBonds( false )
  //   - SetBondRadius( 0.1 )
  void UseLiquoriceStickSettings();

  // Description:
  // Set ivars to use fast settings that may be useful for rendering
  // extremely large molecules where the overall shape is more
  // important than the details of the atoms/bond. This is equivalent
  // to the following:
  //   - SetRenderAtoms( true )
  //   - SetRenderBonds( true )
  //   - SetAtomicRadiusType( UnitRadius )
  //   - SetAtomicRadiusScaleFactor( 0.60 )
  //   - SetBondColorMode( SingleColor )
  //   - SetBondColor( 50, 50, 50 )
  //   - SetUseMultiCylindersForBonds( false )
  //   - SetBondRadius( 0.075 )
  void UseFastSettings();

  // Description:
  // Get/Set whether or not to render atoms. Default: On.
  vtkGetMacro(RenderAtoms, bool);
  vtkSetMacro(RenderAtoms, bool);
  vtkBooleanMacro(RenderAtoms, bool);

  // Description:
  // Get/Set whether or not to render bonds. Default: On.
  vtkGetMacro(RenderBonds, bool);
  vtkSetMacro(RenderBonds, bool);
  vtkBooleanMacro(RenderBonds, bool);

  enum {
    CovalentRadius = 0,
    VDWRadius,
    UnitRadius
  };

  // Description:
  // Get/Set the type of radius used to generate the atoms. Default:
  // VDWRadius.
  vtkGetMacro(AtomicRadiusType, int);
  vtkSetMacro(AtomicRadiusType, int);
  const char * GetAtomicRadiusTypeAsString();
  void SetAtomicRadiusTypeToCovalentRadius()
  {
    this->SetAtomicRadiusType(CovalentRadius);
  }
  void SetAtomicRadiusTypeToVDWRadius()
  {
    this->SetAtomicRadiusType(VDWRadius);
  }
  void SetAtomicRadiusTypeToUnitRadius()
  {
    this->SetAtomicRadiusType(UnitRadius);
  }

  // Description:
  // Get/Set the uniform scaling factor applied to the atoms. Default:
  // 0.3.
  vtkGetMacro(AtomicRadiusScaleFactor, float);
  vtkSetMacro(AtomicRadiusScaleFactor, float);

  // Description:
  // Get/Set whether multicylinders will be used to represent multiple
  // bonds. Default: On.
  vtkGetMacro(UseMultiCylindersForBonds, bool);
  vtkSetMacro(UseMultiCylindersForBonds, bool);
  vtkBooleanMacro(UseMultiCylindersForBonds, bool);

  enum {
    SingleColor = 0,
    DiscreteByAtom
  };

  // Description:
  // Get/Set the method by which bonds are colored.
  //
  // If 'SingleColor' is used, all bonds will be the same color. Use
  // SetBondColor to set the rgb values used.
  //
  // If 'DiscreteByAtom' is selected, each bond is colored using the
  // same lookup table as the atoms at each end, with a sharp color
  // boundary at the bond center.
  vtkGetMacro(BondColorMode, int);
  vtkSetMacro(BondColorMode, int);
  const char * GetBondColorModeAsString();
  void SetBondColorModeToSingleColor()
  {
    this->SetBondColorMode(SingleColor);
  }
  void SetBondColorModeToDiscreteByAtom()
  {
    this->SetBondColorMode(DiscreteByAtom);
  }

  // Description:
  // Get/Set the color of the bonds as an rgb tuple.
  // Default: {50, 50, 50} (dark grey)
  vtkGetVector3Macro(BondColor, unsigned char);
  vtkSetVector3Macro(BondColor, unsigned char);

  // Description:
  // Get/Set the radius of the bond cylinders. Default: 0.075
  vtkGetMacro(BondRadius, float);
  vtkSetMacro(BondRadius, float);

  // Description:
  // Extract the ids atoms and/or bonds rendered by this molecule from a
  // vtkSelection object. The vtkIdTypeArray
  virtual void GetSelectedAtomsAndBonds(vtkSelection *selection,
                                        vtkIdTypeArray *atomIds,
                                        vtkIdTypeArray *bondIds);
  virtual void GetSelectedAtoms(vtkSelection *selection,
                                vtkIdTypeArray *atomIds)
  {
    this->GetSelectedAtomsAndBonds(selection, atomIds, NULL);
  }
  virtual void GetSelectedBonds(vtkSelection *selection,
                                vtkIdTypeArray *bondIds)
  {
    this->GetSelectedAtomsAndBonds(selection, NULL, bondIds);
  }

  // Description:
  // Reimplemented from base class
  virtual void Render(vtkRenderer *, vtkActor *);
  virtual void ReleaseGraphicsResources(vtkWindow *);
  double * GetBounds();
  void GetBounds(double bounds[6]) { vtkAbstractMapper3D::GetBounds(bounds); }
  virtual int FillInputPortInformation(int port, vtkInformation* info);
  virtual bool GetSupportsSelection() {return true;}

protected:
  vtkMoleculeMapper();
  ~vtkMoleculeMapper();

  // Description:
  // Customize atom rendering
  bool RenderAtoms;
  int AtomicRadiusType;
  float AtomicRadiusScaleFactor;

  // Description:
  // Customize bond rendering
  bool RenderBonds;
  int BondColorMode;
  bool UseMultiCylindersForBonds;
  float BondRadius;
  unsigned char BondColor[3];

  // Description:
  // Internal render methods
  void GlyphRender(vtkRenderer *ren, vtkActor *act);

  // Description:
  // Cached variables and update methods
  vtkNew<vtkPolyData> AtomGlyphPolyData;
  vtkNew<vtkTrivialProducer> AtomGlyphPointOutput;
  vtkNew<vtkPolyData> BondGlyphPolyData;
  vtkNew<vtkTrivialProducer> BondGlyphPointOutput;
  bool GlyphDataInitialized;
  void UpdateGlyphPolyData();
  void UpdateAtomGlyphPolyData();
  void UpdateBondGlyphPolyData();

  // Description:
  // Internal mappers
  vtkNew<vtkGlyph3DMapper> AtomGlyphMapper;
  vtkNew<vtkGlyph3DMapper> BondGlyphMapper;

  // Description:
  // Periodic table for lookups
  vtkNew<vtkPeriodicTable> PeriodicTable;

private:
  vtkMoleculeMapper(const vtkMoleculeMapper&);  // Not implemented.
  void operator=(const vtkMoleculeMapper&);  // Not implemented.
};

#endif
