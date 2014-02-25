/*
   Configuration for the paint bot's arm
 */

#ifndef ARMCONFIG_H_
#define ARMCONFIG_H_

struct Link;
struct Base;
struct Brush;

enum Motion {
   CW    = 1,
   CCW   = 2,
   LEFT  = 4 | CCW,
   RIGHT = 4 | CW
};

class RobotArm {
public:
   static const int NUM_LINKS = 3;

   RobotArm();
   ~RobotArm();

   Link* getLink(int link);
   Link* getBase()  {return links[0];}
   Link* getBrush() {return links[NUM_LINKS+1];}

   void moveJoint(Link* link, Motion motion);

private:
   // the size of links is NUM_LINKS + base + brush (hence the +2)
   Link* links[NUM_LINKS+2];
};

#endif

