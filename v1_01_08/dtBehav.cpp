#include <QThread>
//#include <QApplication>
#include <QtCore>
#include <QDir>
//#include <algorithm>
#include "dtBehav.h"
#include "can_ids.h"

#include <unistd.h>

//-----------------------------------------------------------------------------
//--- Constructor
//-----------------------------------------------------------------------------
TDtBehav::TDtBehav() : m_nNextBlockSize(0)
{
  qDebug()<<"TDtBehav class device initialisation start.";
  syslog(LOG_INFO,"TDtBehav class device initialisation start.");
  Optics_uC=0;
  Temp_uC=0;
  Motor_uC=0;
  Display_uC=0;
  FX2=0;
  clientConnected=0;
  pClient=0;
  fatalCanErr=true;

  QString ap=QCoreApplication::applicationDirPath();
  iniFileRead((QString&)(ap+"/setup.ini"));
  qDebug()<<"Current dir"<<ap;
  QDir appDir;
  appDir.mkpath(ap+"/log/");

  videoDirName=ap+"/VIDEO";

  canDevName<<"None: "<<"Usb: "<<"Opt: "<<"Temp: "<<"Mot: "<<"Tft: ";
  abort = false;
  fMotVersion=6.1; // unknown version of motor controller
  sectors=1;ledsMap=3;count_block=1; tBlType="UNKNOWN"; // set individual parameters of device

  logSys=new TLogObject(ap+"/log/system");
  logSys->setLogEnable(true); // enable system log into file only
  logSys->logMessage(tr("Start Dt-server. Ver. ")+APP_VERSION+" "+QString("%1 %2").arg(__DATE__).arg(__TIME__));
  logSys->logMessage("Try start work with device name "+devName+QString(" Port=%1").arg(nPort));

  //... NetWork ...
  m_ptcpServer = new QTcpServer(this);
  if(!m_ptcpServer->listen(QHostAddress::Any, nPort))
  {
    m_ptcpServer->close();
    syslog( LOG_INFO, "Can't setup TCP connection. Network error! Program terminated.");
    qDebug()<<"Can't setup TCP connection. Network error! Program terminated.";
    logSys->logMessage("Can't setup TCP connection. Network error! Program terminated.");
    this->~TDtBehav();
    exit(0);
  }

  logSrv=new TLogObject(ap+"/log/server",logSize,logCount);
  logNw=new TLogObject(ap+"/log/network",logSize,logCount);
  logDev=new TLogObject(ap+"/log/device",logSize,logCount);
  logClientClr();

// set enable/disable logs
  logSrv->setLogEnable(logSrvBool);
  logNw->setLogEnable(logNwBool);
  logDev->setLogEnable(logDevBool);

  connect(m_ptcpServer, SIGNAL(newConnection()), this, SLOT(slotNewConnection()));
  connect(&tAlrm,SIGNAL(timeout()),this,SLOT(timeAlarm()));

  for(int i=0;i<ALLREQSTATES;i++) allStates[i]=READY;
  phase = GETINFO_STATE;
  nextPhase=READY;
  timerAlrm=false;
  testMeas=false;
  gVideo=0;
  BufVideo.resize(H_IMAGE*W_IMAGE*COUNT_SIMPLE_MEASURE*COUNT_CH);
  num_meas=0;
  type_dev=96;

// init GPIO pins
  devGpio=new TGpio();
  if(devGpio->pins_export()) logSrv->logMessage(tr("Can't export GPIO pins."));
  if(devGpio->init_gpios())   logSrv->logMessage(tr("Can't init GPIO pins."));
  devGpio->start(QThread::NormalPriority);
  connect(this,SIGNAL(setBackLight(char*)),devGpio,SLOT(slot_setBackLight(char*)));
  connect(this,SIGNAL(setInterfaces(bool)),devGpio,SLOT(slot_setInterfaces(bool)));
  qDebug()<<"Initialize libusb";
  if(libusb_init(NULL)>=0) { libUsbOpen=true; qDebug()<<"Ok";} else {libUsbOpen=false; qDebug()<<"Error";}
  FX2 = new TFx2Usb(vendorID,productID);
  if(!FX2){
    syslog( LOG_INFO, "Can't create USB object. USB error! Program terminated.");
    qDebug()<<"Can't create USB object. USB error! Program terminated.";
    logSys->logMessage("Can't create USB object. USB error! Program terminated.");
    this->~TDtBehav();
    exit(0);
  }
  scanner=new THidUsb();
  scanner->start(QThread::NormalPriority);
  //qDebug()<<scaner->serNum;
  simpleMode=true;
  powerMode=0; // wake up device
  if(!libUsbOpen){
    syslog( LOG_INFO, "Can't open libusb. USB error! Program work in simple mode (with CAN only).");
    qDebug()<<"Can't open libusb. USB error! Program work in simple mode (with CAN only).";
    logSys->logMessage("Can't open libusb. USB error! Program work in simple mode (with CAN only).");
  }
  Display_uC = new TCANSrvCh(UCDISP_CANID,barcodeDevice); // must be first
//  connect(Display_uC,SIGNAL(sendBarCode(QString)),this,SLOT(getBarCode(QString)));
  tAlrm.start(SAMPLE_DEVICE); //start timer for sample device
  syslog(LOG_INFO,"TDtBehav class device initialisation compleat.");
  logSys->logMessage("TDtBehav class device initialisation compleat.");
  connect(FX2,SIGNAL(sendUSBArrive(void)),scanner,SLOT(usbDeviceArrive(void)));
  connect(FX2,SIGNAL(sendUSBLeft(void)),scanner,SLOT(usbDeviceLeft(void)));
  //connect(scanner,SIGNAL(sendBarCode(QString)),Display_uC,SLOT(getBarCode(QString)));
  connect(scanner,SIGNAL(sendBarCode(QString)),this,SLOT(getBarCode(QString)));
  qDebug()<<"TDtBehav constructor compleat.";
}

//-----------------------------------------------------------------------------
//--- Destructor
//-----------------------------------------------------------------------------
TDtBehav::~TDtBehav()
{
  qDebug()<<"Device TDtBehav deinitialisation begin.";
  syslog( LOG_INFO, "Device TDtBehav deinitialisation begin.");
  logSys->logMessage("Device TDtBehav deinitialisation begin.");
  tAlrm.stop();


  if(m_ptcpServer) { m_ptcpServer->close(); delete m_ptcpServer; m_ptcpServer=0; }

  if(devGpio) { delete devGpio; devGpio=0; }
  //if(FX2)
   // if(FX2->isOpen()) FX2->Close();
  if(Optics_uC) { delete Optics_uC; Optics_uC=0;}
  if(Temp_uC) { delete Temp_uC; Temp_uC=0;}
  if(Motor_uC) { delete Motor_uC; Motor_uC=0;}
  if(Display_uC) { delete Display_uC; Display_uC=0;}
  if(scanner) {
    //scanner->setAbort(true); scanner->terminate(); scanner->wait(1500);
    delete scanner; scanner=0;
  }
  if(FX2) { delete FX2; FX2=0;}
  logSys->logMessage("Device TDtBehav deinitialisation compleat.");
  if(logSrv) { delete logSrv; logSrv=0; }
  if(logNw)  { delete logNw; logNw=0;   }
  if(logDev) { delete logDev; logDev=0; }
  if(logSys) { delete logSys; logSys=0; }
  //if(scanner)
  //  if(scanner->isOpen()) scanner->Close();

  BufVideo.clear();
  parameters.clear();
  map_dataFromDevice.clear();
  map_ReadSector.clear();
  map_SaveSector.clear();
  map_BarCode.clear();
  map_ConnectedStatus.clear();
  map_SaveParameters.clear();
  map_GetPicData.clear();
  map_inpGetPicData.clear();
  map_ExecCmd.clear();
  map_Measure.clear();
  map_Run.clear();
  map_InfoData.clear();
  map_InfoDevice.clear();

  if(libUsbOpen){
    libusb_exit(NULL);
  }
  syslog( LOG_INFO, "Device TDtBehav deinitialisation compleat.");
  qDebug()<<"Device TDtBehav deinitialisation compleat.";
}

// read settings
void TDtBehav::iniFileRead(QString& setingsFileName)
{
  QSettings setings(setingsFileName,QSettings::IniFormat);
  bool ok;
  logSrvBool=setings.value("logSystem",true).toBool();
  logNwBool=setings.value("logNW",true).toBool();
  logDevBool=setings.value("logDT",true).toBool();

  logSize=setings.value("logFileSize",100000).toInt(&ok); if(!ok) logSize=100000;
  logCount=setings.value("logFileCount",10).toInt(&ok); if(!ok) logCount=100000;

  barcodeDevice=setings.value("serialScanner","/dev/ttymxc2").toString();
  useSerialScanner=setings.value("useSerialScanner",false).toBool();

  vendorID=setings.value("vendorID","0x199a").toString().toUInt(&ok,16);
  if(!ok) vendorID=0x199a;
  productID=setings.value("productID","0x1964").toString().toUInt(&ok,16);
  if(!ok) productID=0x1964;

  devName=setings.value("deviceName","A5UN01").toString();
  nPort=setings.value("port",9002).toUInt(&ok); if(!ok) nPort=9002;
}

// remove catalogue
void clearVideoFiles(const QString &path)
{
  QDir dir(path);
  if(dir.exists()){
    QStringList fileList = dir.entryList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  |QDir::Files);
    for(int i = 0; i < fileList.count(); ++i){
      QFile::remove(path+"/"+fileList.at(i));
      qDebug()<<"remove "<<fileList.at(i);
    }
  }
  dir.mkdir(path);
}

// qDebug operator owerwrite for print states in debug mode
QDebug operator <<(QDebug dbg, const CPhase &t)
{
  dbg.nospace() <<"STATE=";
  switch(t){
  case READY: dbg.space()                  << "READY" ; break;
  case DEVICE_ERROR_STATE: dbg.space()     << "DEVICE_ERROR_STATE"; break;
  case GETINFO_STATE: dbg.space()          << "GETINFO_STATE" ; break;
  case OPENBLOCK_STATE: dbg.space()        << "OPENBLOCK_STATE" ; break;
  case CHECK_OPENBLOCK_STATE: dbg.space()  << "CHECK_OPENBLOCK_STATE" ; break;
  case CLOSEBLOCK_STATE: dbg.space()       << "CLOSEBLOCK_STATE" ; break;
  case CHECK_CLOSEBLOCK_STATE: dbg.space() << "CHECK_CLOSEBLOCK_STATE" ; break;
  case STARTRUN_STATE: dbg.space()         << "STARTRUN_STATE" ; break;
  case STARTMEASURE_STATE: dbg.space()     << "STARTMEASURE_STATE" ; break;
  case GETPARREQ_STATE: dbg.space()        << "GETPARREQ_STATE" ; break;
  case STOPRUN_STATE: dbg.space()          << "STOPRUN_STATE" ; break;
  case PAUSERUN_STATE: dbg.space()         << "PAUSERUN_STATE" ; break;
  case CONTRUN_STATE: dbg.space()          << "CONTRUN_STATE" ; break;
  case EXECCMD_STATE: dbg.space()          << "EXECCMD_STATE" ; break;
  case CHECKING_STATE: dbg.space()         << "CHECKING_STATE" ; break;
  case GETPICTURE_STATE: dbg.space()       << "GETPICTURE_STATE" ; break;
  case SAVEPARAMETERS_STATE: dbg.space()   << "SAVEPARAMETERS_STATE" ; break;
  case SAVESECTOR_STATE:  dbg.space()      << "SAVESECTOR_STATE" ; break;
  case READSECTOR_STATE:   dbg.space()     << "READSECTOR_STATE" ; break;
  case CHECK_ONMEAS_STATE:  dbg.space()    << "CHECKONMEAS_STATE" ; break;
  case ONMEAS_STATE:        dbg.space()    << "ONMEAS_STATE" ; break;
  case SETPOWER_STATE:        dbg.space()  << "SETPOWER_STATE" ; break;
  case CHANGEINT_STATE:       dbg.space()  << "CHANGEINT_STATE" ; break;
  case MEASHIGHTUBE_STATE:  dbg.space()    << "MEASHIGHTUBE_STATE" ; break;
  case CHECK_MEASHIGHTUBE_STATE: dbg.space()<< "CHECK_MEASHIGHTUBE_STATE" ; break;
  case HIGHTUBESAVE_STATE:dbg.space()      << "HIGHTUBESAVE_STATE" ; break;
  case GETAMPINSIMPLE_STATE: dbg.space()   << "GETAMPINSIMPLE_STATE"; break;
  default:  dbg.space()                    << "UNKNOWN_STATE" ; break;
  }
  return dbg.nospace() ;//<< endl;;
}
//-----------------------------------------------------------------------------
//--- State all error Status for all stadies
//-----------------------------------------------------------------------------
void TDtBehav::setErrorSt(short int st)
{
  stReadInfoErr=st;stCycleErr=st;stOpenErr=st;stCloseErr=st;stRunErr=st;
  stMeasErr=st; stStopErr=st; stPauseErr=st; stContErr=st; stExecErr=st; stCheckingErr=st;
  stGetPictureErr=st; stSaveParametersErr=st; stSaveSectorErr=st; stReadSectorErr=st;
  stMeasHighTubeErr=st; stMeasGetAmpProgErr=st;
}

