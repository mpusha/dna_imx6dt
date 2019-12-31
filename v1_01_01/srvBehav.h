#ifndef SRVBEHAV_H
#define SRVBEHAV_H

#include <QtGui>

#include <QMainWindow>
#include <QApplication>
#include <QDesktopWidget>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QIcon>
#include <QAction>
#include <QMessageBox>
#include <QEvent>
#include <QCloseEvent>
#include <QTimer>
#include <QLabel>
#include <QDebug>

#include "logobject.h"
#include "dtBehav.h"
#include "gpio.h"

class TServer_Dt : public QThread
{
    Q_OBJECT

public:
  TServer_Dt(QString dName = "A5UN01", int nPort = 9001);
  ~TServer_Dt();

  void IniFileRead(void);
  void setLogEnable(void);

protected:


private slots:        

// private variables
private:
  QMessageBox message;
  //QTimer dev_timer;
  TDtBehav *device;
  TLogObject *logSys;
  TGpio *dtGpio;
  bool logSrv,logNW,logDT;

  QString setingsFileName;
  int log_Size,log_Count; // size of log file and count zips archives get from settings
  QString loc; // localisation RU ENG Auto

  int PORT;     // connection port
  QString NAME; // request device name

};

#endif // srvBehav_H
