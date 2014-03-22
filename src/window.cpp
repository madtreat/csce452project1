
#include "window.h"
#include "canvas.h"
#include "canvaswidget.h"
#include "robotarm.h"

#include <QLabel>
#include <QDebug>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QTimer>
#include <QPushButton>
#include <QFile>
#include <QApplication>
#include <QGroupBox>
#include <QSpacerItem>

Window::Window(ConnectionInfo _info)
: QWidget(),
conn(_info),
socket(NULL),
server(NULL)
{
   initStyles();
   initCanvas();
   initLayout();
   QWidget* controlLabel = initControlPanel();

   QWidget* jointPanel = initJointControls();
   QWidget* worldPanel = initWorldControls();
   QWidget* brushPanel = initBrushControls();

   controlLayout->addWidget(controlLabel, 0, 0, 1, 2);
   controlLayout->addWidget(brushPanel,   1, 0, 1, 2);
   controlLayout->addWidget(jointPanel,   2, 0, 1, 1);
   controlLayout->addWidget(worldPanel,   2, 1, 1, 1);

   connect( canvasWidget,  SIGNAL(jointsChanged()),
            this,          SLOT  (updateBrushPos()));
   connect( canvasWidget,  SIGNAL(brushPosChanged()),
            this,          SLOT  (updateJointPos()));

   //--------------------------------------------------------//
   // Add control panel to main grid layout
   //--------------------------------------------------------//
   layout->addWidget(controlPanel, 0, 1);

   // Set up the connection
   bool success = false;
   if (conn.type == SERVER)
      success = startServer();
   else if (conn.type == CLIENT)
      success = connectToServer();

   if (!success)
   {
      qDebug() << "Error establishing connection...exiting.";
      exit(1);
   }

   // Start animating
   QTimer* timer = new QTimer(this);
   connect(timer, SIGNAL(timeout()), canvasWidget, SLOT(animate()));
   timer->start();

   setWindowTitle("PaintBot");
}

Window::~Window()
{
   delete canvas;
}

bool Window::startServer()
{
   if ( conn.port < 1000 )
   {
      qDebug() << "Invalid port";
      return false;
   }

   server = new QTcpServer(this);
   connect( server,  SIGNAL(newConnection()),
            this,    SLOT  (connectClient()));
   server->listen(conn.host, conn.port);

   qDebug() << "Server started successfully!";
   qDebug() << "Listening for connections on port" << conn.port << "\n";
   return true;
}

// used for clients connecting to a server
bool Window::connectToServer()
{
   if ( conn.port < 1000 )
   {
      qDebug() << "Inalid port";
      return false;
   }

   socket->connectToHost(conn.host, conn.port);
   connect(socket, SIGNAL(connected()), this, SLOT(connectionEstablished()));
   connect(socket, SIGNAL(readyRead()), this, SLOT(readMessage()));
   if (socket->waitForConnected(4000))
   {
      return true;
   }
   else
   {
      qDebug() << "Error connecting to server:" << socket->errorString();
      return false;
   }
}

// used for connecting a client to this server
void Window::connectClient()
{
   socket = server->nextPendingConnection();
   connect(socket, SIGNAL(disconnected()), this, SLOT(disconnectClient()));
   connect(socket, SIGNAL(readyRead()),    this, SLOT(readMessage()));

   // now that there is a connected client, make the server stop listening 
   // for new connections
   server->close();
}

// used for disconnecting a client from this server
// and start listening for new connections again
void Window::disconnectClient()
{
   if (server)
      server->listen(conn.host, conn.port);
   else
      startServer();
}

// read a message from the client (if a server) or from the server (if a client)
void Window::readMessage()
{
   int bytes = socket->bytesAvailable();
   QString data = socket->readAll();

   if (conn.type == SERVER)
      processMessageFromClient(data);
   else if (conn.type == CLIENT)
      processMessageFromServer(data);
}

// send a message to the client (if a server) or to the server (if a client)
void Window::sendMessage(QString msg)
{
   if (socket)
      socket->write(msg.toLatin1().data());
   else
      qDebug() << "Error: socket not connected";
}

