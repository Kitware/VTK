//VTK::System::Dec

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFXAAFilterFS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Fragment shader for vtkOpenGLFXAAFilter.
//
// Based on the following implementation and description:
//
// Whitepaper:
// http://developer.download.nvidia.com/assets/gamedev/files/sdk/11/FXAA_WhitePaper.pdf
//
// Sample implementation:
// https://github.com/NVIDIAGameWorks/GraphicsSamples/blob/master/samples/es3-kepler/FXAA/FXAA3_11.h

//VTK::Output::Dec

//======================== Debugging Options: ==================================

// Output a greyscale image showing the detected amount of subpixel aliasing.
//#define FXAA_DEBUG_SUBPIXEL_ALIASING

// Output vertical edges in red, and horizontal edges in blue.
//#define FXAA_DEBUG_EDGE_DIRECTION

// Output (number of steps taken) / (EndpointSearchIterations). Negative steps
// in the red channel, positive steps in the blue.
//#define FXAA_DEBUG_EDGE_NUM_STEPS

// Output degrees of red if the edge is near the negative edge endpoint, or
// shades of blue if near the positive edge endpoint. Pixels near an edge but
// not eligible for edgeAA (e.g. they are on the unaliased side of an edge)
// are shown in yellow.
//#define FXAA_DEBUG_EDGE_DISTANCE

// Output the length of the edge anti-aliasing offset vector in the red channel.
//#define FXAA_DEBUG_EDGE_SAMPLE_OFFSET

// Only apply a single form of anti-aliasing:
// 1 - Only apply sub-pixel anti-aliasing.
// 2 - Only apply edge anti-aliasing.
// Other / undefined - Apply both sub-pixel and edge anti-aliasing.
//#define FXAA_DEBUG_ONLY_SUBPIX_AA
//#define FXAA_DEBUG_ONLY_EDGE_AA

// Replacement stub for vtkShaderProgram::Substitute:
//VTK::DebugOptions::Def

//========================== Tuning Define: ====================================

// Which edge search implementation to use. If defined, use VTK's endpoint
// algorithm, otherwise use NVIDIA's.
//
// NVIDIA is faster, but gives poor results on single pixel lines (e.g.
// vtkPolyDataMapper's wireframe/edges). VTK is slower, but gives nicer results
// on single pixel lines.
//#define FXAA_USE_HIGH_QUALITY_ENDPOINTS;

// Replacement stub for vtkShaderProgram::Substitute:
//VTK::EndpointAlgo::Def

//========================= Input Parameters: ==================================

// Current fragment texture coordinate:
in vec2 texCoord;

// Aliased color buffer (should be sRGB, ideally)
uniform sampler2D Input;

// 1.f/Input.width, 1.f/Input.height:
uniform vec2 InvTexSize;

//======================== Tuning Parameters: ==================================

// See the vtkOpenGLFXAAFilter class documentation for details on these.

// Minimum change in luminosity (relative to maxLum) to use FXAA:
uniform float RelativeContrastThreshold;

// Absolute minimum lum change required for FXAA (overrides
// RelativeContrastThreshold value, not scaled):
uniform float HardContrastThreshold;

// Maximum amount of lowpass blending for subpixel anti-aliasing:
uniform float SubpixelBlendLimit;

// Ignore subpixel anti-aliasing that contributes less than this amount to the
// total contrast:
uniform float SubpixelContrastThreshold;

// Maximum number of steps to take when searching for line edges:
uniform int EndpointSearchIterations;

//============================ Helper Methods ==================================
// Converts rgb to luminosity:
const vec3 LUMINOSITY_VEC = vec3(0.299, 0.587, 0.114);
float luminosity(vec3 rgb)
{
  return dot(rgb, LUMINOSITY_VEC);
}

