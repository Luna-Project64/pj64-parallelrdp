/* Copyright  (C) 2010-2020 The RetroArch team
 *
 * ---------------------------------------------------------------------------------------
 * The following license statement only applies to this file (matrix_4x4.h).
 * ---------------------------------------------------------------------------------------
 *
 * Permission is hereby granted, free of charge,
 * to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

typedef struct math_matrix_4x4
{
    float data[16];
} math_matrix_4x4;

#define MAT_ELEM_4X4(mat, row, column) ((mat).data[4 * (column) + (row)])

#define matrix_4x4_ortho(mat, left, right, bottom, top, znear, zfar) \
{ \
   float rl                = (right) - (left); \
   float tb                = (top)   - (bottom); \
   float fn                = (zfar)  - (znear); \
   MAT_ELEM_4X4(mat, 0, 0) =  2.0f / rl; \
   MAT_ELEM_4X4(mat, 0, 1) =  0.0f; \
   MAT_ELEM_4X4(mat, 0, 2) =  0.0f; \
   MAT_ELEM_4X4(mat, 0, 3) = -((left) + (right))  / rl; \
   MAT_ELEM_4X4(mat, 1, 0) =  0.0f; \
   MAT_ELEM_4X4(mat, 1, 1) =  2.0f / tb; \
   MAT_ELEM_4X4(mat, 1, 2) =  0.0f; \
   MAT_ELEM_4X4(mat, 1, 3) = -((top)  + (bottom)) / tb; \
   MAT_ELEM_4X4(mat, 2, 0) =  0.0f; \
   MAT_ELEM_4X4(mat, 2, 1) =  0.0f; \
   MAT_ELEM_4X4(mat, 2, 2) = -2.0f / fn; \
   MAT_ELEM_4X4(mat, 2, 3) = -((zfar) + (znear))  / fn; \
   MAT_ELEM_4X4(mat, 3, 0) =  0.0f; \
   MAT_ELEM_4X4(mat, 3, 1) =  0.0f; \
   MAT_ELEM_4X4(mat, 3, 2) =  0.0f; \
   MAT_ELEM_4X4(mat, 3, 3) =  1.0f; \
}

/*
 * Builds a rotation matrix using the
 * rotation around the Z-axis.
 */
#define matrix_4x4_rotate_z(mat, radians) \
{ \
   float cosine            = cosf(radians); \
   float sine              = sinf(radians); \
   MAT_ELEM_4X4(mat, 0, 0) = cosine; \
   MAT_ELEM_4X4(mat, 0, 1) = -sine; \
   MAT_ELEM_4X4(mat, 0, 2) = 0.0f; \
   MAT_ELEM_4X4(mat, 0, 3) = 0.0f; \
   MAT_ELEM_4X4(mat, 1, 0) = sine; \
   MAT_ELEM_4X4(mat, 1, 1) = cosine; \
   MAT_ELEM_4X4(mat, 1, 2) = 0.0f; \
   MAT_ELEM_4X4(mat, 1, 3) = 0.0f; \
   MAT_ELEM_4X4(mat, 2, 0) = 0.0f; \
   MAT_ELEM_4X4(mat, 2, 1) = 0.0f; \
   MAT_ELEM_4X4(mat, 2, 2) = 1.0f; \
   MAT_ELEM_4X4(mat, 2, 3) = 0.0f; \
   MAT_ELEM_4X4(mat, 3, 0) = 0.0f; \
   MAT_ELEM_4X4(mat, 3, 1) = 0.0f; \
   MAT_ELEM_4X4(mat, 3, 2) = 0.0f; \
   MAT_ELEM_4X4(mat, 3, 3) = 1.0f; \
}