//-----------------------------------------------------------------------------
//--- timer timeout event. On this event can get data from device
//-----------------------------------------------------------------------------
void TDtBehav::timeAlarm(void)
{
  if(timerAlrm) {
    allStates[GETPARREQ_STATE]=GETPARREQ_STATE;
    timerAlrm=false;
  }
  int la=FX2->leftArriveDev();

  if(la==0) return; // all USB on place
  if(la==1) { // USB left
    simpleMode=true;
    phase =  GETINFO_STATE;
    nextPhase=READY;
    return;
  }
  if(la==2) { // USB arrive
    simpleMode=false;
    for(int i=0;i<ALLREQSTATES;i++) allStates[i]=READY;
    phase =  GETINFO_STATE;
    nextPhase=READY;
  }
}

//-----------------------------------------------------------------------------
//--- Run process. Main cycle with state machine
//-----------------------------------------------------------------------------
void TDtBehav::run()
{
  CPhase deb=READY;//for debug only
  QEventLoop *loop ;
  loop= new QEventLoop();
  setErrorSt(NOTREADY_ERR);
  msleep(1000); // wait if devise turn on in this time
  while(!abort) { // run until destructor not set abort
    loop->processEvents();
    mutex.lock();
    if(phase==READY){
      for(int i=0;i<ALLREQSTATES;i++) { // read all statese request and run if state!= READY state. high priority has low index
        if(allStates[i]!=READY){
          phase=allStates[i];
          allStates[i]=READY;
          break;
        }
      }
    }
    mutex.unlock();
    msleep(10);
    loop->processEvents();

    if(deb!=phase)  { qDebug()<<phase;deb=phase;}
// State machine
    switch(phase) {
      case READY: {
      //  QApplication::processEvents();
        msleep(1);
        loop->processEvents();
        if(testMeas){  //get pic data after measure
          unsigned char dstate;
          FX2->VendRead(&dstate,1,0x23,0,0);
          if(dstate & 0x08){ //measure
            map_InfoData.insert(INFO_isMeasuring,"1");
            getPictureAfterMeas();
          }
        }
        break;
      }//end case READY:
// processing hardware errors in device (CAN, bulk)
      case DEVICE_ERROR_STATE: {
        logSrv->logMessage(tr("Found device error!"));
        QStringList err=devError.readDevTextErrorList();
        for(int i=0;i<err.size();i++) logSrv->logMessage(err.at(i));
        logSrv->logMessage(QString(tr("Count of errors: Opt=%1 Amp=%2 Mot=%3 Tft=%4 Usb=%5 Network=%6")).
                           arg(devError.readOptErr()).arg(devError.readAmpErr()).arg(devError.readMotErr()).
                           arg(devError.readTftErr()).arg(devError.readUsbErr()).arg(devError.readNWErr()));
        if(fatalCanErr) {
          phase = GETINFO_STATE;
         }
         else {
           phase = READY;
           nextPhase=READY;
         }
         break;
       }// end case DEVICE_ERROR_STATE

// connect to device and get info about device
      case GETINFO_STATE: {
        devError.clearDevError();
        logSrv->logMessage(tr("Information collection about the device is begin."));
        getInfoDevice();
        //setErrorSt(::CODEERR::NONE);
        timerAlrm=true;
        stReadInfoErr=devError.analyseError();
        if(stReadInfoErr){
          logSrv->logMessage(tr("Information collection error."));
          phase=DEVICE_ERROR_STATE;
        }
        else {
          logSrv->logMessage(tr("Information collection about the device is complete."));
          logSrv->logMessage(tr("Begin work with device."));
          phase = READY;
          nextPhase=READY;
        }
        break;
      }//end case GETINFO_STATE

// Sample device request from timer
      case GETPARREQ_STATE: {
        devError.clearDevError();
        getInfoData();
        timerAlrm=true; // can get data again
        allStates[GETPARREQ_STATE]=READY; // reset state
        stCycleErr=devError.analyseError(); // set error, if present
        phase = nextPhase; // set next phase from stack (for work with continue process in state machine for.ex open/close
        break;
      }//end case GETPARREQ_STATE

// --SM Open cover begin -----------------------------------------------------
      case OPENBLOCK_STATE: {
        logSrv->logMessage(tr("Open cover request."));
        openBlock();
        allStates[OPENBLOCK_STATE]=READY;
        phase=CHECK_OPENBLOCK_STATE;
        stOpenErr=devError.analyseError();
        if(stOpenErr){
          phase=DEVICE_ERROR_STATE;
          sendToClient(pClient, OPENBLOCK_REQUEST,stOpenErr);
          logSrv->logMessage(tr("Can't compleate open cover request."));
        }
        break;
      }// end case OPENBLOCK_STATE
      case CHECK_OPENBLOCK_STATE: {
        bool chk=checkMotorState();
        stOpenErr=devError.analyseError();
        if(stOpenErr){
          phase=DEVICE_ERROR_STATE;
          sendToClient(pClient, OPENBLOCK_REQUEST,stOpenErr);
          logSrv->logMessage(tr("Can't compleate open cover request."));
          break;
        }
        if(chk){
          phase=READY;
          nextPhase=CHECK_OPENBLOCK_STATE;
        }
        else{
          phase=READY;
          nextPhase=READY;
          logSrv->logMessage(tr("Cover is open."));
          sendToClient(pClient, OPENBLOCK_REQUEST,stOpenErr);
        }
        break;
      }// end case CHECK_OPENBLOCK_STATE
// --SM Open cover end

// --SM Close cover begin-----------------------------------------------------
      case CLOSEBLOCK_STATE: {
        logSrv->logMessage(tr("Close cover request."));
        map_BarCode.insert(barcode_name,"");
        closeBlock();
        allStates[CLOSEBLOCK_STATE]=READY;
        phase=CHECK_CLOSEBLOCK_STATE;
        stCloseErr=devError.analyseError();
        if(stCloseErr){
          phase=DEVICE_ERROR_STATE;
          sendToClient(pClient, CLOSEBLOCK_REQUEST,stCloseErr);
          logSrv->logMessage(tr("Can't compleate close cover request."));
        }
        break;
      }// end case CLOSEBLOCK_STATE
      case CHECK_CLOSEBLOCK_STATE: {
        bool chk=checkMotorState();
        stCloseErr=devError.analyseError();
        if(stCloseErr){
          phase=DEVICE_ERROR_STATE;
          sendToClient(pClient, CLOSEBLOCK_REQUEST,stCloseErr);
          logSrv->logMessage(tr("Can't compleate close cover request."));
          break;
        }
        if(chk){
          phase=READY;
          nextPhase=CHECK_CLOSEBLOCK_STATE;
        }
        else{
          phase=READY;
          nextPhase=READY;
          QString ans="";
          USBCy_RW("DRBC",ans,Display_uC); //read barcode from dispaly controller
          qDebug()<<"READ from RBC"<<ans;
          map_BarCode.insert(barcode_name,ans);
          Display_uC->setBarCodeStr("");
          logSrv->logMessage(tr("Cover is close. Read barcode")+ans);
          sendToClient(pClient, CLOSEBLOCK_REQUEST,stCloseErr);
        }
        break;
      }// end case CHECK_CLOSEBLOCK_STATE
// --SM Close cover end

// --SM get picture intime RUN begin-----------------------------------------------------
      case ONMEAS_STATE: {
        logSrv->logMessage(tr("Get picture on flat request."));
        BufVideo.fill(0);
        num_meas=0;
        allStates[ONMEAS_STATE]=READY;
        phase=CHECK_ONMEAS_STATE;
        break;
      }// end case ONMEAS_STATE

      case CHECK_ONMEAS_STATE: {
        unsigned char dstate;
        FX2->VendRead(&dstate,1,0x23,0,0);
        if(dstate & 0x08){
          // is measuring
          phase=READY;
          nextPhase=CHECK_ONMEAS_STATE;
        }
        else {
          // not measuring
          phase=READY;
          nextPhase=READY;
          logSrv->logMessage(tr("Get picture on flat request was finished."));
          QString answer;
          if(gVideo) {
            QByteArray buf;
            buf.resize(LEN);
            readFromUSB("FPIC","Ok",(unsigned char*) buf.data(),LEN); // free buffer with videodata
            buf.clear();
            USBCy_RW("FMODE 1",answer,Optics_uC);
            testMeas=true;
          }
          else {
            testMeas=false;
          }
        }
        break;
      }// end case CHECK_ONMEAS_STATE
// --SMget picture intime RUN end

// --SM run precheking begin-----------------------------------------------------
      case CHECKING_STATE: {
        logSrv->logMessage(tr("Check before RUN request."));
        checking();
        allStates[CHECKING_STATE]=READY;
        stCheckingErr=devError.analyseError();
        if(stCheckingErr){
          phase=DEVICE_ERROR_STATE;
          logSrv->logMessage(tr("Can't compleate preparation run operation."));
        }
        else {
          phase=READY;
          nextPhase=READY;
          logSrv->logMessage(tr("Preparation run operation compleate."));
        }
        sendToClient(pClient, PRERUN_REQUEST,stCheckingErr);
        break;
      }// end case CHECKING_STATE
// --SM run precheking end

// --SM Start Run begin-----------------------------------------------------
      case STARTRUN_STATE: {
        logSrv->logMessage(tr("Start RUN request."));
        startRun();
        allStates[STARTRUN_STATE]=READY;
        stRunErr=devError.analyseError();
        if(stRunErr){
          phase=DEVICE_ERROR_STATE;
           logSrv->logMessage(tr("Run can't begin."));
        }
        else {
          phase=READY;
          nextPhase=READY;
          gVideo=0;
          logSrv->logMessage(tr("Run is begin."));
        }
        sendToClient(pClient, RUN_REQUEST,stRunErr);
      break;
      }// end case STARTRUN_STATE
// --SM Start RUN end

// --SM Start Measure begin-----------------------------------------------------
      case STARTMEASURE_STATE: {
        //logSrv->logMessage(tr("Measure request."));
        startMeasure();
        allStates[STARTMEASURE_STATE]=READY;
        stMeasErr=devError.analyseError();
        if(stMeasErr){
          phase=DEVICE_ERROR_STATE;
          logSrv->logMessage(tr("Get measure error."));
        }
        else {
          phase=READY; nextPhase=READY;
          //logSrv->logMessage(tr("Measure completed."));
        }
        sendToClient(pClient, MEASURE_REQUEST,stMeasErr);
        break;
      }// end case STARTMEASURE_STATE
// --SM Start Measure end

// --SM Stop Run begin-----------------------------------------------------
      case STOPRUN_STATE: {
        logSrv->logMessage(tr("Stop RUN request."));
        stopRun();
        allStates[STOPRUN_STATE]=READY;
        stStopErr=devError.analyseError();
        if(stStopErr) {
          phase=DEVICE_ERROR_STATE;
          logSrv->logMessage(tr("Can't stop RUN."));
        }
        else{
          if(gVideo) {
            gVideo=0; phase=ONMEAS_STATE;
          }
          else
            phase=READY;
          nextPhase=READY;
          logSrv->logMessage(tr("RUN was stopped."));
        }
        sendToClient(pClient, STOP_REQUEST,stStopErr);
        break;
      }// end case STOPTRUN_STATE
// --SM Stop RUN end

// --SM Pause Run begin-----------------------------------------------------
      case PAUSERUN_STATE: {
        logSrv->logMessage(tr("Pause RUN request."));
        pauseRun();
        allStates[PAUSERUN_STATE]=READY;
        stPauseErr=devError.analyseError();
        if(stPauseErr) {
          phase=DEVICE_ERROR_STATE;
          logSrv->logMessage(tr("Can't pause RUN."));
        }
        else{
          phase=READY;
          nextPhase=READY;
          logSrv->logMessage(tr("RUN was paused."));
        }
        sendToClient(pClient, PAUSE_REQUEST,stPauseErr);
        break;
      }// end case PAUSERUN_STATE
// --SM Pause RUN end

// --SM Continue Run begin-----------------------------------------------------
      case CONTRUN_STATE: {
        logSrv->logMessage(tr("Continue RUN request."));
        contRun();
        allStates[CONTRUN_STATE]=READY;
        stContErr=devError.analyseError();
        if(stContErr) {
          phase=DEVICE_ERROR_STATE;
          logSrv->logMessage(tr("Can't continue RUN."));
        }
        else{
          phase=READY;
          nextPhase=READY;
          logSrv->logMessage(tr("RUN was continued."));
        }
        sendToClient(pClient,CONTINUE_REQUEST,stContErr);
        break;
      }// end case CONTRUN_STATE
// --SM Continue RUN end

// --SM Execute cmd begin-----------------------------------------------------
      case EXECCMD_STATE: {
        execCommand();
        allStates[EXECCMD_STATE]=READY;
        stExecErr=devError.analyseError();
        if(stExecErr) {
          phase=DEVICE_ERROR_STATE;
          logSrv->logMessage(tr("Can't execute external command."));
        }
        else{
          phase=READY;
          nextPhase=READY;
        }
        sendToClient(pClient, EXECCMD_REQUEST,stExecErr);
        break;
      }// end case EXECCMD_STATE
// --SM Execute cmd end

// --SM Get picture begin-----------------------------------------------------
      case GETPICTURE_STATE: {
        logSrv->logMessage(tr("Get picture request."));
        getPicture();
        allStates[GETPICTURE_STATE]=READY;
        stGetPictureErr=devError.analyseError();
        if(stGetPictureErr) {
          phase=DEVICE_ERROR_STATE;
          logSrv->logMessage(tr("Can't get picture."));
        }
        else{
          phase=READY;
          nextPhase=READY;
          logSrv->logMessage(tr("Get picture completed."));
        }
        sendToClient(pClient, GETPIC_REQUEST,stGetPictureErr);
        break;
      }// end case GETPICTURE_STATE
// --SM Get picture  end

// --SM Save parameters begin-----------------------------------------------------
      case SAVEPARAMETERS_STATE: {
        logSrv->logMessage(tr("Save device parameters request."));
        devError.clearDevError();
        saveParameters();
        allStates[SAVEPARAMETERS_STATE]=READY;
        stSaveParametersErr=devError.analyseError();
        if(stSaveParametersErr) {
          phase=DEVICE_ERROR_STATE;
          logSrv->logMessage(tr("Can't save parameters."));
        }
        else{
          phase=READY; nextPhase=READY;
          logSrv->logMessage(tr("Save parameters completed."));
        }
        sendToClient(pClient, SAVEPAR_REQUEST,stSaveParametersErr);
        break;
      }// end case SAVEPARAMETERS_STATE
// --SM Save parameters end

// --SM Save sector begin-----------------------------------------------------
      case SAVESECTOR_STATE: {
        logSrv->logMessage(tr("Save sector request."));
        devError.clearDevError();
        saveSector();
        allStates[SAVESECTOR_STATE]=READY;
        stSaveSectorErr=devError.analyseError();
        if(stSaveSectorErr) {
          phase=DEVICE_ERROR_STATE;
          logSrv->logMessage(tr("Can't save sector."));
        }
        else{
          phase=READY; nextPhase=READY;
          logSrv->logMessage(tr("Save sector completed."));
        }
        sendToClient(pClient, SECTORWRITE_REQUEST,stSaveSectorErr);
        break;
      }// end case SAVESECTOR_STATE
// --SM Save sector end

// --SM Read sector begin-----------------------------------------------------
      case READSECTOR_STATE: {
        logSrv->logMessage(tr("Read sector request."));
        devError.clearDevError();
        readSector();
        allStates[READSECTOR_STATE]=READY;
        stReadSectorErr=devError.analyseError();
        if(stReadSectorErr) {
          phase=DEVICE_ERROR_STATE;
          logSrv->logMessage(tr("Can't read sector."));
        }
        else{
          phase=READY; nextPhase=READY;
          logSrv->logMessage(tr("Read sector completed."));
        }
       sendToClient(pClient, SECTORREAD_REQUEST,stReadSectorErr);
        break;      
      }// end case READSECTOR_STATE
      case SETPOWER_STATE: {
        logSrv->logMessage(tr("Set device parameters."));
        if(powerMode==0){
          wakeUp();
          emit setBackLight((char*)"64");
          qDebug()<<"wakeup";
          sendToClient(pClient, CONTROL_REQUEST,0);

        }
        else if(powerMode==1) {
          standBy();
          emit setBackLight((char*)"16");
          sendToClient(pClient, CONTROL_REQUEST,0);
        }
        else if(powerMode==2) {
          QProcess sys;
          sys.start("shutdown -hP 0");
          qDebug()<<"Shutdown";
          sys.waitForFinished();
        }
        allStates[SETPOWER_STATE]=READY;
        phase=READY;
        break;
      }// end case SETPOWER_STATE
      case CHANGEINT_STATE: {
        if(simpleMode){ //simple
          logSrv->logMessage(tr("Switch device to simple mode."));
          emit setInterfaces(simpleMode);
          qDebug()<<"wakeup";
        }
        else {
         logSrv->logMessage(tr("Switch device to normal mode."));
          emit setInterfaces(simpleMode);
        }

        sendToClient(pClient, CONTROL_REQUEST,0);
        allStates[CHANGEINT_STATE]=READY;
        phase=READY;
        break;
      }// end case CHANGEINT_STATE
// --SM measure high of tube begin -----------------------------------------------------
      case MEASHIGHTUBE_STATE: {
        logSrv->logMessage(tr("Measure high of tube request."));
        devError.clearDevError();
        timeMeasHT=0;
        tubeHighSave=0;
        tubeHigh=0;
        measHighTubeStart();
        allStates[MEASHIGHTUBE_STATE]=READY;
        phase=CHECK_MEASHIGHTUBE_STATE;
        stMeasHighTubeErr=devError.analyseError();
        if(stMeasHighTubeErr){
          phase=DEVICE_ERROR_STATE;
          sendToClient(pClient, HTMEAS_REQUEST,stMeasHighTubeErr);
          logSrv->logMessage(tr("Can't compleate request measure high of tube."));
        }
        break;
      }// end case MEASHIGHTUBE_STATE
      case CHECK_MEASHIGHTUBE_STATE: {
        devError.clearDevError();
        int chk=checkMeasHighTubeState();
        stMeasHighTubeErr=devError.analyseError();
        if(stMeasHighTubeErr){
          phase=DEVICE_ERROR_STATE;
          sendToClient(pClient, HTMEAS_REQUEST,stMeasHighTubeErr);
          logSrv->logMessage(tr("Can't compleate request measure high of tube."));
          break;
        }
        msleep(100);
        timeMeasHT++;
        if((timeMeasHT>700)||(chk==-1)){ // ждём до ~70000 мсек
          devError.setDevError(MEASHIGHTUBE_ERROR);
          stMeasHighTubeErr=devError.analyseError();
          sendToClient(pClient, HTMEAS_REQUEST,stMeasHighTubeErr);
          logSrv->logMessage(tr("Measure high of tube error."));
          phase=DEVICE_ERROR_STATE;
          break;
        }
        if(chk!=0){
          phase=READY;
          nextPhase=CHECK_MEASHIGHTUBE_STATE;
        }
        else{
          phase=READY;
          nextPhase=READY;
          measHighTubeFinish();
          stMeasHighTubeErr=devError.analyseError();
          if(stMeasHighTubeErr){
            phase=DEVICE_ERROR_STATE;
            sendToClient(pClient, HTMEAS_REQUEST,stMeasHighTubeErr);
            logSrv->logMessage(tr("Can't compleate request measure high of tube."));
            break;
          }
          logSrv->logMessage(tr("Measure high of tube was finished."));
          sendToClient(pClient, HTMEAS_REQUEST,stMeasHighTubeErr);
        }
        break;
      }// end case CHECK_MEASHIGHTUBE_STATE
// --SM Measure high of tube finished
// --SM save measure high of tube begin -----------------------------------------------------
      case HIGHTUBESAVE_STATE: {
        logSrv->logMessage(tr("Measure save high of tube request."));
        devError.clearDevError();
        writeHighTubeValue(tubeHighSave);
        allStates[HIGHTUBESAVE_STATE]=READY;
        phase=READY;
        stMeasHighTubeErr=devError.analyseError();
        if(stMeasHighTubeErr){
          phase=DEVICE_ERROR_STATE;
          logSrv->logMessage(tr("Can't compleate request save measure high of tube."));
        }
        sendToClient(pClient, HIGHTUBE_SAVE,stMeasHighTubeErr);
        break;
      }// end case MEASHIGHTUBE_STATE
// --SM save measure high of tube begin -----------------------------------------------------
      case GETAMPINSIMPLE_STATE: {
        logSrv->logMessage(tr("Get amplification program request."));
        readAMPProg();
        allStates[GETAMPINSIMPLE_STATE]=READY;
        phase=READY;
        stMeasGetAmpProgErr=devError.analyseError();
       // map_ampProgForSimple.clear();
        if(stMeasGetAmpProgErr){
          phase=DEVICE_ERROR_STATE; 
          sendToClient(pClient, GETAMPINSIMPLE_REQUEST,stMeasGetAmpProgErr);
          logSrv->logMessage(tr("Can't compleate request get amplification program."));
          break;
        }
        //map_ampProgForSimple.insert(GETAMPINSIMPLE_DATA,ampProgList);
        sendToClient(pClient,GETAMPINSIMPLE_REQUEST,stMeasGetAmpProgErr);
        break;
      }// end case MEASHIGHTUBE_STATE
      default:
        break;
    } // End Switch main state machine--------------------------------------------------------------------------------------------------------------
  } // end while(!abort)
  logSys->logMessage("Run processing SM was finished.");
}


