
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

Window::Window()
: QWidget(),
painting(false)
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
   //controlLayout->addSpacing(15);
   //controlLayout->addSpacing(15);

   //--------------------------------------------------------//
   // Add control panel to main grid layout
   //--------------------------------------------------------//
   layout->addWidget(controlPanel, 0, 1);

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

   QWidget* joint1 = createWorldControl(1);
   QWidget* joint2 = createWorldControl(2);
   QWidget* joint3 = createWorldControl(3);
   QWidget* brush  = createWorldControl(4);

   QVBoxLayout* worldLayout = new QVBoxLayout(worldControls);
   worldLayout->setContentsMargins(0, 0, 0, 0);
   //worldLayout->addWidget(joint1);
   worldLayout->addWidget(brush);
   worldLayout->addWidget(joint2);
   worldLayout->addWidget(joint3);

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
   painting = enabled;
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

void Window::keyPressEvent(QKeyEvent* event)
{
   //qDebug() << "Key Pressed" << event->key();
   if (event->key() == Qt::Key_Space)
   {
   }
}

void Window::keyReleaseEvent(QKeyEvent* event)
{
   //qDebug() << "Key Released" << event->key() << "(" << Qt::Key_Space << ")";
   if (event->key() == Qt::Key_Space)
   {
      qDebug() << "Toggling paint in canvas widget...";
      painting = !painting;
      paintButton->toggle();
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

   QWidget*     joint   = new QWidget(this);
   joint->setObjectName(name);
   joint->setMinimumHeight(JOINT_HEIGHT);

   QLabel*      jLabel  = new QLabel(label, joint);
   jLabel->setAlignment(Qt::AlignCenter);

   QSpinBox*    jSpin1  = new QSpinBox(joint);
   jSpin1->setObjectName(name);
   jSpin1->setMinimumHeight(JOINT_HEIGHT);
   jSpin1->setRange(j.range_min, j.range_max);
   if (j.type == REVOLUTE)
      jSpin1->setSuffix(" deg");
   else
      jSpin1->setSuffix(" px");
   // allow the joint to wrap if revolute
   jSpin1->setWrapping(j.type == REVOLUTE);

   QSpinBox*    jSpin5  = new QSpinBox(joint);
   jSpin5->setObjectName(name);
   jSpin5->setMinimumHeight(JOINT_HEIGHT);
   jSpin5->setRange(j.range_min, j.range_max);
   jSpin5->setSingleStep(5);
   jSpin5->setSuffix(" x5");
   jSpin5->setWrapping(j.type == REVOLUTE);

   // Connect the signal to the joint's slot
   if (id == 1)
   {
      connect( jSpin1,        SIGNAL(valueChanged(int)),
               canvasWidget,  SLOT  (changeJoint1(int)));
      connect( jSpin5,        SIGNAL(valueChanged(int)),
               canvasWidget,  SLOT  (changeJoint1(int)));
   }
   else if (id == 2)
   {
      connect( jSpin1,        SIGNAL(valueChanged(int)),
               canvasWidget,  SLOT  (changeJoint2(int)));
      connect( jSpin5,        SIGNAL(valueChanged(int)),
               canvasWidget,  SLOT  (changeJoint2(int)));
   }
   else if (id == 3)
   {
      connect( jSpin1,        SIGNAL(valueChanged(int)),
               canvasWidget,  SLOT  (changeJoint3(int)));
      connect( jSpin5,        SIGNAL(valueChanged(int)),
               canvasWidget,  SLOT  (changeJoint3(int)));
   }

   // connect the spin boxes to each other
   connect( jSpin1,        SIGNAL(valueChanged(int)),
            jSpin5,        SLOT  (setValue(int)));
   connect( jSpin5,        SIGNAL(valueChanged(int)),
            jSpin1,        SLOT  (setValue(int)));
   
   // set the default value afterward connecting so it will update the canvas
   jSpin1->setValue(j.rotation);

   QGridLayout* jLayout = new QGridLayout(joint);
   jLayout->setContentsMargins(5, 5, 5, 5);
   jLayout->addWidget(jLabel, 0, 0, 1, 1);
   jLayout->addWidget(jSpin1, 1, 0, 1, 1);
   jLayout->addWidget(jSpin5, 2, 0, 1, 1);

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
   jSpinX->setRange(j.range_min, j.range_max);
   jSpinX->setPrefix("X=");
   // allow the joint to wrap if revolute
   jSpinX->setWrapping(j.type == REVOLUTE);

   QSpinBox* jSpinY = new QSpinBox(joint);
   jSpinY->setObjectName(name);
   jSpinY->setMinimumHeight(JOINT_HEIGHT);
   jSpinY->setRange(j.range_min, j.range_max);
   jSpinY->setPrefix("Y=");
   // allow the joint to wrap if revolute
   jSpinY->setWrapping(j.type == REVOLUTE);

   // Connect the signals to the joint's slot
   if (id == 1)
   {
      connect( jSpinX,        SIGNAL(valueChanged(int)),
               canvasWidget,  SLOT  (changeJoint1LocX(int)));
      connect( jSpinY,        SIGNAL(valueChanged(int)),
               canvasWidget,  SLOT  (changeJoint1LocY(int)));
   }
   else if (id == 2)
   {
      connect( jSpinX,        SIGNAL(valueChanged(int)),
               canvasWidget,  SLOT  (changeJoint2LocX(int)));
      connect( jSpinY,        SIGNAL(valueChanged(int)),
               canvasWidget,  SLOT  (changeJoint2LocY(int)));
   }
   else if (id == 3)
   {
      connect( jSpinX,        SIGNAL(valueChanged(int)),
               canvasWidget,  SLOT  (changeJoint3LocX(int)));
      connect( jSpinY,        SIGNAL(valueChanged(int)),
               canvasWidget,  SLOT  (changeJoint3LocY(int)));
   }
   else if (id == 4)
   {
      connect( jSpinX,        SIGNAL(valueChanged(int)),
               canvasWidget,  SLOT  (changeJoint4LocX(int)));
      connect( jSpinY,        SIGNAL(valueChanged(int)),
               canvasWidget,  SLOT  (changeJoint4LocY(int)));
   }
   
   // set the default value afterward connecting so it will update the canvas
   jSpinX->setValue(j.X);
   jSpinY->setValue(j.Y);

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

   paintLayout->addWidget(paintButton, 0, 0);
   paintLayout->addWidget(bSpin, 0, 1);

   return paintWidget;
}
