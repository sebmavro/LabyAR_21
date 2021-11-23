#ifndef AMAZED_OPENGL_H
#define AMAZED_OPENGL_H

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif

#include "../stream/CameraStream.h"
#include "GlutWindow.h"
#include "GlutMaster.h"
#include "../physics/Ball.h"
#include "../physics/Wall.h"

class OpenGL : public GlutWindow{

private:

    std::string const assetsPath = "../";

    bool anaglyph;

    bool click = true;
    
    /// Nombre d'images par seconde
    int const MAX_FPS = 60;


    /// Parametres du drapeau
    double const FLAG_PIPE_HEIGHT = 0.15;
    double const FLAG_PIPE_RADIUS = 0.007;
    double const FLAG_TOP_SIZE = 0.10;

    /// Index des textures
    int const ID_TEXT_CAM = 1;
    int const ID_TEXT_MAZE = 2;
    int const ID_TEXT_WALL = 3;
    int const ID_TEXT_FLAG = 4;
    GLuint textArray[4];

    /// Matrice de projection
    double *p;

    /// Matrice de Modelview
    double *m;

    double fps = 0.0;

    time_t start;
    int height, width;
    int initPositionX, initPositionY;

    std::vector<Wall> walls;

    cv::Point2d *endPoint;
    Ball *ball;
    CameraStream *cameraStream;
    cv::Mat textCam;
    cv::Mat textMaze;
    cv::Mat textWall;
    cv::Mat textFlag;
    cv::VideoCapture cap;
    
    /// Données de stress du casque Muse
    unsigned int progress;
    std::vector<int> progressData;

    int nbCollisions;

    bool isWritten;

    void drawAxes();
    void drawMazeGround();
    void drawBackground();
    void drawWalls();
    void drawFlag();
    void loadTexture(GLuint id, cv::Mat img);
public:

    static constexpr double FLAG_PIPE_HITBOX = 0.05;
    cv::Point2d *getEndPoint();
    void setEndPoint(cv::Point2d *point);
    void setWalls(const std::vector<Wall> &walls);
    void setProjectionMatrix(const double* p);
    void setModelviewMatrix(const double *m);
    void setFps(double fps);
    void applicateMaterial();
    void applicateLight();
    void shadowMatrix(GLfloat points_plan[3][3], const GLfloat lightPos[4], GLfloat destMat[4][4]);
    void vecteurUnite(float *vector);
    void normal(float v[3][3], float out[3]);
    void startTimer();
    void displayText(float x, float y, int r, int g, int b, const char *string);

    //Fonction pour l'attribut nbCollisions
    int getNBCollisions();
    void incrementNBCollisions(int numUser);

    OpenGL( GlutMaster * glutMaster,
            int setWidth,
            int setHeight,
            int setInitPositionX,
            int setInitPositionY,
            char * title,
            Ball *ball,
            CameraStream * cameraStream,
            bool anaglyph,
            unsigned int progress);

    ~OpenGL() override;
    void CallBackDisplayFunc() override;
    void CallBackReshapeFunc(int w, int h) override;
    void CallBackKeyboardFunc(unsigned char key, int x, int y) override;

    void CallBackIdleFunc() override;

    const std::vector<Wall> &getWalls() const;

    void filtreRouge();

    void filtreBleu();

    void drawElements();

    void offsetCamera(double offset, double *pDouble);

    int getProgress() const;

    void setTextFlag(const cv::Mat &textFlag);

    void setProgress(int progress);

    time_t getStart();
};


#endif //AMAZED_OPENGL_H