//================= NETWORK =========================================================================================================================
//-------------------------------------------------------------------------------------------------
//--- create new connection
//-------------------------------------------------------------------------------------------------
void TDtBehav::slotNewConnection()
{
    map_ConnectedStatus.clear();
    QTcpSocket* pClientSocket = m_ptcpServer->nextPendingConnection();
   // if(clientConnected){ // server has connection with client
   //    map_ConnectedStatus.insert(CONNECT_STATUS,"BUSY");
    //   sendToClient(pClientSocket,CONNECT_REQUEST,0);

    //   logNw->logMessage("The remote host can't connected. Server is busy of application with IP "+pClientSocket->peerAddress().toString());

    //}
    connect(pClientSocket, SIGNAL(disconnected()),
            pClientSocket, SLOT(deleteLater())
           );
    connect(pClientSocket, SIGNAL(readyRead()),
            this,          SLOT(slotReadClient())
           );
    connect(pClientSocket, SIGNAL(error(QAbstractSocket::SocketError)),
                    this,     SLOT(slotError(QAbstractSocket::SocketError)));



    logNw->logMessage(tr("Get request on connection from remote host wit IP: ")+pClientSocket->peerAddress().toString());
    if(clientConnected++) {
     // clientConnected=false;
      map_ConnectedStatus.insert(CONNECT_STATUS,"BUSY");
      logNw->logMessage(tr("The remote host can't connected. Server is busy of application with IP ")+pClientSocket->peerAddress().toString());
    }
    else {
      map_ConnectedStatus.insert(CONNECT_STATUS,"READY");
    }
    sendToClient(pClientSocket,CONNECT_REQUEST,0);

    //after this another client can't set connection
}

//-------------------------------------------------------------------------------------------------
//--- error connection. May be lost
//-------------------------------------------------------------------------------------------------
void TDtBehav::slotError(QAbstractSocket::SocketError err)
{
    QString strError =
                    (err == QAbstractSocket::HostNotFoundError ?
                     tr("The host was not found.") :
                     err == QAbstractSocket::RemoteHostClosedError ?
                     tr("The remote host is closed.") :
                     err == QAbstractSocket::ConnectionRefusedError ?
                     tr("The connection was refused.") :
                     QString(m_ptcpServer->errorString())
                    );
    logNw->logMessage(strError);
    clientConnected--;
    if(clientConnected<0) clientConnected=0;
}

