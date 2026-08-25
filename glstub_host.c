/* Minimal no-op GL for host probes that only need scene_load's CPU side.
   scene.c's geometry, rig and rest arrays are all built without the GPU. */
#include "vitaGL.h"

void glActiveTexture(GLenum unit) { }
void glBindBuffer(GLenum target, GLuint id) { }
void glBindTexture(GLenum t, GLuint id) { }
void glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage) { }
void glClientActiveTexture(GLenum unit) { }
void glDeleteBuffers(GLsizei n, const GLuint *ids) { }
void glDeleteTextures(GLsizei n, const GLuint *ids) { }
void glDisable(GLenum c) { }
void glDisableClientState(GLenum c) { }
void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void *idx) { }
void glEnable(GLenum c) { }
void glEnableClientState(GLenum c) { }
void glGenBuffers(GLsizei n, GLuint *out) { }
void glGenTextures(GLsizei n, GLuint *out) { }
void glGenerateMipmap(GLenum target) { }
void glGetFloatv(GLenum pname, GLfloat *out) { }
void glMultMatrixf(const float *m) { }
/* char.c places an instance through these three, so a probe that wants char.c
   for its placement rather than its drawing needs them here. They discard the
   matrix, which is why nothing linking this stub may assert on a draw -- see
   BUILD.md. chartest keeps a REAL stack for exactly that reason. */
void glRotatef(float a, float x, float y, float z) { }
void glScalef(float x, float y, float z) { }
void glTranslatef(float x, float y, float z) { }
void glPopMatrix(void) { }
void glPushMatrix(void) { }
void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void *p) { }
/* The car's per-vertex light -- scene.c binds it as a colour array and uploads
   it per frame (carlight.h). No-op here: nothing in this harness asserts on a
   draw. */
void glColorPointer(GLint size, GLenum type, GLsizei stride, const void *p)
{ (void)size; (void)type; (void)stride; (void)p; }
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size,
                     const void *data)
{ (void)target; (void)offset; (void)size; (void)data; }
void glTexEnvi(GLenum target, GLenum pname, GLint param) { }
void glTexImage2D(GLenum t, GLint lvl, GLint ifmt, GLsizei w, GLsizei h,
                  GLint border, GLenum fmt, GLenum type, const void *px) { }
void glTexParameteri(GLenum t, GLenum p, GLint v) { }
void glVertexPointer(GLint size, GLenum type, GLsizei stride, const void *p) { }
/* scene_draw_blend's state, added when the transition bands landed. Nothing that
   links this stub draws, so these only have to resolve -- the bands' state is
   recorded and asserted in vis_test part 15, which uses testgl instead. */
void glBlendFunc(GLenum a, GLenum b) { }
void glColor4f(float r, float g, float b, float a) { }
void glDepthFunc(GLenum f) { }
void glDepthMask(GLboolean b) { }