void Window::processMessageFromClient(QString msg)
{
   // read and process a message from the client
   // do stuff
   QStringList parts = msg.split(":");
   QString jointName = parts[0];

   bool ok           = false;
   int newVal        = parts[1].toInt(&ok);
   int jointNum      = jointToNum(jointName);

   if (!ok || jointNum == 0) // the link changing should NEVER be the base/link 0
   {
      qDebug() << "Error processing message from client";
   }
   
   // set the new values
   if (jointNum == 1)
      joint1Spin->setValue(newVal);
   else if (jointNum == 2)
      joint2Spin->setValue(newVal);
   else if (jointNum == 3)
      joint3Spin->setValue(newVal);
   else if (jointName == "BrushX")
      brushSpinX->setValue(newVal);
   else if (jointName == "BrushY")
      brushSpinY->setValue(newVal);

   notifyClient();
}

void Window::processMessageFromServer(QString msg)
{
   QStringList lines = msg.split("\n");
   for (int i = 0; i < lines.size(); i++)
   {
      QStringList parts = lines.at(i).split(":");
      QString joint  = parts.at(0);

      int newSpinVal = parts.at(1).toInt(); // ignoring potential corruption 
      int newX       = parts.at(2).toInt(); // TODO: check corrupted int values
      int newY       = parts.at(3).toInt();

      int linkNum = jointToNum(joint);
      Link* link = arm->getLink(linkNum);
      if (joint == "Brush")
      {
         int brushSize = newSpinVal & 0xff;
         int painting  = newSpinVal >> 8;
         brushSizeSpin->setValue(brushSize);
         paintButton->setChecked((bool) painting);
      }
      else
      {
         link->joint.rotation = newSpinVal;
      }
      link->joint.X        = newX;
      link->joint.Y        = newY;
   }
}

int Window::jointToNum(QString name)
{
   if (name == "Brush" || name == "BrushX" || name == "BrushY")
      return RobotArm::LENGTH-1; // the brush is link 4/joint 4

   // parse the last character of name ( "Joint1" | "Joint2" | etc. )
   QChar digit = name.at(name.size()-1);
   int num = digit.digitValue();

   if (num == -1)
      return 0;
   
   return num;
}

QString Window::numToJoint(int num)
{
   if (num == RobotArm::LENGTH-1)
      return "Brush";

   QString joint = "Joint";
   joint += QString::number(num);
   return joint;
}

/* 
   After the server updates all positions, package up the new values
   and send them to the client
   
   Message structure (spaces added only for easier viewing):
      Joint1 : newVal : newX : newY \n
      Joint2 : newVal : newX : newY \n
      Joint3 : newVal : newX : newY \n
      Brush  : newVal : newX : newY \n
   where 
      newVal = the new spin box value  (joint.rotation)
         EXCEPT:
            brush newVal   = (painting << 8) | brushSize
            where painting = {0, 1} (false or true)
      newX   = the new X value         (joint.X)
      newY   = the new Y value         (joint.Y)
  
 */
void Window::notifyClient()
{
   QString msg = "";
   for (int i = 1; i < RobotArm::LENGTH; i++)
   {
      Link* link = arm->getLink(i);

      msg += numToJoint(i);
      msg += ":";
      msg += link->joint.rotation;
      msg += ":";
      msg += link->joint.X;
      msg += ":";
      msg += link->joint.Y;
      msg += "\n";
   }
   sendMessage(msg);
}

/*
   After the client changes any of the values on the control panel:
   package up the necessary data and send it to the server.

   Message Structure (spaces added only for easier viewing):
      name : newVal
   Where
      name = { 
         Joint1, 
         Joint2, 
         Joint3, 
         BrushX, 
         BrushY, 
         BrushSize, 
         Painting 
      }
      newVal = the new value to be sent corresponding to name
 */
