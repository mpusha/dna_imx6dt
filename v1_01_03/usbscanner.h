#ifndef HIDUSB_H
#define HIDUSB_H

#include <QtCore>
#include <QThread>
#include <linux/input.h>

//#include "usb_hid_keys.h"

#define EV_MAKE 1

class THidUsb : public QObject
{
Q_OBJECT
public:
  THidUsb(void);
  ~THidUsb(void );


protected:
//  void run();

signals:
  void sendHotplugArrive(void);
  void sendHotplugLeft(void);
  void sendBarCode(QString);

private:
  int vendorID,productID;
  QProcess *get_device_process, *scan_process;
  QStringList listManufacturers,listProducts;
  QString barcode;
 // QTimer timer;

public slots:
  void usbDeviceArrive(void);
private slots:
  void scanOutputReady(void);
  void getDeviceProcessFinished(void);

};
#endif // HIDUSB_H

