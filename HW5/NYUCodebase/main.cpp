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

float elapsed;
ShaderProgram* program;


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

class Vector{
public:
    float x;
    float y;
    float length() const{
        return sqrtf(x*x + y*y);
    }

    
};


class Entity{
    
    float rotation;
    Matrix myMatrix;
public:
    float posX, posY, dx, dy;
    float top, bottom, left, right;
    float width, height;
    Vector topleft, topright, bottomright, bottomleft;
    std::vector<Vector> corners[4]; //starts with topleft corner then clockwise
    
    Entity(){};
    Entity(float x, float y, float dx, float dy, float r):
    posX(x), posY(y), dx(x), dy(y), rotation(r)
    {
        top = y+.05;
        bottom = y-.05;
        right = x+.05;
        left = x-.05;
        width = (right-left)/2;
        height = (top-bottom)/2;
        
        myMatrix.identity();
        myMatrix.Translate(x, y, 0.0f);
        myMatrix.setRotation(rotation);
    }
    
    void setCorners(){
        topleft.x = left; topleft.y = top;
        topright.x = right; topright.y = top;
        bottomright.x = right; bottomright.y = bottom;
        bottomleft.x = left; bottomleft.y = bottom;
        
    }
    void Draw(){
        myMatrix.identity();
        myMatrix.Translate(posX, posY, 0.0f);
        myMatrix.setRotation(rotation);
        
        program->setModelMatrix(myMatrix);
        float vertices[] = {-0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5};
        
        glUseProgram(program->programID);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(program->positionAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glDisableVertexAttribArray(program->positionAttribute);

    }
    
};

bool penetrationSort(const Vector &p1, const Vector &p2) {
    return p1.length() < p2.length();
}

bool testSATSeparationForEdge(float edgeX, float edgeY, const std::vector<Vector> &points1, const std::vector<Vector> &points2, Vector &penetration) {
    float normalX = -edgeY;
    float normalY = edgeX;
    float len = sqrtf(normalX*normalX + normalY*normalY);
    normalX /= len;
    normalY /= len;
    
    std::vector<float> e1Projected;
    std::vector<float> e2Projected;
    
    for(int i=0; i < points1.size(); i++) {
        e1Projected.push_back(points1[i].x * normalX + points1[i].y * normalY);
    }
    for(int i=0; i < points2.size(); i++) {
        e2Projected.push_back(points2[i].x * normalX + points2[i].y * normalY);
    }
    
    std::sort(e1Projected.begin(), e1Projected.end());
    std::sort(e2Projected.begin(), e2Projected.end());
    
    float e1Min = e1Projected[0];
    float e1Max = e1Projected[e1Projected.size()-1];
    float e2Min = e2Projected[0];
    float e2Max = e2Projected[e2Projected.size()-1];
    
    float e1Width = fabs(e1Max-e1Min);
    float e2Width = fabs(e2Max-e2Min);
    float e1Center = e1Min + (e1Width/2.0);
    float e2Center = e2Min + (e2Width/2.0);
    float dist = fabs(e1Center-e2Center);
    float p = dist - ((e1Width+e2Width)/2.0);
    
    if(p >= 0) {
        return false;
    }
    
    float penetrationMin1 = e1Max - e2Min;
    float penetrationMin2 = e2Max - e1Min;
    
    float penetrationAmount = penetrationMin1;
    if(penetrationMin2 < penetrationAmount) {
        penetrationAmount = penetrationMin2;
    }
    
    penetration.x = normalX * penetrationAmount;
    penetration.y = normalY * penetrationAmount;
    
    return true;
}

bool checkSATCollision(const std::vector<Vector> &e1Points, const std::vector<Vector> &e2Points, Vector &penetration) {
    std::vector<Vector> penetrations;
    for(int i=0; i < e1Points.size(); i++) {
        float edgeX, edgeY;
        
        if(i == e1Points.size()-1) {
            edgeX = e1Points[0].x - e1Points[i].x;
            edgeY = e1Points[0].y - e1Points[i].y;
        } else {
            edgeX = e1Points[i+1].x - e1Points[i].x;
            edgeY = e1Points[i+1].y - e1Points[i].y;
        }
        Vector penetration;
        bool result = testSATSeparationForEdge(edgeX, edgeY, e1Points, e2Points, penetration);
        if(!result) {
            return false;
        }
        penetrations.push_back(penetration);
    }
    for(int i=0; i < e2Points.size(); i++) {
        float edgeX, edgeY;
        
        if(i == e2Points.size()-1) {
            edgeX = e2Points[0].x - e2Points[i].x;
            edgeY = e2Points[0].y - e2Points[i].y;
        } else {
            edgeX = e2Points[i+1].x - e2Points[i].x;
            edgeY = e2Points[i+1].y - e2Points[i].y;
        }
        Vector penetration;
        bool result = testSATSeparationForEdge(edgeX, edgeY, e1Points, e2Points, penetration);
        
        if(!result) {
            return false;
        }
        penetrations.push_back(penetration);
    }
    
    std::sort(penetrations.begin(), penetrations.end(), penetrationSort);
    penetration = penetrations[0];
    
    Vector e1Center;
    for(int i=0; i < e1Points.size(); i++) {
        e1Center.x += e1Points[i].x;
        e1Center.y += e1Points[i].y;
    }
    e1Center.x /= (float)e1Points.size();
    e1Center.y /= (float)e1Points.size();
    
    Vector e2Center;
    for(int i=0; i < e2Points.size(); i++) {
        e2Center.x += e2Points[i].x;
        e2Center.y += e2Points[i].y;
    }
    e2Center.x /= (float)e2Points.size();
    e2Center.y /= (float)e2Points.size();
    
    Vector ba;
    ba.x = e1Center.x - e2Center.x;
    ba.y = e1Center.y - e2Center.y;
    
    if( (penetration.x * ba.x) + (penetration.y * ba.y) < 0.0f) {
        penetration.x *= -1.0f;
        penetration.y *= -1.0f;
    }
    
    return true;
}

std::vector<Entity> objects;

void Update(float elapsed){
    for(int i = 0; i < objects.size(); ++i){
        objects[i].posX += objects[i].dx * elapsed;
        objects[i].left += objects[i].dx * elapsed;
        objects[i].right += objects[i].dx * elapsed;
        
        objects[i].posY += objects[i].dy * elapsed;
        objects[i].top += objects[i].dy * elapsed;
        objects[i].bottom += objects[i].dy * elapsed;
        objects[i].setCorners();
        //if hits a border
        if(objects[i].top >= 2.0f){
            objects[i].dy *= -1;
            objects[i].posY += objects[i].dy * elapsed;
            objects[i].top += objects[i].dy * elapsed;
            objects[i].bottom += objects[i].dy * elapsed;
            objects[i].setCorners();
        }
        else if(objects[i].bottom <= -2.0f){
            objects[i].dy *= -1;
            objects[i].posY += objects[i].dy * elapsed;
            objects[i].top += objects[i].dy * elapsed;
            objects[i].bottom += objects[i].dy * elapsed;
            objects[i].setCorners();
        }
        else if(objects[i].left <= -3.55){
            objects[i].dx *=-1;
            objects[i].posX += objects[i].dx * elapsed;
            objects[i].left += objects[i].dx * elapsed;
            objects[i].right += objects[i].dx * elapsed;
            objects[i].setCorners();
        }
        else if(objects[i].right >= 3.55){
            objects[i].dx *= -1;
            objects[i].posX += objects[i].dx * elapsed;
            objects[i].left += objects[i].dx * elapsed;
            objects[i].right += objects[i].dx * elapsed;
            objects[i].setCorners();
        }
        //check collision with other objects
        for(int j = 0; j < objects.size(); ++j){
            if(i != j){ //not self
                std::vector<Vector> iCorners;
                iCorners.push_back(objects[i].topleft);
                iCorners.push_back(objects[i].topright);
                iCorners.push_back(objects[i].bottomright);
                iCorners.push_back(objects[i].bottomleft);

                std::vector<Vector> jCorners;
                iCorners.push_back(objects[j].topleft);
                iCorners.push_back(objects[j].topright);
                iCorners.push_back(objects[j].bottomright);
                iCorners.push_back(objects[j].bottomleft);
                
                Vector result;
                if(checkSATCollision(iCorners, jCorners, result)){
                    objects[i].posX += objects[i].dx * elapsed *(result.x/2);
                    objects[i].left += objects[i].dx * elapsed*(result.x/2);
                    objects[i].right += objects[i].dx * elapsed*(result.x/2);
                    
                    objects[i].posY += objects[i].dy * elapsed *(result.y/2);
                    objects[i].top += objects[i].dy * elapsed*(result.y/2);
                    objects[i].bottom += objects[i].dy * elapsed*(result.y/2);
                    objects[i].setCorners();

                    objects[j].posX += objects[j].dx * elapsed *-(result.x/2);
                    objects[j].left += objects[j].dx * elapsed*-(result.x/2);
                    objects[j].right += objects[j].dx * elapsed*-(result.x/2);
                    
                    objects[j].posX += objects[j].dx * elapsed *-(result.y/2);
                    objects[j].left += objects[j].dx * elapsed*-(result.y/2);
                    objects[j].right += objects[j].dx * elapsed*-(result.y/2);
                    objects[j].setCorners();

                }
                
            }
        }
    }
}
void render() {
    for(int i = 0; i < objects.size(); ++i){
        objects[i].Draw();
    }
}

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

