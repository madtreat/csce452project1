
#include "window.h"
#include "consts.h"
#include "canvas.h"
#include "canvaswidget.h"
#include "manager.h"

#include <QLabel>
#include <QDebug>
#include <QGridLayout>
#include <QSpinBox>
#include <QTimer>
#include <QPushButton>
#include <QFile>

#include <unistd.h>


Window::Window(Manager* _manager)
: QWidget(),
manager(_manager),
selection(0)
{
   initStyles();
   initCanvas();
   initLayout();

   setObjectName("main-window");

   // Start animating
   QTimer* timer = new QTimer(this);
	timer->setInterval(30);	//miliseconds
   connect(timer, SIGNAL(timeout()), this, SLOT(windowAnimate()));
   timer->start();

   windowTitle = "Cell Decomposition";
   setWindowTitle(windowTitle + " - Robot");
}

Window::~Window()
{
   delete canvas;
}

void Window::windowAnimate()
{
   // move each car based on positions of light sources
	manager->timeStep();
   // draw the new positions
	canvasWidget->animate();
}

void Window::pathCalculated()
{
   // once the path has been calculated, change the window title
   QString titleSuffix = " - Path Calculated!";
   setWindowTitle(windowTitle + titleSuffix);
}

void Window::initStyles()
{
   QFile file(":/style");
   file.open(QFile::ReadOnly);
   
   controlPanelStyle = QLatin1String(file.readAll());
}

//--------------------------------------------------------//
// Create the canvas and its container widget
//--------------------------------------------------------//
void Window::initCanvas()
{
   if (!manager)
      manager   = new Manager();
   canvas       = new Canvas(manager);
   canvasWidget = new CanvasWidget(canvas, this);
}

void Window::initLayout()
{
   // Create the main/top-level grid layout
   layout = new QGridLayout(this);
   layout->setContentsMargins(0, 0, 0, 0);

   // Add to main grid layout
   if (!canvasWidget)
      initCanvas();
   layout->addWidget(canvasWidget, 0, 0);
}

void Window::keyPressEvent(QKeyEvent* event)
{
   //qDebug() << "Key Pressed" << event->key();
   QString titleSuffix = " - ";

   if (event->key() == Qt::Key_Space)
   {
      // generate path - slightly different logic for title than normal
      titleSuffix += "Calculating Path...";
      setWindowTitle(windowTitle + titleSuffix);
      manager->generatePath();
      pathCalculated();
   }
	if (event->key() == Qt::Key_1)
   {
      // select box 1 for repositioning
      selection = 2;
      titleSuffix += "Box 0";
   }
	if (event->key() == Qt::Key_2)
   {
      // select box 2 for repositioning
      selection = 3;
      titleSuffix += "Box 1";
   }
	if (event->key() == Qt::Key_3)
   {
      // select box 3 for repositioning
      selection = 4;
      titleSuffix += "Box 2";
   }
   if (event->key() == Qt::Key_R)
   {
      // select robot marker for repositioning
      selection = 0;
      titleSuffix += "Robot";
   }
	if (event->key() == Qt::Key_D)
   {
      // select destination marker for repositioning
      selection = 1;
      titleSuffix += "Dest";
   }

   // update title
   if (titleSuffix == " - ")
   {
      // do nothing - no change
   }
   else
   {
      // update the title, it changed
      setWindowTitle(windowTitle + titleSuffix);
   }
}

void Window::keyReleaseEvent(QKeyEvent* event)
{
   //qDebug() << "Key Released" << event->key();

}

void Window::mouseReleaseEvent(QMouseEvent* e)
{
	//qDebug() << "Mouse Released" << event->pos();
	manager->clearCells();
   manager->decompose();
   manager->connectCells();
	switch (selection)
	{
		case 0:		//Robot
		{
			manager->setRobot(Position(e->x(),e->y()));
			break;
		}
		case 1:		//Destination
		{
			manager->setDest(Position(e->x(),e->y()));
			break;
		}
		case 2:		//Box 0
		{
			manager->setBox(0, Position(e->x(),e->y()));
			break;
		}
		case 3:		//Box 1
		{
			manager->setBox(1, Position(e->x(),e->y()));
			break;
		}
		case 4:		//Box 2
		{
			manager->setBox(2, Position(e->x(),e->y()));
			break;
		}
	}
}

