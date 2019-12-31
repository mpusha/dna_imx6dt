#include <QThread>
#include "srvBehav.h"

//-----------------------------------------------------------------------------
//--- Constructor
//-----------------------------------------------------------------------------
TServer_Dt::TServer_Dt(QString dName, int nPort)

{
// Stay for example. On windows platfor don't work becouse change LIB environment variable but windows
// work with PATCH variable
// QCoreApplication::addLibraryPath(QString("C:/DtXml"));
// QCoreApplication::addLibraryPath(QString("C:/Program1/QtSDK/Desktop/Qt/4.7.4/mingw/plugins"));
// qDebug()<<QCoreApplication::libraryPaths();
// qputenv("PATH", "C:\\DtXml\\"); // change PATH in issue proceses!!!
  QString compilationTime = QString("%1 %2").arg(__DATE__).arg(__TIME__);
  qDebug()<<compilationTime;

  PORT = nPort;
  NAME = dName;

  device=0;
  dtGpio=0;

  logSys=new TLogObject("system");
  logSys->setLogEnable(true); // enable system log into file only
  logSys->logMessage(tr("Start Dt-server. Ver. ")+APP_VERSION+" "+compilationTime);
  logSys->logMessage("Device name "+NAME+QString(" Port=%1").arg(PORT));


// read settings
  setingsFileName=QApplication::applicationDirPath()+"/setup.ini" ;
  IniFileRead();


  if((nPort<9000)||(nPort>9100))  //incorrect number of port
  {
    logSys->logMessage(tr("Unable to start the server. Network error. Incorrect port number."));
    QTimer::singleShot(1000,this,SLOT(close()));
    return;
  }
  /*device=new TDtBehav(nPort,dName,log_Size,log_Count);
  if(device->readGlobalErr()){ //connect to network error. Program abort
    logSys->logMessage(device->readGlobalErrText());
    QTimer::singleShot(1000,this,SLOT(close()));
    return;
  }*/
  setLogEnable();

//  connect(device,SIGNAL(commError()),this,SLOT(quiteProg()));
// signals for Log pages
  //connect(device->logSrv,SIGNAL(logScrMessage(QString)),pageSrvD,SLOT(addToLog(QString)));
  //connect(device->logNw,SIGNAL(logScrMessage(QString)),pageNWD,SLOT(addToLog(QString)));
  //connect(device->logDev,SIGNAL(logScrMessage(QString)),pageDTD,SLOT(addToLog(QString)));


  //device->logSrv->logMessage(tr("Start Dt-server. Ver. ")+APP_VERSION+" "+compilationTime);
  //device->start(QThread::NormalPriority);

  dtGpio=new TGpio();
  qDebug()<<"Startr device";
  if(dtGpio->pins_export()) device->logSrv->logMessage(tr("Can't export GPIO pins."));
  if(dtGpio->init_gpios())  device->logSrv->logMessage(tr("Can't init GPIO pins."));
  dtGpio->start(QThread::NormalPriority);
  qDebug()<<"Startr gpio";
}

//-----------------------------------------------------------------------------
//--- Destructor
//-----------------------------------------------------------------------------

TServer_Dt::~TServer_Dt()
{
  qDebug()<<"Server destructor start.";
  if(device) delete device;
  if(dtGpio) delete dtGpio;
  logSys->logMessage(tr("Stop Dt-server."));
  if(logSys) delete logSys;
  qDebug()<<"Server destructor finished.";
}

//-----------------------------------------------------------------------------
//--- Read ini settings
//-----------------------------------------------------------------------------
void TServer_Dt::IniFileRead(void)
{
  QSettings setings(setingsFileName,QSettings::IniFormat) ;
  logSrv=setings.value("logSystem",true).toBool();
  logNW=setings.value("logNW",true).toBool();
  logDT=setings.value("logDT",true).toBool();
  log_Size=setings.value("logFileSize",100000).toInt();
  log_Count=setings.value("logFileCount",10).toInt();
  loc=setings.value("Locale","EN").toString();
}


//-----------------------------------------------------------------------------
//--- Sets enable/disable log information in log object
//-----------------------------------------------------------------------------
void TServer_Dt::setLogEnable(void)
{
  device->logNw->setLogEnable(logNW);
  device->logSrv->setLogEnable(logSrv);
  device->logDev->setLogEnable(logDT);
}




