
#ifndef CONSTS_H_
#define CONSTS_H_


#include <vector>

const double PI = 3.1415926;
const int WIDTH  = 500;		//Window Size
const int HEIGHT = 500;		//Window Size
const int CAR_LENGTH = 11;
const int CAR_WIDTH = 8;
const int LIGHT_RADIUS = 5;

const int BUFFER = 50; // 50 pixel buffer for randomly generated locations
// meaning nothing can be randomly generated along the outer 50 pixels
// of the canvas

const int BOX1_SIZE = 200; 
const int BOX2_SIZE = 150;
const int BOX3_SIZE = 100;

struct Position {
   int X;
   int Y;
   Position(int _x = -1, int _y = -1) : X(_x), Y(_y) {};
	
	int	getX() const {return X;}
   int	getY() const {return Y;}
};

typedef Position Robot;


#endif
