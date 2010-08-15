/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// OpenGL ES 2.0 code

#include <jni.h>
#include <android/log.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define  LOG_TAG    "libgl2jni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

static void printGLString(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);
    LOGI("GL %s = %s\n", name, v);
}

static void checkGlError(const char* op) {
    for (GLint error = glGetError(); error; error
            = glGetError()) {
        LOGI("after %s() glError (0x%x)\n", op, error);
    }
}

static const char gVertexShader[] = 
    "attribute vec4 vPosition;\n"
    "void main() {\n"
    "  gl_Position = vPosition;\n"
    "}\n";

#define GEN_SHADER(num_balls) \
static const char gFragmentShader[] = \
    "precision mediump float;\n" \
    "uniform vec2 dim, hdim;\n" \
    "uniform vec2 balls[" #num_balls "];\n" \
    "uniform vec3 colors[" #num_balls "];\n" \
    "float sqr(float x) { return x*x; }\n" \
    "void main() {\n" \
    "  vec4 lol = vec4(0.0, 0.0, 0.0, 0.0);\n" \
    "  for (int i=0; i<" #num_balls "; ++i) {\n" \
    "    vec2 dist1 = balls[i] - gl_FragCoord.xy;\n" \
    "    vec2 dist = mod(dist1 + hdim, dim) - hdim;\n" \
    "    float val = 1000.0 / (sqr(dist.x) + sqr(dist.y));\n" \
    "    lol += vec4(colors[i], 1.0) * val;\n" \
    "  }\n" \
    "  float a = smoothstep(0.9, 1.0, lol.a);\n" \
    "  lol *= 1.0 / lol.a;\n" \
    "  gl_FragColor = vec4(lol.rgb * a, 1.0);\n" \
    "}\n";

#define NUM_BALLS 5
GEN_SHADER(5)

GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    LOGE("Could not compile shader %d:\n%s\n",
                            shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");
        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    LOGE("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

GLuint gProgram;
GLuint gvPositionHandle;
float gHalfDim[2];
float gDim[2];
const unsigned num_balls = NUM_BALLS;

struct loltype {
    GLuint colors[num_balls];
    GLuint balls[num_balls];
    GLuint dim, hdim;
} gVar;

float colors_hue[num_balls];
float colors_huev[num_balls];
float colors[num_balls][3];
float balls[num_balls][2];
float ballsv[num_balls][2];

// Always positive fmod
float pfmod(double x, double m) {
    x = fmod(x, m);
    if (x < 0.) x += m;
    return x;
}

void set_color(int i, float h, float s, float v) {
    double c = v*s;
    double hp = h / 60.;
    double x = c * (1.0 - fabs(fmod(hp, 2.0) - 1.0));

    double r1, g1, b1;
    
    if (false) { }
    else if (hp < 1.) { r1 = c; g1 = x; b1 = 0; }
    else if (hp < 2.) { r1 = x; g1 = c; b1 = 0; }
    else if (hp < 3.) { r1 = 0; g1 = c; b1 = x; }
    else if (hp < 4.) { r1 = 0; g1 = x; b1 = c; }
    else if (hp < 5.) { r1 = x; g1 = 0; b1 = c; }
    else              { r1 = c; g1 = 0; b1 = x; }

    double m = v - c;
    colors[i][0] = r1 + m;
    colors[i][1] = g1 + m;
    colors[i][2] = b1 + m;
}

void init_balls() {
    srand(time(0));

    for (int i=0; i<num_balls; ++i) {
        colors_hue[i] = 360.0 * (rand() / (double)RAND_MAX);
        colors_huev[i] = 1.0 * (2.0 * rand() / (double)RAND_MAX - 1.0);

        for (int j=0; j<2; ++j) {
            balls[i][j] = (2.0 * rand() / (double)RAND_MAX - 1.0) * 0.7 * gHalfDim[j] + gHalfDim[j];
            ballsv[i][j] = 5.0 * (2.0 * rand() / (double)RAND_MAX - 1.0);
        }
    }
}

bool setupGraphics(int w, int h) {
    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    LOGI("setupGraphics(%d, %d)", w, h);
    gProgram = createProgram(gVertexShader, gFragmentShader);
    if (!gProgram) {
        LOGE("Could not create program.");
        return false;
    }
    gvPositionHandle = glGetAttribLocation(gProgram, "vPosition");
    checkGlError("glGetAttribLocation");
    LOGI("glGetAttribLocation(\"vPosition\") = %d\n", gvPositionHandle);

    gVar.dim = glGetUniformLocation(gProgram, "dim");
    checkGlError("glGetUniformLocation");
    LOGI("glGetUniformLocation(\"dim\") = %d\n", gVar.dim);

    gVar.hdim = glGetUniformLocation(gProgram, "hdim");
    checkGlError("glGetUniformLocation");
    LOGI("glGetUniformLocation(\"hdim\") = %d\n", gVar.hdim);

    for (int i=0; i<num_balls; ++i) {
        char buf[100];
        sprintf(buf, "balls[%i]", i);
        gVar.balls[i] = glGetUniformLocation(gProgram, buf);
        checkGlError("glGetUniformLocation");
        LOGI("glGetUniformLocation(\"balls[%i]\") = %d\n", i, gVar.balls[i]);

        sprintf(buf, "colors[%i]", i);
        gVar.colors[i] = glGetUniformLocation(gProgram, buf);
        checkGlError("glGetUniformLocation");
        LOGI("glGetUniformLocation(\"colors[%i]\") = %d\n", i, gVar.colors[i]);
    }

    gDim[0] = w;
    gDim[1] = h;
    gHalfDim[0] = w * 0.5;
    gHalfDim[1] = h * 0.5;
    glViewport(0, 0, w, h);
    init_balls();
    checkGlError("glViewport");
    return true;
}

const GLfloat gQuadVertices[] = {
    1.0f, 1.0f,
    -1.0f, 1.0f,
    -1.0f, -1.0f,
    1.0f, -1.0f,
};

void renderFrame() {
    for (int i=0; i<num_balls; ++i) {
        // Gravitate to other balls:
        for (int j=0; j<num_balls; ++j) {
            for (int k=0; k<2; ++k) {
                float dist = pfmod(balls[j][k] - balls[i][k], gDim[k]);
                if (dist > gHalfDim[k]) dist -= gDim[k];
                ballsv[i][k] += dist * 0.002;
            }
        }

        // Gravitate to center:
        for (int k=0; k<2; ++k) {
            float dist = gHalfDim[k] - balls[i][k];
            ballsv[i][k] += dist * 0.002;
        }
    }
        
    for (int i=0; i<num_balls; ++i) {
        for (int k=0; k<2; ++k) {
            balls[i][k] = pfmod(balls[i][k] + ballsv[i][k], gDim[k]);
        }

        colors_hue[i] = pfmod(colors_hue[i] + colors_huev[i], 360.);
        set_color(i, colors_hue[i], 0.6, 1.0);
    }

    glUseProgram(gProgram);
    checkGlError("glUseProgram");

    glUniform2fv(gVar.dim, 1, gDim);
    checkGlError("glUniform2fv");

    glUniform2fv(gVar.hdim, 1, gHalfDim);
    checkGlError("glUniform2fv");

    for (int i=0; i<num_balls; ++i) {
        glUniform3fv(gVar.colors[i], 1, colors[i]);
        checkGlError("glUniform3fv");
        glUniform2fv(gVar.balls[i], 1, balls[i]);
        checkGlError("glUniform2fv");
    }
    
    glVertexAttribPointer(gvPositionHandle, 2, GL_FLOAT, GL_FALSE, 0, gQuadVertices);
    checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvPositionHandle);
    checkGlError("glEnableVertexAttribArray");
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    checkGlError("glDrawArrays");

    glUseProgram(0);
    checkGlError("glUseProgram");
}

extern "C" {
    JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_init(JNIEnv * env, jobject obj,  jint width, jint height);
    JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_step(JNIEnv * env, jobject obj);
};

JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_init(JNIEnv * env, jobject obj,  jint width, jint height)
{
    setupGraphics(width, height);
}

JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_step(JNIEnv * env, jobject obj)
{
    renderFrame();
}