//-------------------------------------------------------------------------------------------------
//--- read from TCP socket
//-------------------------------------------------------------------------------------------------
void TDtBehav::slotReadClient()
{
  QTcpSocket* pClientSocket = (QTcpSocket*)sender();
  QDataStream in(pClientSocket);
  in.setVersion(QDataStream::Qt_4_7);
  QString request;

  for(;;) {
    if(!m_nNextBlockSize) {
      if(pClientSocket->bytesAvailable() < sizeof(quint32)) break;
        in >> m_nNextBlockSize;
    }

    if(pClientSocket->bytesAvailable() < m_nNextBlockSize) break;
    m_nNextBlockSize = 0;

    in >> request;
    if(!request.size()) break;

    logNw->logMessage("--> "+request);

//... InfoDevice ......................................................
    if(request == INFO_DEVICE) {
      pClient = pClientSocket;
      sendToClient(pClientSocket,request,stReadInfoErr);
    }
//... InfoData ........................................................
    if(request == INFO_DATA) {
      sendToClient(pClientSocket,request,stCycleErr);
    }
//... OPEN_BLOCK ......................................................
    if(request == OPENBLOCK_REQUEST) {
      pClient = pClientSocket;
      allStates[OPENBLOCK_STATE]=OPENBLOCK_STATE;
    }
 //... CLOSE_BLOCK .....................................................
    if(request == CLOSEBLOCK_REQUEST) {
      pClient = pClientSocket;
      allStates[CLOSEBLOCK_STATE]=CLOSEBLOCK_STATE;
    }
//... Prepare Run .............................................................
    if(request == PRERUN_REQUEST) {
      pClient = pClientSocket;
      allStates[CHECKING_STATE]=CHECKING_STATE;
    }
//... Run .............................................................
    if(request == RUN_REQUEST) {
      map_Run.clear();
      in >> map_Run;
      pClient = pClientSocket;
      allStates[STARTRUN_STATE]=STARTRUN_STATE;
    }
//... Stop ............................................................
    if(request == STOP_REQUEST) {
      pClient = pClientSocket;
      allStates[STOPRUN_STATE]=STOPRUN_STATE;
    }
//... Pause ............................................................
    if(request == PAUSE_REQUEST) {
      pClient = pClientSocket;
      allStates[PAUSERUN_STATE]=PAUSERUN_STATE;
    }
//... Continue ............................................................
    if(request == CONTINUE_REQUEST) {
      pClient = pClientSocket;
      allStates[CONTRUN_STATE]=CONTRUN_STATE;
    }
// Execute external command
   if(request == EXECCMD_REQUEST) {
     map_ExecCmd.clear();
     in >> map_ExecCmd;
     pClient = pClientSocket;
     allStates[EXECCMD_STATE]=EXECCMD_STATE;
   }
 //... MEASURE .............................................................
   if(request == MEASURE_REQUEST) {
     in >> fnGet;
     in >> getActCh;
     qDebug()<<"MEasReq"<<fnGet<<getActCh;
     pClient = pClientSocket;
     allStates[STARTMEASURE_STATE]=STARTMEASURE_STATE;
   }
//... Get picture .............................................................
   if(request == GETPIC_REQUEST) {
     map_inpGetPicData.clear();
     in >> map_inpGetPicData;
     pClient = pClientSocket;
     allStates[GETPICTURE_STATE]=GETPICTURE_STATE;
   }
//... Save parameters..........................................................
   if(request == SAVEPAR_REQUEST) {
     map_SaveParameters.clear();
     in >> map_SaveParameters;
     pClient = pClientSocket;
     allStates[SAVEPARAMETERS_STATE]=SAVEPARAMETERS_STATE;
   }
//... Save sector ..............................................................
   if(request == SECTORWRITE_REQUEST) {
     map_SaveSector.clear();
     in >> map_SaveSector;
     pClient = pClientSocket;
     allStates[SAVESECTOR_STATE]=SAVESECTOR_STATE;
   }
//... Save sector ..............................................................
   if(request == SECTORREAD_REQUEST) {
     map_ReadSector.clear();
     in >> map_ReadSector;
     pClient = pClientSocket;
     allStates[READSECTOR_STATE]=READSECTOR_STATE;
   }
   if(request == CONTROL_REQUEST){
     map_dataToDevice.clear();
     in >>map_dataToDevice;

     QString str=map_dataToDevice.take(CTRL_STRING);
     qDebug()<<"map"<<map_dataToDevice<<str;
     pClient = pClientSocket;

     if(str==PWR_WAKEUP){
         qDebug()<<"WAKEUP";
       allStates[SETPOWER_STATE]=SETPOWER_STATE;
       powerMode=0;
     }
     if(str==PWR_STANDBY){
       allStates[SETPOWER_STATE]=SETPOWER_STATE;
       qDebug()<<"STANDBY";
       powerMode=1;
     }
     if(str==PWR_SHUTDOWN){
       qDebug()<<"SHUTDOWN";
       allStates[SETPOWER_STATE]=SETPOWER_STATE;
       powerMode=2;
     }
     if(str==INTERFACE_USB){
       qDebug()<<"INTERFACE_USB";
       allStates[CHANGEINT_STATE]=CHANGEINT_STATE;
       simpleMode=true;
     }
     if(str==INTERFACE_ETH){
       qDebug()<<"INTERFACE_ETH";
       allStates[CHANGEINT_STATE]=CHANGEINT_STATE;
       simpleMode=false;
     }
   }
//... Measure high of tube
   if(request==HTMEAS_REQUEST) {
     pClient = pClientSocket;
     allStates[MEASHIGHTUBE_STATE]=MEASHIGHTUBE_STATE;
   }
//... Measure high of tube
   if(request==HIGHTUBE_SAVE) {
     in >> tubeHighSave;
     pClient = pClientSocket;
     allStates[HIGHTUBESAVE_STATE]=HIGHTUBESAVE_STATE;
   }
   //... Measure high of tube
   if(request==GETAMPINSIMPLE_REQUEST) {
     pClient = pClientSocket;
     allStates[GETAMPINSIMPLE_STATE]=GETAMPINSIMPLE_STATE;
   }
 } // end for
}

void TDtBehav::getFromclients(QTcpSocket* pSocket, QString request, int st)
{
  sendToClient(pSocket,request,(short int)st);
}

//-------------------------------------------------------------------------------------------------
//--- answer to client
//-------------------------------------------------------------------------------------------------
void TDtBehav::sendToClient(QTcpSocket *pSocket, QString request,short int st)
{
  QByteArray  arrBlock;
  QDataStream out(&arrBlock, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_4_7);
  logNw->logMessage("<-- "+request);
  out << quint32(0) << request<<st;
  // error code short int most 4 bits error in controller Opt Temp Mot Tft  others bits is code of error in code_errors.h

//---------------------------------------------------
//_1. INFO_DATA
  if(request == INFO_DATA) { map_InfoData.insert(INFO_logData,logClient); out << map_InfoData; logClientClr(); }

//_2. INFO_DEVICE
  if(request == INFO_DEVICE) out << map_InfoDevice;

//_3. MEASURE_REQUEST
  if(request == MEASURE_REQUEST){ out << fnGet << map_Measure; }

//_4. EXECCMD_REQUEST
  if(request ==EXECCMD_REQUEST) out << map_ExecCmd;

//_5. GETPIC_REQUEST
  if(request ==GETPIC_REQUEST) out << map_GetPicData;

//_6. CONNECT_REQUEST
  if(request ==CONNECT_REQUEST) out << map_ConnectedStatus;

//_7 CLOSEBLOCK_REQUEST
  if(request ==CLOSEBLOCK_REQUEST) out << map_BarCode;

//_8 SECTORREAD_REQUEST
  if(request==SAVEPAR_REQUEST) out<<map_SaveParameters;

//_9 SECTORREAD_REQUEST
if(request ==SECTORREAD_REQUEST) out << map_ReadSector;

//_10 DEVICE_REQUEST
if(request ==DEVICE_REQUEST) out << map_dataFromDevice;

//_11 HTMEAS_REQUEST
if(request == HTMEAS_REQUEST) out<<tubeHigh;

//_12 GETAMPINSIMPLE_REQUEST
if(request == GETAMPINSIMPLE_REQUEST) out<<map_ampProgForSimple;

//----------------------------------------------------

  out.device()->seek(0);
  out << quint32(arrBlock.size() - sizeof(quint32)); //place length of block for send

  pSocket->write(arrBlock);
}

//================= CAN+USB function ================================================================================================================
//-----------------------------------------------------------------------------
//--- Read/write from CAN
//-----------------------------------------------------------------------------
bool TDtBehav::USBCy_RW(QString cmd, QString &answer, TCANChannels *uC)
{
  unsigned int i;
  static  char buf[64];
  QByteArray str;
  bool sts = false;
  int res=0;

  // -1 usb isn't open
  // -2 uC isn't open
  // -3 timeout
  int n=REPCOM;
  while((!sts)&&((n--)!=0)){//qDebug()<<uC->can_id <<cmd<<sts<<n;
    if(uC->Open()) {
      for(i=0; i<sizeof(buf); i++) buf[i] = '\0';
      str = cmd.toLatin1();
      if(!uC->Cmd((char*)str.constData(), buf, sizeof(buf))) res = -3;   // error -> timeout,communication_error
      else {
        answer = QString::fromLatin1((char*)buf,-1);                                // ok
        if(answer.count() >= 3 && answer[2] == '>') { answer.remove(0,3); sts = true; if(answer[0]=='?') {sts=false; res=-4; break;}}            //
        answer = answer.simplified();                            // whitespace removed: '\t','\n','\v','\f','\r',and ' '
      }
    }
    else res = -2;              // uC isn't open

    if(!uC->Close())            // Close uC
    {
      uC->Reset();
      uC->Close();
    }
  }
  str.clear();
  if(!sts)
  {
    switch(res) {
      default:    answer = "???"; break;
      case -1:    answer = "USB isn't open"; break;
      case -2:    answer = "uC isn't open"; break;
      case -3:    answer = "timeout"; break;
    }
    switch(uC->can_id){
    case UCOPTICS_CANID:
      if(res==-1) devError.setDevError(USB_CONNECTION_ERROR);
      else if(res==-2) devError.setDevError(OPEN_OPT_ERROR);
      else if(res==-3) devError.setDevError(TOUT_OPT_ERROR);
      else devError.setDevError(RW_OPT_ERROR);
    break;
    case UCTEMP_CANID:
      if(res==-1) devError.setDevError(USB_CONNECTION_ERROR);
      else if(res==-2) devError.setDevError(OPEN_AMP_ERROR);
      else if(res==-3) devError.setDevError(TOUT_AMP_ERROR);
      else devError.setDevError(RW_AMP_ERROR);
    break;
    case UCMOTOR_CANID:
      if(res==-1) devError.setDevError(USB_CONNECTION_ERROR);
      else if(res==-2) devError.setDevError(OPEN_MOT_ERROR);
      else if(res==-3) devError.setDevError(TOUT_MOT_ERROR);
      else devError.setDevError(RW_MOT_ERROR);
    break;
    case UCDISP_CANID:
      if(res==-1) devError.setDevError(USB_CONNECTION_ERROR);
      else if(res==-2) devError.setDevError(OPEN_TFT_ERROR);
      else if(res==-3) devError.setDevError(TOUT_TFT_ERROR);
      else devError.setDevError(RW_TFT_ERROR);
    break;
    default: break;
    }
    logDev->logMessage("Error! "+canDevName.at(uC->can_id)+cmd+" -> " + answer);
    logClientAddData("Error! "+canDevName.at(uC->can_id)+cmd+" -> " + answer);

  }
  else {
    logDev->logMessage(canDevName.at(uC->can_id)+cmd+" -> " + answer);
    logClientAddData(canDevName.at(uC->can_id)+cmd+" -> " + answer);
  }
  return(sts);
}
bool TDtBehav::USBCy_RW(QString cmd, QString &answer, TCANSrvCh *uC)
{
  unsigned int i;
  static  char buf[64];
  QByteArray str;
  bool sts = false;
  int res=0;

  if(uC->Open()) {
    for(i=0; i<sizeof(buf); i++) buf[i] = '\0';
    str = cmd.toLatin1();
    uC->Cmd((char*)str.constData(), buf, sizeof(buf));
    answer = QString::fromLatin1((char*)buf,-1);                                // ok
    if(answer.count() >= 3 && answer[2] == '>') { answer.remove(0,3); sts = true; if(answer[0]=='?') {sts=false; res=-4;}}
    answer = answer.simplified();                            // whitespace removed: '\t','\n','\v','\f','\r',and ' '
  }
  else res = -2;              // uC isn't open

  uC->Close();            // Close uC
  str.clear();
  if(!sts)
  {
    switch(res) {
      default:    answer = "???"; break;
      case -1:    answer = "USB isn't open"; break;
      case -2:    answer = "uC isn't open"; break;
      case -3:    answer = "timeout"; break;
      case -4:    answer = "unknown command"; break;
    }
    switch(uC->can_id){
    case UCDISP_CANID:
      if(res==-1) devError.setDevError(USB_CONNECTION_ERROR);
      else if(res==-2) devError.setDevError(OPEN_TFT_ERROR);
      else if(res==-3) devError.setDevError(TOUT_TFT_ERROR);
      else devError.setDevError(RW_TFT_ERROR);
    break;
    default: break;
    }
    logDev->logMessage("Error! "+canDevName.at(uC->can_id)+cmd+" -> " + answer);
    logClientAddData("Error! "+canDevName.at(uC->can_id)+cmd+" -> " + answer);
  }
  else {
    logDev->logMessage(canDevName.at(uC->can_id)+cmd+" -> " + answer);
    logClientAddData(canDevName.at(uC->can_id)+cmd+" -> " + answer);
  }
  return(sts);
}