//======================= Endpoint Search Routines =============================
// Identify the endpoints of a detected edge and compute a sampling offset to
// correct for aliasing. The computed offset accounts for distance from edge
// to create a gradient of antialiased values.
//
// Input parameters:
// - posC: The texture coordinate position of the current pixel.
// - lumC: The luminosity of the current pixel.
// - lumHC: The luminosity of the highest contrast pixel to HC that is
//          perpendicular to the detected edge.
// - lengthSign: Single component magnitude and direction (in texture
//               coordinates) from the center of C pointing to HC.
// - tcPixel: (Width, Height) of a single pixel in texture coordinate units.
// - horzSpan: True if the detected edge is horizontal.
// - posEdgeAA: Output parameter with the position to resample the input texture
//              to get an edge anti-aliased rgb value for the current pixel.
//
// Implementations:
// - nvidiaEndpointSearch: The algorithm proposed by nVidia in their whitepaper
//   and sample implementations. Faster, but poorly handles single-pixel lines.
// - vtkEndpointSearch: Modified endpoint search that does more texture lookups,
//   but does better detection of single pixel line endpoints.
//
// Return values for endpoint searches:
const int FXAA_NO_EDGE_AA = 0;    // Edge AA not required.
const int FXAA_NEED_EDGE_AA = 1;  // Edge AA required.
const int FXAA_ABORT_EDGE_AA = 2; // Instruct to return. Used for debugging.

//================ nVidia's Endpoint Search Implementation =====================

