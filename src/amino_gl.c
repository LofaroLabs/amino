/* -*- mode: C; c-basic-offset: 4; -*- */
/* ex: set shiftwidth=4 tabstop=4 expandtab: */
/*
 * Copyright (c) 2015, Rice University
 * All rights reserved.
 *
 * Author(s): Neil T. Dantam <ntd@rice.edu>
 *
 *   Redistribution and use in source and binary forms, with or
 *   without modification, are permitted provided that the following
 *   conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of copyright holder the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 *   CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 *   USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 *   AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *   POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "amino.h"

#define GL_GLEXT_PROTOTYPES

#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL.h>

#include "amino/rx/amino_gl.h"
#include "amino/rx/scene_geom.h"
#include "amino/rx/scene_geom_internal.h"

#define CHECK_GL_STATUS(TYPE,handle,pname) {                            \
        GLint status;                                                   \
        glGet ## TYPE ## iv(handle, pname, &status);                    \
        if (GL_FALSE == status) {                                       \
            GLint logLength;                                            \
            glGet ## TYPE ## iv(handle, GL_INFO_LOG_LENGTH, &logLength); \
            char infoLog[logLength+2];                                  \
            glGet ## TYPE ## InfoLog(handle, logLength, NULL, infoLog); \
            fprintf(stderr, "%d: %d, %s\n", __LINE__, pname, infoLog);  \
        }                                                               \
    }

void
aa_gl_qutr2glmat( const double E[AA_RESTRICT 7],
                  GLfloat M[AA_RESTRICT 16] )
{
    double T[12];
    aa_tf_qutr2tfmat(E,T);
    aa_gl_tfmat2glmat(T,M);
}


void
aa_gl_tfmat2glmat( const double T[AA_RESTRICT 12],
                   GLfloat M[AA_RESTRICT 16] )
{
    for( size_t j = 0; j < 4; j++ ) {
        for( size_t i = 0; i < 3; i++ ) {
            M[j*4 + i] = (GLfloat)T[j*3 + i];
        }
    }

    for( size_t j = 0; j < 3; j++ ) {
        M[j*4 + 3] = 0;
    }

    M[3*4 + 3] = 1;
}

AA_API GLuint aa_gl_create_shader(
    GLenum shader_type, const char* source)
{
    GLuint shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, (const GLchar **)&source, NULL);
    glCompileShader(shader);

    CHECK_GL_STATUS(Shader, shader, GL_COMPILE_STATUS);

    return shader;
}

AA_API GLuint aa_gl_create_program(GLuint vert_shader, GLuint frag_shader)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, vert_shader);
    glAttachShader(program, frag_shader);
    glLinkProgram(program);
    CHECK_GL_STATUS(Program, program, GL_LINK_STATUS);
    return program;
}

static const char aa_gl_vertex_shader[] =
    "#version 130\n"
    "in vec4 position;"
    "in vec4 color;"
    "uniform mat4 matrix;"
    "smooth out vec4 vColor;"
    "void main() {"
    "  gl_Position = matrix * position;"
    "  vColor = color;"
    "}";

static const char aa_gl_fragment_shader[] =
    "#version 130\n"
    "smooth in vec4 vColor;"
    "void main() {"
    "  gl_FragColor = vColor;"
    "}";

static int aa_gl_do_init = 1;
#define AA_GL_INIT if(aa_gl_do_init) aa_gl_init();

static GLuint aa_gl_id_program;
static GLuint aa_gl_id_position;
static GLuint aa_gl_id_color;
static GLint aa_gl_id_matrix;

AA_API void aa_gl_init()
{
    GLuint vertexShader = aa_gl_create_shader(GL_VERTEX_SHADER, aa_gl_vertex_shader);
    GLuint fragmentShader = aa_gl_create_shader(GL_FRAGMENT_SHADER, aa_gl_fragment_shader);
    aa_gl_id_program = aa_gl_create_program(vertexShader, fragmentShader);

    aa_gl_id_position = glGetAttribLocation(aa_gl_id_program, "position");
    aa_gl_id_color = glGetAttribLocation(aa_gl_id_program, "color");
    aa_gl_id_matrix = glGetUniformLocation(aa_gl_id_program, "matrix");

    aa_gl_do_init = 0;
}


static void check_error( const char *name ){
    for (GLenum err = glGetError(); err != GL_NO_ERROR; err = glGetError()) {
        fprintf(stderr, "error %s: %d: %s\n",  name,  (int)err, gluErrorString(err));
    }
}

AA_API void aa_gl_draw_tf (
    const double *E,
    const struct aa_gl_buffers *buffers )
{
    AA_GL_INIT;


    glUseProgram(aa_gl_id_program);
    check_error("glUseProgram");

    glBindBuffer(GL_ARRAY_BUFFER, buffers->values);
    check_error("glBindBuffer");

    glEnableVertexAttribArray(aa_gl_id_position);
    check_error("glEnableVert");

    glVertexAttribPointer(aa_gl_id_position, 3, GL_FLOAT, GL_FALSE, 0, 0);
    check_error("glVertAttribPointer");

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ARRAY_BUFFER, buffers->colors);
    check_error("glBindBuffer");

    glEnableVertexAttribArray(aa_gl_id_color);
    check_error("glEnabeVert");

    glVertexAttribPointer(aa_gl_id_color, 4, GL_FLOAT, GL_FALSE, 0, 0);
    check_error("glVerteAttribPointer");

    GLfloat M[16];
    aa_gl_qutr2glmat( E, M);

    glUniformMatrix4fv(aa_gl_id_matrix, 1, GL_FALSE, M);
    check_error("uniform mat");

    if( buffers->has_indices ) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers->indices);
        glDrawElements(
            GL_TRIANGLES,      // mode
            buffers->count,    // count
            GL_UNSIGNED_INT,   // type
            (void*)0           // element array buffer offset
            );

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, buffers->count);
    }
    check_error("glDraw");


    glDisableVertexAttribArray(aa_gl_id_position);
    glDisableVertexAttribArray(aa_gl_id_color);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);

}


static void aa_gl_buffers_destroy( struct aa_gl_buffers *bufs ) {
    if( bufs->has_indices ) glDeleteProgram(bufs->indices);
    if( bufs->has_colors ) glDeleteProgram(bufs->colors);
    if( bufs->has_values ) glDeleteProgram(bufs->values);
}

static void quad_tr( unsigned *indices,
                     unsigned pp,
                     unsigned pm,
                     unsigned mp,
                     unsigned mm )
{
    size_t j = 0;
    indices[j++] = pp;
    indices[j++] = pm;
    indices[j++] = mp;

    indices[j++] = mm;
    indices[j++] = pm;
    indices[j++] = mp;
}


AA_API void aa_geom_gl_buffers_init_box (
    struct aa_rx_geom_box *geom
    )
{
    GLfloat values[3*4*2]; // 3 values, 4 corners, two squares
    //GLfloat colors[3*4*2]; // same as vertices
    GLfloat colors[6*2*4]; // same as vertices
    unsigned indices[6*2*3]; // 6 sides, 2 triangles, 3 vertices
    double *d = geom->shape.dimension;

    // fill vertices
    double a[2] = {1,-1};
    for( size_t x = 0, j=0; x < 2; x ++ ) {
        for( size_t y = 0; y < 2; y ++ ) {
            for( size_t z = 0; z < 2; z ++ ) {
                values[j++] = (GLfloat)(a[x]*d[0]);
                values[j++] = (GLfloat)(a[y]*d[1]);
                values[j++] = (GLfloat)(a[z]*d[2]);
            }
        }
    }

    /*    xyz
     *    ===
     * 0: +++
     * 1: ++-
     * 2: +-+
     * 3: +--
     *
     * 4: -++
     * 5: -+-
     * 6: --+
     * 7: ---
     */

    {
        size_t j = 0;
        // +x
        quad_tr(indices+j, 0,1,2,3 );
        j+=6;
        // -x
        quad_tr(indices+j, 4,5,6,7 );
        j+=6;
        // +y
        quad_tr(indices+j, 0,1,4,5 );
        j+=6;
        // -y
        quad_tr(indices+j, 2,3,6,7 );
        j+=6;
        // +z
        quad_tr(indices+j, 0,2,4,6 );
        j+=6;
        // -z
        quad_tr(indices+j, 1,3,5,7 );
        j+=6;
    }

    for( int i = 0; i < sizeof(colors)/(4*sizeof(*colors)); i ++ ) {
        for( int j = 0; j < 4; j ++ ) {
            colors[4*i + j ] = (GLfloat)geom->base.opt.color[j];
        }
    }

    struct aa_gl_buffers *bufs = AA_NEW0(struct aa_gl_buffers);
    geom->base.gl_buffers = bufs;
    bufs->count = sizeof(indices)/sizeof(*indices);

    glGenBuffers(1, &bufs->values);
    glBindBuffer(GL_ARRAY_BUFFER, bufs->values);
    glBufferData(GL_ARRAY_BUFFER, sizeof(values), values, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    bufs->has_values = 1;

    glGenBuffers(1, &bufs->colors);
    glBindBuffer(GL_ARRAY_BUFFER, bufs->colors);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    bufs->has_colors = 1;


    glGenBuffers(1, &bufs->indices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufs->indices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    bufs->has_indices = 1;

}


AA_API void aa_geom_gl_buffers_init (
    struct aa_rx_geom_base *geom
    )
{
    if( geom->gl_buffers ) aa_gl_buffers_destroy( geom->gl_buffers );
    geom->gl_buffers = AA_NEW0(struct aa_gl_buffers);

    switch( geom->type ) {
    case AA_RX_BOX:
        aa_geom_gl_buffers_init_box((struct aa_rx_geom_box*)geom);
        break;
    default: abort();
    }
}