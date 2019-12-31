#ifndef COMMOBJECT_H
#define COMMOBJECT_H

#include <QThread>
#include <QObject>
#include <QSerialPort>
#include <libusb-1.0/libusb.h>
#include "can_commsock.h"

#define USB_EVENT_INTERVAL 1000

int CANEventHandler(unsigned char,unsigned char);

class TScanner: public QSerialPort
{
//Q_OBJECT
public:
  TScanner(void);
  ~TScanner(void);
  bool startScan(void);
  bool readScanData(QString &data);
private:

};

class Sleeper : public QThread
{
  Q_OBJECT
public:
  static void msleep(int ms) {QThread::msleep(ms);}
};

// CAN server for display answer and recive events from CAN
class TCANSrvCh : public QObject
{
  Q_OBJECT
public:
  TCANSrvCh(int can_id=2);
  ~TCANSrvCh(void);
  void eventCANHandler(unsigned char evid,unsigned char srcnode);
  static int CMDparse(unsigned char *msg, unsigned char *reply,int TrID);
  static void cmds(int chan, unsigned char *cmd);
  QString barCodeStr;
  int can_id;  // CAN id number 2 - opt 3-temp 4-motor 5- display

  bool Open(void)  {return true; }
  bool Close(void) {return true; }
  bool Reset(void) {return true; }
  bool Cmd(char *cmd, char *reply, int buflen);
signals:
   void getBar();
public slots:
  void getBARCode(void);
private:
  TScanner *scanner;
};

int hotplug_callback(struct libusb_context *ctx, struct libusb_device *dev,
                   libusb_hotplug_event event, void *user_data);
class TCANChannels
{
public:
  TCANChannels(int can_id=2, int tout=5);
  ~TCANChannels(void );

  int can_id;  // CAN id number 2 - opt 3-temp 4-motor 5- display

  bool Open(void);
  bool Close(void);
  bool Reset(void);
  bool Cmd(char *cmd, char *reply, int buflen);

private:
  int can_tout; //timeout on CAN
  int CANch;   // CAN Channel number
};

class TUsb : public QObject
{
Q_OBJECT
public:
  TUsb(int DT_VENDOR_ID=0x199a,int DT_PRODUCT_ID=0x1964);
  ~TUsb(void );

  bool VendRead(unsigned char *buf, int len, unsigned char req_code,unsigned char value,unsigned char index);
  bool VendWrite(unsigned char *buf, int len, unsigned char req_code, unsigned char value,unsigned char index);
  bool BulkWrite(unsigned char * buf,unsigned int buflen);
  bool BulkRead(unsigned char * buf,unsigned int buflen);
  bool BulkClear(void);
  bool Open(void);
  bool isOpen(void);
  bool Close(void);
  static int hotplug_callback(struct libusb_context *ctx, struct libusb_device *dev,
                       libusb_hotplug_event event, void *user_data);
  void start(void){tID=startTimer(USB_EVENT_INTERVAL); }
  QString serNum;
  uint16_t PID,VID;
protected:
  void timerEvent(QTimerEvent *);
private:
  int vendorID,productID; //DNA DT-48/192/96/384
  bool libUsbOpen;
  libusb_device_handle *usb_handle;
  libusb_hotplug_callback_handle callback_handle;
  int usb_tout; //timeout in ms
  int tID;
};



#endif // COMMOBJECT_H