//-----------------------------------------------------------------------------
//--- Read from USB one sector
//-----------------------------------------------------------------------------
bool TDtBehav::readFromUSB512(QString cmd,QString templ,unsigned char* buf)
{
  if(simpleMode) {
    devError.setDevError(USB_CONNECTION_ERROR);
    return false;
  }
  QString answer;
  answer.resize(128);
  int okRead=REPCOM,okCmd=REPCOM,errRead=1,errCmd=1;

  while(okRead--)
  {
    while(okCmd--){
      if(USBCy_RW(cmd,answer,Optics_uC)) {
       if(templ.size()){
         if(answer.endsWith(templ,Qt::CaseInsensitive)) {errCmd=0; break;} // cmd ok
       }
       else {
         errCmd=0; break;
       } // cmd may be ok
      }
      USBCy_RW("CEV 2",answer,Optics_uC);
      msleep(100);
    }

    if(errCmd) {
      devError.setDevError(CMD_USBBULK_ERROR); logDev->logMessage(tr("Error answer on command: ")+cmd);
      logClientAddData(tr("Error answer on command: ")+cmd);
      return(false);
    }

    if(FX2->BulkRead((unsigned char *)buf,BYTES_IN_SECTOR)) {errRead=0; break;} // read ok

    FX2->BulkClearRead();
    msleep(100);
  }
  answer.clear();
  if(errRead) {
    devError.setDevError(RW_USBBULK_ERROR); logDev->logMessage(tr("USB bulk read error on command: ")+cmd);
    logClientAddData(tr("USB bulk read error on command: ")+cmd);
    return(false);
  }//can't read from bulk

  return(true);
}

//-----------------------------------------------------------------------------
//--- Read from USB n bytes
//-----------------------------------------------------------------------------
bool TDtBehav::readFromUSB(QString cmd,QString templ,unsigned char* buf,int len)
{
  if(simpleMode) {
    devError.setDevError(USB_CONNECTION_ERROR);
    return false;
  }
  QString answer;
  answer.resize(128);
  int okRead=REPCOM,okCmd=REPCOM,errRead=1,errCmd=1;

  while(okRead--)
  {
    while(okCmd--){
      if(USBCy_RW(cmd,answer,Optics_uC)) {
        if(templ.size()){
         if(answer.endsWith(templ,Qt::CaseInsensitive)) {errCmd=0; break;} // cmd ok
        }
        else {
          errCmd=0; break;
        } // cmd may be ok
      }
      USBCy_RW("CEV 2",answer,Optics_uC);
      msleep(100);
    }

    if(errCmd)  {
      devError.setDevError(CMD_USBBULK_ERROR); logDev->logMessage(tr("Error answer on command: ")+cmd);
      logClientAddData(tr("Error answer on command: ")+cmd);
      return(false);
    }

    if(FX2->BulkRead((unsigned char *)buf,len)) {errRead=0; break;} // read ok

    FX2->BulkClearRead();
    msleep(100);
  }
  answer.clear();
  if(errRead) {
    devError.setDevError(RW_USBBULK_ERROR); logDev->logMessage(tr("USB bulk read error on command: ")+cmd);
    logClientAddData(tr("USB bulk read error on command: ")+cmd);
    return(false);
  }//can't read from bulk

  return(true);
}

//-----------------------------------------------------------------------------
//--- Write into USB
//-----------------------------------------------------------------------------
bool TDtBehav::writeIntoUSB512(QString cmd,QString templ,unsigned char* buf)
{
  if(simpleMode) {
    devError.setDevError(USB_CONNECTION_ERROR);
    return false;
  }
  QString answer;answer.resize(128);

  int okWrite=REPCOM,okCmd=REPCOM,errWrite=1,errCmd=1;

  while(okWrite--)
  {
    if(!FX2->BulkWrite((unsigned char *)buf,BYTES_IN_SECTOR)) { // write error
      FX2->BulkClearWrite();
      msleep(100);
    }
    else
      errWrite=0;
    if(!errWrite) {
      while(okCmd--){
        if(USBCy_RW(cmd,answer,Optics_uC)) {
          if(templ.size()) {
            if(answer.endsWith(templ,Qt::CaseInsensitive)) { errCmd=0; okWrite=0; break; } // cmd ok
          }
          else {
            errCmd=0; okWrite=0; break;
          } // cmd may be ok
        }
        msleep(100);
      }
    }
    FX2->BulkClearWrite();
    msleep(100);
  }
  if(errCmd)  {
    devError.setDevError(CMD_USBBULK_ERROR); logDev->logMessage(tr("Error answer on command: ")+cmd);
    logClientAddData(tr("Error answer on command: ")+cmd);
    return(false);
  }
  if(errWrite) {
    devError.setDevError(RW_USBBULK_ERROR); logDev->logMessage(tr("USB bulk write error on command: ")+cmd);
    logClientAddData(tr("USB bulk write error on command: ")+cmd);
    return(false);
  }//can't write into bulk
  answer.clear();
  if(errCmd) return(false);
  return(true);
}

//================= DEVICE PART ================================================================================================================
//-----------------------------------------------------------------------------
//--- Initialise device
//-----------------------------------------------------------------------------
void TDtBehav::initialDevice(void)
{
  qDebug()<<"Initial device";
  if(fatalCanErr) {
    QProcess sys;
    sys.start("ifconfig can0 down");
    qDebug()<<"can0 down";
    sys.waitForFinished();
    sys.start("ifconfig can0 up");
    qDebug()<<"can0 up";
    sys.waitForFinished();
  }
  if(Optics_uC) { delete Optics_uC; Optics_uC=0;}
  if(Temp_uC) { delete Temp_uC; Temp_uC=0;}
  if(Motor_uC) { delete Motor_uC; Motor_uC=0;}

  logSys->logMessage(tr("Assign serial number from setup.ini")+devName.toUpper());
  qDebug()<<"Assign serial number from setup.ini"<<devName.toUpper();
  int num;
  if( FX2->Open()) {
    if(FX2->checkSpeed()) {
      num = FX2->serNum.mid(1,1).toInt();
      logSys->logMessage(tr("Found devices with serial number ")+FX2->serNum);
      qDebug()<<"Found devices with serial number "<<FX2->serNum;
      if (QString::compare(FX2->serNum.toUpper(),devName.toUpper(), Qt::CaseInsensitive)==0)  {
        logSys->logMessage(tr("Connect to devices ")+FX2->serNum.toUpper());
        qDebug()<<"Connect to devices "<<FX2->serNum.toUpper();
        switch(num) {
          case 5:   sectors=1; pumps=96; tubes=96;  count_block=1; logSrv->logMessage(tr("Device observe as DT96")); break;
          case 6:   sectors=4; pumps=96; tubes=384; count_block=2; logSrv->logMessage(tr("Device observe as DT384"));break;
          case 7:   sectors=1; pumps=48; tubes=48;  count_block=1; logSrv->logMessage(tr("Device observe as DT48"));break;
          case 8:   sectors=2; pumps=96; tubes=192; count_block=1; logSrv->logMessage(tr("Device observe as DT192"));break;
          default:  sectors=1; pumps=96; tubes=96;  count_block=1; logSrv->logMessage(tr("UNKNOWN device")) ;break;
        }
        type_dev=tubes;
      }
      else{
        logSys->logMessage(tr("Can't found device with serial number ")+devName.toUpper());
        qDebug()<<"Can't found device with serial number"<<devName.toUpper();
      }
      simpleMode=false;
      parameters.resize(BYTES_IN_SECTOR*sectors);
    }
    else{
      logSys->logMessage(tr("The device has been connected not to high speed USB port. Connection was terminated."));
      qDebug()<<"The device has been connected not to high speed USB port. Connection was terminated.";
      FX2->Close();
    }
  }
  else { // no USB connection found
    logSys->logMessage(tr("Can't open USB connection."));
    syslog( LOG_INFO, "Can't open USB connection with device vID=%x pID=%x",vendorID,productID);
    qDebug()<<"Can't open USB connection with device pID vID"<<hex<<vendorID<<productID;
    simpleMode=true;
  }
  if(simpleMode) {
    qDebug()<<"Device work in simple mode.";
    logSys->logMessage(tr("Device is set in simple mode of work."));
  }else {
    qDebug()<<"Device work in normal mode.";
     logSys->logMessage(tr("Device is set in normal mode of work."));
  }

  //Display_uC = new TCANSrvCh(UCDISP_CANID,barcodeDevice); // must be first

  Optics_uC = new TCANChannels(UCOPTICS_CANID);
  Temp_uC = new TCANChannels(UCTEMP_CANID);
  Motor_uC = new TCANChannels(UCMOTOR_CANID);
}

//-------------------------------------------------------------------------------------------------
//--- Get information about device
//-------------------------------------------------------------------------------------------------
void TDtBehav::getInfoDevice()
{
  int i,j,k;

  QString text, answer;

  initialDevice();

  fatalCanErr=false;
// 1. INFODEV_version
  if(USBCy_RW("FVER",answer,Optics_uC)) text += answer; else fatalCanErr=true;
  if(USBCy_RW("FVER",answer,Temp_uC)) text += "\r\n" + answer; else fatalCanErr=true;
  if(USBCy_RW("FVER",answer,Motor_uC)) text += "\r\n" + answer; else fatalCanErr=true;
  if(USBCy_RW("FVER",answer,Display_uC)) text += "\r\n" + answer;
  map_InfoDevice.insert(INFODEV_version, text);

// 2. INFODEV_serName

  text = FX2->serNum; //device name from USB
  qDebug()<<"FX2 ser number"<<text;
  if(!text.size())
    if(!USBCy_RW("FSN",text,Optics_uC)) text="A5UN01";
  qDebug()<<"Ser number"<<text;
  map_InfoDevice.insert(INFODEV_serName, text); // get device name from CAN
  if(fatalCanErr) return;

  QByteArray buf;
  buf.resize(BYTES_IN_SECTOR);
  buf.fill(0,BYTES_IN_SECTOR);
// 3. INFODEV_devMask get present channel map in device
  int ch=0;
  ledsMap=0;
  for(int i=0;i<MAX_OPTCHAN;i++){
    QString request=QString("FLCFG %1").arg(i);
    if(USBCy_RW(request,answer,Optics_uC)){
      QTextStream(&answer) >> ch;
      if(ch>0) ledsMap|=(1<<i);
    }
  }
  map_InfoDevice.insert(INFODEV_devMask, QString("0x%1").arg(ledsMap,0,16));

// 4. INFODEV_thermoBlock get thermoblock type
  QString a1,a2,a3;
  tBlType="UNKNOWN";
  if(USBCy_RW("DLRS 9",answer,Temp_uC)){
    QTextStream(&answer)>>a1>>a2>>a3;
    if(a1.contains("R:TY",Qt::CaseInsensitive)){
      QStringList tbType;
      tbType.clear();
      tbType<<"B96A"<<"B96B"<<"B96C"<<"B384"<<"BLK_W384"<<"B48A"<<"B48B"<<"B192"<<"B322";
      if(tbType.contains(a3,Qt::CaseInsensitive)) tBlType=a3;
      else{
        tbType.clear();
        tbType<<"A5X112"<<"A5X206"<<"A5Y201"<<"A5Y809"<<"A5Z202"<<"A5Z806"<<"A5Z910"<<"A5AN14"<<"A5AN15"<<"A5BN09"<<"A5BD06"<<"A5C113"<<"A5C302"<<"A5CD11"<<"A5CD26";
        if(tbType.contains(devName,Qt::CaseInsensitive)) tBlType="B96B";
        else {
          tbType.clear();
          tbType<<"A5X111"<<"A5X207"<<"A5X409"<<"A5Z008"<<"A5A603"<<"A5AN09"<<"A5B730"<<"A5BN01"<<"A5C510"<<"A5CD15"<<"A5CD16";
          if(tbType.contains(devName,Qt::CaseInsensitive)) tBlType="B96C";
        }
      }
    }
  }
  map_InfoDevice.insert(INFODEV_thermoBlock,tBlType);

// 5. INFODEV_parameters Device Parameters: geometry, param of optical channels
  parameters.fill(0,BYTES_IN_SECTOR*sectors);
  active_ch=0;
  if(readFromUSB512("FPGET 0","Ok",(unsigned char*)parameters.data())) {
    Save_Par *param=(Save_Par*)parameters.data();
    for(int i=0;i<MAX_OPTCHAN;i++){ // get expositions for version of optic FW<3.04
      expVal0[i]=param->SavePar_96.optics_ch[i].exp[0];
      expVal1[i]=param->SavePar_96.optics_ch[i].exp[1];
      qDebug()<<"Exp"<<expVal0[i]<<expVal1[i];
      ledCurrent[i]=(short int)param->SavePar_96.optics_ch[i].light;
      filterNumber[i]=(short int)param->SavePar_96.optics_ch[i].filter;
      ledNumber[i]=(short int)param->SavePar_96.optics_ch[i].led ;
      if((int)(param->SavePar_96.optics_ch[i].nexp)>0) active_ch|=(1<<(i*4)); // set mask on active ch
    }
  }
  text="";
  for(int i=0;i<sectors;i++){
    readFromUSB512(QString("FPGET %1").arg(i),"Ok",(unsigned char*)(parameters.data()+BYTES_IN_SECTOR*i));
  }
  text = parameters.toBase64();
  map_InfoDevice.insert(INFODEV_parameters, text);

// 6. INFODEV_SpectralCoeff SpectralCoeff
  readFromUSB512("CRDS 0","0",(unsigned char*)buf.data());
  text = buf.toBase64();
  map_InfoDevice.insert(INFODEV_SpectralCoeff, text);

// 7. INFODEV_OpticalCoeffOpticalCoeff
  QByteArray buf_sum;
  QString cmd = "CRDS ";
  QString tmp = "";
  for(i=0; i<COUNT_CH; i++) {
    for(k=0; k<count_block; k++) {
      j = (i+1)*10+k;
      text = cmd + QString::number(j);
      readFromUSB512(text,"0",(unsigned char*)buf.data());
      buf_sum.append(buf);
    }
  }
  tmp = buf_sum.toBase64();
  map_InfoDevice.insert(INFODEV_OpticalCoeff, tmp);
  buf_sum.clear();

// 8.INFODEV_UnequalCoeff UnequalCoeff
  cmd = "CRDS ";
  j = (COUNT_CH+1)*10;
  text = cmd + QString::number(j);
  readFromUSB512(text,"0",(unsigned char*)buf.data());
  tmp = buf.toBase64();
  map_InfoDevice.insert(INFODEV_UnequalCoeff, tmp);

  buf.clear();
  USBCy_RW("DSAV 1",answer,Display_uC);
}

