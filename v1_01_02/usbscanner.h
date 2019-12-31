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

  //int leftArriveDev(void);
  static int hotplug_callback(struct libusb_context *ctx, struct libusb_device *dev,
                       libusb_hotplug_event event, void *user_data);

protected:
  void run();

signals:
  void sendHotplugArrive(void);
  void sendHotplugLeft(void);
  void sendBarCode(QString);

private:
  int vendorID,productID;
 // QTimer timer;

  hid_device *usb_handle;
  libusb_hotplug_callback_handle callback_handle;
  bool arrive, left;
private slots:
  void getHotPlugArrive(void);
  void getHotPlugLeft(void);
};
#endif // HIDUSB_H

