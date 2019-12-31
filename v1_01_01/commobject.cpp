#include <QDebug>

#include "commobject.h"


//-------------------------------------------------CAN server TFT--------------------------------------
namespace commobj
{
  TCANSrvCh *extComm;
}
// class for TFT contoller on CAN bus emulations from side server.
// and CAN function realisation from others controllers
// BarCode function realisation
TCANSrvCh::TCANSrvCh(int id, QString devN)
{
    using namespace commobj;

    scanner=new TScanner(devN);
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
}

// parse command string from CAN to display controller send from others controller
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
  else if(!strncmp((const char *)msg,"DRBC",4)) {
    sprintf(reply,"DY>%s",extComm->barCodeStr.toLatin1().data());
    return 1;
  }
  else if(!strncmp((const char *)msg,"DSAV 1",6)) {// set button enable in menu
    sprintf(reply,"DY>Ok");
    return 1;
  }
  else if(!strncmp((const char *)msg,"DRAV",4)) {// set button enable in menu
    sprintf(reply,"DY>Ok");
    return 1;
  }
  else if(!strncmp((const char *)msg,"DBCR",4)) {// set button enable in menu{
    extComm->getBARCode();
    sprintf(reply,"DY>%s",extComm->barCodeStr.toLatin1().data());
    return 1;
  }
  else
    return -1;  // not known cmd
  return 0;
}

// answer on command from CAN to display controller send to others controllers
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
      CAN_SendEventN(CEV_SYNCRPL,5); //!!!
      //CAN_SendEventN(CEV_SYNCRPL,4); //!!!
    break;
    case CEV_SYNCRPL:
      break;
    }
}

// get BarCode from serial device
void TCANSrvCh::getBARCode(void)
{
  scanner->startScan();
  scanner->readScanData(barCodeStr);
  qDebug()<<"BarCode"<<barCodeStr;
}

//-------------------------------------------------Bar Code reader--------------------------------------
// Serial scanner device function
TScanner::TScanner(QString devN)
{
  //sp=new QSerialPort("/dev/ttymxc2");
  QSerialPort::setPortName(devN);
  QSerialPort::setBaudRate(QSerialPort::Baud9600);
  QSerialPort::open(QIODevice::ReadWrite);
  qDebug()<<"TScanner object create for Bar Code Reader device"<<devN;
  //startScan();
  //printf("pointer %p\n",sp);
}

TScanner::~TScanner(void)
{
  QSerialPort::close();
  qDebug()<<"TScanner object delete.";
  //if(sp) delete sp;
}

bool TScanner::startScan(void)
{
  QSerialPort::putChar('E');
  return true;
}
bool TScanner::readScanData(QString &data)
{
  if(QSerialPort::waitForReadyRead(1000))
  {
    data.clear();
    data.append(QSerialPort::readAll());
    return true;
  }
  else{
    data.clear();
    return false;
  }
}

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
  qDebug()<<"TScanner object delete"<<can_id;
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

//-------------------------------------------------USB--------------------------------------
namespace usbobj
{
  TUsb *usbComm;
}
TUsb::TUsb(int DT_VENDOR_ID, int DT_PRODUCT_ID)
{
  using namespace usbobj;
  vendorID=DT_VENDOR_ID;
  productID=DT_PRODUCT_ID;
  syslog( LOG_INFO, "TUsb object initialisation start. vID=%x pID=%x",vendorID,productID);
  qDebug()<<"TUsb object initialisation start with pID vID"<<hex<<vendorID<<productID;
  usb_handle=0;
  usb_tout=5000;
  libUsbOpen=false;
  arrive=false;
  left=false;
  qDebug()<<"Initialise libusb";
  if(libusb_init(NULL)>=0) libUsbOpen=true;
  libusb_hotplug_register_callback(NULL,  (libusb_hotplug_event )(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED |
                                      LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT), (libusb_hotplug_flag) 0, vendorID, productID,
                                      LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL,
                                      &callback_handle);
  usbComm=this;
  qDebug()<<"libusb was initialised";
}

TUsb::~TUsb()
{
  if(libUsbOpen){
    libusb_hotplug_deregister_callback(NULL, callback_handle);
    if(usb_handle) {
      libusb_close(usb_handle);
    }
    libusb_exit(NULL);
  }
  syslog( LOG_INFO, "TUsb object delete.");
  qDebug()<<"TUsb object destructor delete.";
}
bool TUsb::VendRead(unsigned char *buf, int len, unsigned char req_code,unsigned char value,unsigned char index)
{
  int rc;
  if(usb_handle){
    rc=libusb_control_transfer(usb_handle,
                    LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR, // VENDOR,read
                    req_code,
                    value,index,
                    buf,len,
                    usb_tout);
     if (rc<0) return false;
     return true;
   }
   return false;
}