//-------------------------------------------------------------------------------------------------
//--- Get information about current state of device on timer request  after SAMPLE_DEVICE ms
//-------------------------------------------------------------------------------------------------
void TDtBehav::getInfoData()
{
  QString answer = "";
  answer.resize(255);
  unsigned char dstate;
  int state_dev=0;
  if(logClient.size()>10000) logClientClr(); // client not read data alarm clear log
  if(USBCy_RW("XGS",answer,Temp_uC)) {
    map_InfoData.insert(INFO_status,answer);
    //sscanf(answer.toStdString().c_str(),"%d", &state_dev);
    QTextStream(&answer) >> state_dev;
  }

  if(USBCy_RW("XGT",answer,Temp_uC)) {
    map_InfoData.insert(INFO_Temperature,answer);
  }
  if(USBCy_RW("FN",answer,Optics_uC)) {
    map_InfoData.insert(INFO_fn,answer);
    bool ok;
    fn=answer.toInt(&ok); if(!ok) fn=-1;
  }
  map_InfoData.insert(INFO_fmode,QString("%1").arg(gVideo));
  if(state_dev) {
    if(USBCy_RW("RDEV",answer,Temp_uC)) {
      map_InfoData.insert(INFO_rdev,answer);
    }
    dstate=0;
    if(!simpleMode) {
      FX2->VendRead(&dstate,1,0x23,0,0);
    }
    else {
      dstate=Display_uC->getDState();
      //if(USBCy_RW("XID",answer,Temp_uC)){
      //  int p1,p2,p3;
      //  QTextStream(&answer) >> p1 >> p2 >> p3;
      //  if(p3==1) dstate=0x8;
     // }
    }

    if(dstate & 0x08)
      map_InfoData.insert(INFO_isMeasuring,"1");
    else
      map_InfoData.insert(INFO_isMeasuring,"0");
    if(USBCy_RW("TI",answer,Temp_uC)) {
      map_InfoData.insert(INFO_time,answer);
    }
    if(USBCy_RW("XID",answer,Temp_uC)) {
      map_InfoData.insert(INFO_Levels,answer);
    }
  }
/*  else {
    if(USBCy_RW("DRAV",answer,Display_uC)) {
      map_InfoData.insert(INFO_pressRunButton,answer);
      if(answer.toInt()){ // press Run Button
        map_dataFromDevice.insert(PRESS_BTN_RUN,"1");
        sendToClient(pClient,DEVICE_REQUEST,0);
        if(!clientConnected){USBCy_RW("DPA Application program don't run!",answer,Display_uC);}
      }
    }
  }*/
}

void TDtBehav::getBarCode(QString bc)
{
  qDebug()<<"BAR code read"<<bc;
  Display_uC->setBarCodeStr(bc);
  map_dataFromDevice.insert(barcode_name,bc);
  if(pClient) {
    sendToClient(pClient,DEVICE_REQUEST,0);
  }
}

//-----------------------------------------------------------------------------
//--- Start open block procedure OPENBLOCK_STATE
//-----------------------------------------------------------------------------
void TDtBehav::openBlock(void)
{
  devError.clearDevError();
  QString answer;
  int i;
  if(USBCy_RW("RSTS",answer,Temp_uC)) { // check RUN program on t-controller
    QTextStream(&answer) >> i;
    if(i==1) { devError.setDevError(OPENINRUN_ERROR); return ; } // try open in run time
    USBCy_RW("HOPEN",answer,Motor_uC);
  }
}

//-----------------------------------------------------------------------------
//--- Check motor when block open CHECK_OPENBLOCK_STATE
//-----------------------------------------------------------------------------
bool TDtBehav::checkMotorState(void)
{
  devError.clearDevError();

  QString answer;
  int i;
  if(USBCy_RW("HSTS",answer,Motor_uC)) {
    QTextStream(&answer) >> i; // > 0 in progres <0 alarm == 0 stop mootor
    if(i<0) { devError.setDevError(MOTORALARM_ERROR); return(false); } //motor stop with alarm
    if(i==0) return(false); // motor stop normaly
  }
  return(true); // motor in progress
}

//-----------------------------------------------------------------------------
//--- Start close block procedure CLOSEBLOCK_STATE
//-----------------------------------------------------------------------------
void TDtBehav::closeBlock(void)
{
  devError.clearDevError();
  QString answer;
  int i;
  if(!USBCy_RW("RSTS",answer,Temp_uC)) return;
  QTextStream(&answer) >> i;
  if(i==1) { devError.setDevError(OPENINRUN_ERROR); return ; } // try close in run time
  USBCy_RW("HCLOSE",answer,Motor_uC);
}

//-----------------------------------------------------------------------------
//--- Start measure high of tube MEASHIGHTUBE_STATE
//-----------------------------------------------------------------------------
void TDtBehav::measHighTubeStart(void)
{
  devError.clearDevError();
  QString answer;
  USBCy_RW("HTUBE",answer,Motor_uC);
}
//-----------------------------------------------------------------------------
//--- Finish measure high of tube MEASHIGHTUBE_STATE
//-----------------------------------------------------------------------------
void TDtBehav::measHighTubeFinish(void)
{
  devError.clearDevError();
  QString answer;
  int i=-1;
  if(!USBCy_RW("HDIST",answer,Motor_uC)) return;
  QTextStream(&answer) >> i;
  tubeHigh=i;
}

//-----------------------------------------------------------------------------
//--- Start measure high of tube CHECK_MEASHIGHTUBE_STATE
// return 0 on finish measure
//-----------------------------------------------------------------------------
int TDtBehav::checkMeasHighTubeState(void)
{
  devError.clearDevError();
  QString answer;
  int i;
  if(!USBCy_RW("HSTS",answer,Motor_uC)) return(-2);
  QTextStream(&answer) >> i;
  return(i);
}

//-----------------------------------------------------------------------------
//--- write high og tube in the motor controller
//-----------------------------------------------------------------------------
void TDtBehav::writeHighTubeValue(int value)
{
  QString answer;
  USBCy_RW(QString("STUB %1 0 0 0").arg(value),answer,Motor_uC);
}

//-------------------------------------------------------------------------------------------------
//---  Start Run process STARTRUN_STATE
//-------------------------------------------------------------------------------------------------
void TDtBehav::startRun()
{
  QString answer,text;
  QStringList list;
  bool ok;
  int i;
  testMeas=false;
  devError.clearDevError();
// 1.
// test on close cover. if don't close, return error state
  if(USBCy_RW("HPOS",answer,Motor_uC)) { // check
    QTextStream(&answer) >> i;
    if(i!=2){ devError.setDevError(STARTWITHOPENCOVER_ERROR); return ; } //run
  }
  if(devError.analyseError()){
    return ; //fatal error
  }

// 2. write optical chanels
  USBCy_RW("ST",answer,Temp_uC);
  USBCy_RW("XSP 0",answer,Temp_uC);

  active_ch = map_Run.value(run_activechannel).toInt(&ok,16);
  int numActiveCh=0;
  for(i=0; i<COUNT_CH; i++)
  {
    if(active_ch & (0xf<<i*4)) {
      text = QString("FCEXP %1 2 %2 %3").arg(i).arg(expVal0[i]).arg(expVal1[i]);
      numActiveCh++;
    }
    else text = QString("FCEXP %1 0 %2 %3").arg(i).arg(expVal0[i]).arg(expVal1[i]);
    USBCy_RW(text,answer,Optics_uC);
  }
  USBCy_RW("FMODE 0",answer,Optics_uC); // don't write optical image on SD
  USBCy_RW("FPSAVE",answer,Optics_uC);
  msleep(1);//usleep(1000);
  if(devError.analyseError()){
    return ; //fatal error
  }

// 3. write temperature program

  text = map_Run.value(run_programm);
  list = text.split("\t");
  int expLevel=0;
  expLevels=0;
  for(i=0; i<list.size(); i++)
  {
    text = list.at(i);
    if(list.at(i).size()){
      if(text[0]==' ') break;
      qDebug()<<text;
      USBCy_RW(text,answer,Temp_uC);
// find all cycle with expositions
      QStringList levelList=text.split(' ');
      if(levelList.size()>1){
        if(levelList.at(0).toLower()=="xlev") {
          bool ok; int exp=levelList.at(6).toInt(&ok);
          if(ok) if(exp&1) expLevel+=exp;
        }
        if(levelList.at(0).toLower()=="xcyc") {
          bool ok; int cyc=levelList.at(1).toInt(&ok);
          if(ok) expLevels+=expLevel*cyc;
          expLevel=0;
        }
      }
      msleep(1);
    }
  }
  msleep(1);//usleep(200);
  qDebug()<<"all exposition"<<expLevels*numActiveCh*COUNT_SIMPLE_MEASURE<<expLevels<<numActiveCh;
  if(devError.analyseError()){
    return ; //fatal error
  }

// 4. write protocol
  Protocol_Sec0 protBuf;
  strncpy(protBuf.ProtocolSec0_96.version,MIN_VERSION,5); // for compatible with V.7
  strncpy(protBuf.ProtocolSec0_96.version+5,APP_VERSION,19);
// operator name  command to vc PONM
  strncpy(protBuf.ProtocolSec0_96.Operator,map_Run.value(run_operator).toLocal8Bit(),18);
// current date and time command to vc PDATE
  QDateTime now=QDateTime::currentDateTime();
  QString strDateTime=now.toString("dd-MM-yyyy, hh:mm:ss");
  strcpy(protBuf.ProtocolSec0_96.date,strDateTime.toLatin1());
// ProtocolNum it's run name command to vc PNUM
//  strncpy(protBuf.ProtocolSec0_96.num_protocol,map_Run.value(run_name).section(".",-2,-2).toLatin1(),10);
  strncpy(protBuf.ProtocolSec0_96.num_protocol,map_Run.value(run_name).toLatin1(),10);
  qDebug()<<map_Run;
// Temperature program name get from tc by command XGN
  strcpy(protBuf.ProtocolSec0_96.program,text.toLatin1());

  writeIntoUSB512("PWRS 0","",(unsigned char*) protBuf.byte_buf); // if error not fatal

// may be write "FTIM" in temperature controller

// 5. run commad
  USBCy_RW("RN",answer,Temp_uC);
  msleep(2);
// remove old video directory and create new
  if(videoDirName.size()){
    QDir dir ;
    if(!dir.exists(videoDirName)) dir.mkdir(videoDirName);
    currentVideoDirName=videoDirName+"/"+devName.toUpper();
    clearVideoFiles(currentVideoDirName);
  }
}

//-------------------------------------------------------------------------------------------------
//--- Get Measure STARTMEASURE_STATE
//-------------------------------------------------------------------------------------------------
void TDtBehav::startMeasure(void)
{
  int i,j;
  QString text;
  QByteArray buf;
  int num = 0;
  map_Measure.clear();
  if(fnGet <= 0) return ;
  buf.resize(BYTES_IN_SECTOR);
  devError.clearDevError();

  for(i=0; i<COUNT_CH; i++)
  {
    if(active_ch & (0x0f<<i*4)) {
      for(j=0; j<sectors; j++) {
        text = QString("FDB %1 %2 %3").arg(fnGet-1).arg(i).arg(j);
        readFromUSB512(text,"Ok",(unsigned char*)buf.data());
        text = QString("_%1").arg(num);
        text = MEASURE_Data + text;
        map_Measure.insert(text,buf);
        //msleep(200);
        num++;
      }
    }
  }
  buf.clear();
}

