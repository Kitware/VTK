#ifndef vtkgluPickMatrix_h
#define vtkgluPickMatrix_h

// This function was copied from Mesa and sets up the pick matrix

inline void vtkgluPickMatrix( GLdouble x, GLdouble y,
			      GLdouble width, GLdouble height,
			      const GLint viewport[4] )
{
  
  GLfloat m[16];
  GLfloat sx, sy;
  GLfloat tx, ty;
  
  sx = viewport[2] / width;
  sy = viewport[3] / height;
  tx = (viewport[2] + 2.0 * (viewport[0] - x)) / width;
  ty = (viewport[3] + 2.0 * (viewport[1] - y)) / height;

#define M(row,col)  m[col*4+row]
   M(0,0) = sx;   M(0,1) = 0.0;  M(0,2) = 0.0;  M(0,3) = tx;
   M(1,0) = 0.0;  M(1,1) = sy;   M(1,2) = 0.0;  M(1,3) = ty;
   M(2,0) = 0.0;  M(2,1) = 0.0;  M(2,2) = 1.0;  M(2,3) = 0.0;
   M(3,0) = 0.0;  M(3,1) = 0.0;  M(3,2) = 0.0;  M(3,3) = 1.0;
#undef M

   glMultMatrixf( m );
}

#endif
