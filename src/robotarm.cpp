/*
   Implements a specific configuration of the robot arm.
 */

#include "utils.h"
#include "robotarm.h"
#include "link.h"
#include "base.h"
#include "brush.h"

RobotArm::RobotArm()
{
   // The joints and links of this paintbot
   Base*  base  = new Base(300);
   Link*  link1 = new Link(Joint(PRISMATIC, 0, 300), 150, base);
   Link*  link2 = new Link(Joint(REVOLUTE, 0, 360), 100, link1);
   Link*  link3 = new Link(Joint(REVOLUTE, 0, 360), 75, link2);
   Brush* brush = new Brush(link3);

   base->next_link = link1;
   link1->next_link = link2;
   link2->next_link = link3;
   link3->next_link = brush;
   
   links[0] = base;
   links[1] = link1;
   links[2] = link2;
   links[3] = link3;
   links[4] = brush;
}

RobotArm::~RobotArm()
{
}

Link* RobotArm::getLink(int link)
{
   if (link < 0 || link > NUM_LINKS + 1)
      return 0;

   if (link == 0)
      return getBase();

   if (link == NUM_LINKS + 1)
      return getBrush();

   return links[link];
}


