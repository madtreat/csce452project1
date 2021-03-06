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
   RIGHT = 4 | CW,
   X     = 8,
   X_DEC = X | LEFT,
   X_INC = X | RIGHT,
   Y     = 16,
   Y_DEC = Y | LEFT,
   Y_INC = Y | RIGHT
};

class RobotArm {
public:
   static const int NUM_LINKS = 3;
   // the size of links is NUM_LINKS + base + brush (hence the +2)
   static const int LENGTH = NUM_LINKS + 2;
	
	// offset for X values to center them on the canvas
	static const int OFFSET_X = 640/2 - 150;
	static const int OFFSET_Y = 480-50;
	//TODO:-----possibly move these^^^-----------

   RobotArm();
   ~RobotArm();

   Link* getLink(int link);
   Link* getBase()  {return links[0];}
   Link* getBrush() {return links[LENGTH-1];}
   
	void rotateJoint(Link* link, Motion motion, int deg);
   void translateJoint(Link* link, Motion motion, int amt);
	void translateBrush(Link* brush, int newX, int newY);

private:
   Link* links[LENGTH];
   
};

#endif