//-----------------------------------------------------------------------------
//--- Stop Run temperature program STOPRUN_STATE
//-----------------------------------------------------------------------------
void TDtBehav::stopRun(void)
{
  devError.clearDevError();
  QString answer;
  USBCy_RW("ST",answer,Temp_uC);
}

//-----------------------------------------------------------------------------
//--- Pause Run temperature program PAUSERUN_STATE
//-----------------------------------------------------------------------------
void TDtBehav::pauseRun(void)
{
  devError.clearDevError();
  QString answer;
  USBCy_RW("XSP 1",answer,Temp_uC);
}

//-----------------------------------------------------------------------------
//--- Continue Run temperature program CONTRUN_STATE
//-----------------------------------------------------------------------------
void TDtBehav::contRun(void)
{
  devError.clearDevError();
  QString answer;
  USBCy_RW("XSP 0",answer,Temp_uC);
}

//-----------------------------------------------------------------------------
//--- Execute external command EXECCMD_STATE
//-----------------------------------------------------------------------------
void TDtBehav::execCommand(void)
{
  devError.clearDevError();
  QString answer;
  short int uc=map_ExecCmd.value(EXECCMD_UC).toInt();

  switch(uc){
    case UCIO_CANID: {
    QStringList list=map_ExecCmd.value(EXECCMD_CMD).split(' ');
    if(list.size()>1){
      if(list.at(0).toLower()=="fmode") {
        bool ok; int t=list.at(1).toInt(&ok);
        if(ok) gVideo=t;
        logSrv->logMessage(tr("Set request video capture mode ")+QString("%1").arg(gVideo));
        if(gVideo){
          QStorageInfo storage=QStorageInfo::root();

          if(storage.bytesFree()<expLevels*W_IMAGE*H_IMAGE*2+10024000){
            answer="1";
            gVideo=0;
            break;
          }
        }
        answer="0";
        allStates[ONMEAS_STATE]=ONMEAS_STATE;
      }
    }
    break;
    }
    case UCOPTICS_CANID: {
      USBCy_RW(map_ExecCmd.value(EXECCMD_CMD).toLatin1(),answer,Optics_uC);
      break;
    }
    case UCTEMP_CANID: {
      USBCy_RW(map_ExecCmd.value(EXECCMD_CMD).toLatin1(),answer,Temp_uC);
      break;
    }
    case UCMOTOR_CANID: {
      USBCy_RW(map_ExecCmd.value(EXECCMD_CMD).toLatin1(),answer,Motor_uC);
      break;
    }
    case UCDISP_CANID: {
      USBCy_RW(map_ExecCmd.value(EXECCMD_CMD).toLatin1(),answer,Display_uC);
      break;
    }
  }
  map_ExecCmd.insert(EXECCMD_ANSWER,answer);
}

//-----------------------------------------------------------------------------
//--- Memory test
//-----------------------------------------------------------------------------
bool TDtBehav::makeMemoryTest(void)
{
  QString answer,s1,s2;
  int mWordW,mWordR;
  bool ok;

  // test memory
  mWordW=0x9001;
  QString cmd=QString("FMWR 100 %1").arg(mWordW,0,16);
  USBCy_RW(cmd,answer,Optics_uC); // write magic word 1001000000000001 0x9001
  if(!answer.compare("mem write OK",Qt::CaseInsensitive)){
    USBCy_RW("FMRD 100",answer,Optics_uC);
    QTextStream(&answer)>>s1>>s1>>s1;
    mWordR=s1.toInt(&ok,16);
    if(mWordW==mWordR){ // first compare
      mWordW=(~mWordW)&0xffff;
      cmd=QString("FMWR 100 %1").arg(mWordW,0,16);
      USBCy_RW(cmd,answer,Optics_uC); // write magic word 110111111111110 0x6ffe
      if(!answer.compare("mem write OK",Qt::CaseInsensitive)){
        USBCy_RW("FMRD 100",answer,Optics_uC);
        QTextStream(&answer)>>s1>>s1>>s1;
        mWordR=s1.toInt(&ok,16);
        if(mWordW==mWordR){ //second compare
          return true;
        }
      }
    }
  }
  devError.setDevError(PRERUNMEMTEST_ERROR);
  return false;
}

//-----------------------------------------------------------------------------
//--- Prepere measure set num_filter - выбор светофильтра, exp_led - выбор экспозиции(мсек),
//---                     adc_led - яркости, id_led - номера прожектора
//-----------------------------------------------------------------------------
bool TDtBehav::prepareMeasure(int num_filter, int exp_led, int adc_led, int id_led)
{
  QString answer;
  int i,value;

//.... выбрать светофильтр:
  QString cmd=QString("MP %1").arg(num_filter);
  USBCy_RW(cmd,answer,Motor_uC);
//.... выбрать экспозицию и номер прожектора

  cmd=QString("FEXP %1").arg(exp_led);     // экспозиция was (int)(exp_led/0.308) now get from parameters
  USBCy_RW(cmd,answer,Optics_uC);

  cmd=QString("FLT %1").arg(adc_led);     // яркость
  USBCy_RW(cmd,answer,Optics_uC);

  cmd=QString("FLED %1").arg(id_led+1);     // номер прожектора
  USBCy_RW(cmd,answer,Optics_uC);

//..... дождаться когда остановится двигатель
  i = 0;
  do {
    msleep(100);
    i++;
        USBCy_RW("MBUSY",answer,Motor_uC);
        value = -1;
        QTextStream(&answer) >> value;
  }
  while(value != 0 && i<15);    // ждём до 1500 мсек
  if(value != 0 || i >= 15) { devError.setDevError(MOTORDRVLED_ERROR); return false;}
  return true;
}

//-----------------------------------------------------------------------------
//--- Test device before start RUN STARTCHECKING_STATE
//-----------------------------------------------------------------------------
bool TDtBehav::measureTest(void)
{
  int i,j;
  unsigned int len=256*1023*2;

  QString answer,cmd;
  int time_OUT;
  unsigned char dstate;
  QByteArray buf;

  buf.resize(len);

// 0. memory test
  if(!makeMemoryTest()) return false;

// 1. prepare measure
  for(i=0;i<MAX_OPTCHAN;i++){
    if(ledsMap&(1<<i)){
      if(!prepareMeasure(filterNumber[i],expVal0[i],ledCurrent[i],ledNumber[i])) return false;
      break;
    }
  }
//--- если предыдущая картинка не считана, то считываем(очищаем)
  readFromUSB("FPIC","Ok",(unsigned char*)buf.data(),len); // send DMA start cmd ,read buf

// 2. Измерения
  FX2->VendWrite(NULL, 0, 0x21, 0, 0);	// программный запуск измерений
  i=0;
  time_OUT = 75;	// 15 sec
  do{
    msleep(200);
    FX2->VendRead(&dstate, 1, 0x23, 0, 0);
    i++;
  } while(!(dstate & 0x4) && i < time_OUT); 		// wait for new frame ready to xfer
   // waiting before 15 seconds
// 3. выключить прожектор
  USBCy_RW("FLED 0",answer,Optics_uC);

  j=0;
  buf.clear();
  QVector <short unsigned int> uBuf,Digitize_DT964_buf;
  uBuf.resize(BYTES_IN_SECTOR/2);
  Digitize_DT964_buf.resize(pumps*sectors);

  while(sectors > j) {
    cmd=QString("FFD %1").arg(j);
    readFromUSB512(cmd,"Ok",(unsigned char*)uBuf.data());// read buf
    for(i=0; i<pumps; i++) { Digitize_DT964_buf[i+j*pumps] = uBuf[i+64]; }
    msleep(30);
    j++;
  }

  int max_value=*std::max_element(Digitize_DT964_buf.begin(),Digitize_DT964_buf.end());

  uBuf.clear();
  Digitize_DT964_buf.clear();

  if(max_value < 130)  { devError.setDevError(PRERUNANALYSE_ERROR); return false;}

  return true;
}

//-----------------------------------------------------------------------------
//--- Test device before start RUN STARTCHECKING_STATE
//-----------------------------------------------------------------------------
void TDtBehav::getPicture(void)
{
  QString answer;
  bool ok;
  unsigned char dstate;
  map_GetPicData.clear();
  devError.clearDevError();
  unsigned int len=256*1023*2;
  int ch=map_inpGetPicData.take(GETPIC_CHANNEL).toInt(&ok);
  int exp=(int)(map_inpGetPicData.take(GETPIC_EXP).toInt(&ok)/0.308);
  int ctrl=map_inpGetPicData.take(GETPIC_CTRL).toInt(&ok);
  QByteArray buf;
   buf.resize(len);
  if(!prepareMeasure(filterNumber[ch],exp,ledCurrent[ch],ledNumber[ch])) return ;

  //--- если предыдущая картинка не считана, то считываем(очищаем)
  readFromUSB("FPIC","Ok",(unsigned char*)buf.data(),len); // send DMA start cmd ,read buf
//---
  FX2->VendWrite(NULL, 0, 0x21, 0, 0);	// программный запуск измерений
  int i=0,time_OUT;
  time_OUT = 75;	// 15 sec
  do{
    msleep(200);
    FX2->VendRead(&dstate, 1, 0x23, 0, 0);
    i++;
  } while(!(dstate & 0x4) && i < time_OUT); 		// wait for new frame ready to xfer
   // waiting before 15 seconds
// 3. выключить прожектор
  USBCy_RW("FLED 0",answer,Optics_uC);

  if(ctrl==0){  // get video picture only
    buf.resize(len);
    readFromUSB("FPIC","Ok",(unsigned char*) buf.data(),len);
    map_GetPicData.insert(GETPIC_VIDEO,buf);
  }
  else if(ctrl==1){ // get analyse data
    buf.resize(BYTES_IN_SECTOR*sectors);
    for(i=0;i<sectors;i++){
      readFromUSB512( QString("FFD %1").arg(i),"",(unsigned char*)buf.data()+BYTES_IN_SECTOR*i);
    }
    map_GetPicData.insert(GETPIC_DATA,buf);
  }
  else{ // get video+data
    buf.resize(len);
    readFromUSB("FPIC","Ok",(unsigned char*) buf.data(),len);
    map_GetPicData.insert(GETPIC_VIDEO,buf);
    buf.resize(BYTES_IN_SECTOR*sectors);
    for(i=0;i<sectors;i++){
      readFromUSB512( QString("FFD %1").arg(i),"",(unsigned char*)buf.data()+BYTES_IN_SECTOR*i);
    }
    map_GetPicData.insert(GETPIC_DATA,buf);
  }
  buf.clear();
}

//-----------------------------------------------------------------------------
//--- Test device before start RUN STARTCHECKING_STATE
//-----------------------------------------------------------------------------
void TDtBehav::checking(void)
{
  QString answer;
  devError.clearDevError();
  // test on close cover. if don't close, return error state
  if(USBCy_RW("HPOS",answer,Motor_uC)) { // check
    int i;
    QTextStream(&answer) >> i;
    if(i!=2){ devError.setDevError(STARTWITHOPENCOVER_ERROR); return ; } //run
  }

  // press cover
  int k=-1,t=0;
  if(USBCy_RW("MPLC",answer,Motor_uC)) { // check place of cover
    QTextStream(&answer) >> k;
    if((k!=2)&&(fMotVersion>=6.00)){
      if(USBCy_RW("HPRESS",answer,Motor_uC)){
        QTextStream(&answer) >> k;
        switch(k){
        case 0:
          t=0;
          while(t<10){ // wait 10 sec & check
            t++;
            msleep((1000));
            USBCy_RW("MPLC",answer,Motor_uC);
            k=-1;
            QTextStream(&answer) >> k;
            if(k==2) break;
          }
          break;
        default:
          k=-1;
          break;
        }
      }
    }
  }
  if(k!=2){ devError.setDevError(STARTWITHOPENCOVER_ERROR); return ; } //run

 // make measure and analise measure
  if(!measureTest()){ //first error
    USBCy_RW("TBCK 40000",answer,Optics_uC);

    USBCy_RW("fver",answer,Optics_uC);
    if(!measureTest()){ //second error
      devError.setDevError(PRERUNFAULT_ERROR);
      return;
    }
    devError.setDevError(PRERUNSEMIFAULT_ERROR);
  }
}

