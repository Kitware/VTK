#include "vtkDEMReader.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkProperty.h"
#include "vtkCamera.h"
#include "vtkImageDataGeometryFilter.h"
#include "vtkWarpScalar.h"
#include "vtkPolyDataNormals.h"
#include "vtkLODActor.h"
#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkPolyDataCollection.h"
#include "vtkTriangleFilter.h"
#include "vtkImageResample.h"
#include "vtkInteractorEventRecorder.h"

#include "vtkContourWidget.h"
#include "vtkOrientedGlyphContourRepresentation.h"
#include "vtkPolygonalSurfacePointPlacer.h"
#include "vtkPolygonalSurfaceContourLineInterpolator.h"
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

char TestDijkstraGraphGeodesicPathLog[] =
"# StreamVersion 1\n"
"EnterEvent 260 15 0 0 0 0 0 i\n"
"MouseMoveEvent 186 15 0 0 0 0 0 i\n"
"MouseMoveEvent 124 15 0 0 0 0 0 i\n"
"MouseMoveEvent 74 11 0 0 0 0 0 i\n"
"MouseMoveEvent 30 7 0 0 0 0 0 i\n"
"LeaveEvent -5 3 0 0 0 0 0 i\n"
"EnterEvent 7 5 0 0 0 0 0 i\n"
"MouseMoveEvent 17 15 0 0 0 0 0 i\n"
"MouseMoveEvent 29 29 0 0 0 0 0 i\n"
"MouseMoveEvent 37 41 0 0 0 0 0 i\n"
"MouseMoveEvent 45 59 0 0 0 0 0 i\n"
"MouseMoveEvent 55 77 0 0 0 0 0 i\n"
"MouseMoveEvent 63 93 0 0 0 0 0 i\n"
"MouseMoveEvent 71 111 0 0 0 0 0 i\n"
"MouseMoveEvent 81 127 0 0 0 0 0 i\n"
"MouseMoveEvent 87 143 0 0 0 0 0 i\n"
"MouseMoveEvent 95 157 0 0 0 0 0 i\n"
"MouseMoveEvent 97 169 0 0 0 0 0 i\n"
"MouseMoveEvent 99 175 0 0 0 0 0 i\n"
"MouseMoveEvent 99 183 0 0 0 0 0 i\n"
"MouseMoveEvent 99 184 0 0 0 0 0 i\n"
"MouseMoveEvent 98 184 0 0 0 0 0 i\n"
"MouseMoveEvent 90 184 0 0 0 0 0 i\n"
"MouseMoveEvent 87 184 0 0 0 0 0 i\n"
"MouseMoveEvent 79 182 0 0 0 0 0 i\n"
"MouseMoveEvent 73 180 0 0 0 0 0 i\n"
"MouseMoveEvent 72 179 0 0 0 0 0 i\n"
"MouseMoveEvent 71 179 0 0 0 0 0 i\n"
"MouseMoveEvent 70 179 0 0 0 0 0 i\n"
"MouseMoveEvent 69 180 0 0 0 0 0 i\n"
"MouseMoveEvent 68 181 0 0 0 0 0 i\n"
"MouseMoveEvent 67 182 0 0 0 0 0 i\n"
"MouseMoveEvent 67 184 0 0 0 0 0 i\n"
"MouseMoveEvent 66 185 0 0 0 0 0 i\n"
"MouseMoveEvent 62 189 0 0 0 0 0 i\n"
"MouseMoveEvent 61 191 0 0 0 0 0 i\n"
"MouseMoveEvent 60 192 0 0 0 0 0 i\n"
"MouseMoveEvent 58 193 0 0 0 0 0 i\n"
"MouseMoveEvent 57 194 0 0 0 0 0 i\n"
"MouseMoveEvent 56 195 0 0 0 0 0 i\n"
"MouseMoveEvent 54 196 0 0 0 0 0 i\n"
"MouseMoveEvent 53 197 0 0 0 0 0 i\n"
"MouseMoveEvent 47 199 0 0 0 0 0 i\n"
"MouseMoveEvent 46 200 0 0 0 0 0 i\n"
"MouseMoveEvent 45 201 0 0 0 0 0 i\n"
"MouseMoveEvent 43 201 0 0 0 0 0 i\n"
"MouseMoveEvent 42 202 0 0 0 0 0 i\n"
"MouseMoveEvent 41 203 0 0 0 0 0 i\n"
"MouseMoveEvent 40 203 0 0 0 0 0 i\n"
"MouseMoveEvent 39 204 0 0 0 0 0 i\n"
"MouseMoveEvent 38 204 0 0 0 0 0 i\n"
"MouseMoveEvent 37 204 0 0 0 0 0 i\n"
"LeftButtonPressEvent 37 204 0 0 0 0 0 i\n"
"RenderEvent 37 204 0 0 0 0 0 i\n"
"LeftButtonReleaseEvent 37 204 0 0 0 0 0 i\n"
"MouseMoveEvent 67 198 0 0 0 0 0 i\n"
"MouseMoveEvent 68 198 0 0 0 0 0 i\n"
"MouseMoveEvent 69 197 0 0 0 0 0 i\n"
"MouseMoveEvent 70 197 0 0 0 0 0 i\n"
"MouseMoveEvent 71 196 0 0 0 0 0 i\n"
"MouseMoveEvent 72 196 0 0 0 0 0 i\n"
"MouseMoveEvent 73 196 0 0 0 0 0 i\n"
"MouseMoveEvent 73 195 0 0 0 0 0 i\n"
"MouseMoveEvent 74 195 0 0 0 0 0 i\n"
"MouseMoveEvent 75 195 0 0 0 0 0 i\n"
"MouseMoveEvent 76 195 0 0 0 0 0 i\n"
"MouseMoveEvent 77 195 0 0 0 0 0 i\n"
"LeftButtonPressEvent 77 195 0 0 0 0 0 i\n"
"RenderEvent 77 195 0 0 0 0 0 i\n"
"LeftButtonReleaseEvent 77 195 0 0 0 0 0 i\n"
"MouseMoveEvent 105 159 0 0 0 0 0 i\n"
"LeftButtonPressEvent 105 159 0 0 0 0 0 i\n"
"RenderEvent 105 159 0 0 0 0 0 i\n"
"LeftButtonReleaseEvent 105 159 0 0 0 0 0 i\n"
"MouseMoveEvent 103 122 0 0 0 0 0 i\n"
"MouseMoveEvent 103 121 0 0 0 0 0 i\n"
"MouseMoveEvent 104 120 0 0 0 0 0 i\n"
"MouseMoveEvent 105 119 0 0 0 0 0 i\n"
"MouseMoveEvent 106 119 0 0 0 0 0 i\n"
"MouseMoveEvent 107 119 0 0 0 0 0 i\n"
"MouseMoveEvent 108 119 0 0 0 0 0 i\n"
"MouseMoveEvent 108 118 0 0 0 0 0 i\n"
"MouseMoveEvent 109 118 0 0 0 0 0 i\n"
"MouseMoveEvent 110 118 0 0 0 0 0 i\n"
"MouseMoveEvent 111 117 0 0 0 0 0 i\n"
"MouseMoveEvent 112 117 0 0 0 0 0 i\n"
"MouseMoveEvent 112 116 0 0 0 0 0 i\n"
"MouseMoveEvent 113 116 0 0 0 0 0 i\n"
"MouseMoveEvent 114 116 0 0 0 0 0 i\n"
"LeftButtonPressEvent 114 116 0 0 0 0 0 i\n"
"RenderEvent 114 116 0 0 0 0 0 i\n"
"LeftButtonReleaseEvent 114 116 0 0 0 0 0 i\n"
"MouseMoveEvent 141 82 0 0 0 0 0 i\n"
"MouseMoveEvent 142 82 0 0 0 0 0 i\n"
"MouseMoveEvent 143 81 0 0 0 0 0 i\n"
"MouseMoveEvent 144 81 0 0 0 0 0 i\n"
"MouseMoveEvent 146 81 0 0 0 0 0 i\n"
"MouseMoveEvent 150 77 0 0 0 0 0 i\n"
"MouseMoveEvent 152 76 0 0 0 0 0 i\n"
"MouseMoveEvent 154 75 0 0 0 0 0 i\n"
"MouseMoveEvent 156 75 0 0 0 0 0 i\n"
"MouseMoveEvent 156 74 0 0 0 0 0 i\n"
"MouseMoveEvent 157 74 0 0 0 0 0 i\n"
"MouseMoveEvent 158 74 0 0 0 0 0 i\n"
"MouseMoveEvent 159 73 0 0 0 0 0 i\n"
"MouseMoveEvent 160 72 0 0 0 0 0 i\n"
"MouseMoveEvent 161 71 0 0 0 0 0 i\n"
"MouseMoveEvent 162 71 0 0 0 0 0 i\n"
"MouseMoveEvent 163 70 0 0 0 0 0 i\n"
"MouseMoveEvent 164 68 0 0 0 0 0 i\n"
"MouseMoveEvent 166 67 0 0 0 0 0 i\n"
"MouseMoveEvent 168 66 0 0 0 0 0 i\n"
"MouseMoveEvent 172 62 0 0 0 0 0 i\n"
"MouseMoveEvent 173 61 0 0 0 0 0 i\n"
"MouseMoveEvent 174 61 0 0 0 0 0 i\n"
"MouseMoveEvent 175 61 0 0 0 0 0 i\n"
"MouseMoveEvent 176 60 0 0 0 0 0 i\n"
"MouseMoveEvent 180 56 0 0 0 0 0 i\n"
"MouseMoveEvent 182 55 0 0 0 0 0 i\n"
"MouseMoveEvent 184 54 0 0 0 0 0 i\n"
"MouseMoveEvent 190 50 0 0 0 0 0 i\n"
"MouseMoveEvent 191 49 0 0 0 0 0 i\n"
"MouseMoveEvent 192 49 0 0 0 0 0 i\n"
"MouseMoveEvent 193 49 0 0 0 0 0 i\n"
"MouseMoveEvent 193 50 0 0 0 0 0 i\n"
"MouseMoveEvent 193 51 0 0 0 0 0 i\n"
"MouseMoveEvent 193 52 0 0 0 0 0 i\n"
"MouseMoveEvent 193 53 0 0 0 0 0 i\n"
"MouseMoveEvent 193 54 0 0 0 0 0 i\n"
"MouseMoveEvent 193 55 0 0 0 0 0 i\n"
"RightButtonPressEvent 193 55 0 0 0 0 0 i\n"
"RenderEvent 193 55 0 0 0 0 0 i\n"
"RightButtonReleaseEvent 193 55 0 0 0 0 0 i\n"
"MouseMoveEvent 193 54 0 0 0 0 0 i\n"
"RenderEvent 193 54 0 0 0 0 0 i\n"
"MouseMoveEvent 193 53 0 0 0 0 0 i\n"
"MouseMoveEvent 193 52 0 0 0 0 0 i\n"
"MouseMoveEvent 193 51 0 0 0 0 0 i\n"
"MouseMoveEvent 193 50 0 0 0 0 0 i\n"
"MouseMoveEvent 193 49 0 0 0 0 0 i\n"
"MouseMoveEvent 193 48 0 0 0 0 0 i\n"
"MouseMoveEvent 193 47 0 0 0 0 0 i\n"
"RenderEvent 193 47 0 0 0 0 0 i\n"
"MouseMoveEvent 194 47 0 0 0 0 0 i\n"
"MouseMoveEvent 195 47 0 0 0 0 0 i\n"
"MouseMoveEvent 197 47 0 0 0 0 0 i\n"
"MouseMoveEvent 198 47 0 0 0 0 0 i\n"
"MouseMoveEvent 199 47 0 0 0 0 0 i\n"
"MouseMoveEvent 200 47 0 0 0 0 0 i\n"
"MouseMoveEvent 201 47 0 0 0 0 0 i\n"
"MouseMoveEvent 202 48 0 0 0 0 0 i\n"
"MouseMoveEvent 203 49 0 0 0 0 0 i\n"
"MouseMoveEvent 203 50 0 0 0 0 0 i\n"
"MouseMoveEvent 204 51 0 0 0 0 0 i\n"
"MouseMoveEvent 204 52 0 0 0 0 0 i\n"
"MouseMoveEvent 205 53 0 0 0 0 0 i\n"
"MouseMoveEvent 205 54 0 0 0 0 0 i\n"
"MouseMoveEvent 205 55 0 0 0 0 0 i\n"
"MouseMoveEvent 205 56 0 0 0 0 0 i\n"
"MouseMoveEvent 205 57 0 0 0 0 0 i\n"
"MouseMoveEvent 205 58 0 0 0 0 0 i\n"
"MouseMoveEvent 206 59 0 0 0 0 0 i\n"
"MouseMoveEvent 206 60 0 0 0 0 0 i\n"
"MouseMoveEvent 206 61 0 0 0 0 0 i\n"
"MouseMoveEvent 206 62 0 0 0 0 0 i\n"
"MouseMoveEvent 206 63 0 0 0 0 0 i\n"
"MouseMoveEvent 206 65 0 0 0 0 0 i\n"
"MouseMoveEvent 206 66 0 0 0 0 0 i\n"
"MouseMoveEvent 206 67 0 0 0 0 0 i\n"
"MouseMoveEvent 206 69 0 0 0 0 0 i\n"
"MouseMoveEvent 206 70 0 0 0 0 0 i\n"
"MouseMoveEvent 206 71 0 0 0 0 0 i\n"
"MouseMoveEvent 206 72 0 0 0 0 0 i\n"
"MouseMoveEvent 206 73 0 0 0 0 0 i\n"
"MouseMoveEvent 206 74 0 0 0 0 0 i\n"
"MouseMoveEvent 206 75 0 0 0 0 0 i\n"
"MouseMoveEvent 206 76 0 0 0 0 0 i\n"
"MouseMoveEvent 206 77 0 0 0 0 0 i\n"
"MouseMoveEvent 206 78 0 0 0 0 0 i\n"
"MouseMoveEvent 206 79 0 0 0 0 0 i\n"
"MouseMoveEvent 206 80 0 0 0 0 0 i\n"
"MouseMoveEvent 206 81 0 0 0 0 0 i\n"
"MouseMoveEvent 206 82 0 0 0 0 0 i\n"
"MouseMoveEvent 206 83 0 0 0 0 0 i\n"
"KeyPressEvent 206 83 0 0 116 1 t i\n"
"CharEvent 206 83 0 0 116 1 t i\n"
"KeyReleaseEvent 206 83 0 0 116 1 t i\n"
"RightButtonPressEvent 206 83 0 0 0 0 t i\n"
"RenderEvent 206 85 0 0 0 0 t i\n"
"RenderEvent 206 86 0 0 0 0 t i\n"
"RenderEvent 206 87 0 0 0 0 t i\n"
"RenderEvent 206 90 0 0 0 0 t i\n"
"RenderEvent 206 91 0 0 0 0 t i\n"
"RenderEvent 206 92 0 0 0 0 t i\n"
"RenderEvent 206 93 0 0 0 0 t i\n"
"RenderEvent 206 94 0 0 0 0 t i\n"
"RenderEvent 206 96 0 0 0 0 t i\n"
"RenderEvent 206 97 0 0 0 0 t i\n"
"RenderEvent 206 98 0 0 0 0 t i\n"
"RenderEvent 206 99 0 0 0 0 t i\n"
"RenderEvent 206 100 0 0 0 0 t i\n"
"RenderEvent 206 101 0 0 0 0 t i\n"
"RenderEvent 206 102 0 0 0 0 t i\n"
"RenderEvent 206 103 0 0 0 0 t i\n"
"RenderEvent 206 105 0 0 0 0 t i\n"
"RenderEvent 206 106 0 0 0 0 t i\n"
"RenderEvent 206 107 0 0 0 0 t i\n"
"RenderEvent 206 109 0 0 0 0 t i\n"
"RenderEvent 206 110 0 0 0 0 t i\n"
"RenderEvent 206 111 0 0 0 0 t i\n"
"RenderEvent 206 113 0 0 0 0 t i\n"
"RenderEvent 206 114 0 0 0 0 t i\n"
"RenderEvent 206 115 0 0 0 0 t i\n"
"RenderEvent 206 117 0 0 0 0 t i\n"
"RenderEvent 206 119 0 0 0 0 t i\n"
"RenderEvent 206 121 0 0 0 0 t i\n"
"RenderEvent 206 123 0 0 0 0 t i\n"
"RenderEvent 206 126 0 0 0 0 t i\n"
"RenderEvent 206 127 0 0 0 0 t i\n"
"RenderEvent 205 130 0 0 0 0 t i\n"
"RenderEvent 205 133 0 0 0 0 t i\n"
"RenderEvent 205 134 0 0 0 0 t i\n"
"RenderEvent 205 138 0 0 0 0 t i\n"
"RenderEvent 205 142 0 0 0 0 t i\n"
"RenderEvent 205 146 0 0 0 0 t i\n"
"RenderEvent 205 148 0 0 0 0 t i\n"
"RenderEvent 205 150 0 0 0 0 t i\n"
"RenderEvent 205 152 0 0 0 0 t i\n"
"RenderEvent 206 154 0 0 0 0 t i\n"
"RenderEvent 206 156 0 0 0 0 t i\n"
"RenderEvent 206 158 0 0 0 0 t i\n"
"RenderEvent 207 159 0 0 0 0 t i\n"
"RenderEvent 207 160 0 0 0 0 t i\n"
"RenderEvent 207 162 0 0 0 0 t i\n"
"RenderEvent 207 163 0 0 0 0 t i\n"
"RenderEvent 207 165 0 0 0 0 t i\n"
"RenderEvent 207 166 0 0 0 0 t i\n"
"RenderEvent 207 167 0 0 0 0 t i\n"
"RenderEvent 207 169 0 0 0 0 t i\n"
"RenderEvent 207 170 0 0 0 0 t i\n"
"RenderEvent 207 171 0 0 0 0 t i\n"
"RenderEvent 207 172 0 0 0 0 t i\n"
"RenderEvent 207 173 0 0 0 0 t i\n"
"RenderEvent 207 174 0 0 0 0 t i\n"
"RenderEvent 207 175 0 0 0 0 t i\n"
"RenderEvent 207 176 0 0 0 0 t i\n"
"RenderEvent 207 177 0 0 0 0 t i\n"
"RenderEvent 207 178 0 0 0 0 t i\n"
"RenderEvent 208 179 0 0 0 0 t i\n"
"RenderEvent 208 180 0 0 0 0 t i\n"
"RenderEvent 208 181 0 0 0 0 t i\n"
"RenderEvent 208 182 0 0 0 0 t i\n"
"RenderEvent 209 183 0 0 0 0 t i\n"
"RenderEvent 209 185 0 0 0 0 t i\n"
"RenderEvent 209 186 0 0 0 0 t i\n"
"RenderEvent 209 187 0 0 0 0 t i\n"
"RenderEvent 210 189 0 0 0 0 t i\n"
"RenderEvent 210 190 0 0 0 0 t i\n"
"RenderEvent 210 191 0 0 0 0 t i\n"
"RenderEvent 210 193 0 0 0 0 t i\n"
"RenderEvent 210 194 0 0 0 0 t i\n"
"RenderEvent 210 195 0 0 0 0 t i\n"
"RenderEvent 210 197 0 0 0 0 t i\n"
"RenderEvent 211 198 0 0 0 0 t i\n"
"RenderEvent 211 199 0 0 0 0 t i\n"
"RenderEvent 211 201 0 0 0 0 t i\n"
"RenderEvent 212 202 0 0 0 0 t i\n"
"RenderEvent 212 204 0 0 0 0 t i\n"
"RenderEvent 213 206 0 0 0 0 t i\n"
"RenderEvent 213 208 0 0 0 0 t i\n"
"RenderEvent 213 209 0 0 0 0 t i\n"
"RenderEvent 214 210 0 0 0 0 t i\n"
"RenderEvent 214 211 0 0 0 0 t i\n"
"RenderEvent 214 212 0 0 0 0 t i\n"
"RenderEvent 214 213 0 0 0 0 t i\n"
"RenderEvent 214 214 0 0 0 0 t i\n"
"RenderEvent 215 214 0 0 0 0 t i\n"
"RenderEvent 215 212 0 0 0 0 t i\n"
"RenderEvent 215 211 0 0 0 0 t i\n"
"RenderEvent 215 210 0 0 0 0 t i\n"
"RenderEvent 214 208 0 0 0 0 t i\n"
"RenderEvent 214 207 0 0 0 0 t i\n"
"RenderEvent 214 206 0 0 0 0 t i\n"
"RenderEvent 214 205 0 0 0 0 t i\n"
"RenderEvent 214 204 0 0 0 0 t i\n"
"RenderEvent 214 203 0 0 0 0 t i\n"
"RenderEvent 214 201 0 0 0 0 t i\n"
"RenderEvent 214 199 0 0 0 0 t i\n"
"RenderEvent 213 198 0 0 0 0 t i\n"
"RenderEvent 213 195 0 0 0 0 t i\n"
"RenderEvent 212 194 0 0 0 0 t i\n"
"RenderEvent 212 192 0 0 0 0 t i\n"
"RenderEvent 212 191 0 0 0 0 t i\n"
"RenderEvent 211 189 0 0 0 0 t i\n"
"RenderEvent 211 187 0 0 0 0 t i\n"
"RenderEvent 211 185 0 0 0 0 t i\n"
"RenderEvent 211 183 0 0 0 0 t i\n"
"RenderEvent 210 182 0 0 0 0 t i\n"
"RenderEvent 210 180 0 0 0 0 t i\n"
"RenderEvent 209 179 0 0 0 0 t i\n"
"RenderEvent 209 178 0 0 0 0 t i\n"
"RenderEvent 208 176 0 0 0 0 t i\n"
"RenderEvent 208 175 0 0 0 0 t i\n"
"RenderEvent 208 174 0 0 0 0 t i\n"
"RenderEvent 207 171 0 0 0 0 t i\n"
"RenderEvent 207 170 0 0 0 0 t i\n"
"RenderEvent 207 168 0 0 0 0 t i\n"
"RenderEvent 206 167 0 0 0 0 t i\n"
"RenderEvent 206 165 0 0 0 0 t i\n"
"RenderEvent 205 164 0 0 0 0 t i\n"
"RenderEvent 204 163 0 0 0 0 t i\n"
"RenderEvent 204 160 0 0 0 0 t i\n"
"RenderEvent 203 159 0 0 0 0 t i\n"
"RenderEvent 203 158 0 0 0 0 t i\n"
"RenderEvent 202 155 0 0 0 0 t i\n"
"RenderEvent 202 152 0 0 0 0 t i\n"
"RenderEvent 201 151 0 0 0 0 t i\n"
"RenderEvent 200 148 0 0 0 0 t i\n"
"RenderEvent 199 144 0 0 0 0 t i\n"
"RenderEvent 199 143 0 0 0 0 t i\n"
"RenderEvent 199 139 0 0 0 0 t i\n"
"RenderEvent 198 138 0 0 0 0 t i\n"
"RenderEvent 198 136 0 0 0 0 t i\n"
"RenderEvent 198 133 0 0 0 0 t i\n"
"RenderEvent 197 131 0 0 0 0 t i\n"
"RenderEvent 197 130 0 0 0 0 t i\n"
"RenderEvent 197 127 0 0 0 0 t i\n"
"RenderEvent 197 126 0 0 0 0 t i\n"
"RenderEvent 196 125 0 0 0 0 t i\n"
"RenderEvent 196 123 0 0 0 0 t i\n"
"RenderEvent 196 122 0 0 0 0 t i\n"
"RenderEvent 196 121 0 0 0 0 t i\n"
"RenderEvent 196 120 0 0 0 0 t i\n"
"RenderEvent 196 119 0 0 0 0 t i\n"
"RenderEvent 196 118 0 0 0 0 t i\n"
"RenderEvent 196 117 0 0 0 0 t i\n"
"RenderEvent 196 116 0 0 0 0 t i\n"
"RenderEvent 195 116 0 0 0 0 t i\n"
"RenderEvent 195 116 0 0 0 0 t i\n"
"MouseMoveEvent 196 114 0 0 0 0 t i\n"
"MouseMoveEvent 198 108 0 0 0 0 t i\n"
"MouseMoveEvent 199 106 0 0 0 0 t i\n"
"MouseMoveEvent 203 100 0 0 0 0 t i\n"
"MouseMoveEvent 205 94 0 0 0 0 t i\n"
"MouseMoveEvent 209 88 0 0 0 0 t i\n"
"MouseMoveEvent 210 86 0 0 0 0 t i\n"
"MouseMoveEvent 211 84 0 0 0 0 t i\n"
"MouseMoveEvent 211 83 0 0 0 0 t i\n"
"MouseMoveEvent 211 82 0 0 0 0 t i\n"
"MouseMoveEvent 211 80 0 0 0 0 t i\n"
"MouseMoveEvent 211 79 0 0 0 0 t i\n"
"MouseMoveEvent 212 77 0 0 0 0 t i\n"
"MouseMoveEvent 212 76 0 0 0 0 t i\n"
"MouseMoveEvent 212 75 0 0 0 0 t i\n"
"MouseMoveEvent 212 74 0 0 0 0 t i\n"
"MouseMoveEvent 212 72 0 0 0 0 t i\n"
"MouseMoveEvent 212 71 0 0 0 0 t i\n"
"MouseMoveEvent 212 69 0 0 0 0 t i\n"
"MouseMoveEvent 212 67 0 0 0 0 t i\n"
"MouseMoveEvent 212 65 0 0 0 0 t i\n"
"MouseMoveEvent 212 63 0 0 0 0 t i\n"
"MouseMoveEvent 212 62 0 0 0 0 t i\n"
"MouseMoveEvent 212 59 0 0 0 0 t i\n"
"MouseMoveEvent 212 58 0 0 0 0 t i\n"
"MouseMoveEvent 212 56 0 0 0 0 t i\n"
"MouseMoveEvent 212 55 0 0 0 0 t i\n"
"MouseMoveEvent 212 53 0 0 0 0 t i\n"
"MouseMoveEvent 212 51 0 0 0 0 t i\n"
"MouseMoveEvent 211 50 0 0 0 0 t i\n"
"MouseMoveEvent 211 48 0 0 0 0 t i\n"
"MouseMoveEvent 211 47 0 0 0 0 t i\n"
"MouseMoveEvent 211 46 0 0 0 0 t i\n"
"MouseMoveEvent 211 45 0 0 0 0 t i\n"
"MouseMoveEvent 211 44 0 0 0 0 t i\n"
"MouseMoveEvent 211 43 0 0 0 0 t i\n"
"MouseMoveEvent 211 42 0 0 0 0 t i\n"
"MouseMoveEvent 211 41 0 0 0 0 t i\n"
"MouseMoveEvent 210 40 0 0 0 0 t i\n"
"MouseMoveEvent 210 39 0 0 0 0 t i\n"
"MouseMoveEvent 209 39 0 0 0 0 t i\n"
"MouseMoveEvent 208 38 0 0 0 0 t i\n"
"MouseMoveEvent 208 36 0 0 0 0 t i\n"
"MouseMoveEvent 207 35 0 0 0 0 t i\n"
"MouseMoveEvent 207 34 0 0 0 0 t i\n"
"MouseMoveEvent 207 33 0 0 0 0 t i\n"
"RenderEvent 207 33 0 0 0 0 t i\n"
"MouseMoveEvent 207 32 0 0 0 0 t i\n"
"MouseMoveEvent 207 31 0 0 0 0 t i\n"
"MouseMoveEvent 207 30 0 0 0 0 t i\n"
"MouseMoveEvent 206 30 0 0 0 0 t i\n"
"MouseMoveEvent 206 29 0 0 0 0 t i\n"
"MouseMoveEvent 206 28 0 0 0 0 t i\n"
"MouseMoveEvent 206 27 0 0 0 0 t i\n"
"MouseMoveEvent 206 26 0 0 0 0 t i\n"
"LeftButtonPressEvent 206 26 0 0 0 0 t i\n"
"MouseMoveEvent 207 27 0 0 0 0 t i\n"
"RenderEvent 207 27 0 0 0 0 t i\n"
"MouseMoveEvent 225 49 0 0 0 0 t i\n"
"RenderEvent 225 49 0 0 0 0 t i\n"
"MouseMoveEvent 232 63 0 0 0 0 t i\n"
"RenderEvent 232 63 0 0 0 0 t i\n"
"LeftButtonReleaseEvent 232 63 0 0 0 0 t i\n"
"MouseMoveEvent 241 71 0 0 0 0 t i\n"
"RenderEvent 241 71 0 0 0 0 t i\n"
"MouseMoveEvent 241 72 0 0 0 0 t i\n"
"MouseMoveEvent 241 73 0 0 0 0 t i\n"
"MouseMoveEvent 241 75 0 0 0 0 t i\n"
"MouseMoveEvent 241 77 0 0 0 0 t i\n"
"MouseMoveEvent 241 79 0 0 0 0 t i\n"
"MouseMoveEvent 241 81 0 0 0 0 t i\n"
"MouseMoveEvent 241 83 0 0 0 0 t i\n"
"MouseMoveEvent 241 84 0 0 0 0 t i\n"
"MouseMoveEvent 241 86 0 0 0 0 t i\n"
"MouseMoveEvent 241 88 0 0 0 0 t i\n"
"MouseMoveEvent 242 89 0 0 0 0 t i\n"
"MouseMoveEvent 242 91 0 0 0 0 t i\n"
"MouseMoveEvent 242 92 0 0 0 0 t i\n"
"MouseMoveEvent 243 92 0 0 0 0 t i\n"
"MouseMoveEvent 247 96 0 0 0 0 t i\n"
"MouseMoveEvent 251 100 0 0 0 0 t i\n"
"MouseMoveEvent 257 106 0 0 0 0 t i\n"
"MouseMoveEvent 265 108 0 0 0 0 t i\n"
"MouseMoveEvent 269 114 0 0 0 0 t i\n"
"MouseMoveEvent 270 115 0 0 0 0 t i\n"
"KeyPressEvent 270 115 0 0 113 1 q i\n"
"CharEvent 270 115 0 0 113 1 q i\n"
"ExitEvent 270 115 0 0 113 1 q i\n"
;