int nvidiaEndpointSearch(vec2 posC, float lumC, float lumHC, float lengthSign,
                         vec2 tcPixel, bool horzSpan, out vec2 posEdgeAA)
{
  /*****************************************************************************
   * End of Edge Search                                                        *
   *===========================================================================*
   * Search along the direction of the detected edge to find both endpoints.   *
   *                                                                           *
   * We define HC as the Highest Contrast neighbor perpendicular to the edge   *
   * direction (i.e. the pixel on the other side of the edge).                 *
   *                                                                           *
   * The luminosity of HC is lumHC, the contrast between C and HC is           *
   * contrastCHC, and the average luminosity of HC and C is lumAveCHC.         *
   *                                                                           *
   * We'll walk along the edge boundary in both direction, sampling the average*
   * luminosity of the pixels on both sides of the edge: lumAveN for the       *
   * negative direction, lumAveP for the positive direction. We determine the  *
   * end of the edge to be where:                                              *
   *                                                                           *
   * abs(lumAve[NP] - lumCHC) >= contrastHC / 4.                               *
   *                                                                           *
   * which indicates that the average luminosities have diverged enough to no  *
   * longer be considered part of the edge.                                    *
   ****************************************************************************/

  float contrastCHC = abs(lumC - lumHC);

  // Point on the boundary of C and HC:
  vec2 boundaryCHC = posC; // Will be shifted later.

  // Direction of the edge
  vec2 edgeDir = vec2(0.f); // Component is set below:

  if (horzSpan)
    {
    boundaryCHC.y += lengthSign * 0.5f;
    edgeDir.x = tcPixel.x;
    }
  else
    {
    boundaryCHC.x += lengthSign * 0.5f;
    edgeDir.y = tcPixel.y;
    }

  // Prepare for the search loop:
  float contrastThreshold = contrastCHC / 4.f;
  float lumAveCHC = 0.5f * (lumC + lumHC);
  float lumAveN;
  float lumAveP;
  bool doneN = false;
  bool doneP = false;
  vec2 posN = boundaryCHC - edgeDir;
  vec2 posP = boundaryCHC + edgeDir;

#ifdef FXAA_DEBUG_EDGE_NUM_STEPS
  int stepsN = 0;
  int stepsP = 0;
#endif // FXAA_DEBUG_EDGE_NUM_STEPS

  for (int i = 0; i < EndpointSearchIterations; ++i)
    {
#ifdef FXAA_DEBUG_EDGE_NUM_STEPS
    if (!doneN) stepsN += 1;
    if (!doneP) stepsP += 1;
#endif // FXAA_DEBUG_EDGE_NUM_STEPS

    // Sample on the edge boundary in both directions:
    if (!doneN) lumAveN = luminosity(texture2D(Input, posN).rgb);
    if (!doneP) lumAveP = luminosity(texture2D(Input, posP).rgb);

    // Edge endpoint is where the contrast changes significantly:
    doneN = doneN || (abs(lumAveN - lumAveCHC) >= contrastThreshold);
    doneP = doneP || (abs(lumAveP - lumAveCHC) >= contrastThreshold);
    if (doneN && doneP) break;

    // Step to next pixel:
    if (!doneN) posN -= edgeDir;
    if (!doneP) posP += edgeDir;
    }

#ifdef FXAA_DEBUG_EDGE_NUM_STEPS
  gl_FragData[0] = vec4(float(stepsN) / float(EndpointSearchIterations), 0.f,
                        float(stepsP) / float(EndpointSearchIterations), 1.f);
  return FXAA_ABORT_EDGE_AA;
#endif // FXAA_DEBUG_EDGE_NUM_STEPS

  /*****************************************************************************
   * Edge Search Analysis                                                      *
   *===========================================================================*
   * We've located the ends of the edge at this point. Next we figure out how *
   * to interpolate the edge.                                                 *
   *                                                                          *
   * First we need to find out which end of the edge (N or P) is changing     *
   * contrast relative to boundaryCHC. This is best explained visually:       *
   *                                                                          *
   * +------------+                                                           *
   * |XX   E      |                                                           *
   * |NXXXHXXP    |                                                           *
   * |N   C  PXXXX|                                                           *
   * |           X|                                                           *
   * +------------+                                                           *
   *                                                                          *
   * In the above, an X represents a dark pixel, and a blank space is a light *
   * pixel. C is the current pixel, and H is pixel HC. The negative endpoint N*
   * of the edge is the midpoint between the first set of blank pixels to the *
   * left of C and H, while the positive endpoint P is the first set of dark  *
   * pixels to the right. The pixels under the "N" are light, while the pixels*
   * under "P" are dark. The "P" side of the edge is changing contrast        *
   * relative to C. We compute this condition as:                             *
   *                                                                          *
   * bool lumCLessThanAve = lumC < lumAveCHC;                                 *
   * bool lumNLessThanAve = lumAveN < lumAveCHC;                              *
   * bool lumPLessThanAve = lumAveP < lumAveCHC;                              *
   * bool shadeIfNearN = lumCLessThanAve != lumNLessThanAve;                  *
   * bool shadeIfNearP = lumCLessThanAve != lumPLessThanAve;                  *
   *                                                                          *
   * If shadeIfNearN is true, N is changing contrast relative to C. The same  *
   * is true for P. Thus, the change in the average contrast of the the       *
   * endpoint relative to lumAveHC must be opposite to the change in contrast *
   * from C to lumAveHC.                                                      *
   *                                                                          *
   * In addition to checking the change in contrast, we also identify which   *
   * endpoint is nearest to C. As the variable names suggest, we will only    *
   * apply edge anti-aliasing if we're nearest an endpoint that has the       *
   * desired contrast change. This prevents shading edge neighbors that do not*
   * follow the direction of the line, such as point E in the diagram.        *
   *                                                                          *
   * bool CisNearN = (norm(posN - boundaryCHC) < norm(posP - boundaryCHC));   *
   *                                                                          *
   * If both of the above conditions are met (the nearest endpoint has the    *
   * proper contrast change), then we compute the ratio of C's distance from  *
   * the desired endpoint to the total length of the edge. This ratio is the  *
   * fraction of a pixel that we shift C towards HC to resample C for         *
   * anti-aliasing.                                                           *
   ****************************************************************************/

  // Check both endpoints for the contrast change condition:
  bool lumCLessThanAve = lumC < lumAveCHC;
  bool lumNLessThanAve = lumAveN < lumAveCHC;
  bool lumPLessThanAve = lumAveP < lumAveCHC;
  bool shadeIfNearN = lumCLessThanAve != lumNLessThanAve;
  bool shadeIfNearP = lumCLessThanAve != lumPLessThanAve;

  // Identify the closest point:
  float dstN;
  float dstP;
  if (horzSpan)
    {
    dstN = boundaryCHC.x - posN.x;
    dstP = posP.x - boundaryCHC.x;
    }
  else
    {
    dstN = boundaryCHC.y - posN.y;
    dstP = posP.y - boundaryCHC.y;
    }
  bool nearestEndpointIsN = dstN < dstP;
  float dst = min(dstN, dstP);

  // Finally determine if we need shading:
  bool needEdgeAA = nearestEndpointIsN ? shadeIfNearN : shadeIfNearP;

#ifdef FXAA_DEBUG_EDGE_DISTANCE
  if (needEdgeAA)
    {
    float maxDistance = EndpointSearchIterations;
    if (nearestEndpointIsN)
      {
      gl_FragData[0] = vec4(1.f - dstN / maxDistance, 0.f, 0.f, 1.f);
      }
    else
      {
      gl_FragData[0] = vec4(0.f, 0.f, 1.f - dstP / maxDistance, 1.f);
      }
    }
  else
    {
    gl_FragData[0] = vec4(1.f, 1.f, 0.f, 1.f);
    }
  return FXAA_ABORT_EDGE_AA;
#endif // FXAA_DEBUG_EDGE_DISTANCE

  // Compute the pixel offset:
  float invNegSpanLength = -1.f / (dstN + dstP);
  float pixelOffset = dst * invNegSpanLength + 0.5;

#ifdef FXAA_DEBUG_EDGE_SAMPLE_OFFSET
  if (needEdgeAA)
    { // x2, since the max value is 0.5:
    gl_FragData[0] = vec4(-2.f * dst * invNegSpanLength, 0.f, 0.f, 1.f);
    return FXAA_ABORT_EDGE_AA;
    }
#endif // FXAA_DEBUG_EDGE_SAMPLE_OFFSET

  // Resample the edge anti-aliased value:
  posEdgeAA = posC;
  if (horzSpan)
    {
    posEdgeAA.y += pixelOffset * lengthSign;
    }
  else
    {
    posEdgeAA.x += pixelOffset * lengthSign;
    }

  return needEdgeAA ? 1 : 0;
}

