#ifndef HIDUSB_H
#define HIDUSB_H

#include <QtCore>
#include <QThread>
#include <libusb-1.0/libusb.h>
#include <syslog.h>
#include "hidapi.h"
#include "usb_hid_keys.h"

class THidUsb : public QThread
{
Q_OBJECT
public:
  THidUsb(void);
  ~THidUsb(void );

  void getInfo(void);
  bool Open(void);
  bool isOpen(void);
  bool Close(void);
  void setAbort(bool a) { abort=1;}

protected:
  void run();

signals:

  void sendBarCode(QString);

private:
  int vendorID,productID;
 // QTimer timer;

  hid_device *usb_handle;
  bool abort;

public slots:
  void usbDeviceArrive(void);
  void usbDeviceLeft(void);

};
#endif // HIDUSB_H



