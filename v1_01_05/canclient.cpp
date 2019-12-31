#include <QDebug>

#include "canclient.h"


//-------------------------------------------------CAN--------------------------------------
// class for CAN function realisation. On request from others controllers
TCANChannels::TCANChannels( int id, int tout)
{

    CANch = -1;
    can_id = id;
    can_tout = tout;
    qDebug()<<"TCANChannels object create. ID="<<id;
}

TCANChannels::~TCANChannels(void)
{
    if (!Close()) {
        Reset();  // reset channel to close it
        Close();
    }
  qDebug()<<"TCANChannels object delete"<<can_id;
}

bool TCANChannels::Open(void)
{
  CANch = CAN_OpenChan(can_id); // if <0 error with open chanal
  if (CANch<0) { // can't open channel
    return false;
  }
  return true;
}

bool TCANChannels::Close(void)
{
  signed char res;

  if (CANch<0) {
    return true; // channel is already closed
  }
  res=CAN_CloseChan(CANch);
  if (res<0) { // can't close channel
    return false;
  }
  CANch = -1;
  return true;
}

bool TCANChannels::Reset(void)
{
  signed char res;

  if (CANch<0) {
    return true; // channel is closed
  }
  res=CAN_ResetChan(CANch);
  if (res<0) { // can't reset channel
    return false;
  }
  return true;
}

bool TCANChannels::Cmd(char *cmd,char * reply,int buflen)
{
  strcpy(reply,cmd);
  int ret = CAN_ClientChan(CANch, reply, buflen, can_tout); // buflen bytes length timeout 5 s for back answer
  if (ret<0) return false;
  return true;
 }




