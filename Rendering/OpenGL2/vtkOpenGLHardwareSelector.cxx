// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenGLHardwareSelector.h"

#include "vtk_glad.h"

#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLState.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"

#include "vtkOpenGLError.h"

// #define vtkOpenGLHardwareSelectorDEBUG
#ifdef vtkOpenGLHardwareSelectorDEBUG
#include "vtkImageImport.h"
#include "vtkNew.h"
#include "vtkPNMWriter.h"
#if defined(__EMSCRIPTEN__)
#include <emscripten.h> // for EM_ASM
#endif
#include "vtkWindows.h" // OK on UNix etc
#include <sstream>
#endif

#define ID_OFFSET 1

VTK_ABI_NAMESPACE_BEGIN
namespace
{
void annotate(const std::string& str)
{
  vtkOpenGLRenderUtilities::MarkDebugEvent(str);
}
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLHardwareSelector);

//------------------------------------------------------------------------------
#ifdef vtkOpenGLHardwareSelectorDEBUG
vtkOpenGLHardwareSelector::vtkOpenGLHardwareSelector()
{
  std::cout << "=====vtkOpenGLHardwareSelector::vtkOpenGLHardwareSelector" << endl;
}

//------------------------------------------------------------------------------
vtkOpenGLHardwareSelector::~vtkOpenGLHardwareSelector()
{
  std::cout << "=====vtkOpenGLHardwareSelector::~vtkOpenGLHardwareSelector" << endl;
}
#else
vtkOpenGLHardwareSelector::vtkOpenGLHardwareSelector() = default;
vtkOpenGLHardwareSelector::~vtkOpenGLHardwareSelector() = default;
#endif

//------------------------------------------------------------------------------
void vtkOpenGLHardwareSelector::PreCapturePass(int pass)
{
  annotate(std::string("Starting pass: ") + this->PassTypeToString(static_cast<PassTypes>(pass)));

  // Disable blending
  auto rwin = vtkOpenGLRenderWindow::SafeDownCast(this->Renderer->GetRenderWindow());
  vtkOpenGLState* ostate = rwin->GetState();

  this->OriginalBlending = ostate->GetEnumState(GL_BLEND);
  ostate->vtkglDisable(GL_BLEND);
}

//------------------------------------------------------------------------------
void vtkOpenGLHardwareSelector::PostCapturePass(int pass)
{
  // Restore blending.
  auto rwin = vtkOpenGLRenderWindow::SafeDownCast(this->Renderer->GetRenderWindow());
  vtkOpenGLState* ostate = rwin->GetState();

  ostate->SetEnumState(GL_BLEND, this->OriginalBlending);
  annotate(std::string("Pass complete: ") + this->PassTypeToString(static_cast<PassTypes>(pass)));
}

//------------------------------------------------------------------------------
void vtkOpenGLHardwareSelector::BeginSelection()
{
  auto rwin = vtkOpenGLRenderWindow::SafeDownCast(this->Renderer->GetRenderWindow());

  this->OriginalMultiSample = rwin->GetMultiSamples();
  rwin->SetMultiSamples(0);

  vtkOpenGLState* ostate = rwin->GetState();
  ostate->Reset();
  ostate->Push();

  // render normally to set the zbuffer
  if (this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    vtkOpenGLState::ScopedglEnableDisable bsaver(ostate, GL_BLEND);
    ostate->vtkglDisable(GL_BLEND);

    rwin->Render();
    this->Renderer->PreserveDepthBufferOn();
  }

  this->Superclass::BeginSelection();
}

//------------------------------------------------------------------------------
void vtkOpenGLHardwareSelector::EndSelection()
{
  // render normally to set the zbuffer
  if (this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    this->Renderer->PreserveDepthBufferOff();
  }

  auto rwin = vtkOpenGLRenderWindow::SafeDownCast(this->Renderer->GetRenderWindow());
  rwin->SetMultiSamples(this->OriginalMultiSample);
  vtkOpenGLState* ostate = rwin->GetState();
  ostate->Pop();

  this->Superclass::EndSelection();
}