void Window::notifyServer()
{
   QString msg;

   QString name = "Joint1";
   int newVal = 0;
   
   msg += name;
   msg += ":";
   msg += newVal;
   sendMessage(msg);
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
   // Create RobotArm here so we can call canvas::drawLinks
   arm          = new RobotArm();
   canvas       = new Canvas(arm);
   canvasWidget = new CanvasWidget(canvas, arm, this);
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

//--------------------------------------------------------//
// Create the control panel layout and widgets
//--------------------------------------------------------//
QWidget* Window::initControlPanel()
{
   QSizePolicy policy(QSizePolicy::Fixed, QSizePolicy::Fixed);

   controlPanel  = new QWidget(this);
   controlPanel->setMinimumWidth(2*CONTROL_WIDTH+24);
   controlPanel->setSizePolicy(policy);
   controlPanel->setStyleSheet(controlPanelStyle);

   controlLayout = new QGridLayout(controlPanel);

   // Control Panel Label
   QLabel* controlLabel = new QLabel(tr("Controls"));
   controlLabel->setAlignment(Qt::AlignHCenter);
   return controlLabel;
}

QWidget* Window::initJointControls()
{
   QWidget* jointPanel = new QWidget(this);
   jointPanel->setMinimumWidth(CONTROL_WIDTH);
   jointPanel->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred));//Fixed));

   jointControls = new QWidget(this);
   jointControls->setObjectName("container");

   QWidget* joint1 = createJointControl(1);
   QWidget* joint2 = createJointControl(2);
   QWidget* joint3 = createJointControl(3);

   QVBoxLayout* jointLayout = new QVBoxLayout(jointControls);
   jointLayout->setContentsMargins(0, 0, 0, 0);
   jointLayout->addWidget(joint1);
   jointLayout->addWidget(joint2);
   jointLayout->addWidget(joint3);

   QString label = "Joint Mode";
   jointButton = new QPushButton(label, this);
   jointButton->setObjectName("controls");
   jointButton->setCheckable(true);
   connect(jointButton, SIGNAL(toggled(bool)),
           this,        SLOT  (toggleJointControlsVisible(bool)));
   jointButton->setChecked(true);

   QVBoxLayout* panelLayout = new QVBoxLayout(jointPanel);
   panelLayout->setContentsMargins(5, 5, 5, 5);
   panelLayout->addWidget(jointButton);
   panelLayout->addWidget(jointControls);

   return jointPanel;
}

QWidget* Window::initWorldControls()
{
   QWidget* worldPanel = new QWidget(this);
   worldPanel->setMinimumWidth(CONTROL_WIDTH);
   worldPanel->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred));//Fixed));

   worldControls = new QWidget(this);
   worldControls->setObjectName("container");

   //*
   //QWidget* joint1 = createWorldControl(1);
   QWidget* joint2 = createWorldControl(2);
   QWidget* joint3 = createWorldControl(3);
   // */
   QWidget* brush  = createWorldControl(4);

   QVBoxLayout* worldLayout = new QVBoxLayout(worldControls);
   worldLayout->setContentsMargins(0, 0, 0, 0);
   //worldLayout->addWidget(joint1);
   worldLayout->addWidget(brush);
   //*
   worldLayout->addWidget(joint2);
   worldLayout->addWidget(joint3);
   // */

   QString label = "World Mode";
   worldButton = new QPushButton(label, this);
   worldButton->setObjectName("controls");
   worldButton->setCheckable(true);
   connect(worldButton, SIGNAL(toggled(bool)),
           this,        SLOT  (toggleWorldControlsVisible(bool)));
   worldButton->setChecked(true);

   QVBoxLayout* panelLayout = new QVBoxLayout(worldPanel);
   panelLayout->setContentsMargins(5, 5, 5, 5);
   panelLayout->addWidget(worldButton);
   panelLayout->addWidget(worldControls);

   return worldPanel;
}

QWidget* Window::initBrushControls()
{
   QWidget* brushPanel = createBrushControl();

   return brushPanel;
}

void Window::togglePaintText(bool enabled)
{
   if (enabled)
      paintButton->setText("Stop Painting");
   else
      paintButton->setText("Paint");
}

void Window::toggleJointControlsVisible(bool enabled)
{
   jointControls->setVisible(enabled);
   /*
   if (enabled)
      jointButton->setText("Joint Mode");
   else
      jointButton->setText("Show Joint Mode");
   // */
}

