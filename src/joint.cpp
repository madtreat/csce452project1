
#include "joint.h"

Joint::Joint(JointType _type, int _min, int _max)
: type(_type),
  range_min(_min),
  range_max(_max)
{}
