/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenVROverlay
 * @brief   OpenVR overlay
 *
 * vtkOpenVROverlay support for VR overlays
*/

#ifndef vtkOpenVROverlay_h
#define vtkOpenVROverlay_h

#include "vtkRenderingOpenVRModule.h" // For export macro
#include "vtkObject.h"
#define SDL_MAIN_HANDLED
#include <SDL.h> // for ivars
#include <openvr.h> // for ivars
#include <vector> // ivars
#include "vtkNew.h" // for ivars
#include "vtkWeakPointer.h" // for ivars

class vtkOpenVROverlaySpot;
class vtkOpenVRRenderWindow;
class vtkTextureObject;
class vtkOpenVRCameraPose;
class vtkOpenVRCamera;

class VTKRENDERINGOPENVR_EXPORT vtkOpenVROverlay : public vtkObject
{
public:
  static vtkOpenVROverlay *New();
  vtkTypeMacro(vtkOpenVROverlay, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Render the overlay
   */
  virtual void Render();

  /**
   * Create the overlay
   */
  virtual void Create(vtkOpenVRRenderWindow *rw);

  /**
   * Get handle to the overlay
   */
  vr::VROverlayHandle_t GetOverlayHandle() {
    return this->OverlayHandle; }

  /**
   * Get handle to the overlay texture
   */
  vtkTextureObject *GetOverlayTexture() {
    return this->OverlayTexture.Get(); }

  //@{
  /**
   * methods to support events on the overlay
   */
  virtual void MouseMoved(int x, int y);
  virtual void MouseButtonPress(int x, int y);
  virtual void MouseButtonRelease(int x, int y);
  //@}

  vtkOpenVROverlaySpot *GetLastSpot() {
    return this->LastSpot; }

  std::vector<vtkOpenVROverlaySpot> &GetSpots() {
    return this->Spots; }

  /***
   * update the texture because this spot has changed
   */
  virtual void UpdateSpot(vtkOpenVROverlaySpot *spot);

  //@{
  /**
   * Set/Get a prefix for saving camera poses
   */
  void SetSessionName(const std::string& name) {
    this->SessionName = name; }
  std::string GetSessionName() {
    return this->SessionName; }
  //@}

  //@{
  /**
   * Set/Get a file for the dashboard image
   */
  void SetDashboardImageFileName(const std::string& name) {
    this->DashboardImageFileName = name; }
  std::string GetDashboardImageFileName() {
    return this->DashboardImageFileName; }
  //@}

  vtkOpenVRCameraPose *GetSavedCameraPose(size_t i);
  virtual void WriteCameraPoses();
  virtual void ReadCameraPoses();
  virtual void SaveCameraPose(int num);
  virtual void LoadCameraPose(int num);
  virtual void LoadNextCameraPose();

  // not used for dashboard overlays
  void Show();
  void Hide();

protected:
  vtkOpenVROverlay();
  ~vtkOpenVROverlay();

  virtual void SetupSpots() {};

  vr::IVRSystem *VRSystem;

  // for the overlay
  vr::VROverlayHandle_t OverlayHandle;
  vr::VROverlayHandle_t OverlayThumbnailHandle;
  vtkNew<vtkTextureObject> OverlayTexture;

  // std::vector<vtkOpenVRActiveSpot> ActiveSpots;
  unsigned char *OriginalTextureData;
  unsigned char *CurrentTextureData;

  std::vector<vtkOpenVROverlaySpot> Spots;
  vtkOpenVROverlaySpot *LastSpot;

  std::string SessionName;
  std::string DashboardImageFileName;
  std::vector<vtkOpenVRCameraPose> SavedCameraPoses;

  vtkWeakPointer<vtkOpenVRRenderWindow> Window;
  int LastCameraPoseIndex;

  double LastSpotIntensity;
  double ActiveSpotIntensity;

private:
  vtkOpenVROverlay(const vtkOpenVROverlay&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenVROverlay&) VTK_DELETE_FUNCTION;
};

#endif