void Window::toggleWorldControlsVisible(bool enabled)
{
   worldControls->setVisible(enabled);
   /*
   if (enabled)
      worldButton->setText("World Mode");
   else
      worldButton->setText("Show World Mode");
   // */
}

void Window::updateBrushPos()
{
   if (!brushSpinX || !brushSpinY)
      return;

   // block all signals from being emitted from the canvasWidget while all values are updated
   // to avoid infinite signal-update loops
   canvasWidget->blockSignals(true);

   // update the brush spin boxes' values
   brushSpinX->setValue(arm->getBrush()->joint.X);
   brushSpinY->setValue(arm->getBrush()->joint.Y);

   // allow canvasWidget's signals to emit again
   canvasWidget->blockSignals(false);
}

void Window::updateJointPos()
{
   if (!joint1Spin || !joint2Spin || !joint3Spin)
      return;

   // block all signals from being emitted from the canvasWidget while all values are updated
   // to avoid infinite signal-update loops
   canvasWidget->blockSignals(true);

   // update brush spin boxes' values
   joint1Spin->setValue(arm->getLink(1)->joint.rotation);
   joint2Spin->setValue(arm->getLink(2)->joint.rotation);
   joint3Spin->setValue(arm->getLink(3)->joint.rotation);

   // allow canvasWidget's signals to emit again
   canvasWidget->blockSignals(false);
}

void Window::keyPressEvent(QKeyEvent* event)
{
   qDebug() << "Key Pressed" << event->key();
   if (event->key() == Qt::Key_Space)
   {
   }
   // move right
   else if (event->key() == Qt::Key_Right || event->key() == Qt::Key_D)
   {
      int newX = 2 + arm->getBrush()->joint.X;
      canvasWidget->changeBrushLocX(newX);
   }
   // move left
   else if (event->key() == Qt::Key_Left || event->key() == Qt::Key_A)
   {
      int newX = -2 + arm->getBrush()->joint.X;
      canvasWidget->changeBrushLocX(newX);
   }
   // move up
   else if (event->key() == Qt::Key_Up || event->key() == Qt::Key_W)
   {
      int newY = 2 + arm->getBrush()->joint.Y;
      canvasWidget->changeBrushLocY(newY);
   }
   // move down
   else if (event->key() == Qt::Key_Down || event->key() == Qt::Key_S)
   {
      int newY = -2 + arm->getBrush()->joint.Y;
      canvasWidget->changeBrushLocY(newY);
   }
}

