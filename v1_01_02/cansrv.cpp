#include "cansrv.h"

//-------------------------------------------------CAN server TFT--------------------------------------
namespace commobj
{
  TCANSrvCh *extComm;
}
// class for TFT contoller on CAN bus emulations from side server.
// and CAN function realisation from others controllers
// BarCode function realisation
TCANSrvCh::TCANSrvCh(int id, QString devN,bool useSerialScannerDev)
{
    using namespace commobj;
    useSerialScanner=useSerialScannerDev;
    scanner=new TSerialScanner(devN,useSerialScanner);
    barCodeStr="";
    extComm=this;
    CAN_init(cmds,NULL);
    CAN_ServerEvent = (CANEventHandler);
    QObject::connect(this,SIGNAL(getBar()),this,SLOT(getBARCode()));
    can_id=id;
    qDebug()<<"TCANSRVch object create. ID="<<id;
}

TCANSrvCh::~TCANSrvCh(void)
{
  using namespace commobj;
  qDebug()<<"TCANSRVch object delete.";
  if(scanner) delete scanner;
  CAN_exit();
}

// parse command string from display controller send the others controllers
int TCANSrvCh::CMDparse(unsigned char *msg, char *reply,int TrID)
{
  uint i;using namespace commobj;
  printf("Req controller %d %s\n",TrID,msg);
  for (i=0;i<strlen((const char*)msg);i++) msg[i] = toupper(msg[i]);

// comands without reply

// comands with reply
  if (!strncmp((const char *)msg,"FVER",4)) {
     sprintf((char*)reply,"TFT %s %s",APP_VERSION,__DATE__);
     return 1;
  }
//  else if(!strncmp((const char *)msg,"DRBC",4)) {
//    sprintf(reply,"DY>%s",extComm->barCodeStr.toLatin1().data());
//    return 1;
//  }
  else if(!strncmp((const char *)msg,"DSAV 1",6)) {// set button enable in menu
    sprintf(reply,"DY>Ok");
    return 1;
  }
  else if(!strncmp((const char *)msg,"DRAV",4)) {// set button enable in menu
    sprintf(reply,"DY>Ok");
    return 1;
  }
//  else if(!strncmp((const char *)msg,"DBCR",4)) {// set button enable in menu{
//    extComm->getBARCode();
//    sprintf(reply,"DY>%s",extComm->barCodeStr.toLatin1().data());
//    return 1;
//  }
  else
    return -1;  // not known cmd
  return 0;
}

// CAN answer on command from display controller send the others controllers
void TCANSrvCh::cmds(int chan, unsigned char *cmd)
{
    char str[80];
    int ret;
    if ((ret=CMDparse(cmd,(char *)(str+3),CAN_GetCID(chan)))>=0) {
      if (!ret) CAN_SendString(chan,(unsigned char *)"DY>Ok",1); // send ACK reply
      else {
        strncpy(str,"DY>",3);
        CAN_SendString(chan,(unsigned char *)str,1); // send data reply
      }
    }
    else {
      sprintf(str,"DY>?%s",cmd);
      CAN_SendString(chan,(unsigned char *)str,1);
    }
}

// answer on command from server (emulate CAN from server request side)
bool TCANSrvCh::Cmd(char *cmd, char *reply, int buflen)
{
  if(buflen<60) return false;
  for (uint i=0;i<strlen((const char*)cmd);i++) cmd[i] = toupper(cmd[i]);
  // comands with reply
  if (!strncmp((const char *)cmd,"FVER",4))
    sprintf(reply,"DY>TFT %s %s",APP_VERSION,__DATE__);
  else if(!strncmp((const char *)cmd,"DRBC",4))
    sprintf(reply,"DY>%s",barCodeStr.toLatin1().data());
  // comands with reply
  else if(!strncmp((const char *)cmd,"DSAV 1",6)) // set button enable in menu
    sprintf(reply,"DY>Ok");
  else if(!strncmp((const char *)cmd,"DRAV",4)) // set button enable in menu
    sprintf(reply,"DY>Ok");
  else if(!strncmp((const char *)cmd,"DBCR",4)) {// set button enable in menu{
    getBARCode();
    sprintf(reply,"DY>%s",barCodeStr.toLatin1().data());
  }
  else
    sprintf(reply,"DY>?%s",cmd);
  return true;
}

// CAN event function
int  CANEventHandler(unsigned char evid,unsigned char srcnode)
{
  using namespace commobj;
  extComm->eventCANHandler(evid,srcnode); // call wrapper
  return 0;
}

// CAN event wrapper
void TCANSrvCh::eventCANHandler(unsigned char evid,unsigned char srcnode)
{
    printf("Event %d from %d\n",evid,srcnode);
    switch (evid) {
    case CEV_PICTEMP:
       break;
    case CEV_CAPT:  // get picture frame, continue to run HET  to generate CCD control signals
      break;
    case CEV_WHEELRDY:
      break;
    case CEV_READBC:
       emit getBar();
      break;
    case CEV_TPROGCNT:
      break;
    case CEV_TPROGSTR:
      break;
    case CEV_SYNCREQ:
      //CAN_SendEvent(CEV_SYNCRPL);
      //CAN_SendEventN(CEV_SYNCRPL,5); //!!!
      //CAN_SendEventN(CEV_SYNCRPL,4); //!!!
    break;
    case CEV_SYNCRPL:
      break;
    }
}

// get BarCode from serial device
void TCANSrvCh::getBARCode(void)
{
  if(useSerialScanner) {
    scanner->startScan();
    scanner->readScanData(barCodeStr);
  }
  //qDebug()<<"BarCode"<<barCodeStr;
  emit sendBarCode(barCodeStr);
}