//================== VTK's Endpoint Search Implementation ======================

int vtkEndpointSearch(vec2 posC, float lumC, float lumHC, float lengthSign,
                      vec2 tcPixel, bool horzSpan, out vec2 posEdgeAA)
{
  /*****************************************************************************
   * End of Edge Search                                                        *
   *===========================================================================*
   * Search along the direction of the detected edge to find both endpoints.   *
   * +------------+                                                            *
   * |X           | nVidia's endpoint detector handles this case poorly. If C  *
   * | XXXXXX  C  | is the current pixel, it will detect N as the leftmost     *
   * |       XXHXX| column of pixels, since it samples the average luminosity  *
   * |           X| at the border of the rows containing C and HC. The actual  *
   * +------------+ endpoint is 3 pixels to the left from C, but the average   *
   * luminosity does not change at this point.                                 *
   *                                                                           *
   * We adapt the algorithm to sample both rows/columns containing C and HC on *
   * the texel centers, rather than the interpolated border. We then detect    *
   * the edge endpoints when:                                                  *
   *                                                                           *
   * abs(lumHCN - lumHC) > abs(lumHCN - lumC) ||                               *
   * abs(lumCN -  lumC)  > abs(lumCN  - lumHC)                                 *
   *                                                                           *
   * where lumHCN is the luminosity of the sample in HC's row in the negative  *
   * direction, lumCN is the luminosity of the sample in C's row in the        *
   * negative direction, lumHC is the luminosity of HC, and lumC is the        *
   * luminosity of C. Thus, the endpoint is where a sampled luminosity in C's  *
   * row is closer to HC, or vice-versa. The positive endpoint is determined   *
   * similarly.                                                                *
   *                                                                           *
   * After the endpoints has been determined, we decide whether or not the     *
   * current pixel needs resampling. This is similar to nVidia's algorithm.    *
   * We determine if the luminosity of the nearest endpoint's C sample is      *
   * closer to C or HC. If it's closer to HC, it gets shaded. The resampling   *
   * offset is computed identically to nVidia's algorithm.                     *
   ****************************************************************************/

  // Point on the boundary of C and HC:
  vec2 posHC = posC; // Will be shifted later.

  // Direction of the edge
  vec2 edgeDir = vec2(0.f); // Component is set below:

  if (horzSpan)
    {
    posHC.y += lengthSign;
    edgeDir.x = tcPixel.x;
    }
  else
    {
    posHC.x += lengthSign;
    edgeDir.y = tcPixel.y;
    }

  // Prepare for the search loop:
  float lumHCN;
  float lumHCP;
  float lumCN;
  float lumCP;
  bool doneN = false;
  bool doneP = false;
  vec2 posHCN = posHC - edgeDir;
  vec2 posHCP = posHC + edgeDir;
  vec2 posCN  = posC - edgeDir;
  vec2 posCP  = posC + edgeDir;

#ifdef FXAA_DEBUG_EDGE_NUM_STEPS
  int stepsN = 0;
  int stepsP = 0;
#endif // FXAA_DEBUG_EDGE_NUM_STEPS

  for (int i = 0; i < EndpointSearchIterations; ++i)
    {
#ifdef FXAA_DEBUG_EDGE_NUM_STEPS
    if (!doneN) stepsN += 1;
    if (!doneP) stepsP += 1;
#endif // FXAA_DEBUG_EDGE_NUM_STEPS

    // Sample the luminosities along the edge:
    if (!doneN)
      {
      lumHCN = luminosity(texture2D(Input, posHCN).rgb);
      lumCN  = luminosity(texture2D(Input, posCN).rgb);
      }
    if (!doneP)
      {
      lumHCP = luminosity(texture2D(Input, posHCP).rgb);
      lumCP  = luminosity(texture2D(Input, posCP).rgb);
      }

    // Check contrast to detect endpoint:
    doneN = doneN || abs(lumHCN - lumHC) > abs(lumHCN - lumC)
                  || abs(lumCN  - lumC)  > abs(lumCN  - lumHC);
    doneP = doneP || abs(lumHCP - lumHC) > abs(lumHCP - lumC)
                  || abs(lumCP  - lumC)  > abs(lumCP  - lumHC);

    if (doneN && doneP)
      {
      break;
      }

    // Take next step.
    if (!doneN)
      {
      posHCN -= edgeDir;
      posCN  -= edgeDir;
      }
    if (!doneP)
      {
      posHCP += edgeDir;
      posCP  += edgeDir;
      }
    }

#ifdef FXAA_DEBUG_EDGE_NUM_STEPS
  gl_FragData[0] = vec4(float(stepsN) / float(EndpointSearchIterations), 0.f,
                        float(stepsP) / float(EndpointSearchIterations), 1.f);
  return FXAA_ABORT_EDGE_AA;
#endif // FXAA_DEBUG_EDGE_NUM_STEPS

  // Identify the closest point:
  float dstN;
  float dstP;

  if (horzSpan)
    {
    dstN = posC.x - posCN.x;
    dstP = posCP.x - posC.x;
    }
  else
    {
    dstN = posC.y - posCN.y;
    dstP = posCP.y - posC.y;
    }

  bool nearestEndpointIsN = dstN < dstP;
  float dst = min(dstN, dstP);
  float lumCNear = nearestEndpointIsN ? lumCN : lumCP;

  // Resample if the nearest endpoint sample in C's row is closer in luminosity
  // to HC than C.
  bool needEdgeAA = abs(lumCNear - lumHC) < abs(lumCNear - lumC);

#ifdef FXAA_DEBUG_EDGE_DISTANCE
  if (needEdgeAA)
    {
    float maxDistance = EndpointSearchIterations;
    if (nearestEndpointIsN)
      {
      gl_FragData[0] = vec4(1.f - dstN / maxDistance, 0.f, 0.f, 1.f);
      }
    else
      {
      gl_FragData[0] = vec4(0.f, 0.f, 1.f - dstP / maxDistance, 1.f);
      }
    }
  else
    {
    gl_FragData[0] = vec4(1.f, 1.f, 0.f, 1.f);
    }
  return FXAA_ABORT_EDGE_AA;
#endif // FXAA_DEBUG_EDGE_DISTANCE

  // Compute the pixel offset:
  float invNegSpanLength = -1.f / (dstN + dstP);
  float pixelOffset = dst * invNegSpanLength + 0.5f;

#ifdef FXAA_DEBUG_EDGE_SAMPLE_OFFSET
  if (needEdgeAA)
    { // x2, since the max value is 0.5:
    gl_FragData[0] = vec4(-2.f * dst * invNegSpanLength, 0.f, 0.f, 1.f);
    return FXAA_ABORT_EDGE_AA;
    }
#endif // FXAA_DEBUG_EDGE_SAMPLE_OFFSET

  // Resample the edge anti-aliased value:
  posEdgeAA = posC;
  if (horzSpan)
    {
    posEdgeAA.y += pixelOffset * lengthSign;
    }
  else
    {
    posEdgeAA.x += pixelOffset * lengthSign;
    }

  return needEdgeAA ? 1 : 0;
}