void Window::keyReleaseEvent(QKeyEvent* event)
{
   qDebug() << "Key Released" << event->key() << "(" << Qt::Key_Space << ")";
   if (event->key() == Qt::Key_Space)
   {
      qDebug() << "Toggling paint in canvas widget...";
      paintButton->toggle();
   }
   else if (event->key() == Qt::Key_1)
   {
      // focus joint 1 control
      //joint1Spin->setFocus();
   }
   else if (event->key() == Qt::Key_2)
   {
      // focus joint 2 control
      //joint2Spin->setFocus();
   }
   else if (event->key() == Qt::Key_3)
   {
      // focus joint 3 control
      //joint3Spin->setFocus();
   }
   else if (event->key() == Qt::Key_4)
   {
      // focus brush control
      //brushSpinX->setFocus();
   }
   // move right
   else if (event->key() == Qt::Key_Right || event->key() == Qt::Key_D)
   {
      int newX = 2 + arm->getBrush()->joint.X;
      canvasWidget->changeBrushLocX(newX);
   }
   // move left
   else if (event->key() == Qt::Key_Left || event->key() == Qt::Key_A)
   {
      int newX = -2 + arm->getBrush()->joint.X;
      canvasWidget->changeBrushLocX(newX);
   }
   // move up (NOT, it is actually reversed)
   else if (event->key() == Qt::Key_Up || event->key() == Qt::Key_W)
   {
      int newY = -2 + arm->getBrush()->joint.Y;
      canvasWidget->changeBrushLocY(newY);
   }
   // move down (NOT, it is actually reversed)
   else if (event->key() == Qt::Key_Down || event->key() == Qt::Key_S)
   {
      int newY = 2 + arm->getBrush()->joint.Y;
      canvasWidget->changeBrushLocY(newY);
   }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - -//
// Joint Controls: widget { gridlayout { label, spinbox } }
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -//
QWidget* Window::createJointControl(int id)
{
   // This joint
   Joint j        = arm->getLink(id)->joint;
   QString name   = "joint" + QString::number(id);
   QString label  = "Joint " + QString::number(id);
   if (j.type == REVOLUTE)
      label += " (deg)";
   else
      label += " (px)";

   QWidget*     joint   = new QWidget(this);
   joint->setObjectName(name);
   joint->setMinimumHeight(JOINT_HEIGHT);

   QLabel*      jLabel  = new QLabel(label, joint);
   jLabel->setAlignment(Qt::AlignCenter);

   QSpinBox*    jSpin1  = new QSpinBox(joint);
   jSpin1->setObjectName(name);
   jSpin1->setMinimumHeight(JOINT_HEIGHT);
   jSpin1->setRange(j.range_min, j.range_max);
   jSpin1->setSuffix(" (x1)");
   jSpin1->setWrapping(j.type == REVOLUTE);
   jSpin1->setKeyboardTracking(false);

   QSpinBox*    jSpin5  = new QSpinBox(joint);
   jSpin5->setObjectName(name);
   jSpin5->setMinimumHeight(JOINT_HEIGHT);
   jSpin5->setRange(j.range_min, j.range_max);
   jSpin5->setSingleStep(5);
   jSpin5->setSuffix(" (x5)");
   jSpin5->setWrapping(j.type == REVOLUTE);
   jSpin5->setKeyboardTracking(false);

	// connect the spin boxes to each other
   connect( jSpin1,        SIGNAL(valueChanged(int)),
            jSpin5,        SLOT  (setValue(int)));
   connect( jSpin5,        SIGNAL(valueChanged(int)),
            jSpin1,        SLOT  (setValue(int)));
	
   // Connect the signal to the joint's slot
   if (id == 1)
   {
      connect( jSpin1,        SIGNAL(valueChanged(int)),
               canvasWidget,  SLOT  (changeJoint1(int)));
      /*
		connect( jSpin5,        SIGNAL(valueChanged(int)),
               canvasWidget,  SLOT  (changeJoint1(int)));
		// */
      joint1Spin = jSpin5;
   }
   else if (id == 2)
   {
      connect( jSpin1,        SIGNAL(valueChanged(int)),
               canvasWidget,  SLOT  (changeJoint2(int)));
      /*
		connect( jSpin5,        SIGNAL(valueChanged(int)),
               canvasWidget,  SLOT  (changeJoint2(int)));
		// */
      joint2Spin = jSpin5;
   }
   else if (id == 3)
   {
      connect( jSpin1,        SIGNAL(valueChanged(int)),
               canvasWidget,  SLOT  (changeJoint3(int)));
      /*
		connect( jSpin5,        SIGNAL(valueChanged(int)),
               canvasWidget,  SLOT  (changeJoint3(int)));
		// */
      joint3Spin = jSpin5;
   }
   
   // set the default value afterward connecting so it will update the canvas
   jSpin1->setValue(j.rotation);

   // set tab order
   QWidget::setTabOrder(jSpin5, jSpin1);

   QGridLayout* jLayout = new QGridLayout(joint);
   jLayout->setContentsMargins(5, 5, 5, 5);
   jLayout->addWidget(jLabel, 0, 0, 1, 1);
   jLayout->addWidget(jSpin1, 2, 0, 1, 1);
   jLayout->addWidget(jSpin5, 1, 0, 1, 1);

   return joint;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - -//
// World Controls: widget { gridlayout { label, spinbox, spinbox } }
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -//
QWidget* Window::createWorldControl(int id)
{
   // This joint
   Joint j = arm->getLink(id)->joint;
   QString name  = "joint" + QString::number(id);
   QString label = "Joint " + QString::number(id);
   if (id == 4)
      label = "Brush";

   //QGroupBox* joint = new QGroupBox(label, this);
   QWidget* joint = new QWidget(this);
   joint->setObjectName(name);
   joint->setMinimumHeight(JOINT_HEIGHT);

   QLabel* jLabel = new QLabel(label, joint);
   jLabel->setAlignment(Qt::AlignCenter);

   QSpinBox* jSpinX = new QSpinBox(joint);
   jSpinX->setObjectName(name);
   jSpinX->setMinimumHeight(JOINT_HEIGHT);
   if (id == 4)
      jSpinX->setRange(arm->getBrush()->range_min_x, arm->getBrush()->range_max_x);
   else
      jSpinX->setRange(j.range_min, j.range_max);
   jSpinX->setPrefix("X=");
   jSpinX->setWrapping(j.type == REVOLUTE);
   jSpinX->setKeyboardTracking(false);

   QSpinBox* jSpinY = new QSpinBox(joint);
   jSpinY->setObjectName(name);
   jSpinY->setMinimumHeight(JOINT_HEIGHT);
   if (id == 4)
      jSpinY->setRange(arm->getBrush()->range_min_y, arm->getBrush()->range_max_y);
   else
      jSpinY->setRange(j.range_min, j.range_max);
   jSpinY->setPrefix("Y=");
   jSpinY->setWrapping(j.type == REVOLUTE);
   jSpinY->setKeyboardTracking(false);

   // Connect the signals to the joint's slot
   if (id == 4)
   {
      connect( jSpinX,        SIGNAL(valueChanged(int)),
               canvasWidget,  SLOT  (changeBrushLocX(int)));
      connect( jSpinY,        SIGNAL(valueChanged(int)),
               canvasWidget,  SLOT  (changeBrushLocY(int)));
   }
   
   // set the default value afterward connecting so it will update the canvas
   jSpinX->setValue(j.X);
   jSpinY->setValue(j.Y);

   // set tab order
   QWidget::setTabOrder(jSpinX, jSpinY);
   brushSpinX = jSpinX;
   brushSpinY = jSpinY;

   QGridLayout* jLayout = new QGridLayout(joint);
   jLayout->setContentsMargins(5, 5, 5, 5);
   jLayout->addWidget(jLabel, 0, 0);
   jLayout->addWidget(jSpinX, 1, 0);
   jLayout->addWidget(jSpinY, 2, 0);

   return joint;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - -//
// Add the brush activator button to the control panel
// - - - - - - - - - - - - - - - - - - - - - - - - - - - -//
QWidget* Window::createBrushControl()
{
   QString name = "brush";

   QWidget*     paintWidget = new QWidget(this);
   QGridLayout* paintLayout = new QGridLayout(paintWidget);

   paintWidget->setObjectName(name);
   paintWidget->setMinimumHeight(JOINT_HEIGHT);
   paintLayout->setContentsMargins(5, 5, 5, 5);

   paintButton = new QPushButton("Paint", paintWidget);
   paintButton->setCheckable(true);

   QSpinBox* bSpin = new QSpinBox(paintWidget);
   bSpin->setObjectName(name);
   bSpin->setMinimumHeight(JOINT_HEIGHT);
   bSpin->setRange(BRUSH_MIN, BRUSH_MAX);
   bSpin->setPrefix("Size=");
   // do not allow the brush size to wrap
   bSpin->setWrapping(false);

   // connect the brush spin box to the canvas widget
   connect( bSpin,         SIGNAL(valueChanged(int)),
            canvasWidget,  SLOT  (changeBrushSize(int)));

   // connect the button to the canvas widget
   connect( paintButton,  SIGNAL (toggled(bool)),
            canvasWidget, SLOT   (togglePaint(bool)));
   // connect the button to change the text
   connect( paintButton,  SIGNAL (toggled(bool)),
            this,         SLOT   (togglePaintText(bool)));

   brushSizeSpin = bSpin;

   paintLayout->addWidget(paintButton, 0, 0);
   paintLayout->addWidget(bSpin, 0, 1);

   return paintWidget;
}