bool TUsb::VendWrite(unsigned char *buf, int len, unsigned char req_code, unsigned char value,unsigned char index)
{
  int rc;
  if(usb_handle){
    rc=libusb_control_transfer(usb_handle,
                    LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR, // VENDOR, write
                    req_code,
                    value,index,
                    buf,len,
                    usb_tout);
    if (rc<0) return false;
    return true;
  }
  return false;
}

bool TUsb::BulkWrite(unsigned char * buf,unsigned int buflen)
{qDebug()<<"BulkWrite";
  if(usb_handle) {
    int transferred;
    libusb_bulk_transfer(usb_handle,
    0x02, //endpoint 2
    buf,
    buflen,
    &transferred,
    usb_tout
    );
    return true;
  }
  return false;
}

bool TUsb::BulkRead(unsigned char * buf,unsigned int buflen)
{qDebug()<<"BulkRead";
  if(usb_handle) {
    int transferred;
    libusb_bulk_transfer(usb_handle,
    0x86, //endpoint 86
    buf,
    buflen,
    &transferred,
    usb_tout
    );
    return true;
  }
  return false;
}
bool TUsb::BulkClearRead(void)
{
  qDebug()<<"BulkClearRead";
  if(usb_handle) {
    libusb_clear_halt(usb_handle,0x86);
    return(true);
  }
  else
    return(false);
}
bool TUsb::BulkClearWrite(void)
{
  qDebug()<<"BulkClearWrite";
  if(usb_handle) {
    libusb_clear_halt(usb_handle,0x02);
    return(true);
  }
  else
    return(false);
}

bool TUsb::Open(void)
{
  if(usb_handle) return true;
  usb_handle=libusb_open_device_with_vid_pid(0,vendorID,productID);

  if(usb_handle){
    libusb_claim_interface(usb_handle,0);
    struct libusb_device_descriptor desc;
    int sn_idx;
    unsigned char buf[100];
    libusb_get_device_descriptor(libusb_get_device(usb_handle), &desc);

    sn_idx=desc.iSerialNumber;
    libusb_get_string_descriptor_ascii(usb_handle,sn_idx,buf,100);
    serNum=QString::fromLocal8Bit((const char *)buf);

  }
  return(usb_handle!=0);
}

bool TUsb::isOpen(void)
{
  return(usb_handle!=0);
}

bool TUsb::Close(void)
{
  if(usb_handle) {
    libusb_release_interface(usb_handle,0);
    libusb_close(usb_handle);
    usb_handle=0;
    return true;
  }else
    return false;
}


int TUsb::hotplug_callback(struct libusb_context *ctx, struct libusb_device *dev,
                     libusb_hotplug_event event, void *user_data)
{
 // static libusb_device_handle *dev_handle = NULL;
  using namespace usbobj;
  struct libusb_device_descriptor desc;
  (void)libusb_get_device_descriptor(dev, &desc);
  if (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == event) {
    //VID=desc.idVendor; PID=desc.idProduct;
    //libusb_get_string_descriptor_ascii(usb_handle,sn_idx,buf1,100);
   // rc = libusb_open(dev, &dev_handle);
    //if (LIBUSB_SUCCESS != rc) {
     // printf("Could not open USB device\n");
    //}

   //usbComm->Open();
   usbComm->arrive=true;
   printf("Device arrive\n");
  } else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event) {
      usbComm->Close();
      usbComm->left=true;
      //usbComm->usb_handle=0;
 printf("Device LEFT\n");
    //if (dev_handle) {
    //  libusb_close(dev_handle);
    //  dev_handle = NULL;
    //}
  } else {
    printf("Unhandled event %d\n", event);
  }
  //count++;
  return 0;
}
//return 1 if device left 2 if device arrive 0 if not changes
int TUsb::leftArriveDev(void)
{
  //int comp=1;
  //libusb_handle_events_completed(NULL, &comp); //process leave/arrive device
  struct timeval ti;
  ti.tv_sec=0;ti.tv_usec=0;
  libusb_handle_events_timeout_completed(NULL,&ti,NULL);
  bool tmpLeft=left,tmpArrive=arrive;
  if(left) left=false;
  if(arrive) arrive=false;
  if(tmpLeft) return 1;
  else if(tmpArrive) return 2;
  return 0;
}