int TestDijkstraGraphGeodesicPath(int argc, char*argv[])
{
  if (argc < 2)
    {
    cerr
      << "Demonstrates editing capabilities of a contour widget on polygonal \n"
      << "data. For consistency, this accepts a DEM data as input, (to compare\n"
      << "it with the TerrainPolylineEditor example. However, it converts the DEM\n"
      << "data to a polygonal data before feeding it to the contour widget.\n\n"
      << "Usage args: [height_offset]." 
      << endl;
    return EXIT_FAILURE;
    }

  // Read height field. 
  char* fname = 
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SainteHelens.dem");
  
  // Read height field. 
  //
  vtkDEMReader *demReader = vtkDEMReader::New();
  demReader->SetFileName(fname);
  delete [] fname;

  vtkImageResample * resample = vtkImageResample::New();
  resample->SetInput(demReader->GetOutput());
  resample->SetDimensionality(2);
  resample->SetAxisMagnificationFactor(0,1.0);
  resample->SetAxisMagnificationFactor(1,1.0);
  
  // Extract geometry
  vtkImageDataGeometryFilter *surface = vtkImageDataGeometryFilter::New();
  surface->SetInput(resample->GetOutput());
  resample->Delete();

  // The Dijkistra interpolator will not accept cells that aren't triangles
  vtkTriangleFilter *triangleFilter = vtkTriangleFilter::New();
  triangleFilter->SetInput( surface->GetOutput() );
  triangleFilter->Update();
  
  vtkWarpScalar *warp = vtkWarpScalar::New();
  warp->SetInput(triangleFilter->GetOutput());
  warp->SetScaleFactor(1);
  warp->UseNormalOn();
  warp->SetNormal(0, 0, 1);
  surface->Delete();
  warp->Update();
  triangleFilter->Delete();

  // Define a LUT mapping for the height field 

  double lo = demReader->GetOutput()->GetScalarRange()[0];
  double hi = demReader->GetOutput()->GetScalarRange()[1];

  vtkLookupTable *lut = vtkLookupTable::New();
  lut->SetHueRange(0.6, 0);
  lut->SetSaturationRange(1.0, 0);
  lut->SetValueRange(0.5, 1.0);
  
  vtkPolyDataNormals *normals = vtkPolyDataNormals::New();

  bool   distanceOffsetSpecified = false;
  double distanceOffset = 0.0;
  for (int i = 0; i < argc-1; i++)
    {
    if (strcmp("-DistanceOffset", argv[i]) == 0)
      {
      distanceOffset = atof(argv[i+1]);
      }
    }

  if (distanceOffsetSpecified)
    {
    normals->SetInput(warp->GetPolyDataOutput());
    normals->SetFeatureAngle(60);
    normals->SplittingOff();

    // vtkPolygonalSurfacePointPlacer needs cell normals
    // vtkPolygonalSurfaceContourLineInterpolator needs vertex normals
    normals->ComputeCellNormalsOn(); 
    normals->ComputePointNormalsOn();
    normals->Update();
    }

  vtkPolyData *pd = (distanceOffsetSpecified) ? normals->GetOutput()
                                            : warp->GetPolyDataOutput();

  vtkPolyDataMapper *demMapper = vtkPolyDataMapper::New();
  demMapper->SetInput(pd);
  demMapper->SetScalarRange(lo, hi);
  demMapper->SetLookupTable(lut);

  lut->Delete();
  normals->Delete();
  warp->Delete();

  vtkActor *demActor = vtkActor::New();
  demActor->SetMapper(demMapper);
  demMapper->Delete();

  // Create the RenderWindow, Renderer and the DEM + path actors.
 
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  
  // Add the actors to the renderer, set the background and size
  
  ren1->AddActor(demActor);

  ren1->GetActiveCamera()->SetViewUp(0, 0, 1);
  ren1->GetActiveCamera()->SetPosition(-99900, -21354, 131801);
  ren1->GetActiveCamera()->SetFocalPoint(41461, 41461, 2815);
  ren1->ResetCamera();
  ren1->GetActiveCamera()->Dolly(4.2);
  ren1->ResetCameraClippingRange();

  // Here comes the contour widget stuff.....

  vtkContourWidget *contourWidget = vtkContourWidget::New();
  contourWidget->SetInteractor(iren);
  vtkOrientedGlyphContourRepresentation *rep = 
      vtkOrientedGlyphContourRepresentation::SafeDownCast(
                        contourWidget->GetRepresentation());
  rep->GetLinesProperty()->SetColor(1, 0.2, 0);
  rep->GetLinesProperty()->SetLineWidth(3.0);

  vtkPolygonalSurfacePointPlacer * pointPlacer 
        = vtkPolygonalSurfacePointPlacer::New();
  pointPlacer->AddProp(demActor);
  demActor->GetProperty()->SetRepresentationToWireframe();
  pointPlacer->GetPolys()->AddItem( pd );
  rep->SetPointPlacer(pointPlacer);

  vtkPolygonalSurfaceContourLineInterpolator * interpolator =
    vtkPolygonalSurfaceContourLineInterpolator::New();
  interpolator->GetPolys()->AddItem( pd );
  rep->SetLineInterpolator(interpolator);
  interpolator->Delete();
  if (distanceOffsetSpecified)
    {
    pointPlacer->SetDistanceOffset( distanceOffset );
    interpolator->SetDistanceOffset( distanceOffset );
    }
  
  vtkInteractorEventRecorder *recorder = vtkInteractorEventRecorder::New();
  recorder->SetInteractor(iren);
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(TestDijkstraGraphGeodesicPathLog); 
  recorder->EnabledOn();

  renWin->Render();
  iren->Initialize();

  contourWidget->EnabledOn();

  recorder->Play();
    
  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  // Cleanups
  recorder->Delete();
  contourWidget->Delete();
  pointPlacer->Delete();
  demReader->Delete();
  demActor->Delete();
  iren->Delete();
  renWin->Delete();
  ren1->Delete();
  
  return retVal;
}

