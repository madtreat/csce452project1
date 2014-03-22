
#ifndef WINDOW_H_
#define WINDOW_H_

#include <QWidget>
#include <QString>
#include <QColor>
#include <QGridLayout>
#include <QPushButton>
#include <QKeyEvent>
#include <QSpinBox>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QTcpServer>

class RobotArm;
class Canvas;
class CanvasWidget;

enum AppType {
   NOCONN = 0,
   SERVER = 1,
   CLIENT = 2
};

struct ConnectionInfo {
   AppType        type;    // type of connection
   QHostAddress   host;    // host address
   quint16        port;    // port number of server
   int            delay;   // time delay in seconds for connected applications
};

class Window : public QWidget
{
   Q_OBJECT

public:
   // sizes for the control panel widgets
   static const int CONTROL_WIDTH   = 132;
   static const int JOINT_HEIGHT    = 30;

   // range for the brush
   static const int BRUSH_MIN       = 5;
   static const int BRUSH_MAX       = 40;

   Window(ConnectionInfo _info);
   ~Window();

   // connection functions
   bool startServer();     // for servers
   bool connectToServer(); // for clients
   void processMessage(QString cmd);  // parse and execute an incoming command

   void initStyles();
   void initCanvas();
   void initLayout();
   QWidget* initControlPanel();

public slots:
   void connectClient();
   void disconnectClient();
   void readMessage();
   void sendMessage(QString);

   void togglePaintText(bool);
   void toggleJointControlsVisible(bool);
   void toggleWorldControlsVisible(bool);
   void updateBrushPos();
   void updateJointPos();

protected:
   void keyPressEvent(QKeyEvent* event);
   void keyReleaseEvent(QKeyEvent* event);

private:
   RobotArm*      arm;
   Canvas*        canvas;
   CanvasWidget*  canvasWidget;

   ConnectionInfo conn;
   QTcpSocket*    socket; // used by clients and servers - the client's connection
   QTcpServer*    server; // used by server only

   QString        controlPanelStyle;
   bool           painting;

   QGridLayout*   layout;
   QWidget*       controlPanel;
   QWidget*       jointControls;
   QWidget*       worldControls;

   QGridLayout*   controlLayout;
   QPushButton*   paintButton;
   QPushButton*   jointButton;
   QPushButton*   worldButton;

   // for number keys switching between joints and keeping all values up to date
   QSpinBox*      joint1Spin;
   QSpinBox*      joint2Spin;
   QSpinBox*      joint3Spin;
   QSpinBox*      brushSpinX;
   QSpinBox*      brushSpinY;
   QSpinBox*      brushSizeSpin;
   
   QWidget* initJointControls();
   QWidget* initWorldControls();
   QWidget* initBrushControls();

   QString  getControlStyle(QColor widget);

   QWidget* createJointControl(int id);
   QWidget* createWorldControl(int id);
   QWidget* createBrushControl();
};

#endif

