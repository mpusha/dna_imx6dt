#ifndef CANCLIENT_H
#define CANCLIENT_H

#include <QThread>
#include <QObject>
//#include <QTimer>
//#include <QSerialPort>

//#include <syslog.h>
#include "can_commsock.h"

//int hotplug_callback(struct libusb_context *ctx, struct libusb_device *dev,
//                   libusb_hotplug_event event, void *user_data);
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



#endif // COMMOBJECT_H
