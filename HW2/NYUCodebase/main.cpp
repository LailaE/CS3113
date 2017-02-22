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
        std::cout << "Unable to load image\n";
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

void draw(ShaderProgram program, Matrix mM, float vertices[]){
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program.positionAttribute);
    
    program.setModelMatrix(mM);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(program.positionAttribute);
}

struct Paddle{
    float top, bottom, right, left;
    Paddle(float t, float b, float r, float l):top(t), bottom(b), right(r), left(l){}
};

struct Ball{
    float x, y, dx, dy;
    Ball(float x, float y, float dx, float dy):x(x), y(y), dx(dx), dy(dy){}
};

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Laila Esa - Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
    glewInit();
#endif
    
    
    glViewport(0, 0, 640, 360);
    ShaderProgram program(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
    
   
    Matrix projectionMatrix;
    Matrix leftPaddleModelMatrix;
    Matrix viewMatrix;
    Matrix rightPaddleModelMatrix;
    Matrix ballModelMatrix;
    
    projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);


    float lastFrameTicks = 0.0f;
    float PADDLEMOVEMENT = 0.8f;
    float BALLSPEED = .1f;
    float ticks = (float)SDL_GetTicks()/1000.0f;
    float elapsed = ticks - lastFrameTicks;
    lastFrameTicks = ticks;

    glUseProgram(program.programID);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.15f, 0.0f, 1.0f);
    
    SDL_Event event;
    Paddle left(.5, -0.5, -3.35,-3.45);
    Paddle right(.5, -0.5, 3.45, 3.35);
    Ball ball(0.0, 0.0f, cosf((float) rand()), sinf((float) rand()));
    bool done = false;
    bool gameOn = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }else if(event.type == SDL_KEYDOWN) {
                if(event.key.keysym.scancode == SDL_SCANCODE_S) {
                    if(left.bottom >= -2){
                        left.bottom -= PADDLEMOVEMENT*elapsed;
                        left.top -= PADDLEMOVEMENT*elapsed;
                        leftPaddleModelMatrix.Translate(0.0f, -PADDLEMOVEMENT*elapsed, 0.0f);
                    }
                } else if(event.key.keysym.scancode == SDL_SCANCODE_W) {
                    if(left.top <= 2){
                        left.bottom += PADDLEMOVEMENT*elapsed;
                        left.top += PADDLEMOVEMENT*elapsed;
                        leftPaddleModelMatrix.Translate(0.0f, PADDLEMOVEMENT*elapsed, 0.0f);
                    }
                }if(event.key.keysym.scancode == SDL_SCANCODE_DOWN) {
                    if(right.bottom >= -2){
                        right.bottom -= PADDLEMOVEMENT*elapsed;
                        right.top -= PADDLEMOVEMENT*elapsed;
                        rightPaddleModelMatrix.Translate(0.0f, -PADDLEMOVEMENT*elapsed, 0.0f);
                    }
                }else if(event.key.keysym.scancode == SDL_SCANCODE_UP) {
                    if(right.top <= 2){
                        right.top += PADDLEMOVEMENT*elapsed;
                        right.bottom += PADDLEMOVEMENT*elapsed;
                        rightPaddleModelMatrix.Translate(0.0f, PADDLEMOVEMENT * elapsed, 0.0f);
                    }
                }else if(event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
                    gameOn = true;
                }
            }
        }
       
        
        glClear(GL_COLOR_BUFFER_BIT); //clear buffer in memory
        program.setModelMatrix(leftPaddleModelMatrix);
        program.setProjectionMatrix(projectionMatrix);
        program.setViewMatrix(viewMatrix);


        //Left Paddle
        float leftVertices[] = {-3.45f, -0.5f, -3.35f, -0.5f, -3.35f, 0.5f, -3.45f, -0.5f, -3.35f, 0.5f, -3.45f, 0.5f };
        draw(program,leftPaddleModelMatrix, leftVertices);
       
        //Right Paddle
        float rightVertices[] = {3.45f, -0.5f, 3.35f, -0.5f, 3.35f, 0.5f, 3.45f, -0.5f, 3.35f, 0.5f, 3.45f, 0.5f };
        draw(program,rightPaddleModelMatrix,rightVertices);

        //Ball
        float ballVertices[] = {0.0f, 0.0f, .065f, 0.0f, 0.065f, 0.065f,0.0f, 0.0f, 0.065f, 0.065f, 0.0f, 0.065f};
        draw(program,ballModelMatrix, ballVertices);
        
        if(gameOn){
            
            //Left wins
            if(ball.x -.1>= right.left){
                ballModelMatrix.Translate(-ball.x, -ball.y, 0.0f);
                ball.x = 0.0f;
                ball.y = 0.0f;
                std::cout << "Left Wins \n";
                gameOn = false;
                
            }
            
            //Right wins
            else if(ball.x <= left.right ){
                ballModelMatrix.Translate(-ball.x, -ball.y, 0.0f);
                ball.x = 0.0f;
                ball.y = 0.0f;
                std::cout<< "Right Wins\n";
                gameOn = false;
                
            }
            //Hits Border
            else if(ball.y >= 2.0 || ball.y <= -2.0){
                ball.dy *= -1;
                ballModelMatrix.Translate(ball.dx * elapsed * BALLSPEED, ball.dy * elapsed * BALLSPEED, 0.0f);
                ball.x += ball.dx * elapsed * BALLSPEED;
                ball.y += ball.dy * elapsed * BALLSPEED;
            }
            
            //Hits a paddle
            else if(((ball.y -.1 <= left.top) && (ball.y +.1 >=left.bottom )&& (ball.x -.1 <= left.right) && (ball.x +.1 >= left.left)) ||
                    ((ball.y -.1 <= right.top) && (ball.y +.1 >= right.bottom) && (ball.x -.1 <= right.right) && (ball.x +.1 >= right.left))){
                ball.dx *= -1;
                ballModelMatrix.Translate(ball.dx * elapsed *.1, ball.dy * elapsed *BALLSPEED, 0.0f);
                ball.x += ball.dx * elapsed * BALLSPEED;
                ball.y += ball.dy * elapsed * BALLSPEED;
            }
            else{
                ballModelMatrix.Translate(( ball.dx *elapsed *BALLSPEED), ball.dy * elapsed * BALLSPEED, 0.0f);
                ball.x += ball.dx * elapsed * BALLSPEED;
                ball.y += ball.dy * elapsed * BALLSPEED;
            }
        }
        
        
        SDL_GL_SwapWindow(displayWindow);
    }
   
    
    SDL_Quit();
    return 0;
}