//------------------------------------------------------------------------------
// just add debug output if compiled with vtkOpenGLHardwareSelectorDEBUG
void vtkOpenGLHardwareSelector::SavePixelBuffer(int passNo)
{
  this->Superclass::SavePixelBuffer(passNo);

#ifdef vtkOpenGLHardwareSelectorDEBUG

  vtkNew<vtkImageImport> ii;
  ii->SetImportVoidPointer(this->PixBuffer[passNo], 1);
  ii->SetDataScalarTypeToUnsignedChar();
  ii->SetNumberOfScalarComponents(3);
  ii->SetDataExtent(this->Area[0], this->Area[2], this->Area[1], this->Area[3], 0, 0);
  ii->SetWholeExtent(this->Area[0], this->Area[2], this->Area[1], this->Area[3], 0, 0);

  // change this to somewhere on your system
  // hardcoded as with MPI/parallel/client server it can be hard to
  // find these images sometimes.
  std::string fname = "C:/Users/ken.martin/Documents/pickbuffer_";

#if defined(__EMSCRIPTEN__)
  // clang-format off
  // Displays a table with pixel coordinates and the r,g,b values.
  // This table is rendered inside a div with id='pick_buffer_debug'.
  //
  // | PASS_NAME           | ANOTHER_PASS_NAME   |..|
  // | [x0, y0] = (r, g, b)| [x0, y0] = (r, g, b)|..|
  // | ....                | ....                |..|
  EM_ASM({
    const passNo = $0;
    const divId = 'pick_buffer_debug';
    let div = document.getElementById(divId);
    const passTypes = new Array('ACTOR_PASS', 'COMPOSITE_INDEX_PASS', 'POINT_ID_LOW24', 'POINT_ID_HIGH24', 'PROCESS_PASS', 'CELL_ID_LOW24', 'CELL_ID_HIGH24');
    if (div === null) {
      // create the layout of caption and canvas for the first time.
      div = document.createElement('div');
      div.setAttribute('id', divId);
      div.style.display = 'flex';
      for (const passType of passTypes) {
        const bufferTableDiv = document.createElement('div');
        bufferTableDiv.style.border = '2px solid #000000';
        const passTitle = document.createElement('text');
        passTitle.innerHTML = passType;
        bufferTableDiv.appendChild(passTitle);
        const passPixelDiv = document.createElement('text');
        passPixelDiv.setAttribute('id', passType.toLowerCase());
        bufferTableDiv.appendChild(passPixelDiv);
        div.appendChild(bufferTableDiv);
      }
      document.body.appendChild(div);
    }
    const xmin = $2;
    const xmax = $3;
    const ymin = $4;
    const ymax = $5;
    const width = xmax - xmin + 1;
    const height = ymax - ymin + 1;
    const pixels = new Uint8ClampedArray(Module.HEAPU8.subarray($1, $1 + width * height * 3));
    const passPixelDiv = document.getElementById(passTypes[passNo].toLowerCase());
    passPixelDiv.innerHTML = '<p></p>';
    let i = 0;
    for (let y = ymin; y <= ymax; ++y) {
      for (let x = xmin; x <= xmax; ++x) {
        passPixelDiv.innerHTML += '<p>[' + x + ',' + y + '] = (' + pixels[i++] + ',' + pixels[i++] + ',' + pixels[i++] + ')</p>';
      }
    }
  }, passNo, this->PixBuffer[passNo], this->Area[0], this->Area[2], this->Area[1], this->Area[3]);
  // clang-format on
  std::cout
    << "=====vtkOpenGLHardwareSelector wrote to table in <div id=pick_buffer_debug></div>\n";
  return;
#elif defined(_WIN32)
  std::ostringstream toString;
  toString.str("");
  toString.clear();
  toString << GetCurrentProcessId();
  fname += toString.str();
  fname += "_";
#endif
  fname += ("0" + std::to_string(passNo));
  fname += ".pnm";
  vtkNew<vtkPNMWriter> pw;
  pw->SetInputConnection(ii->GetOutputPort());
  pw->SetFileName(fname.c_str());
  pw->Write();
  std::cout << passNo << ":" << int(this->PixBuffer[passNo][0]) << ","
            << int(this->PixBuffer[passNo][1]) << "," << int(this->PixBuffer[passNo][2]) << ","
            << '\n';
  std::cout << "=====vtkOpenGLHardwareSelector wrote " << fname << "\n";
#endif
}

//------------------------------------------------------------------------------
void vtkOpenGLHardwareSelector::BeginRenderProp(vtkRenderWindow*)
{
#ifdef vtkOpenGLHardwareSelectorDEBUG
  std::cout << "=====vtkOpenGLHardwareSelector::BeginRenderProp" << endl;
#endif
}

//------------------------------------------------------------------------------
void vtkOpenGLHardwareSelector::BeginRenderProp()
{
  this->InPropRender++;
  if (this->InPropRender != 1)
  {
    return;
  }

  // device specific prep
  vtkRenderWindow* renWin = this->Renderer->GetRenderWindow();
  this->BeginRenderProp(renWin);

  if (this->CurrentPass == ACTOR_PASS)
  {
    int propid = this->PropID;
    if (propid >= 0xfffffe)
    {
      vtkErrorMacro("Too many props. Currently only " << 0xfffffe << " props are supported.");
      return;
    }
    // Since 0 is reserved for nothing selected, we offset propid by 1.
    propid = propid + ID_OFFSET;
    this->SetPropColorValue(propid);
  }
  else if (this->CurrentPass == PROCESS_PASS)
  {
    this->SetPropColorValue(this->ProcessID + 1);
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLHardwareSelector::EndRenderProp(vtkRenderWindow*)
{
#ifdef vtkOpenGLHardwareSelectorDEBUG
  std::cout << "=====vtkOpenGLHardwareSelector::EndRenderProp" << endl;
#endif
}

//------------------------------------------------------------------------------
void vtkOpenGLHardwareSelector::EndRenderProp()
{
  this->Superclass::EndRenderProp();
}

//------------------------------------------------------------------------------
void vtkOpenGLHardwareSelector::RenderCompositeIndex(unsigned int index)
{
  if (this->CurrentPass == COMPOSITE_INDEX_PASS)
  {
    if (index > 0xffffff)
    {
      vtkErrorMacro("Indices > 0xffffff are not supported.");
      return;
    }
    this->SetPropColorValue(static_cast<vtkIdType>(0xffffff & index));
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLHardwareSelector::RenderProcessId(unsigned int processid)
{
  if (this->CurrentPass == PROCESS_PASS && this->UseProcessIdFromData)
  {
    if (processid >= 0xffffff)
    {
      vtkErrorMacro("Invalid id: " << processid);
      return;
    }
    this->SetPropColorValue(static_cast<vtkIdType>(processid + 1));
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLHardwareSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
