#ifndef DTBEHAV_H
#define DTBEHAV_H
#include <QtCore/QCoreApplication>
#include <QFile>
#include <QDebug>
#include <QString>
#include <QTime>
#include <QTimer>
#include <QStringList>
//#include <QTableWidget>
#include <QThread>
#include <QMutex>
#include <QDebug>
#include <QWaitCondition>
#include <QSettings>

#include <QTcpServer>
#include <QTcpSocket>
#include <QAbstractSocket>
#include <QDataStream>
#include <QByteArray>
#include <QStorageInfo>
//for get free disk space
#include <sys/stat.h>
#include <sys/statfs.h>
#include <syslog.h>


#include "logobject.h"
#include "canclient.h"
#include "cansrv.h"
#include "progerror.h"
#include "../request_dev.h"
#include "digres.h"
#include "gpio.h"
#include "usbscanner.h"
#include "fx2usb.h"
#define SAMPLE_DEVICE 1800 // time in ms for request device data (timer)
#define USB_EVENT_INTERVAL 1000
/*typedef struct
{
    QString SerNum;
    uint16_t PID;
    uint16_t VID;
    int status;
    QString IP_port;
}USB_Info;
*/
typedef enum
{
  GETPARREQ_STATE,
  OPENBLOCK_STATE,
  CLOSEBLOCK_STATE,
  STARTRUN_STATE,
  STARTMEASURE_STATE,
  STOPRUN_STATE,
  PAUSERUN_STATE,
  CONTRUN_STATE,
  EXECCMD_STATE,
  CHECKING_STATE,
  GETPICTURE_STATE,
  SAVEPARAMETERS_STATE,
  SAVESECTOR_STATE,
  READSECTOR_STATE,
  ONMEAS_STATE,
  SETPOWER_STATE,
  CHANGEINT_STATE,
  MEASHIGHTUBE_STATE,
  HIGHTUBESAVE_STATE,
  GETAMPINSIMPLE_STATE,
  ALLREQSTATES,    //limiter on process request states
    READY,
    DEVICE_ERROR_STATE,
    GETINFO_STATE,
    CHECK_OPENBLOCK_STATE,
    CHECK_CLOSEBLOCK_STATE,
    CHECK_ONMEAS_STATE,
    CHECK_MEASHIGHTUBE_STATE
}CPhase;  // phases of state machine

void clearVideoFiles(const QString &);


class TDtBehav : public QObject
{
  Q_OBJECT
public:
  TDtBehav();
  ~TDtBehav();
  void msleep(qint64 msec)
  {
      QEventLoop loop;
      QTimer::singleShot(msec, &loop, SLOT(quit()));
      loop.exec();
  }
  void iniFileRead(QString&); //read setup from file
  void setAbort(bool a) {abort=a;}
// global error function wrapper
  //QString readGlobalErrText(void){ return globalError.readProgTextError(); }
  //int readGlobalErr(void){ return globalError.readProgError(); }
// working wit HW
  bool readFromUSB512(QString cmd,QString templ,unsigned char* buf);
  bool writeIntoUSB512(QString cmd,QString templ,unsigned char* buf);
  bool USBCy_RW(QString cmd, QString &answer, TCANChannels *uC);
  bool USBCy_RW(QString cmd, QString &answer, TCANSrvCh *uC);
  bool readFromUSB(QString cmd,QString templ,unsigned char* buf,int len);
  void ProcessVideoImage(QVector<ushort>&, /*QVector<ushort>&*/ ushort *);
  void SaveVideoData(int count);

  TLogObject *logSys;
  TLogObject *logSrv;
  TLogObject *logNw;
  TLogObject *logDev;

  //QStringList canDevName;
  void setErrorSt(short int); // set errors in state of statemachine for all stadies
  //function for work with log of device
  void logClientClr(void) { logClient.clear();}
  void logClientAddData(QString str){ logClient.append(QDateTime::currentDateTime().toString("hh:mm:ss ")+"> " +str+"\n") ;}
// function for work with device
  void initialDevice(void);
  void getInfoDevice();
  void getInfoData();
  void openBlock(void);
  bool checkMotorState(void);
  void closeBlock(void);
  void measHighTubeStart(void);
  void measHighTubeFinish(void);
  int  checkMeasHighTubeState(void);
  void writeHighTubeValue(int);
  void startRun(void);
  void checking(void);
  void startMeasure(void);
  void stopRun(void);
  void pauseRun(void);
  void contRun(void);
  void execCommand(void);
  void getPicture(void);
  void saveParameters(void);
  void saveSector(void);
  void readSector(void);
  void getPictureAfterMeas(void);
  void readAMPProg(void); // read amplification program

  void wakeUp(void);
  void standBy(void);

// -- function for Run Checking
  bool makeMemoryTest(void);
  bool prepareMeasure(int num_filter, int exp_led, int adc_led, int id_led);
  bool measureTest(void);
  bool analyseMeasure(void);
//---
public slots:
  void timeAlarm(void);

//protected:
public :
    void run();

private:
  bool libUsbOpen;
// Error processing
  TDevErrors devError;  // hardware error in bits represantation
  bool fatalCanErr; // fatal error on CAN node. After this going on INITIAL_DEVICE phase
  QStringList canDevName; // names of CAN device in printing string of error message