    program = new ShaderProgram(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
    SDL_Event event;
    bool done = false;
   /*
    Matrix projectionMatrix;
    Matrix leftPaddleModelMatrix;
    Matrix viewMatrix;
    Matrix rightPaddleModelMatrix;
    Matrix ballModelMatrix;
    */
    projectionMatrix.setOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);


    float lastFrameTicks = 0.0f;
    float ticks = (float)SDL_GetTicks()/10000.0f;
    elapsed = ticks - lastFrameTicks;
    lastFrameTicks = ticks;
    
    glUseProgram(program->programID);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.15f, 0.0f, 1.0f);
    
    Entity p1(-2.0f, 0.0f, .002f, .002f, 25.0f);
    Entity p2(0.0f, 1.0f, .003f, .003f, 35.0f);
    Entity p3(1.0f, -1.0f, .004f, .004f, 45.0f);
    objects.push_back(p1);
    objects.push_back(p2);
    objects.push_back(p3);

    

    bool gameOn = true;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }else if(event.type == SDL_KEYDOWN) {
                if(event.key.keysym.scancode == SDL_SCANCODE_S) {
                    
                } else if(event.key.keysym.scancode == SDL_SCANCODE_W) {
                    
                }if(event.key.keysym.scancode == SDL_SCANCODE_DOWN) {
                    
                }else if(event.key.keysym.scancode == SDL_SCANCODE_UP) {
                    
                }else if(event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
                    gameOn = true;
                }
            }
        }
       
        
        glClear(GL_COLOR_BUFFER_BIT); //clear buffer in memory
        program->setModelMatrix(modelMatrix);
        program->setProjectionMatrix(projectionMatrix);
        program->setViewMatrix(viewMatrix);
        
        if(gameOn){
            Update(elapsed);
            render();
            
            /*
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
             */
        }
        
        
        SDL_GL_SwapWindow(displayWindow);
    }
   
    
    SDL_Quit();
    return 0;
}
