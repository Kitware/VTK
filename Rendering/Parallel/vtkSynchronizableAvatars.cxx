// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSynchronizableAvatars.h"
#include "vtkCullerCollection.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLAvatar.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkTextProperty.h"

VTK_ABI_NAMESPACE_BEGIN

class vtkSynchronizableAvatars::vtkInternals
{
public:
  struct AvatarInfo
  {
    double HeadPosition[3];
    double HeadOrientation[3];
    double LeftHandPosition[3];
    double LeftHandOrientation[3];
    double RightHandPosition[3];
    double RightHandOrientation[3];
    double UpVector[3];
    double Scale[3];
    double Color[3];
    bool UseLeftHand;
    bool UseRightHand;
    bool ShowHandsOnly;
    std::string Label;

    // Save/restore the struct to/from a stream.
    void SaveAvatar(vtkMultiProcessStream& stream)
    {
      stream << this->HeadPosition[0] << this->HeadPosition[1] << this->HeadPosition[2]
             << this->HeadOrientation[0] << this->HeadOrientation[1] << this->HeadOrientation[2]
             << this->LeftHandPosition[0] << this->LeftHandPosition[1] << this->LeftHandPosition[2]
             << this->LeftHandOrientation[0] << this->LeftHandOrientation[1]
             << this->LeftHandOrientation[2] << this->RightHandPosition[0]
             << this->RightHandPosition[1] << this->RightHandPosition[2]
             << this->RightHandOrientation[0] << this->RightHandOrientation[1]
             << this->RightHandOrientation[2] << this->UpVector[0] << this->UpVector[1]
             << this->UpVector[2] << this->Scale[0] << this->Scale[1] << this->Scale[2]
             << this->Color[0] << this->Color[1] << this->Color[2] << this->UseLeftHand
             << this->UseRightHand << this->ShowHandsOnly << this->Label;
    }

    void RestoreAvatar(vtkMultiProcessStream& stream)
    {
      stream >> this->HeadPosition[0] >> this->HeadPosition[1] >> this->HeadPosition[2] >>
        this->HeadOrientation[0] >> this->HeadOrientation[1] >> this->HeadOrientation[2] >>
        this->LeftHandPosition[0] >> this->LeftHandPosition[1] >> this->LeftHandPosition[2] >>
        this->LeftHandOrientation[0] >> this->LeftHandOrientation[1] >>
        this->LeftHandOrientation[2] >> this->RightHandPosition[0] >> this->RightHandPosition[1] >>
        this->RightHandPosition[2] >> this->RightHandOrientation[0] >>
        this->RightHandOrientation[1] >> this->RightHandOrientation[2] >> this->UpVector[0] >>
        this->UpVector[1] >> this->UpVector[2] >> this->Scale[0] >> this->Scale[1] >>
        this->Scale[2] >> this->Color[0] >> this->Color[1] >> this->Color[2] >> this->UseLeftHand >>
        this->UseRightHand >> this->ShowHandsOnly >> this->Label;
    }

    void CopyFrom(vtkOpenGLAvatar* avatar)
    {
      avatar->GetHeadPosition(this->HeadPosition);
      avatar->GetHeadOrientation(this->HeadOrientation);
      avatar->GetLeftHandPosition(this->LeftHandPosition);
      avatar->GetLeftHandOrientation(this->LeftHandOrientation);
      avatar->GetRightHandPosition(this->RightHandPosition);
      avatar->GetRightHandOrientation(this->RightHandOrientation);
      avatar->GetUpVector(this->UpVector);
      avatar->GetScale(this->Scale);
      avatar->GetProperty()->GetColor(this->Color);
      this->UseLeftHand = avatar->GetUseLeftHand();
      this->UseRightHand = avatar->GetUseRightHand();
      this->ShowHandsOnly = avatar->GetShowHandsOnly();

      const char* label = static_cast<const char*>(avatar->GetLabel());
      if (label)
      {
        this->Label = label;
      }
    }

    void CopyTo(vtkOpenGLAvatar* avatar)
    {
      avatar->SetHeadPosition(this->HeadPosition);
      avatar->SetHeadOrientation(this->HeadOrientation);
      avatar->SetLeftHandPosition(this->LeftHandPosition);
      avatar->SetLeftHandOrientation(this->LeftHandOrientation);
      avatar->SetRightHandPosition(this->RightHandPosition);
      avatar->SetRightHandOrientation(this->RightHandOrientation);
      avatar->SetUpVector(this->UpVector);
      avatar->SetScale(this->Scale);
      avatar->GetProperty()->SetColor(this->Color);
      avatar->GetLabelTextProperty()->SetColor(this->Color);
      avatar->SetUseLeftHand(this->UseLeftHand);
      avatar->SetUseRightHand(this->UseRightHand);
      avatar->SetShowHandsOnly(this->ShowHandsOnly);

      if (!this->Label.empty())
      {
        avatar->SetLabel(this->Label.c_str());
      }
    }
  };

