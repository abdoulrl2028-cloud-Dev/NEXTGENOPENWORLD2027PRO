#ifndef NEXTGEN_UI_UICANVAS_H
#define NEXTGEN_UI_UICANVAS_H

#include <GLES3/gl3.h>

#include <string>

class UiCanvas {
public:
    bool create(const std::string &vertexSource, const std::string &fragmentSource, std::string &error);
    void destroy();
    void begin(int screenWidth, int screenHeight);
    void end();
    void drawSolid(float x, float y, float w, float h, float red, float green, float blue, float alpha);
    void drawTextured(
            float x,
            float y,
            float w,
            float h,
            GLuint texture,
            float u0,
            float v0,
            float u1,
            float v1,
            float red,
            float green,
            float blue,
            float alpha);

private:
    void drawQuad(
            float x,
            float y,
            float w,
            float h,
            GLuint texture,
            float u0,
            float v0,
            float u1,
            float v1,
            float red,
            float green,
            float blue,
            float alpha);

    GLuint program_ = 0;
    GLuint vao_ = 0;
    GLuint buffer_ = 0;
    GLuint white_ = 0;
    GLint resolution_ = -1;
    GLint color_ = -1;
    int width_ = 0;
    int height_ = 0;
};

#endif