//=============================== FXAA Body ====================================

void main()
{
  // Pixel step size in texture coordinate units:
  vec2 tcPixel = InvTexSize;

  /****************************************************************************
   * Compute Local Contrast Range And Early Abort                             *
   *==========================================================================*
   * Determine the contrast range for the current pixel and its neightbors    *
   * to the North, South, West, and East. If the range is less than both of:  *
   *                                                                          *
   * a) RelativeContrastThreshold * lumMax                                    *
   *                                                                          *
   * and                                                                      *
   *                                                                          *
   * b) HardContrastThreshold                                                 *
   *                                                                          *
   * then skip anti-aliasing for this pixel.                                  *
   ****************************************************************************/

  // First compute the texture coordinates:
  vec2 tcC = texCoord;
  vec2 tcN = texCoord + vec2( 0.f,       -tcPixel.y);
  vec2 tcS = texCoord + vec2( 0.f,        tcPixel.y);
  vec2 tcW = texCoord + vec2(-tcPixel.x,  0.f);
  vec2 tcE = texCoord + vec2( tcPixel.x,  0.f);

  // Extract the rgb values of these pixels:
  vec3 rgbC = texture2D(Input, tcC).rgb;
  vec3 rgbN = texture2D(Input, tcN).rgb;
  vec3 rgbS = texture2D(Input, tcS).rgb;
  vec3 rgbW = texture2D(Input, tcW).rgb;
  vec3 rgbE = texture2D(Input, tcE).rgb;

  // Convert to luminosity:
  float lumC = luminosity(rgbC);
  float lumN = luminosity(rgbN);
  float lumS = luminosity(rgbS);
  float lumW = luminosity(rgbW);
  float lumE = luminosity(rgbE);

  // The min, max, and range of luminosity for CNSWE:
  float lumMin = min(lumC, min(min(lumN, lumS), min(lumW, lumE)));
  float lumMax = max(lumC, max(max(lumN, lumS), max(lumW, lumE)));
  float lumRange = lumMax - lumMin;
  float lumThresh = max(HardContrastThreshold,
                        RelativeContrastThreshold * lumMax);

  // Don't apply FXAA unless there's a significant change in luminosity around
  // the current pixel:
  if (lumRange < lumThresh)
    {
    gl_FragData[0] = vec4(rgbC, 1.f); // original color
    return;
    }

  /****************************************************************************
   * Fetch texels for complete 3x3 neighborhood.                              *
   ****************************************************************************/

  // Fetch additional texels for edge detection / subpixel antialiasing:
  vec2 tcNE = texCoord + vec2( tcPixel.x, -tcPixel.y);
  vec2 tcSE = texCoord + vec2( tcPixel.x,  tcPixel.y);
  vec2 tcNW = texCoord + vec2(-tcPixel.x, -tcPixel.y);
  vec2 tcSW = texCoord + vec2(-tcPixel.x,  tcPixel.y);
  vec3 rgbNE = texture2D(Input, tcNE).rgb;
  vec3 rgbSE = texture2D(Input, tcSE).rgb;
  vec3 rgbNW = texture2D(Input, tcNW).rgb;
  vec3 rgbSW = texture2D(Input, tcSW).rgb;
  float lumNE = luminosity(rgbNE);
  float lumSE = luminosity(rgbSE);
  float lumNW = luminosity(rgbNW);
  float lumSW = luminosity(rgbSW);

  // Precompute some combined luminosities. These are reused later.
  float lumNS = lumN + lumS;
  float lumWE = lumW + lumE;
  float lumNSWE = lumNS + lumWE;
  float lumNWNE = lumNW + lumNE;
  float lumSWSE = lumSW + lumSE;
  float lumNWSW = lumNW + lumSW;
  float lumNESE = lumNE + lumSE;

  /****************************************************************************
   * Subpixel Anti-aliasing                                                   *
   *==========================================================================*
   * Check if the current pixel is very high contrast to it's neighbors (e.g. *
   * specular aliasing, noisy shadow textures, etc). If it is, compute the    *
   * average color over the 3x3 neighborhood and a blending factor.           *
   *                                                                          *
   * The blending factor is computed as the minimum of:                       *
   *                                                                          *
   * 1) max(0.f, abs([average NSWE lum] - lumC) - SubpixelContrastThreshold)  *
   *                  FXAA_SUBPIX_TRIM_SCALE                                  *
   *                                                                          *
   * or                                                                       *
   *                                                                          *
   * 2) SubpixelBlendLimit                                                    *
   ****************************************************************************/

  // Check for sub-pixel aliasing (e.g. current pixel has high contrast from
  // neighbors):
  float lumAveNSWE = 0.25f * (lumNSWE);
  float lumSubRange = abs(lumAveNSWE - lumC);

  // Compute the subpixel blend amount:
  float blendSub = max(0.f, (lumSubRange / lumRange) -
                            SubpixelContrastThreshold);
  blendSub = min(SubpixelBlendLimit,
                 blendSub * (1.f / (1.f - SubpixelContrastThreshold)));

#ifdef FXAA_DEBUG_SUBPIXEL_ALIASING
  if (blendSub > 0.f)
    {
    gl_FragData[0] = vec4(vec3(blendSub / SubpixelBlendLimit), 1.f);
    }
  else
    {
    gl_FragData[0] = vec4(rgbC, 1.f);
    }
  return;
#endif // FXAA_DEBUG_SUBPIXEL_ALIASING

  // Compute the subpixel blend color. Average the 3x3 neighborhood:
  vec3 rgbSub = (1.f/9.f) *
      (rgbNW + rgbN + rgbNE +
       rgbW  + rgbC + rgbE  +
       rgbSW + rgbS + rgbSE);

  /****************************************************************************
   * Edge Testing                                                             *
   *==========================================================================*
   * Apply vertical and horizontal edge detection techniques to determine the *
   * direction of any edges in the 3x3 neighborhood.                          *
   ****************************************************************************/

  // Check for vertical edge. Pixel coeffecients are:
  //  1 -2  1
  //  2 -4  2
  //  1 -2  1
  // The absolute value of each row is taken, summed, and divided by 12.
  // Operations are decomposed here to take advantage of FMA ops.
  float edgeVertRow1 = abs(-2.f * lumN + lumNWNE);
  float edgeVertRow2 = abs(-2.f * lumC + lumWE);
  float edgeVertRow3 = abs(-2.f * lumS + lumSWSE);
  float edgeVert = ((2.f * edgeVertRow2 + edgeVertRow1) + edgeVertRow3) / 12.f;

  // Check for horizontal edge. Pixel coeffecients are:
  //  1  2  1
  // -2 -4 -2
  //  1  2  1
  // The absolute value of each column is taken, summed, and divided by 12.
  // Operations are decomposed here to take advantage of FMA ops.
  float edgeHorzCol1 = abs(-2.f * lumW + lumNWSW);
  float edgeHorzCol2 = abs(-2.f * lumC + lumNS);
  float edgeHorzCol3 = abs(-2.f * lumE + lumNESE);
  float edgeHorz = ((2.f * edgeHorzCol2 + edgeHorzCol1) + edgeHorzCol3) / 12.f;

  // Indicates that the edge span is horizontal:
  bool horzSpan = edgeHorz >= edgeVert;

#ifdef FXAA_DEBUG_EDGE_DIRECTION
  gl_FragData[0] = horzSpan ? vec4(0.f, 0.f, 1.f, 1.f)
                            : vec4(1.f, 0.f, 0.f, 1.f);
  return;
#endif // FXAA_DEBUG_EDGE_DIRECTION

  /****************************************************************************
   * Endpoint Search Preparation                                              *
   *==========================================================================*
   * Compute inputs for an endpoint detection algorithm. Mainly concerned     *
   * locating HC -- the Highest Contrast pixel (relative to C) that's on the  *
   * opposite side of the detected edge from C.                               *
   ****************************************************************************/

  // The two neighbor pixels perpendicular to the edge:
  float lumHC1;
  float lumHC2;

  // Single-pixel texture coordinate offset that points from C to HC.
  float lengthSign;

  if (horzSpan)
    {
    lumHC1 = lumN;
    lumHC2 = lumS;
    lengthSign = -tcPixel.y; // Assume N for now.
    }
  else
    {
    lumHC1 = lumW;
    lumHC2 = lumE;
    lengthSign = -tcPixel.x; // Assume W for now.
    }

  // Luminosity of the NSWE pixel perpendicular to the edge with the highest
  // contrast to C:
  float lumHC;
  if (abs(lumC - lumHC1) >= abs(lumC - lumHC2))
    {
    lumHC = lumHC1;
    }
  else
    {
    lumHC = lumHC2;
    // Also reverse the offset direction in this case:
    lengthSign = -lengthSign;
    }

  vec2 posEdgeAA; // Position to resample C at to get edge-antialiasing.

#ifdef FXAA_USE_HIGH_QUALITY_ENDPOINTS
  int endpointResult = vtkEndpointSearch(tcC, lumC, lumHC, lengthSign,
                                         tcPixel, horzSpan, posEdgeAA);
#else // FXAA_USE_HIGH_QUALITY_ENDPOINTS
  int endpointResult = nvidiaEndpointSearch(tcC, lumC, lumHC, lengthSign,
                                            tcPixel, horzSpan, posEdgeAA);
#endif // FXAA_USE_HIGH_QUALITY_ENDPOINTS

  // Only sample texture if needed. Reuse rgbC otherwise.
  vec3 rgbEdgeAA = rgbC;

  switch (endpointResult)
    {
    case FXAA_ABORT_EDGE_AA: // Used for debugging (endpoint search set colors)
      return;

    case FXAA_NEED_EDGE_AA: // Resample the texture at the requested position.
      rgbEdgeAA = texture2D(Input, posEdgeAA).rgb;
      break;

    case FXAA_NO_EDGE_AA: // Current pixel does not need edge anti-aliasing.
    default:
      break;
    }

#ifdef FXAA_DEBUG_ONLY_SUBPIX_AA
  rgbEdgeAA = rgbC;
#endif // FXAA_DEBUG_ONLY_SUBPIX_AA
#ifdef FXAA_DEBUG_ONLY_EDGE_AA
  blendSub = 0.f;
#endif // FXAA_DEBUG_ONLY_EDGE_AA

  // Blend the edgeAA and subpixelAA results together:
  gl_FragData[0] = vec4(mix(rgbEdgeAA, rgbSub, blendSub), 1.f);
}
