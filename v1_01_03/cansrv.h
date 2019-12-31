#ifndef CANSRV_H
#define CANSRV_H

#include <QtCore>
#include "can_commsock.h"
#include "serialscanner.h"

int CANEventHandler(unsigned char,unsigned char);

//class Sleeper : public QThread
//{
//  Q_OBJECT
//public:
//  static void msleep(int ms) {QThread::msleep(ms);}
//};

// CAN server for display answer and recive events from CAN
class TCANSrvCh : public QObject
{
  Q_OBJECT
public:
  TCANSrvCh(int can_id=5, QString devN="/dev/ttymxc2", bool useSerialScannerDev=false);
  ~TCANSrvCh(void);
  void eventCANHandler(unsigned char evid,unsigned char srcnode);
  static int CMDparse(unsigned char *msg, char *reply,int TrID);
  static void cmds(int chan, unsigned char *cmd);
  QString barCodeStr;
  int can_id;  // CAN id number 2 - opt 3-temp 4-motor 5- display

  bool Open(void)  {return true; }
  bool Close(void) {return true; }
  bool Reset(void) {return true; }
  bool Cmd(char *cmd, char *reply, int buflen);
signals:
   void getBar();
   void sendBarCode(QString);
public slots:
  void getBARCode(void);
private:
  TSerialScanner *scanner;
  bool useSerialScanner;
};
#endif // CANSRV_H

