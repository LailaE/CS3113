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
#include <vector>
#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

Matrix modelMatrix;
Matrix projectionMatrix;
Matrix viewMatrix;

ShaderProgram* program;
GLuint fontTexture;
GLuint spriteSheetTexture;


enum GameState {TITLE_SCREEN, GAME_LEVEL};
int state = TITLE_SCREEN;
bool gameOn = true;
float elapsed;



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

void draw(ShaderProgram program, float vertices[], float texCoord[]){
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program.positionAttribute);
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoord);
    glEnableVertexAttribArray(program.texCoordAttribute);
    //glBindTexture(GL_TEXTURE_2D, fontTexture);
    
    //program.setModelMatrix(mM);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);

}

void DrawText(ShaderProgram *program, int fontTexture, std::string text, float size, float spacing) {
    float texture_size = 1.0/16.0f;
    std::vector<float> vertexData;
    std::vector<float> texCoordData;
    for(int i=0; i < text.size(); i++) {
        int spriteIndex = (int)text[i];
        float texture_x = (float)(spriteIndex % 16) / 16.0f;
        float texture_y = (float)(spriteIndex / 16) / 16.0f;
        vertexData.insert(vertexData.end(), {
            ((size+spacing) * i) + (-0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (-0.5f * size), -0.5f * size,
            ((size+spacing) * i) + (0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (0.5f * size), -0.5f * size,
            ((size+spacing) * i) + (0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (-0.5f * size), -0.5f * size,
        });
        texCoordData.insert(texCoordData.end(), {
            texture_x, texture_y,
            texture_x, texture_y + texture_size,
            texture_x + texture_size, texture_y,
            texture_x + texture_size, texture_y + texture_size,
            texture_x + texture_size, texture_y,
            texture_x, texture_y + texture_size,
        });
    }
    //Matrix m;
    glUseProgram(program->programID);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
    glEnableVertexAttribArray(program->positionAttribute);
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
    glEnableVertexAttribArray(program->texCoordAttribute);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    
    glDrawArrays(GL_TRIANGLES, 0, (int)text.size() * 6);
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);

    //draw(*program, vertexData.data(), texCoordData.data());
}

void renderTitle(){
    modelMatrix.identity();
    modelMatrix.Translate(-2.2f, 0.5f, 0.0f);
    program->setModelMatrix(modelMatrix);
    DrawText(program, fontTexture, "Space Invaders", .35f, .001f);
    modelMatrix.Translate(-0.2f, -1.0f, 0.0f);
    program->setModelMatrix(modelMatrix);
    DrawText(program, fontTexture, "Press SPACE to play Game", .2f, .01f);

}

struct Entity{
    float posX, posY, dx, dy;
    float top, bottom, left, right;
    float u, v, width, height;
    Matrix myMatrix;
    Entity(){};
    Entity(float x, float y, float dx, float dy, float u, float v, float w, float h):
    posX(x), posY(y), dx(dx), dy(dy), u(u), v(v), width(w), height(h)
    {
        top = y+.05;
        bottom = y-.05;
        right = x+.05;
        left = x-.05;
        myMatrix.identity();
        myMatrix.Translate(x, y, 0.0f);
    }
    void Draw(){
        myMatrix.identity();
        myMatrix.Translate(posX, posY, 0.0f);
        //myMatrix.Rotate(sinf(45*(180/3.1415926)));

        program->setModelMatrix(myMatrix);
        
        float vertices[] = {-0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5};
        float texCoords[] = {u, v+height,
            u+width, v,
            u, v,
            u+width, v,
            u, v+height,
            u+width, v+height};
        
        glUseProgram(program->programID);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(program->positionAttribute);
        glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(program->texCoordAttribute);
        glBindTexture(GL_TEXTURE_2D, spriteSheetTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glDisableVertexAttribArray(program->positionAttribute);
        glDisableVertexAttribArray(program->texCoordAttribute);
    }
    
};



Entity player;
std::vector<Entity> enemies;
std::vector<Entity> playerBullets;
std::vector<Entity> enemyBullets;

void renderGame(){
    player.Draw();
    for(size_t i = 0; i < enemies.size(); ++i){
        enemies[i].Draw();
    }
    for(size_t i =0; i < playerBullets.size(); ++i){
        playerBullets[i].Draw();
    }
    for(size_t i = 0; i < enemyBullets.size(); ++i){
        enemyBullets[i].Draw();
    }
}

void Update(float elapsed){
    for(size_t i = 0; i < enemies.size(); ++i){
        enemies[i].posX += enemies[i].dx * elapsed;
        enemies[i].posY -= enemies[i].dy * elapsed;
        enemies[i].top -=enemies[i].dy * elapsed;
        enemies[i].bottom -=enemies[i].dy * elapsed;
        enemies[i].left += enemies[i].dx * elapsed;
        enemies[i].right += enemies[i].dx * elapsed;
        if((enemies[i].posX > 3.5 && enemies[i].dx != 0) || (enemies[i].posX < -3.5 && enemies[i].dx != 0)){
            enemies[i].dx *= -1;
        }
        if(enemies[i].bottom <= player.top && enemies[i].top >= player.bottom && enemies[i].left <= player.right && enemies[i].right >= player.left){
            gameOn = false;
            std::cout << "Touched an enemy\n";
        }

    }
    for(size_t i = 0; i < playerBullets.size(); ++i){
        playerBullets[i].posY += playerBullets[i].dy * elapsed;
        playerBullets[i].top += playerBullets[i].dy * elapsed;
        playerBullets[i].bottom += playerBullets[i].dy * elapsed;
        for(size_t j = 0; j < enemies.size(); ++j){
            if(playerBullets[i].bottom <= enemies[j].top && playerBullets[i].top <= enemies[j].bottom && playerBullets[i].left <= enemies[j].right && playerBullets[i].right <= enemies[j].left){
                enemies.erase(enemies.begin() +j);
            }
        }
    }
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
    program = new ShaderProgram(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    SDL_Event event;
    bool done = false;
    
    projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
    
    float lastFrameTicks = 0.0f;
    float ticks = (float)SDL_GetTicks()/1000.0f;
    elapsed = ticks - lastFrameTicks;
    lastFrameTicks = ticks;


    glUseProgram(program->programID);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(.0f,.0f,.0f,1.0f);
    fontTexture = LoadTexture("font1.png");
    spriteSheetTexture = LoadTexture("ships_Bullets.png");
    
    player = Entity(0.0f, -1.5f, .2f, 0.0f, (float)((82% 15)/15.0), (float)((82)/15.0)/10.0, 1.0/15.0, 1.0/10.0);
    for(int i = 0; i<10; ++i){
        enemies.push_back(Entity(-3.4 + (i%5), 1.5 - (i/5) , 0.02f, .001f,(float)((0% 15)/15.0), (float)((0)/15.0)/10.0, 1.0/15.0, 1.0/10.0));
    }
    
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }else if(event.type == SDL_KEYDOWN){
                if(event.key.keysym.scancode == SDL_SCANCODE_SPACE){
                    if(state == TITLE_SCREEN){
                        state = GAME_LEVEL;
                    }else{
                        playerBullets.push_back(Entity(player.posX, player.posY, 0.0f, 0.2, (float)((12% 15)/15.0), (float)((12)/15.0)/10.0, 1.0/15.0, 1.0/10.0));
                    }
                }else if(event.key.keysym.scancode == SDL_SCANCODE_RIGHT){
                    player.posX += .1f * elapsed;
                }else if(event.key.keysym.scancode == SDL_SCANCODE_LEFT){
                    player.posX -= .1f * elapsed;
                }
            }
        }
        glClear(GL_COLOR_BUFFER_BIT);
        program->setModelMatrix(modelMatrix);
        program->setProjectionMatrix(projectionMatrix);
        program->setViewMatrix(viewMatrix);
        
        if(gameOn){
            if(state == TITLE_SCREEN){
                renderTitle();
            }else if(state == GAME_LEVEL){
                Update(elapsed);
                renderGame();
            }
        }
        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}
