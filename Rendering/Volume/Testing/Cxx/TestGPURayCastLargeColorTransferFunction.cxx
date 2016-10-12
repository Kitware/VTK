/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that the vtkGPUVolumeRayCastMapper doesn't unexpectedly
// scale down a large color transfer function, which could create artifacts.
// See http://www.na-mic.org/Bug/view.php?id=2165

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkFixedPointVolumeRayCastMapper.h"
#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTransform.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkXMLImageDataReader.h"

#define GPU_MAPPER


//----------------------------------------------------------------------------
int TestGPURayCastLargeColorTransferFunction(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  // Color table 'hncma-atlas-lut' extracted from
  // nac-hncma-atlas-2015Nov-Slicer4-4Version.mrb on
  // http://www.spl.harvard.edu/publications/item/view/2037
  vtkSmartPointer<vtkLookupTable> lut =
    vtkSmartPointer<vtkLookupTable>::New();

  // Initialize vtkLookupTable
  int const NumValues = 5023;
  lut->SetNumberOfTableValues(NumValues);
  lut->SetTableRange(0, NumValues-1);
  for (int i = 0; i < NumValues; i++)
  {
    lut->SetTableValue(i, 0.0, 0.0, 0.0, 0.0);
  }

  lut->SetTableValue(0, 0 / 255.0, 0 / 255.0, 0 / 255.0, 0 / 255.0);
  lut->SetTableValue(2, 250 / 255.0, 250 / 255.0, 225 / 255.0, 255 / 255.0);
  lut->SetTableValue(3, 225 / 255.0, 190 / 255.0, 150 / 255.0, 255 / 255.0);
  lut->SetTableValue(4, 88 / 255.0, 106 / 255.0, 215 / 255.0, 255 / 255.0);
  lut->SetTableValue(5, 88 / 255.0, 106 / 255.0, 215 / 255.0, 255 / 255.0);
  lut->SetTableValue(7, 180 / 255.0, 210 / 255.0, 120 / 255.0, 255 / 255.0);
  lut->SetTableValue(8, 230 / 255.0, 150 / 255.0, 35 / 255.0, 255 / 255.0);
  lut->SetTableValue(11, 30 / 255.0, 111 / 255.0, 85 / 255.0, 255 / 255.0);
  lut->SetTableValue(12, 210 / 255.0, 157 / 255.0, 166 / 255.0, 255 / 255.0);
  lut->SetTableValue(13, 15 / 255.0, 50 / 255.0, 255 / 255.0, 255 / 255.0);
  lut->SetTableValue(15, 88 / 255.0, 106 / 255.0, 215 / 255.0, 255 / 255.0);
  lut->SetTableValue(17, 220 / 255.0, 215 / 255.0, 20 / 255.0, 255 / 255.0);
  lut->SetTableValue(18, 98 / 255.0, 153 / 255.0, 112 / 255.0, 255 / 255.0);
  lut->SetTableValue(19, 88 / 255.0, 106 / 255.0, 215 / 255.0, 255 / 255.0);
  lut->SetTableValue(24, 88 / 255.0, 106 / 255.0, 215 / 255.0, 255 / 255.0);
  lut->SetTableValue(25, 255 / 255.0, 165 / 255.0, 0 / 255.0, 255 / 255.0);
  lut->SetTableValue(26, 165 / 255.0, 0 / 255.0, 255 / 255.0, 255 / 255.0);
  lut->SetTableValue(27, 148 / 255.0, 128 / 255.0, 72 / 255.0, 255 / 255.0);
  lut->SetTableValue(28, 165 / 255.0, 40 / 255.0, 40 / 255.0, 255 / 255.0);
  lut->SetTableValue(31, 90 / 255.0, 105 / 255.0, 215 / 255.0, 255 / 255.0);
  lut->SetTableValue(34, 139 / 255.0, 126 / 255.0, 177 / 255.0, 255 / 255.0);
  lut->SetTableValue(35, 50 / 255.0, 50 / 255.0, 135 / 255.0, 255 / 255.0);
  lut->SetTableValue(40, 145 / 255.0, 92 / 255.0, 109 / 255.0, 255 / 255.0);
  lut->SetTableValue(41, 250 / 255.0, 250 / 255.0, 225 / 255.0, 255 / 255.0);
  lut->SetTableValue(43, 88 / 255.0, 106 / 255.0, 215 / 255.0, 255 / 255.0);
  lut->SetTableValue(44, 88 / 255.0, 106 / 255.0, 215 / 255.0, 255 / 255.0);
  lut->SetTableValue(46, 180 / 255.0, 210 / 255.0, 120 / 255.0, 255 / 255.0);
  lut->SetTableValue(47, 230 / 255.0, 150 / 255.0, 35 / 255.0, 255 / 255.0);
  lut->SetTableValue(50, 30 / 255.0, 111 / 255.0, 85 / 255.0, 255 / 255.0);
  lut->SetTableValue(51, 210 / 255.0, 157 / 255.0, 166 / 255.0, 255 / 255.0);
  lut->SetTableValue(52, 15 / 255.0, 50 / 255.0, 255 / 255.0, 255 / 255.0);
  lut->SetTableValue(53, 220 / 255.0, 215 / 255.0, 20 / 255.0, 255 / 255.0);
  lut->SetTableValue(54, 98 / 255.0, 153 / 255.0, 112 / 255.0, 255 / 255.0);
  lut->SetTableValue(58, 165 / 255.0, 0 / 255.0, 255 / 255.0, 255 / 255.0);
  lut->SetTableValue(60, 165 / 255.0, 40 / 255.0, 40 / 255.0, 255 / 255.0);
  lut->SetTableValue(61, 135 / 255.0, 205 / 255.0, 235 / 255.0, 255 / 255.0);
  lut->SetTableValue(63, 90 / 255.0, 105 / 255.0, 215 / 255.0, 255 / 255.0);
  lut->SetTableValue(66, 0 / 255.0, 108 / 255.0, 112 / 255.0, 255 / 255.0);
  lut->SetTableValue(71, 0 / 255.0, 108 / 255.0, 112 / 255.0, 255 / 255.0);
  lut->SetTableValue(72, 253 / 255.0, 135 / 255.0, 192 / 255.0, 255 / 255.0);
  lut->SetTableValue(77, 216 / 255.0, 220 / 255.0, 84 / 255.0, 255 / 255.0);
  lut->SetTableValue(78, 156 / 255.0, 171 / 255.0, 108 / 255.0, 255 / 255.0);
  lut->SetTableValue(79, 255 / 255.0, 150 / 255.0, 10 / 255.0, 255 / 255.0);
  lut->SetTableValue(83, 255 / 255.0, 165 / 255.0, 0 / 255.0, 255 / 255.0);
  lut->SetTableValue(84, 255 / 255.0, 150 / 255.0, 10 / 255.0, 255 / 255.0);
  lut->SetTableValue(85, 99 / 255.0, 106 / 255.0, 24 / 255.0, 255 / 255.0);
  lut->SetTableValue(96, 205 / 255.0, 10 / 255.0, 125 / 255.0, 255 / 255.0);
  lut->SetTableValue(100, 125 / 255.0, 140 / 255.0, 180 / 255.0, 255 / 255.0);
  lut->SetTableValue(142, 220 / 255.0, 225 / 255.0, 70 / 255.0, 255 / 255.0);
  lut->SetTableValue(200, 125 / 255.0, 140 / 255.0, 180 / 255.0, 255 / 255.0);
  lut->SetTableValue(215, 216 / 255.0, 220 / 255.0, 84 / 255.0, 255 / 255.0);
  lut->SetTableValue(216, 156 / 255.0, 171 / 255.0, 108 / 255.0, 255 / 255.0);
  lut->SetTableValue(300, 235 / 255.0, 63 / 255.0, 159 / 255.0, 255 / 255.0);
  lut->SetTableValue(301, 31 / 255.0, 175 / 255.0, 72 / 255.0, 255 / 255.0);
  lut->SetTableValue(302, 214 / 255.0, 27 / 255.0, 150 / 255.0, 255 / 255.0);
  lut->SetTableValue(303, 220 / 255.0, 39 / 255.0, 44 / 255.0, 255 / 255.0);
  lut->SetTableValue(304, 49 / 255.0, 95 / 255.0, 178 / 255.0, 255 / 255.0);
  lut->SetTableValue(305, 241 / 255.0, 235 / 255.0, 27 / 255.0, 255 / 255.0);
  lut->SetTableValue(306, 158 / 255.0, 56 / 255.0, 161 / 255.0, 255 / 255.0);
  lut->SetTableValue(307, 56 / 255.0, 120 / 255.0, 63 / 255.0, 255 / 255.0);
  lut->SetTableValue(308, 149 / 255.0, 75 / 255.0, 64 / 255.0, 255 / 255.0);
  lut->SetTableValue(309, 116 / 255.0, 64 / 255.0, 163 / 255.0, 255 / 255.0);
  lut->SetTableValue(310, 124 / 255.0, 139 / 255.0, 88 / 255.0, 255 / 255.0);
  lut->SetTableValue(311, 245 / 255.0, 213 / 255.0, 21 / 255.0, 255 / 255.0);
  lut->SetTableValue(312, 115 / 255.0, 204 / 255.0, 130 / 255.0, 255 / 255.0);
  lut->SetTableValue(313, 220 / 255.0, 113 / 255.0, 174 / 255.0, 255 / 255.0);
  lut->SetTableValue(314, 216 / 255.0, 106 / 255.0, 116 / 255.0, 255 / 255.0);
  lut->SetTableValue(315, 43 / 255.0, 169 / 255.0, 230 / 255.0, 255 / 255.0);
  lut->SetTableValue(316, 237 / 255.0, 229 / 255.0, 148 / 255.0, 255 / 255.0);
  lut->SetTableValue(317, 174 / 255.0, 116 / 255.0, 152 / 255.0, 255 / 255.0);
  lut->SetTableValue(318, 103 / 255.0, 164 / 255.0, 114 / 255.0, 255 / 255.0);
  lut->SetTableValue(319, 167 / 255.0, 123 / 255.0, 89 / 255.0, 255 / 255.0);
  lut->SetTableValue(320, 164 / 255.0, 123 / 255.0, 187 / 255.0, 255 / 255.0);
  lut->SetTableValue(321, 189 / 255.0, 199 / 255.0, 129 / 255.0, 255 / 255.0);
  lut->SetTableValue(322, 243 / 255.0, 231 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(323, 243 / 255.0, 182 / 255.0, 191 / 255.0, 255 / 255.0);
  lut->SetTableValue(324, 171 / 255.0, 216 / 255.0, 143 / 255.0, 255 / 255.0);
  lut->SetTableValue(327, 243 / 255.0, 241 / 255.0, 133 / 255.0, 255 / 255.0);
  lut->SetTableValue(328, 223 / 255.0, 162 / 255.0, 214 / 255.0, 255 / 255.0);
  lut->SetTableValue(329, 147 / 255.0, 210 / 255.0, 194 / 255.0, 255 / 255.0);
  lut->SetTableValue(330, 216 / 255.0, 190 / 255.0, 132 / 255.0, 255 / 255.0);
  lut->SetTableValue(331, 175 / 255.0, 176 / 255.0, 221 / 255.0, 255 / 255.0);
  lut->SetTableValue(332, 236 / 255.0, 245 / 255.0, 196 / 255.0, 255 / 255.0);
  lut->SetTableValue(333, 241 / 255.0, 216 / 255.0, 238 / 255.0, 255 / 255.0);
  lut->SetTableValue(334, 235 / 255.0, 63 / 255.0, 159 / 255.0, 255 / 255.0);
  lut->SetTableValue(335, 31 / 255.0, 175 / 255.0, 72 / 255.0, 255 / 255.0);
  lut->SetTableValue(336, 214 / 255.0, 27 / 255.0, 150 / 255.0, 255 / 255.0);
  lut->SetTableValue(337, 220 / 255.0, 39 / 255.0, 44 / 255.0, 255 / 255.0);
  lut->SetTableValue(338, 49 / 255.0, 95 / 255.0, 178 / 255.0, 255 / 255.0);
  lut->SetTableValue(339, 241 / 255.0, 235 / 255.0, 27 / 255.0, 255 / 255.0);
  lut->SetTableValue(340, 158 / 255.0, 56 / 255.0, 161 / 255.0, 255 / 255.0);
  lut->SetTableValue(341, 56 / 255.0, 120 / 255.0, 63 / 255.0, 255 / 255.0);
  lut->SetTableValue(342, 149 / 255.0, 75 / 255.0, 64 / 255.0, 255 / 255.0);
  lut->SetTableValue(343, 116 / 255.0, 64 / 255.0, 163 / 255.0, 255 / 255.0);
  lut->SetTableValue(344, 124 / 255.0, 139 / 255.0, 88 / 255.0, 255 / 255.0);
  lut->SetTableValue(345, 245 / 255.0, 213 / 255.0, 21 / 255.0, 255 / 255.0);
  lut->SetTableValue(346, 115 / 255.0, 204 / 255.0, 130 / 255.0, 255 / 255.0);
  lut->SetTableValue(347, 220 / 255.0, 113 / 255.0, 174 / 255.0, 255 / 255.0);
  lut->SetTableValue(348, 216 / 255.0, 106 / 255.0, 116 / 255.0, 255 / 255.0);
  lut->SetTableValue(349, 43 / 255.0, 169 / 255.0, 230 / 255.0, 255 / 255.0);
  lut->SetTableValue(350, 237 / 255.0, 229 / 255.0, 148 / 255.0, 255 / 255.0);
  lut->SetTableValue(351, 174 / 255.0, 116 / 255.0, 152 / 255.0, 255 / 255.0);
  lut->SetTableValue(352, 103 / 255.0, 164 / 255.0, 114 / 255.0, 255 / 255.0);
  lut->SetTableValue(353, 167 / 255.0, 123 / 255.0, 89 / 255.0, 255 / 255.0);
  lut->SetTableValue(354, 164 / 255.0, 123 / 255.0, 187 / 255.0, 255 / 255.0);
  lut->SetTableValue(355, 189 / 255.0, 199 / 255.0, 129 / 255.0, 255 / 255.0);
  lut->SetTableValue(356, 243 / 255.0, 231 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(357, 243 / 255.0, 182 / 255.0, 191 / 255.0, 255 / 255.0);
  lut->SetTableValue(358, 171 / 255.0, 216 / 255.0, 143 / 255.0, 255 / 255.0);
  lut->SetTableValue(361, 243 / 255.0, 241 / 255.0, 133 / 255.0, 255 / 255.0);
  lut->SetTableValue(362, 223 / 255.0, 162 / 255.0, 214 / 255.0, 255 / 255.0);
  lut->SetTableValue(363, 147 / 255.0, 210 / 255.0, 194 / 255.0, 255 / 255.0);
  lut->SetTableValue(364, 216 / 255.0, 190 / 255.0, 132 / 255.0, 255 / 255.0);
  lut->SetTableValue(365, 175 / 255.0, 176 / 255.0, 221 / 255.0, 255 / 255.0);
  lut->SetTableValue(366, 236 / 255.0, 245 / 255.0, 196 / 255.0, 255 / 255.0);
  lut->SetTableValue(367, 241 / 255.0, 216 / 255.0, 238 / 255.0, 255 / 255.0);
  lut->SetTableValue(371, 100 / 255.0, 200 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(373, 255 / 255.0, 0 / 255.0, 0 / 255.0, 255 / 255.0);
  lut->SetTableValue(375, 100 / 255.0, 200 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(377, 255 / 255.0, 0 / 255.0, 0 / 255.0, 255 / 255.0);
  lut->SetTableValue(380, 243 / 255.0, 136 / 255.0, 62 / 255.0, 255 / 255.0);
  lut->SetTableValue(381, 135 / 255.0, 247 / 255.0, 4 / 255.0, 255 / 255.0);
  lut->SetTableValue(382, 147 / 255.0, 69 / 255.0, 18 / 255.0, 255 / 255.0);
  lut->SetTableValue(383, 4 / 255.0, 235 / 255.0, 125 / 255.0, 255 / 255.0);
  lut->SetTableValue(384, 125 / 255.0, 38 / 255.0, 205 / 255.0, 255 / 255.0);
  lut->SetTableValue(385, 243 / 255.0, 136 / 255.0, 62 / 255.0, 255 / 255.0);
  lut->SetTableValue(386, 135 / 255.0, 247 / 255.0, 4 / 255.0, 255 / 255.0);
  lut->SetTableValue(387, 147 / 255.0, 69 / 255.0, 18 / 255.0, 255 / 255.0);
  lut->SetTableValue(388, 4 / 255.0, 235 / 255.0, 125 / 255.0, 255 / 255.0);
  lut->SetTableValue(389, 125 / 255.0, 38 / 255.0, 205 / 255.0, 255 / 255.0);
  lut->SetTableValue(390, 200 / 255.0, 25 / 255.0, 140 / 255.0, 255 / 255.0);
  lut->SetTableValue(391, 200 / 255.0, 25 / 255.0, 140 / 255.0, 255 / 255.0);
  lut->SetTableValue(500, 200 / 255.0, 200 / 255.0, 200 / 255.0, 255 / 255.0);
  lut->SetTableValue(501, 200 / 255.0, 200 / 255.0, 200 / 255.0, 255 / 255.0);
  lut->SetTableValue(502, 125 / 255.0, 250 / 255.0, 20 / 255.0, 255 / 255.0);
  lut->SetTableValue(503, 125 / 255.0, 250 / 255.0, 20 / 255.0, 255 / 255.0);
  lut->SetTableValue(504, 100 / 255.0, 180 / 255.0, 255 / 255.0, 255 / 255.0);
  lut->SetTableValue(505, 100 / 255.0, 180 / 255.0, 255 / 255.0, 255 / 255.0);
  lut->SetTableValue(506, 63 / 255.0, 105 / 255.0, 225 / 255.0, 255 / 255.0);
  lut->SetTableValue(507, 63 / 255.0, 105 / 255.0, 225 / 255.0, 255 / 255.0);
  lut->SetTableValue(508, 255 / 255.0, 25 / 255.0, 130 / 255.0, 255 / 255.0);
  lut->SetTableValue(509, 255 / 255.0, 25 / 255.0, 130 / 255.0, 255 / 255.0);
  lut->SetTableValue(510, 60 / 255.0, 190 / 255.0, 130 / 255.0, 255 / 255.0);
  lut->SetTableValue(511, 60 / 255.0, 190 / 255.0, 130 / 255.0, 255 / 255.0);
  lut->SetTableValue(512, 190 / 255.0, 180 / 255.0, 105 / 255.0, 255 / 255.0);
  lut->SetTableValue(513, 190 / 255.0, 180 / 255.0, 105 / 255.0, 255 / 255.0);
  lut->SetTableValue(514, 255 / 255.0, 213 / 255.0, 0 / 255.0, 255 / 255.0);
  lut->SetTableValue(515, 255 / 255.0, 213 / 255.0, 0 / 255.0, 255 / 255.0);
  lut->SetTableValue(516, 60 / 255.0, 180 / 255.0, 180 / 255.0, 255 / 255.0);
  lut->SetTableValue(517, 60 / 255.0, 180 / 255.0, 180 / 255.0, 255 / 255.0);
  lut->SetTableValue(518, 205 / 255.0, 130 / 255.0, 0 / 255.0, 255 / 255.0);
  lut->SetTableValue(519, 205 / 255.0, 130 / 255.0, 0 / 255.0, 255 / 255.0);
  lut->SetTableValue(520, 175 / 255.0, 195 / 255.0, 220 / 255.0, 255 / 255.0);
  lut->SetTableValue(521, 175 / 255.0, 195 / 255.0, 220 / 255.0, 255 / 255.0);
  lut->SetTableValue(522, 225 / 255.0, 170 / 255.0, 105 / 255.0, 255 / 255.0);
  lut->SetTableValue(523, 225 / 255.0, 170 / 255.0, 105 / 255.0, 255 / 255.0);
  lut->SetTableValue(524, 230 / 255.0, 130 / 255.0, 130 / 255.0, 255 / 255.0);
  lut->SetTableValue(525, 230 / 255.0, 130 / 255.0, 130 / 255.0, 255 / 255.0);
  lut->SetTableValue(1000, 125 / 255.0, 5 / 255.0, 25 / 255.0, 255 / 255.0);
  lut->SetTableValue(1001, 25 / 255.0, 100 / 255.0, 4 / 255.0, 255 / 255.0);
  lut->SetTableValue(1002, 125 / 255.0, 100 / 255.0, 160 / 255.0, 255 / 255.0);
  lut->SetTableValue(1003, 100 / 255.0, 25 / 255.0, 0 / 255.0, 255 / 255.0);
  lut->SetTableValue(1005, 220 / 255.0, 20 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(1006, 185 / 255.0, 15 / 255.0, 10 / 255.0, 255 / 255.0);
  lut->SetTableValue(1007, 180 / 255.0, 220 / 255.0, 140 / 255.0, 255 / 255.0);
  lut->SetTableValue(1008, 185 / 255.0, 90 / 255.0, 185 / 255.0, 255 / 255.0);
  lut->SetTableValue(1009, 180 / 255.0, 40 / 255.0, 120 / 255.0, 255 / 255.0);
  lut->SetTableValue(1010, 140 / 255.0, 20 / 255.0, 140 / 255.0, 255 / 255.0);
  lut->SetTableValue(1011, 20 / 255.0, 30 / 255.0, 140 / 255.0, 255 / 255.0);
  lut->SetTableValue(1012, 35 / 255.0, 75 / 255.0, 50 / 255.0, 255 / 255.0);
  lut->SetTableValue(1013, 225 / 255.0, 140 / 255.0, 140 / 255.0, 255 / 255.0);
  lut->SetTableValue(1014, 200 / 255.0, 35 / 255.0, 75 / 255.0, 255 / 255.0);
  lut->SetTableValue(1015, 160 / 255.0, 100 / 255.0, 50 / 255.0, 255 / 255.0);
  lut->SetTableValue(1016, 244 / 255.0, 244 / 255.0, 24 / 255.0, 255 / 255.0);
  lut->SetTableValue(1017, 60 / 255.0, 175 / 255.0, 80 / 255.0, 255 / 255.0);
  lut->SetTableValue(1018, 220 / 255.0, 180 / 255.0, 140 / 255.0, 255 / 255.0);
  lut->SetTableValue(1019, 220 / 255.0, 140 / 255.0, 180 / 255.0, 255 / 255.0);
  lut->SetTableValue(1020, 220 / 255.0, 60 / 255.0, 20 / 255.0, 255 / 255.0);
  lut->SetTableValue(1021, 120 / 255.0, 100 / 255.0, 60 / 255.0, 255 / 255.0);
  lut->SetTableValue(1022, 195 / 255.0, 40 / 255.0, 40 / 255.0, 255 / 255.0);
  lut->SetTableValue(1023, 220 / 255.0, 180 / 255.0, 220 / 255.0, 255 / 255.0);
  lut->SetTableValue(1024, 95 / 255.0, 75 / 255.0, 175 / 255.0, 255 / 255.0);
  lut->SetTableValue(1025, 160 / 255.0, 140 / 255.0, 180 / 255.0, 255 / 255.0);
  lut->SetTableValue(1026, 80 / 255.0, 20 / 255.0, 140 / 255.0, 255 / 255.0);
  lut->SetTableValue(1027, 75 / 255.0, 50 / 255.0, 125 / 255.0, 255 / 255.0);
  lut->SetTableValue(1028, 50 / 255.0, 160 / 255.0, 150 / 255.0, 255 / 255.0);
  lut->SetTableValue(1029, 20 / 255.0, 180 / 255.0, 140 / 255.0, 255 / 255.0);
  lut->SetTableValue(1030, 140 / 255.0, 220 / 255.0, 220 / 255.0, 255 / 255.0);
  lut->SetTableValue(1031, 80 / 255.0, 160 / 255.0, 20 / 255.0, 255 / 255.0);
  lut->SetTableValue(1032, 100 / 255.0, 0 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(1033, 70 / 255.0, 70 / 255.0, 70 / 255.0, 255 / 255.0);
  lut->SetTableValue(1034, 150 / 255.0, 150 / 255.0, 200 / 255.0, 255 / 255.0);
  lut->SetTableValue(1035, 145 / 255.0, 50 / 255.0, 65 / 255.0, 255 / 255.0);
  lut->SetTableValue(2000, 125 / 255.0, 5 / 255.0, 25 / 255.0, 255 / 255.0);
  lut->SetTableValue(2001, 25 / 255.0, 100 / 255.0, 40 / 255.0, 255 / 255.0);
  lut->SetTableValue(2002, 125 / 255.0, 100 / 255.0, 160 / 255.0, 255 / 255.0);
  lut->SetTableValue(2003, 100 / 255.0, 25 / 255.0, 0 / 255.0, 255 / 255.0);
  lut->SetTableValue(2005, 220 / 255.0, 20 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(2006, 185 / 255.0, 15 / 255.0, 10 / 255.0, 255 / 255.0);
  lut->SetTableValue(2007, 180 / 255.0, 220 / 255.0, 140 / 255.0, 255 / 255.0);
  lut->SetTableValue(2008, 185 / 255.0, 90 / 255.0, 185 / 255.0, 255 / 255.0);
  lut->SetTableValue(2009, 180 / 255.0, 40 / 255.0, 120 / 255.0, 255 / 255.0);
  lut->SetTableValue(2010, 140 / 255.0, 20 / 255.0, 140 / 255.0, 255 / 255.0);
  lut->SetTableValue(2011, 20 / 255.0, 30 / 255.0, 140 / 255.0, 255 / 255.0);
  lut->SetTableValue(2012, 35 / 255.0, 75 / 255.0, 50 / 255.0, 255 / 255.0);
  lut->SetTableValue(2013, 225 / 255.0, 140 / 255.0, 140 / 255.0, 255 / 255.0);
  lut->SetTableValue(2014, 200 / 255.0, 35 / 255.0, 75 / 255.0, 255 / 255.0);
  lut->SetTableValue(2015, 160 / 255.0, 100 / 255.0, 50 / 255.0, 255 / 255.0);
  lut->SetTableValue(2016, 244 / 255.0, 244 / 255.0, 24 / 255.0, 255 / 255.0);
  lut->SetTableValue(2017, 60 / 255.0, 175 / 255.0, 80 / 255.0, 255 / 255.0);
  lut->SetTableValue(2018, 220 / 255.0, 180 / 255.0, 140 / 255.0, 255 / 255.0);
  lut->SetTableValue(2019, 220 / 255.0, 140 / 255.0, 180 / 255.0, 255 / 255.0);
  lut->SetTableValue(2020, 220 / 255.0, 60 / 255.0, 20 / 255.0, 255 / 255.0);
  lut->SetTableValue(2021, 120 / 255.0, 100 / 255.0, 60 / 255.0, 255 / 255.0);
  lut->SetTableValue(2022, 195 / 255.0, 40 / 255.0, 40 / 255.0, 255 / 255.0);
  lut->SetTableValue(2023, 220 / 255.0, 180 / 255.0, 220 / 255.0, 255 / 255.0);
  lut->SetTableValue(2024, 95 / 255.0, 75 / 255.0, 175 / 255.0, 255 / 255.0);
  lut->SetTableValue(2025, 160 / 255.0, 140 / 255.0, 180 / 255.0, 255 / 255.0);
  lut->SetTableValue(2026, 80 / 255.0, 20 / 255.0, 140 / 255.0, 255 / 255.0);
  lut->SetTableValue(2027, 75 / 255.0, 50 / 255.0, 125 / 255.0, 255 / 255.0);
  lut->SetTableValue(2028, 50 / 255.0, 160 / 255.0, 150 / 255.0, 255 / 255.0);
  lut->SetTableValue(2029, 20 / 255.0, 180 / 255.0, 140 / 255.0, 255 / 255.0);
  lut->SetTableValue(2030, 140 / 255.0, 220 / 255.0, 220 / 255.0, 255 / 255.0);
  lut->SetTableValue(2031, 80 / 255.0, 160 / 255.0, 20 / 255.0, 255 / 255.0);
  lut->SetTableValue(2032, 100 / 255.0, 0 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(2033, 70 / 255.0, 70 / 255.0, 70 / 255.0, 255 / 255.0);
  lut->SetTableValue(2034, 150 / 255.0, 150 / 255.0, 200 / 255.0, 255 / 255.0);
  lut->SetTableValue(2035, 145 / 255.0, 50 / 255.0, 65 / 255.0, 255 / 255.0);
  lut->SetTableValue(2108, 230 / 255.0, 250 / 255.0, 230 / 255.0, 255 / 255.0);
  lut->SetTableValue(3000, 230 / 255.0, 155 / 255.0, 215 / 255.0, 255 / 255.0);
  lut->SetTableValue(3001, 230 / 255.0, 155 / 255.0, 215 / 255.0, 255 / 255.0);
  lut->SetTableValue(3002, 155 / 255.0, 230 / 255.0, 255 / 255.0, 255 / 255.0);
  lut->SetTableValue(3003, 155 / 255.0, 230 / 255.0, 255 / 255.0, 255 / 255.0);
  lut->SetTableValue(3004, 97 / 255.0, 113 / 255.0, 158 / 255.0, 255 / 255.0);
  lut->SetTableValue(3005, 64 / 255.0, 123 / 255.0, 147 / 255.0, 255 / 255.0);
  lut->SetTableValue(3007, 64 / 255.0, 123 / 255.0, 147 / 255.0, 255 / 255.0);
  lut->SetTableValue(3008, 35 / 255.0, 195 / 255.0, 35 / 255.0, 255 / 255.0);
  lut->SetTableValue(3011, 60 / 255.0, 143 / 255.0, 83 / 255.0, 255 / 255.0);
  lut->SetTableValue(3012, 92 / 255.0, 162 / 255.0, 109 / 255.0, 255 / 255.0);
  lut->SetTableValue(4001, 153 / 255.0, 0 / 255.0, 51 / 255.0, 255 / 255.0);
  lut->SetTableValue(4002, 153 / 255.0, 0 / 255.0, 51 / 255.0, 255 / 255.0);
  lut->SetTableValue(4003, 133 / 255.0, 0 / 255.0, 51 / 255.0, 255 / 255.0);
  lut->SetTableValue(4004, 133 / 255.0, 0 / 255.0, 51 / 255.0, 255 / 255.0);
  lut->SetTableValue(4005, 110 / 255.0, 0 / 255.0, 51 / 255.0, 255 / 255.0);
  lut->SetTableValue(4006, 110 / 255.0, 0 / 255.0, 51 / 255.0, 255 / 255.0);
  lut->SetTableValue(4007, 90 / 255.0, 0 / 255.0, 51 / 255.0, 255 / 255.0);
  lut->SetTableValue(4008, 90 / 255.0, 0 / 255.0, 51 / 255.0, 255 / 255.0);
  lut->SetTableValue(4011, 90 / 255.0, 79 / 255.0, 130 / 255.0, 255 / 255.0);
  lut->SetTableValue(4012, 90 / 255.0, 79 / 255.0, 130 / 255.0, 255 / 255.0);
  lut->SetTableValue(4013, 111 / 255.0, 88 / 255.0, 147 / 255.0, 25 / 255.0);
  lut->SetTableValue(4014, 111 / 255.0, 88 / 255.0, 147 / 255.0, 255 / 255.0);
  lut->SetTableValue(4015, 123 / 255.0, 98 / 255.0, 130 / 255.0, 255 / 255.0);
  lut->SetTableValue(4016, 123 / 255.0, 98 / 255.0, 130 / 255.0, 255 / 255.0);
  lut->SetTableValue(4017, 135 / 255.0, 108 / 255.0, 144 / 255.0, 255 / 255.0);
  lut->SetTableValue(4018, 135 / 255.0, 108 / 255.0, 144 / 255.0, 255 / 255.0);
  lut->SetTableValue(4019, 150 / 255.0, 120 / 255.0, 160 / 255.0, 255 / 255.0);
  lut->SetTableValue(4020, 150 / 255.0, 120 / 255.0, 160 / 255.0, 255 / 255.0);
  lut->SetTableValue(4021, 170 / 255.0, 120 / 255.0, 160 / 255.0, 255 / 255.0);
  lut->SetTableValue(4022, 170 / 255.0, 120 / 255.0, 160 / 255.0, 255 / 255.0);
  lut->SetTableValue(4027, 163 / 255.0, 191 / 255.0, 218 / 255.0, 255 / 255.0);
  lut->SetTableValue(4028, 163 / 255.0, 191 / 255.0, 218 / 255.0, 255 / 255.0);
  lut->SetTableValue(4030, 180 / 255.0, 75 / 255.0, 20 / 255.0, 255 / 255.0);
  lut->SetTableValue(4031, 165 / 255.0, 85 / 255.0, 25 / 255.0, 255 / 255.0);
  lut->SetTableValue(4032, 165 / 255.0, 85 / 255.0, 25 / 255.0, 255 / 255.0);
  lut->SetTableValue(4033, 210 / 255.0, 85 / 255.0, 25 / 255.0, 255 / 255.0);
  lut->SetTableValue(4034, 210 / 255.0, 85 / 255.0, 25 / 255.0, 255 / 255.0);
  lut->SetTableValue(4035, 235 / 255.0, 85 / 255.0, 25 / 255.0, 255 / 255.0);
  lut->SetTableValue(4036, 235 / 255.0, 85 / 255.0, 25 / 255.0, 255 / 255.0);
  lut->SetTableValue(4037, 198 / 255.0, 85 / 255.0, 25 / 255.0, 255 / 255.0);
  lut->SetTableValue(4038, 198 / 255.0, 85 / 255.0, 25 / 255.0, 255 / 255.0);
  lut->SetTableValue(4041, 200 / 255.0, 125 / 255.0, 125 / 255.0, 255 / 255.0);
  lut->SetTableValue(4042, 200 / 255.0, 125 / 255.0, 125 / 255.0, 255 / 255.0);
  lut->SetTableValue(4043, 220 / 255.0, 125 / 255.0, 125 / 255.0, 255 / 255.0);
  lut->SetTableValue(4044, 220 / 255.0, 125 / 255.0, 125 / 255.0, 255 / 255.0);
  lut->SetTableValue(4045, 210 / 255.0, 125 / 255.0, 125 / 255.0, 255 / 255.0);
  lut->SetTableValue(4046, 210 / 255.0, 125 / 255.0, 125 / 255.0, 255 / 255.0);
  lut->SetTableValue(4047, 190 / 255.0, 125 / 255.0, 125 / 255.0, 255 / 255.0);
  lut->SetTableValue(4048, 190 / 255.0, 125 / 255.0, 125 / 255.0, 255 / 255.0);
  lut->SetTableValue(4050, 166 / 255.0, 84 / 255.0, 94 / 255.0, 255 / 255.0);
  lut->SetTableValue(4051, 180 / 255.0, 115 / 255.0, 115 / 255.0, 255 / 255.0);
  lut->SetTableValue(4052, 180 / 255.0, 115 / 255.0, 115 / 255.0, 255 / 255.0);
  lut->SetTableValue(4060, 150 / 255.0, 70 / 255.0, 70 / 255.0, 255 / 255.0);
  lut->SetTableValue(4061, 180 / 255.0, 70 / 255.0, 70 / 255.0, 255 / 255.0);
  lut->SetTableValue(4062, 180 / 255.0, 70 / 255.0, 70 / 255.0, 255 / 255.0);
  lut->SetTableValue(4071, 145 / 255.0, 80 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(4072, 145 / 255.0, 80 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(4073, 200 / 255.0, 100 / 255.0, 80 / 255.0, 255 / 255.0);
  lut->SetTableValue(4074, 200 / 255.0, 100 / 255.0, 80 / 255.0, 255 / 255.0);
  lut->SetTableValue(4075, 255 / 255.0, 100 / 255.0, 80 / 255.0, 255 / 255.0);
  lut->SetTableValue(4076, 255 / 255.0, 100 / 255.0, 80 / 255.0, 255 / 255.0);
  lut->SetTableValue(4077, 215 / 255.0, 100 / 255.0, 80 / 255.0, 255 / 255.0);
  lut->SetTableValue(4078, 215 / 255.0, 100 / 255.0, 80 / 255.0, 255 / 255.0);
  lut->SetTableValue(4079, 200 / 255.0, 80 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(4080, 200 / 255.0, 80 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(4081, 175 / 255.0, 80 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(4082, 175 / 255.0, 80 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(4083, 215 / 255.0, 100 / 255.0, 80 / 255.0, 255 / 255.0);
  lut->SetTableValue(4084, 215 / 255.0, 100 / 255.0, 80 / 255.0, 255 / 255.0);
  lut->SetTableValue(4085, 155 / 255.0, 80 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(4086, 155 / 255.0, 80 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(4087, 210 / 255.0, 80 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(4088, 210 / 255.0, 80 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(4089, 160 / 255.0, 80 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(4090, 160 / 255.0, 80 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(4091, 165 / 255.0, 80 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(4092, 165 / 255.0, 80 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(4093, 170 / 255.0, 80 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(4094, 170 / 255.0, 80 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(4095, 180 / 255.0, 80 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(4096, 180 / 255.0, 80 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(4097, 185 / 255.0, 80 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(4098, 185 / 255.0, 80 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(4099, 215 / 255.0, 80 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(4100, 215 / 255.0, 80 / 255.0, 100 / 255.0, 255 / 255.0);
  lut->SetTableValue(5001, 126 / 255.0, 128 / 255.0, 9 / 255.0, 255 / 255.0);
  lut->SetTableValue(5002, 126 / 255.0, 128 / 255.0, 9 / 255.0, 255 / 255.0);
  lut->SetTableValue(5003, 233 / 255.0, 227 / 255.0, 27 / 255.0, 255 / 255.0);
  lut->SetTableValue(5004, 233 / 255.0, 227 / 255.0, 27 / 255.0, 255 / 255.0);
  lut->SetTableValue(5005, 38 / 255.0, 210 / 255.0, 26 / 255.0, 255 / 255.0);
  lut->SetTableValue(5006, 38 / 255.0, 210 / 255.0, 26 / 255.0, 255 / 255.0);
  lut->SetTableValue(5007, 50 / 255.0, 227 / 255.0, 221 / 255.0, 255 / 255.0);
  lut->SetTableValue(5008, 50 / 255.0, 227 / 255.0, 221 / 255.0, 255 / 255.0);
  lut->SetTableValue(5009, 241 / 255.0, 88 / 255.0, 66 / 255.0, 255 / 255.0);
  lut->SetTableValue(5010, 241 / 255.0, 88 / 255.0, 66 / 255.0, 255 / 255.0);
  lut->SetTableValue(5011, 149 / 255.0, 124 / 255.0, 161 / 255.0, 255 / 255.0);
  lut->SetTableValue(5012, 149 / 255.0, 124 / 255.0, 161 / 255.0, 255 / 255.0);
  lut->SetTableValue(5013, 37 / 255.0, 123 / 255.0, 227 / 255.0, 255 / 255.0);
  lut->SetTableValue(5014, 37 / 255.0, 123 / 255.0, 227 / 255.0, 255 / 255.0);
  lut->SetTableValue(5015, 204 / 255.0, 84 / 255.0, 204 / 255.0, 255 / 255.0);
  lut->SetTableValue(5016, 204 / 255.0, 84 / 255.0, 204 / 255.0, 255 / 255.0);
  lut->SetTableValue(5017, 228 / 255.0, 152 / 255.0, 129 / 255.0, 255 / 255.0);
  lut->SetTableValue(5018, 228 / 255.0, 152 / 255.0, 129 / 255.0, 255 / 255.0);
  lut->SetTableValue(5019, 250 / 255.0, 240 / 255.0, 220 / 255.0, 255 / 255.0);
  lut->SetTableValue(5020, 250 / 255.0, 240 / 255.0, 220 / 255.0, 255 / 255.0);
  lut->SetTableValue(5021, 113 / 255.0, 128 / 255.0, 150 / 255.0, 255 / 255.0);
  lut->SetTableValue(5022, 113 / 255.0, 128 / 255.0, 150 / 255.0, 255 / 255.0);

  vtkSmartPointer<vtkPiecewiseFunction> opacity =
    vtkSmartPointer<vtkPiecewiseFunction>::New();
  vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction =
    vtkSmartPointer<vtkColorTransferFunction>::New();
  const int numColors = lut->GetNumberOfAvailableColors();
  double value = lut->GetRange()[0];
  const double step = (lut->GetRange()[1] - lut->GetRange()[0] + 1.0) / numColors;
  double color[4] = { 0.0, 0.0, 0.0, 1.0 };
  const double midPoint = 0.5;
  const double sharpness = 1.0;
  for (int i = 0; i < numColors; i++, value += step)
  {
    lut->GetTableValue(i, color);

    opacity->AddPoint(
      value, color[3], midPoint, sharpness);

    colorTransferFunction->AddRGBPoint(
      value, color[0], color[1], color[2], midPoint, sharpness);
  }

  vtkSmartPointer<vtkXMLImageDataReader> reader =
    vtkSmartPointer<vtkXMLImageDataReader>::New();
  char* filename =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/hncma-atlas.vti");
  reader->SetFileName(filename);
  reader->Update();
  delete [] filename;
  filename = NULL;

#ifdef GPU_MAPPER
  vtkSmartPointer<vtkGPUVolumeRayCastMapper> volumeMapper =
    vtkSmartPointer<vtkGPUVolumeRayCastMapper>::New();
#else
  vtkSmartPointer<vtkFixedPointVolumeRayCastMapper> volumeMapper =
    vtkSmartPointer<vtkFixedPointVolumeRayCastMapper>::New();
#endif

  volumeMapper->SetInputData(reader->GetOutput());

  vtkSmartPointer<vtkVolumeProperty> volumeProperty =
    vtkSmartPointer<vtkVolumeProperty>::New();
  volumeProperty->SetColor(colorTransferFunction);
  volumeProperty->SetScalarOpacity(opacity);
  volumeProperty->SetInterpolationTypeToNearest();

  vtkSmartPointer<vtkVolume> volume =
    vtkSmartPointer<vtkVolume>::New();
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  vtkSmartPointer<vtkTransform> xf =
    vtkSmartPointer<vtkTransform>::New();
  xf->RotateY(-90.0);
  xf->RotateX(180);
  volume->SetUserTransform(xf);

  vtkSmartPointer<vtkRenderWindow> renderWindow =
    vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->SetMultiSamples(0);
  renderWindow->SetSize(400, 400);

  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();
  renderer->AddVolume(volume);
  renderer->GetActiveCamera()->ParallelProjectionOn();
  renderer->GetActiveCamera()->SetFocalPoint(0, 0, -1);
  renderer->GetActiveCamera()->SetPosition(0, 0, 1);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(1.7);
  renderWindow->AddRenderer(renderer);

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkInteractorStyleTrackballCamera> style =
    vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
  iren->SetInteractorStyle(style);
  iren->SetRenderWindow(renderWindow);
  renderWindow->Render();// make sure we have an OpenGL context.

#ifdef GPU_MAPPER
  int valid = volumeMapper->IsRenderSupported(renderWindow, volumeProperty);
#else
  int valid = 1;
#endif

  int retVal;
  if (valid)
  {
    iren->Initialize();

    retVal = vtkRegressionTestImage(renderWindow.GetPointer());
    if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
      iren->Start();
    }
  }
  else
  {
    retVal = vtkTesting::PASSED;
    cout << "Required extensions not supported." << endl;
  }

  return !((retVal == vtkTesting::PASSED) ||
           (retVal == vtkTesting::DO_INTERACTOR));
}