  QString videoDirName; // catalogue with all device Video pictures file
  QString currentVideoDirName; //catalogue with run device Video pictures file

  short int stReadInfoErr,stCycleErr,stOpenErr,stCloseErr,stRunErr,stMeasErr; // errors on all stadies of work
  short int stStopErr,stPauseErr,stContErr,stExecErr,stCheckingErr,stGetPictureErr; // errors on all stadies of work
  short int stSaveParametersErr; // errors on all stadies of work
  short int stSaveSectorErr,stReadSectorErr; // errors on read and save sector
  short int stMeasHighTubeErr; // error meas of high tube
  short int stMeasGetAmpProgErr; // error when get amplification program

  QString devName; // name of device get from settings
  int nPort; // port number get from settings

  TFx2Usb            *FX2;           // usb
  THidUsb         *scanner;
// CAN chanels
  TCANChannels     *Optics_uC;
  TCANChannels     *Temp_uC;
  TCANChannels     *Motor_uC;
  TCANSrvCh        *Display_uC;

  TGpio            *devGpio;
  //QVector<USB_Info*> list_fx2; // structure with list of all USB FX2 devices

// individual device parameters
  double fMotVersion; // version of motor controller
  int fn,fnGet; // fn get from device fnGet - get in measure reqest
  int active_ch; // active chanel in current RUN
  int getActCh;
  int ledsMap;  // map of the leds hex chanel is low
  int sectors;  //sectors for read 384-4 192-2 others 1
  int pumps;  // tubes in sector
  int tubes; // all tubes
  short int expVal0[MAX_OPTCHAN],expVal1[MAX_OPTCHAN]; // exposition get from parameters
  int count_block; // count of block for read of optical coefficient for 384=2, others 1
  short int ledCurrent[MAX_OPTCHAN],filterNumber[MAX_OPTCHAN],ledNumber[MAX_OPTCHAN];
  //current of led 0-255, number filter and number led (0-7) 0-FAM,1-HEX,2-ROX,3-Cy5,4-Cy5.5

  int gVideo; // get video data after measure
  bool testMeas; // true if get request on measure
  int expLevels; // number of all expositiona

  QVector <ushort> BufVideo; // buffer with geting pictures
  int num_meas; // number of get pics meas size (H_IMAGE*W_IMAGE*COUNT_SIMPLE_MEASURE*COUNT_CH)
  int type_dev; // type of device 96,384...


  QString tBlType; // type of thermoblock
  QByteArray parameters; // parameters of device 512*(1 for DT48/96 2for DT192 4 for DT384) opt ch,spots XY radius

//.. NetWork ...
  int clientConnected;
  QTcpServer  *m_ptcpServer;
  quint32  m_nNextBlockSize;
  void sendToClient(QTcpSocket* pSocket, QString request,short int st);
  QTcpSocket *pClient;

  QMap<QString,QString> map_InfoDevice;
  QMap<QString,QString> map_InfoData;
  QMap<QString,QString> map_Run;
  QMap<QString,QByteArray> map_Measure;
  QMap<QString,QString> map_ExecCmd;

  QMap<QString,QString> map_inpGetPicData;     // input data from client: channel, exp, type of output data
  QMap<QString,QByteArray> map_GetPicData;     // video or/and analyse data

  QMap<QString,QString> map_SaveParameters;    // parameters for device (exp, opt ch,spots XY radius  etg)
  QMap<QString,QString> map_ConnectedStatus;   // status of connected

  QMap<QString,QString> map_BarCode; // read barcode from device

  QMap<QString,QString> map_SaveSector; // data for write sector on SD
  QMap<QString,QString> map_ReadSector; // data for write sector on SD
  QMap<QString,QString> map_dataFromDevice;// data with same parameters from device to client (press Run button...)
  QMap<QString,QString> map_dataToDevice;
  QMap<QString,QString> map_ampProgForSimple; // amplification program for run in simple mode


// process state machine
  CPhase phase,nextPhase;
  CPhase allStates[ALLREQSTATES];
  bool abort,timerAlrm;
  QTimer tAlrm;
  bool simpleMode;// going into true if internal USB connection is absent. Device work from CAN
  int powerMode; //0 - wake up, 1 - stand by 2 - shutdown

// log file parameters
  int logSize,logCount;
  bool logDevBool,logNwBool,logSrvBool;
  QString logClient; // string with log data into client
  bool useSerialScanner;
// settings from setup.ini
  QString barcodeDevice; // bar code reader serial port device
  int vendorID,productID; // vendor and product ID for USB

  QMutex mutex;
  QWaitCondition condition;

  int tubeHigh,tubeHighSave; // measurement high of tube
  int timeMeasHT; // meas high of tube timeout var
 // QStringList ampProgList;

signals:
  void sendToClients(QTcpSocket* pSocket, QString request,int st);
  void setBackLight(char *);
  void setInterfaces(bool);
public slots:
  void getFromclients(QTcpSocket* pSocket, QString request, int st);
// network strings processing
private slots:
  virtual void slotNewConnection();
  void slotReadClient();
  void slotError(QAbstractSocket::SocketError err);
// scanner processing
  void getBarCode(QString);
};

#endif // DTBEHAV_H