  // Iterate over actors in the renderer and serialize them to the stream
  void SaveCollection(vtkMultiProcessStream& stream, vtkRenderer* renderer)
  {
    std::vector<vtkOpenGLAvatar*> avatars;
    this->GetOpenGLAvatars(renderer, avatars);

    stream << 2906 << static_cast<unsigned int>(avatars.size());

    AvatarInfo aInfo;

    for (size_t i = 0; i < avatars.size(); ++i)
    {
      vtkOpenGLAvatar* avatar = avatars[i];
      aInfo.CopyFrom(avatar);
      aInfo.SaveAvatar(stream);
    }
  }

  // Read from the stream and update actors in the renderer
  bool RestoreCollection(vtkMultiProcessStream& stream, vtkRenderer* renderer)
  {
    std::vector<vtkOpenGLAvatar*> localAvatars;
    this->GetOpenGLAvatars(renderer, localAvatars);

    int tag;
    stream >> tag;
    if (tag != 2906)
    {
      return false;
    }

    unsigned int numRemoteAvatars;

    stream >> numRemoteAvatars;

    // 1) If we don't have enough avatars locally, make new ones
    // 2) Otherwise we have exactly enough or too many locally, so:
    //     a) go through the stream updating local avatar properties
    //     b) any remaining local avatars at the end are unnecessary, remove them

    AvatarInfo aInfo;
    size_t i = 0;

    for (; i < numRemoteAvatars; ++i)
    {
      vtkOpenGLAvatar* avatar;

      if (i >= localAvatars.size())
      {
        // There are more remote avatars than local ones, so from here on, we need to make
        // new local avatars and add them to the renderer before updating.
        vtkSmartPointer<vtkOpenGLAvatar> newOne = vtkSmartPointer<vtkOpenGLAvatar>::New();
        avatar = newOne.GetPointer();
        renderer->AddActor(avatar);
      }
      else
      {
        // Already have a local one to update
        avatar = localAvatars[i];
      }

      aInfo.RestoreAvatar(stream);
      aInfo.CopyTo(avatar);
    }

    // There are more local avatars than remote ones, so remove any extras from
    // the renderer now.
    for (; i < localAvatars.size(); ++i)
    {
      renderer->RemoveActor(localAvatars[i]);
    }

    return true;
  }

  void GetOpenGLAvatars(vtkRenderer* renderer, std::vector<vtkOpenGLAvatar*>& avatarList)
  {
    vtkCollectionSimpleIterator pit;
    vtkPropCollection* props = renderer->GetViewProps();
    vtkProp* aProp;

    for (props->InitTraversal(pit); (aProp = props->GetNextProp(pit));)
    {
      vtkOpenGLAvatar* oglAvatar = vtkOpenGLAvatar::SafeDownCast(aProp);
      if (oglAvatar)
      {
        avatarList.push_back(oglAvatar);
      }
    }

    this->currentAvatarCount = avatarList.size();
  }

  size_t currentAvatarCount;
};

vtkStandardNewMacro(vtkSynchronizableAvatars);

//------------------------------------------------------------------------------
vtkSynchronizableAvatars::vtkSynchronizableAvatars()
  : Internal(new vtkSynchronizableAvatars::vtkInternals())
{
}

//------------------------------------------------------------------------------
vtkSynchronizableAvatars::~vtkSynchronizableAvatars() = default;

//------------------------------------------------------------------------------
void vtkSynchronizableAvatars::InitializeRenderer(vtkRenderer* ren)
{
  ren->GetCullers()->RemoveAllItems();
}

//------------------------------------------------------------------------------
void vtkSynchronizableAvatars::CleanUpRenderer(vtkRenderer* ren)
{
  std::vector<vtkOpenGLAvatar*> localAvatars;
  this->Internal->GetOpenGLAvatars(ren, localAvatars);
  for (size_t i = 0; i < localAvatars.size(); ++i)
  {
    ren->RemoveActor(localAvatars[i]);
  }
}

//------------------------------------------------------------------------------
void vtkSynchronizableAvatars::SaveToStream(vtkMultiProcessStream& stream, vtkRenderer* ren)
{
  this->Internal->SaveCollection(stream, ren);
}

//------------------------------------------------------------------------------
void vtkSynchronizableAvatars::RestoreFromStream(vtkMultiProcessStream& stream, vtkRenderer* ren)
{
  this->Internal->RestoreCollection(stream, ren);
}

//------------------------------------------------------------------------------
void vtkSynchronizableAvatars::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CurrentAvatarCount: " << this->Internal->currentAvatarCount << endl;
}

VTK_ABI_NAMESPACE_END