#define matrix_4x4_multiply(out, a, b) \
   MAT_ELEM_4X4(out, 0, 0) =  \
      MAT_ELEM_4X4(a, 0, 0) * MAT_ELEM_4X4(b, 0, 0) + \
      MAT_ELEM_4X4(a, 0, 1) * MAT_ELEM_4X4(b, 1, 0) + \
      MAT_ELEM_4X4(a, 0, 2) * MAT_ELEM_4X4(b, 2, 0) + \
      MAT_ELEM_4X4(a, 0, 3) * MAT_ELEM_4X4(b, 3, 0); \
   MAT_ELEM_4X4(out, 0, 1) =  \
      MAT_ELEM_4X4(a, 0, 0) * MAT_ELEM_4X4(b, 0, 1) + \
      MAT_ELEM_4X4(a, 0, 1) * MAT_ELEM_4X4(b, 1, 1) + \
      MAT_ELEM_4X4(a, 0, 2) * MAT_ELEM_4X4(b, 2, 1) + \
      MAT_ELEM_4X4(a, 0, 3) * MAT_ELEM_4X4(b, 3, 1); \
   MAT_ELEM_4X4(out, 0, 2) =  \
      MAT_ELEM_4X4(a, 0, 0) * MAT_ELEM_4X4(b, 0, 2) + \
      MAT_ELEM_4X4(a, 0, 1) * MAT_ELEM_4X4(b, 1, 2) + \
      MAT_ELEM_4X4(a, 0, 2) * MAT_ELEM_4X4(b, 2, 2) + \
      MAT_ELEM_4X4(a, 0, 3) * MAT_ELEM_4X4(b, 3, 2); \
   MAT_ELEM_4X4(out, 0, 3) =  \
      MAT_ELEM_4X4(a, 0, 0) * MAT_ELEM_4X4(b, 0, 3) + \
      MAT_ELEM_4X4(a, 0, 1) * MAT_ELEM_4X4(b, 1, 3) + \
      MAT_ELEM_4X4(a, 0, 2) * MAT_ELEM_4X4(b, 2, 3) + \
      MAT_ELEM_4X4(a, 0, 3) * MAT_ELEM_4X4(b, 3, 3); \
   MAT_ELEM_4X4(out, 1, 0) =  \
      MAT_ELEM_4X4(a, 1, 0) * MAT_ELEM_4X4(b, 0, 0) + \
      MAT_ELEM_4X4(a, 1, 1) * MAT_ELEM_4X4(b, 1, 0) + \
      MAT_ELEM_4X4(a, 1, 2) * MAT_ELEM_4X4(b, 2, 0) + \
      MAT_ELEM_4X4(a, 1, 3) * MAT_ELEM_4X4(b, 3, 0); \
   MAT_ELEM_4X4(out, 1, 1) =  \
      MAT_ELEM_4X4(a, 1, 0) * MAT_ELEM_4X4(b, 0, 1) + \
      MAT_ELEM_4X4(a, 1, 1) * MAT_ELEM_4X4(b, 1, 1) + \
      MAT_ELEM_4X4(a, 1, 2) * MAT_ELEM_4X4(b, 2, 1) + \
      MAT_ELEM_4X4(a, 1, 3) * MAT_ELEM_4X4(b, 3, 1); \
   MAT_ELEM_4X4(out, 1, 2) =  \
      MAT_ELEM_4X4(a, 1, 0) * MAT_ELEM_4X4(b, 0, 2) + \
      MAT_ELEM_4X4(a, 1, 1) * MAT_ELEM_4X4(b, 1, 2) + \
      MAT_ELEM_4X4(a, 1, 2) * MAT_ELEM_4X4(b, 2, 2) + \
      MAT_ELEM_4X4(a, 1, 3) * MAT_ELEM_4X4(b, 3, 2); \
   MAT_ELEM_4X4(out, 1, 3) =  \
      MAT_ELEM_4X4(a, 1, 0) * MAT_ELEM_4X4(b, 0, 3) + \
      MAT_ELEM_4X4(a, 1, 1) * MAT_ELEM_4X4(b, 1, 3) + \
      MAT_ELEM_4X4(a, 1, 2) * MAT_ELEM_4X4(b, 2, 3) + \
      MAT_ELEM_4X4(a, 1, 3) * MAT_ELEM_4X4(b, 3, 3); \
   MAT_ELEM_4X4(out, 2, 0) =  \
      MAT_ELEM_4X4(a, 2, 0) * MAT_ELEM_4X4(b, 0, 0) + \
      MAT_ELEM_4X4(a, 2, 1) * MAT_ELEM_4X4(b, 1, 0) + \
      MAT_ELEM_4X4(a, 2, 2) * MAT_ELEM_4X4(b, 2, 0) + \
      MAT_ELEM_4X4(a, 2, 3) * MAT_ELEM_4X4(b, 3, 0); \
   MAT_ELEM_4X4(out, 2, 1) =  \
      MAT_ELEM_4X4(a, 2, 0) * MAT_ELEM_4X4(b, 0, 1) + \
      MAT_ELEM_4X4(a, 2, 1) * MAT_ELEM_4X4(b, 1, 1) + \
      MAT_ELEM_4X4(a, 2, 2) * MAT_ELEM_4X4(b, 2, 1) + \
      MAT_ELEM_4X4(a, 2, 3) * MAT_ELEM_4X4(b, 3, 1); \
   MAT_ELEM_4X4(out, 2, 2) =  \
      MAT_ELEM_4X4(a, 2, 0) * MAT_ELEM_4X4(b, 0, 2) + \
      MAT_ELEM_4X4(a, 2, 1) * MAT_ELEM_4X4(b, 1, 2) + \
      MAT_ELEM_4X4(a, 2, 2) * MAT_ELEM_4X4(b, 2, 2) + \
      MAT_ELEM_4X4(a, 2, 3) * MAT_ELEM_4X4(b, 3, 2); \
   MAT_ELEM_4X4(out, 2, 3) =  \
      MAT_ELEM_4X4(a, 2, 0) * MAT_ELEM_4X4(b, 0, 3) + \
      MAT_ELEM_4X4(a, 2, 1) * MAT_ELEM_4X4(b, 1, 3) + \
      MAT_ELEM_4X4(a, 2, 2) * MAT_ELEM_4X4(b, 2, 3) + \
      MAT_ELEM_4X4(a, 2, 3) * MAT_ELEM_4X4(b, 3, 3); \
   MAT_ELEM_4X4(out, 3, 0) =  \
      MAT_ELEM_4X4(a, 3, 0) * MAT_ELEM_4X4(b, 0, 0) + \
      MAT_ELEM_4X4(a, 3, 1) * MAT_ELEM_4X4(b, 1, 0) + \
      MAT_ELEM_4X4(a, 3, 2) * MAT_ELEM_4X4(b, 2, 0) + \
      MAT_ELEM_4X4(a, 3, 3) * MAT_ELEM_4X4(b, 3, 0); \
   MAT_ELEM_4X4(out, 3, 1) =  \
      MAT_ELEM_4X4(a, 3, 0) * MAT_ELEM_4X4(b, 0, 1) + \
      MAT_ELEM_4X4(a, 3, 1) * MAT_ELEM_4X4(b, 1, 1) + \
      MAT_ELEM_4X4(a, 3, 2) * MAT_ELEM_4X4(b, 2, 1) + \
      MAT_ELEM_4X4(a, 3, 3) * MAT_ELEM_4X4(b, 3, 1); \
   MAT_ELEM_4X4(out, 3, 2) = \
      MAT_ELEM_4X4(a, 3, 0) * MAT_ELEM_4X4(b, 0, 2) + \
      MAT_ELEM_4X4(a, 3, 1) * MAT_ELEM_4X4(b, 1, 2) + \
      MAT_ELEM_4X4(a, 3, 2) * MAT_ELEM_4X4(b, 2, 2) + \
      MAT_ELEM_4X4(a, 3, 3) * MAT_ELEM_4X4(b, 3, 2); \
   MAT_ELEM_4X4(out, 3, 3) =  \
      MAT_ELEM_4X4(a, 3, 0) * MAT_ELEM_4X4(b, 0, 3) + \
      MAT_ELEM_4X4(a, 3, 1) * MAT_ELEM_4X4(b, 1, 3) + \
      MAT_ELEM_4X4(a, 3, 2) * MAT_ELEM_4X4(b, 2, 3) + \
      MAT_ELEM_4X4(a, 3, 3) * MAT_ELEM_4X4(b, 3, 3)
