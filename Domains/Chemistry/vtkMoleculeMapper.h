// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMoleculeMapper
 * @brief   Mapper that draws vtkMolecule objects
 *
 *
 * vtkMoleculeMapper uses glyphs (display lists) to quickly render a
 * molecule.
 */

#ifndef vtkMoleculeMapper_h
#define vtkMoleculeMapper_h

#include "vtkDomainsChemistryModule.h" // For export macro
#include "vtkMapper.h"
#include "vtkNew.h" // For vtkNew

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkGlyph3DMapper;
class vtkIdTypeArray;
class vtkMolecule;
class vtkPeriodicTable;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkRenderer;
class vtkSelection;
class vtkSphereSource;
class vtkTrivialProducer;

class VTKDOMAINSCHEMISTRY_EXPORT vtkMoleculeMapper : public vtkMapper
{
public:
  static vtkMoleculeMapper* New();
  vtkTypeMacro(vtkMoleculeMapper, vtkMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the input vtkMolecule.
   */
  void SetInputData(vtkMolecule* in);
  vtkMolecule* GetInput();
  ///@}

  /**
   * Set ivars to default ball-and-stick settings. This is equivalent
   * to the following:
   * - SetRenderAtoms( true )
   * - SetRenderBonds( true )
   * - SetAtomicRadiusType( VDWRadius )
   * - SetAtomicRadiusScaleFactor( 0.3 )
   * - SetBondColorMode( DiscreteByAtom )
   * - SetUseMultiCylindersForBonds( true )
   * - SetBondRadius( 0.075 )
   */
  void UseBallAndStickSettings();

  /**
   * Set ivars to default van der Waals spheres settings. This is
   * equivalent to the following:
   * - SetRenderAtoms( true )
   * - SetRenderBonds( true )
   * - SetAtomicRadiusType( VDWRadius )
   * - SetAtomicRadiusScaleFactor( 1.0 )
   * - SetBondColorMode( DiscreteByAtom )
   * - SetUseMultiCylindersForBonds( true )
   * - SetBondRadius( 0.075 )
   */
  void UseVDWSpheresSettings();

  /**
   * Set ivars to default liquorice stick settings. This is
   * equivalent to the following:
   * - SetRenderAtoms( true )
   * - SetRenderBonds( true )
   * - SetAtomicRadiusType( UnitRadius )
   * - SetAtomicRadiusScaleFactor( 0.1 )
   * - SetBondColorMode( DiscreteByAtom )
   * - SetUseMultiCylindersForBonds( false )
   * - SetBondRadius( 0.1 )
   */
  void UseLiquoriceStickSettings();

  /**
   * Set ivars to use fast settings that may be useful for rendering
   * extremely large molecules where the overall shape is more
   * important than the details of the atoms/bond. This is equivalent
   * to the following:
   * - SetRenderAtoms( true )
   * - SetRenderBonds( true )
   * - SetAtomicRadiusType( UnitRadius )
   * - SetAtomicRadiusScaleFactor( 0.60 )
   * - SetBondColorMode( SingleColor )
   * - SetBondColor( 50, 50, 50 )
   * - SetUseMultiCylindersForBonds( false )
   * - SetBondRadius( 0.075 )
   */
  void UseFastSettings();

  ///@{
  /**
   * Get/Set whether or not to render atoms. Default: On.
   */
  vtkGetMacro(RenderAtoms, bool);
  vtkSetMacro(RenderAtoms, bool);
  vtkBooleanMacro(RenderAtoms, bool);
  ///@}

  ///@{
  /**
   * Get/Set whether or not to render bonds. Default: On.
   */
  vtkGetMacro(RenderBonds, bool);
  vtkSetMacro(RenderBonds, bool);
  vtkBooleanMacro(RenderBonds, bool);
  ///@}

  ///@{
  /**
   * Get/Set whether or not to render the unit cell lattice, if present.
   * Default: On.
   */
  vtkGetMacro(RenderLattice, bool);
  vtkSetMacro(RenderLattice, bool);
  vtkBooleanMacro(RenderLattice, bool);
  ///@}

  enum
  {
    CovalentRadius = 0,
    VDWRadius,
    UnitRadius,
    CustomArrayRadius
  };

  ///@{
  /**
   * Get/Set the type of radius used to generate the atoms. Default:
   * VDWRadius. If CustomArrayRadius is used, the VertexData array named
   * 'radii' is used for per-atom radii.
   */
  vtkGetMacro(AtomicRadiusType, int);
  vtkSetMacro(AtomicRadiusType, int);
  const char* GetAtomicRadiusTypeAsString();
  void SetAtomicRadiusTypeToCovalentRadius() { this->SetAtomicRadiusType(CovalentRadius); }
  void SetAtomicRadiusTypeToVDWRadius() { this->SetAtomicRadiusType(VDWRadius); }
  void SetAtomicRadiusTypeToUnitRadius() { this->SetAtomicRadiusType(UnitRadius); }
  void SetAtomicRadiusTypeToCustomArrayRadius() { this->SetAtomicRadiusType(CustomArrayRadius); }
  ///@}

  ///@{
  /**
   * Get/Set the uniform scaling factor applied to the atoms.
   * This is ignored when AtomicRadiusType == CustomArrayRadius.
   * Default: 0.3.
   */
  vtkGetMacro(AtomicRadiusScaleFactor, float);
  vtkSetMacro(AtomicRadiusScaleFactor, float);
  ///@}

  ///@{
  /**
   * Get/Set whether multicylinders will be used to represent multiple
   * bonds. Default: On.
   */
  vtkGetMacro(UseMultiCylindersForBonds, bool);
  vtkSetMacro(UseMultiCylindersForBonds, bool);
  vtkBooleanMacro(UseMultiCylindersForBonds, bool);
  ///@}

  enum
  {
    SingleColor = 0,
    DiscreteByAtom
  };

  ///@{
  /**
   * Get/Set the method by which bonds are colored.

   * If 'SingleColor' is used, all bonds will be the same color. Use
   * SetBondColor to set the rgb values used.

   * If 'DiscreteByAtom' is selected, each bond is colored using the
   * same lookup table as the atoms at each end, with a sharp color
   * boundary at the bond center.
   */
  vtkGetMacro(BondColorMode, int);
  vtkSetClampMacro(BondColorMode, int, SingleColor, DiscreteByAtom);
  void SetBondColorModeToSingleColor() { this->SetBondColorMode(SingleColor); }
  void SetBondColorModeToDiscreteByAtom() { this->SetBondColorMode(DiscreteByAtom); }
  const char* GetBondColorModeAsString();
  ///@}

  ///@{
  /**
   * Get/Set the method by which atoms are colored.
   *
   * If 'SingleColor' is used, all atoms will have the same color. Use
   * SetAtomColor to set the rgb values to be used.
   *
   * If 'DiscreteByAtom' is selected, each atom is colored using the
   * internal lookup table.
   */
  vtkGetMacro(AtomColorMode, int);
  vtkSetClampMacro(AtomColorMode, int, SingleColor, DiscreteByAtom);
  ///@}

  ///@{
  /**
   * Get/Set the color of the atoms as an rgb tuple.
   * Default: {150, 150, 150} (grey)
   */
  vtkGetVector3Macro(AtomColor, unsigned char);
  vtkSetVector3Macro(AtomColor, unsigned char);
  ///@}

  ///@{
  /**
   * Get/Set the color of the bonds as an rgb tuple.
   * Default: {50, 50, 50} (dark grey)
   */
  vtkGetVector3Macro(BondColor, unsigned char);
  vtkSetVector3Macro(BondColor, unsigned char);
  ///@}

  ///@{
  /**
   * Get/Set the radius of the bond cylinders. Default: 0.075
   */
  vtkGetMacro(BondRadius, float);
  vtkSetMacro(BondRadius, float);
  ///@}

  ///@{
  /**
   * Get/Set the color of the bonds as an rgb tuple.
   * Default: {255, 255, 255} (white)
   */
  vtkGetVector3Macro(LatticeColor, unsigned char);
  vtkSetVector3Macro(LatticeColor, unsigned char);
  ///@}

  ///@{
  /**
   * Extract the ids atoms and/or bonds rendered by this molecule from a
   * vtkSelection object. The vtkIdTypeArray
   */
  virtual void GetSelectedAtomsAndBonds(
    vtkSelection* selection, vtkIdTypeArray* atomIds, vtkIdTypeArray* bondIds);
  virtual void GetSelectedAtoms(vtkSelection* selection, vtkIdTypeArray* atomIds)
  {
    this->GetSelectedAtomsAndBonds(selection, atomIds, nullptr);
  }
  virtual void GetSelectedBonds(vtkSelection* selection, vtkIdTypeArray* bondIds)
  {
    this->GetSelectedAtomsAndBonds(selection, nullptr, bondIds);
  }
  ///@}

  ///@{
  /**
   * Reimplemented from base class
   */
  void Render(vtkRenderer*, vtkActor*) override;
  void ReleaseGraphicsResources(vtkWindow*) override;
  double* GetBounds() override;
  void GetBounds(double bounds[6]) override { Superclass::GetBounds(bounds); }
  int FillInputPortInformation(int port, vtkInformation* info) override;
  bool GetSupportsSelection() override { return true; }
  ///@}

  ///@{
  /**
   * Get/Set the atomic radius array name. Default: "radii"
   * It is only used when AtomicRadiusType is set to CustomArrayRadius.
   */
  vtkGetStringMacro(AtomicRadiusArrayName);
  vtkSetStringMacro(AtomicRadiusArrayName);
  ///@}

  /**
   * Helper method to set ScalarMode on both AtomGlyphMapper and BondGlyphMapper.
   * true means VTK_COLOR_MODE_MAP_SCALARS, false VTK_COLOR_MODE_DIRECT_SCALARS.
   */
  virtual void SetMapScalars(bool map);

  /**
   * Accessor to internal structure. This is exposed to make it available for ray tracers.
   */
  vtkPeriodicTable* GetPeriodicTable() { return this->PeriodicTable; }

protected:
  vtkMoleculeMapper();
  ~vtkMoleculeMapper() override;

  ///@{
  /**
   * Customize atom rendering
   */
  bool RenderAtoms;
  int AtomicRadiusType;
  float AtomicRadiusScaleFactor;
  char* AtomicRadiusArrayName;
  int AtomColorMode;
  unsigned char AtomColor[3];
  ///@}

  ///@{
  /**
   * Customize bond rendering
   */
  bool RenderBonds;
  int BondColorMode;
  bool UseMultiCylindersForBonds;
  float BondRadius;
  unsigned char BondColor[3];
  ///@}

  bool RenderLattice;

  /**
   * Internal render methods
   */
  void GlyphRender(vtkRenderer* ren, vtkActor* act);

  ///@{
  /**
   * Cached variables and update methods
   */
  vtkNew<vtkPolyData> AtomGlyphPolyData;
  vtkNew<vtkTrivialProducer> AtomGlyphPointOutput;
  vtkNew<vtkPolyData> BondGlyphPolyData;
  vtkNew<vtkTrivialProducer> BondGlyphPointOutput;
  bool GlyphDataInitialized;
  virtual void UpdateGlyphPolyData();
  virtual void UpdateAtomGlyphPolyData();
  virtual void UpdateBondGlyphPolyData();
  ///@}

  ///@{
  /**
   * Internal mappers
   */
  vtkNew<vtkGlyph3DMapper> AtomGlyphMapper;
  vtkNew<vtkGlyph3DMapper> BondGlyphMapper;
  ///@}

  unsigned char LatticeColor[3];
  vtkNew<vtkPolyData> LatticePolyData;
  vtkNew<vtkPolyDataMapper> LatticeMapper;
  virtual void UpdateLatticePolyData();

  /**
   * Periodic table for lookups
   */
  vtkNew<vtkPeriodicTable> PeriodicTable;

private:
  vtkMoleculeMapper(const vtkMoleculeMapper&) = delete;
  void operator=(const vtkMoleculeMapper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
