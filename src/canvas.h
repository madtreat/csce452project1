
#ifndef CANVAS_H_
#define CANVAS_H_

#include "joint.h"
#include "link.h"
#include "base.h"
#include "brush.h"

class RobotArm;

class Canvas
{
public:
   static const int WIDTH  = 640;
   static const int HEIGHT = 480;
   
   Canvas(RobotArm* _arm);
   ~Canvas();

   void init();

   void setPainting(bool enabled) {painting = enabled;}

   void circle(int x, int y, int radius);
   void drawRobot();
   void drawLinks();
   void paintCurrentLoc();
   void display();

private:
   RobotArm* robot;
   bool painting;
};

#endif

