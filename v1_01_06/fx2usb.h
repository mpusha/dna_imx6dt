#ifndef FX2USB_H
#define FX2USB_H
#include <QtCore>
#include <syslog.h>
#include <libusb-1.0/libusb.h>

class TFx2Usb : public QObject
{
Q_OBJECT
public:
  TFx2Usb(int DT_VENDOR_ID=0x199a,int DT_PRODUCT_ID=0x1964);
  ~TFx2Usb(void );

  bool VendRead(unsigned char *buf, int len, unsigned char req_code,unsigned char value,unsigned char index);
  bool VendWrite(unsigned char *buf, int len, unsigned char req_code, unsigned char value,unsigned char index);
  bool BulkWrite(unsigned char * buf,unsigned int buflen);
  bool BulkRead(unsigned char * buf,unsigned int buflen);
  bool BulkClearRead(void);
  bool BulkClearWrite(void);

  bool Open(void);
  bool isOpen(void);
  bool Close(void);
  bool checkSpeed(void);

  int leftArriveDev(void);
  static int hotplug_callback(struct libusb_context *ctx, struct libusb_device *dev,
                       libusb_hotplug_event event, void *user_data);
  QString serNum;
//  uint16_t PID,VID;

signals:
  void sendFX2Arrive(void);
  void sendFX2Left(void);
  void sendUSBArrive(void);
  void sendUSBLeft(void);

private:
  int vendorID,productID; //DNA DT-48/192/96/384
//  QTimer timer;

  libusb_device_handle *usb_handle;
  libusb_hotplug_callback_handle callback_handle;
  int usb_tout;
  bool arrive, left;
private slots:
  void getFX2Arrive(void);
  void getFX2Left(void);
};

#endif // FX2USB_H

