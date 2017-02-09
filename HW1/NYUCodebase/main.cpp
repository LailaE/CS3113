
#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <SDL_image.h>
#include "ShaderProgram.h"
#include "Matrix.h"
#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

GLuint LoadTexture(const char* filePath){
    int w, h, comp;
    unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
    
    if(image == NULL){
        assert(false);
    }
    
    GLuint retTexture;
    glGenTextures(1, &retTexture);
    glBindTexture(GL_TEXTURE_2D, retTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(image);

    return retTexture;
}

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
    glewInit();
#endif
    
    
    glViewport(0, 0, 640, 360);
    ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    GLuint DarthVader = LoadTexture(RESOURCE_FOLDER"darth.png");
    GLuint r2d2 = LoadTexture(RESOURCE_FOLDER"R2D2.png");
    GLuint c3p0 = LoadTexture(RESOURCE_FOLDER"C3P0.png");
    GLuint bb8 = LoadTexture(RESOURCE_FOLDER"BB8.png");


   
    Matrix projectionMatrix;
    Matrix modelMatrix;
    Matrix viewMatrix;
    Matrix modelMatrix2;
    Matrix modelMatrix3;
    
    projectionMatrix.setOrthoProjection(-2.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
    modelMatrix.Scale(2.0f, 2.0f, 1.0f);
    modelMatrix3.Translate(-.52f, -0.4f, 0.0f);

    float lastFrameTicks = 0.0f;
    
    glUseProgram(program.programID);
    
    SDL_Event event;
    bool done = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }
        float ticks = (float)SDL_GetTicks()/1000.0f;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        
        glClear(GL_COLOR_BUFFER_BIT); //clear buffer in memory
        program.setModelMatrix(modelMatrix);
        program.setProjectionMatrix(projectionMatrix);
        program.setViewMatrix(viewMatrix);

        modelMatrix.identity();
        modelMatrix.Scale(1.0f, 1.7f, 1.0f);
        modelMatrix.Translate(1.0f, 0.0f, 0.0f);
        glBindTexture(GL_TEXTURE_2D, c3p0);
        
        float vertices[] = {-0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5};
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(program.positionAttribute);
        
        float texCoords[] = {0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f};
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(program.texCoordAttribute);

        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glBindTexture(GL_TEXTURE_2D, r2d2);
        program.setModelMatrix(modelMatrix2);
        modelMatrix2.identity();
        modelMatrix2.Translate(0.2f, -0.4f, 0.0f);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glBindTexture(GL_TEXTURE_2D, bb8);
        program.setModelMatrix(modelMatrix3);
        modelMatrix3.Rotate(elapsed*sinf(45)*2);

        modelMatrix3.Translate(0.0f, 0.0f, 0.0f);
        glDrawArrays(GL_TRIANGLES, 0, 6);


        glDisableVertexAttribArray(program.positionAttribute);
        glDisableVertexAttribArray(program.texCoordAttribute);
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
        SDL_GL_SwapWindow(displayWindow);
    }
   
    
    SDL_Quit();
    return 0;
}