//-----------------------------------------------------------------------------
//--- Save device parameters SAVEPARAMETERS_STATE
//-----------------------------------------------------------------------------
void TDtBehav::saveParameters(void)
{
  devError.clearDevError();
  bool ok;

  int ctrl=map_SaveParameters.take(SAVEPAR_CTRL).toInt(&ok);
  if(!ok) { devError.setDevError(GETFRAME_ERROR); return; } //error in inputs
  //ctrl=1; // remove!!!!!
  if(ctrl==0){
    QVector <short int> spots;
    short int number;
    int cnts=0;
    QTextStream stream(&map_SaveParameters[SAVEPAR_DATA]);
    do{
      stream>>number;
      spots.append(number);
      if(++cnts>=(tubes*2+2)) break;
    }while (!stream.atEnd());
    if(spots.size()<(tubes*2+2)) { spots.clear(); devError.setDevError(GETFRAME_ERROR); return; } //error in inputs

    short int *tmp; //make pointer on parameters as short
    tmp=(short int*)parameters.data();

    *(tmp+BYTES_SHIFT_RX/2)=spots.at(spots.size()-2);   //Rx set in parameters block
    *(tmp+BYTES_SHIFT_RX/2+1)=spots.at(spots.size()-1); //Ry set in parameters block

    for(int i=0;i<sectors;i++){
      for(int j=0;j<pumps;j++){
        *(tmp+BYTES_IN_SECTOR/2*i+BYTES_SHIFT_XY/2+j*2)=spots.at(i*pumps*2+j*2 ); //coordinates pulps X is seting in parameters block
        *(tmp+BYTES_IN_SECTOR/2*i+BYTES_SHIFT_XY/2+j*2+1)=spots.at(i*pumps*2+j*2+1); //coordinates pulps Y is seting in parameters block
      }
      writeIntoUSB512(QString("FSDW %1").arg(START_SECTOR_PARAMETERS+i),"",(unsigned char*)(parameters.data()+i*BYTES_IN_SECTOR));
    }
    spots.clear();
  }
  else if(ctrl==1){
    QVector <short int> exps;
    QTextStream stream(&map_SaveParameters[SAVEPAR_DATA]);
    Save_Par *param=(Save_Par*)parameters.data();
    QString numS;
    short int numI;
    do{
      stream>>numS;
      numI=numS.toUShort(&ok);
      if(ok) exps.append(numI);
    }while (!stream.atEnd());
    if(exps.size()%2) { exps.clear(); devError.setDevError(GETFRAME_ERROR); return; } //error in inputs
    if(exps.size()>(MAX_OPTCHAN*2)) { exps.clear(); devError.setDevError(GETFRAME_ERROR); return; } //error in inputs
    for(int i=0;i<exps.size();i+=2){
      param->SavePar_96.optics_ch[i/2].exp[0]=exps.at(i);
      param->SavePar_96.optics_ch[i/2].exp[1]=exps.at(i+1);
    }
    writeIntoUSB512(QString("FSDW %1").arg(START_SECTOR_PARAMETERS),"",(unsigned char*)parameters.data());
    exps.clear();
  }
  else{
     devError.setDevError(GETFRAME_ERROR); return;
  }
  QString answer;
  getInfoDevice();
  USBCy_RW("FPSAVE",answer,Optics_uC);
}

//-----------------------------------------------------------------------------
//--- Save sector SAVESECTOR_STATE
//-----------------------------------------------------------------------------
void TDtBehav::saveSector(void)
{
  devError.clearDevError();

  QString ctrl=map_SaveSector.take(SECTOR_CMD);
  if(!ctrl.size()) { devError.setDevError(GETFRAME_ERROR); return; } //error in inputs

  QTextStream stream(&map_SaveSector[SECTOR_DATA]);
  QByteArray buf;

  int cntBytes=0;
  char by;
  do{
    stream>>by;
    buf.append(by);
    cntBytes++;
  }while (!stream.atEnd());

  QByteArray bufwr;

  bufwr=QByteArray::fromBase64(buf);
  if(bufwr.size()>512) { buf.clear(); bufwr.clear(); devError.setDevError(GETFRAME_ERROR); return; } //error in inputs
  writeIntoUSB512(ctrl,"",(unsigned char*)bufwr.data());
  buf.clear(); bufwr.clear();
  QString answer;
  USBCy_RW("FPSAVE",answer,Optics_uC);
}

//-----------------------------------------------------------------------------
//--- Read sector READSECTOR_STATE
//-----------------------------------------------------------------------------
void TDtBehav::readSector(void)
{
  QString text;
  text.resize(512);

  QString ctrl=map_ReadSector.take(SECTOR_CMD);
  if(!ctrl.size()) { devError.setDevError(GETFRAME_ERROR); return; } //error in inputs
  QByteArray buf;
  buf.resize(BYTES_IN_SECTOR);

  if(!readFromUSB512(ctrl,"0",(unsigned char*)buf.data())) devError.setDevError(USB_CONNECTION_ERROR);
  text = buf.toBase64();

  map_ReadSector.insert(SECTOR_DATA, text);

  buf.clear();
}

//-----------------------------------------------------------------------------
//--- Read picture after meas
//-----------------------------------------------------------------------------
void TDtBehav::getPictureAfterMeas(void)
{
  QString answer;
  int fmode=0,m,ret;

  bool ok;
  unsigned char dstate=0;

  if(USBCy_RW("FMODE",answer,Optics_uC)){
    fmode=answer.toInt(&ok);
    if(!ok) fmode=0;
  }
  if(fmode==0) return;
  QByteArray picbuf_online;
  picbuf_online.resize(LEN);
  QVector<ushort> videoData(LEN/2);

  for(int i=0; i<COUNT_CH; i++) {  // цикл по кол-ву каналов
    for(int j=0; j< COUNT_SIMPLE_MEASURE; j++) {     // цикл по кол-ву экспозиций
      if((active_ch & (1<<(i*4))) != 0) {        // только по активным каналам
//... ожидание готовности окончания измерений
        m =0;
        do {
          msleep(200);
          FX2->VendRead(&dstate, 1, 0x23, 0, 0);
          m++;
        }
        while(!(dstate & 0x4) && m < 75);
//... считываем измерения
        ret=0;
        if(USBCy_RW("FPIC",answer,Optics_uC)){ //send DMA start cmd
          ret=FX2->BulkRead((unsigned char *)picbuf_online.data(),LEN);
        }
//... обработка изображения
        if(ret) {
          memcpy(videoData.data(), picbuf_online.data(), LEN);
          ProcessVideoImage(videoData,&BufVideo[i*H_IMAGE*W_IMAGE + j*COUNT_CH*H_IMAGE*W_IMAGE]);
        }
      }
      else {
        for(int k=0; k<H_IMAGE*W_IMAGE; k++) BufVideo[i*H_IMAGE*W_IMAGE + k + j*COUNT_CH*H_IMAGE*W_IMAGE] = 0;
      }
      num_meas++;    // увеличиваем счётчик кол-ва измерений
    }
  }

//----- сохранение video
 // Buf_Video.clear();
  picbuf_online.clear();
  videoData.clear();

  SaveVideoData(num_meas - COUNT_CH*COUNT_SIMPLE_MEASURE);
}

//-----------------------------------------------------------------------------
//--- Read program of amplification for simple mode view parameters _STATE
//-----------------------------------------------------------------------------
void TDtBehav::readAMPProg(void)
{
  QString answer,text;
  devError.clearDevError();
  map_ampProgForSimple.clear();
  int i,j,k_lev=0;
  int retVal=0;
  int type, vol, num_block, num_level, num_level_inBlock, rep, type_block;
          int temp, time, incr_temp, incr_time, grad, type_fluor, type_grad=0;

  while(1) {
    if(!USBCy_RW("XGP",answer,Temp_uC)) { retVal=1; break; }
    if(answer.contains("-1", Qt::CaseInsensitive)||answer.contains("?", Qt::CaseInsensitive)) { retVal=2; break; }
    QTextStream(&answer) >>type>>vol>>num_block>>num_level>>type_grad;
    text.append(QString("XPRG %1 %2 %3").arg(type).arg(vol).arg(type_grad)+"\t");   // 1-ая строка программы
    for(i=0; i<num_block; i++) {    // цикл по кол-ву блоков
      if(!USBCy_RW(QString("XGB %1").arg(i),answer,Temp_uC)) { retVal=1; break; }
      if(answer.contains("-1", Qt::CaseInsensitive)) { retVal=2; break; }
      QTextStream(&answer) >>rep>>type_block>>num_level_inBlock;
      for(j=0; j<num_level_inBlock; j++){
        if(!USBCy_RW(QString("XGL %1").arg(k_lev),answer,Temp_uC)) { retVal=1; break; }
        k_lev++;
        QTextStream(&answer) >>temp>>time>>incr_temp>>incr_time>>grad>>type_fluor;
        if(temp <=0 || time < 0) {retVal = 2; break; }
        text.append(QString("XLEV %1 %2 %3 %4 %5 %6\t").arg(temp).arg(time).
                                                      arg(incr_temp).       // инфо о полке
                                                      arg(incr_time).       //
                                                      arg(grad).            //
                                                      arg(type_fluor));     //
      }
      if(retVal) break;
      switch (type_block) {
        case 1: text.append(QString("XCYC %1\t").arg(rep)); break;
        case 3: text.append("XHLD\t"); break;
        case 2: text.append("XPAU\t"); break;
      }
    }
    if(USBCy_RW("XGN",answer,Temp_uC)) text.append(QString("XSAV %1").arg(answer));
    map_ampProgForSimple.insert(GETAMPINSIMPLE_AMPPROG,text);

    if(USBCy_RW("PONM",answer,Optics_uC)) map_ampProgForSimple.insert(GETAMPINSIMPLE_PONM,QString("%1").arg(answer)); // operator name
    if(USBCy_RW("PNUM",answer,Optics_uC)) map_ampProgForSimple.insert(GETAMPINSIMPLE_PNUM,QString("%1").arg(answer)); //protocol number
    if(USBCy_RW("PDATE",answer,Optics_uC)) map_ampProgForSimple.insert(GETAMPINSIMPLE_PDATE,QString("%1").arg(answer)); // date & time of run
    if(USBCy_RW("FACS",answer,Optics_uC)) {
        bool ok;
        int tmp1,tmp=answer.toInt(&ok);
        if(!ok) tmp1=0;
        tmp1=(tmp&1)|((tmp&2)<<3)|((tmp&4)<<6)|((tmp&8)<<9)|((tmp&0x10)<<12)|((tmp&0x20)<<15);
        map_ampProgForSimple.insert(GETAMPINSIMPLE_PLEDS,QString("0x%1").arg(tmp1,0,16)); // leds map from protocol max 6 ch
    }
    break;
  }
  if(retVal==2) devError.setDevError(GETAMPPROG_ERROR);
}


void TDtBehav::ProcessVideoImage(QVector<ushort> &a,ushort * b)
{
  int i,j;
  int k,m;
  int left;
  int width_left;

  switch(type_dev) {
    default:
    case 96:
    case 384:
      left = LEFT_OFFSET;
      break;
    case 48:
    case 192:
      left = LEFT_OFFSET_DT48;
      break;
  }

  width_left = left + W_IMAGE;

  k=0;
  for(i=0; i<H_REALIMAGE; i++) {
    if(i<5 || i>296) continue;
    m=0;
    for(j=0; j<W_REALIMAGE; j++) {
      if(j<left || j>width_left-1) continue;
      b[k*W_IMAGE + m]=a.at(i*W_REALIMAGE + j);
      m++;
    }
    k++;
  }
}
void TDtBehav::SaveVideoData(int count)
{
  count=count;// remove warning
  QString name_ch;
  QString fName;
  FILE *F;
  ushort Val;
  QVector<ushort> video_buf(LEN/2);

  for(int k=0; k<COUNT_CH; k++) {
    switch(k) {
      case 0: name_ch = "fam"; break;
      case 1: name_ch = "hex"; break;
      case 2: name_ch = "rox"; break;
      case 3: name_ch = "cy5"; break;
      case 4: name_ch = "cy5.5"; break;
    }
    if((active_ch & (1<<(k*4))) != 0) {
      for(int m=0; m < COUNT_SIMPLE_MEASURE; m++) {
        for(int i=0; i<H_IMAGE; i++) {
            for(int j=0; j<W_IMAGE; j++) {
              Val = BufVideo[H_IMAGE*W_IMAGE*k + i*W_IMAGE + j + m*COUNT_CH*H_IMAGE*W_IMAGE];
              video_buf[j + i*W_IMAGE] = (Val << 4) & 0xffff;
            }
        }
        fName = currentVideoDirName;
        fName +=  QString("/video_%1_%2_%3").arg(name_ch).arg(fn+1).arg(m+1)+".dat";//arg(k*COUNT_SIMPLE_MEASURE + m + count) + ".dat";
        try {
          if((F = fopen(fName.toLatin1(),"wb")) == NULL) continue;
          else {
            fwrite(&video_buf[0], sizeof(unsigned short), H_IMAGE*W_IMAGE, F);
            fclose(F);
           }
         }
         catch(...) {;}
      }
    }
  }
  video_buf.clear();
}

//-----------------------------------------------------------------------------
//--- wake Up CLOSEBLOCK_STATE
//-----------------------------------------------------------------------------
void TDtBehav::wakeUp(void)
{
  QString answer;
  USBCy_RW("cev 35",answer,Optics_uC);
}
//-----------------------------------------------------------------------------
//--- Start close block procedure CLOSEBLOCK_STATE
//-----------------------------------------------------------------------------
void TDtBehav::standBy(void)
{
  QString answer;
  USBCy_RW("cev 34",answer,Optics_uC);
}
