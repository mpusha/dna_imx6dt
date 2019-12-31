//-------------------------------------------------USB--------------------------------------
#include "fx2usb.h"

namespace usbfx2obj
{
  TFx2Usb *usbfx2;
}

TFx2Usb::TFx2Usb(int DT_VENDOR_ID, int DT_PRODUCT_ID)
{
  using namespace usbfx2obj;
  vendorID=DT_VENDOR_ID;
  productID=DT_PRODUCT_ID;
  syslog( LOG_INFO, "TUsb object initialisation start. vID=%x pID=%x",vendorID,productID);
  qDebug()<<"TUsb object initialisation start with pID vID"<<hex<<vendorID<<productID;
  usb_handle=0;
  usb_tout=5000;
  //libUsbOpen=false;
  arrive=false;
  left=false;

  libusb_hotplug_register_callback(NULL,  (libusb_hotplug_event )(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED |
                                      LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT), (libusb_hotplug_flag) 0, vendorID, productID,
                                      LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL,
                                      &callback_handle);
  usbfx2=this;

  QObject::connect(this,SIGNAL(sendHotplugArrive()),this,SLOT(getHotPlugArrive()));
  QObject::connect(this,SIGNAL(sendHotplugLeft()),this,SLOT(getHotPlugLeft()));
}

TFx2Usb::~TFx2Usb()
{
  libusb_hotplug_deregister_callback(NULL, callback_handle);
  if(usb_handle) {
    libusb_close(usb_handle);
  }

  syslog( LOG_INFO, "TFx2Usb object was deleted.");
  qDebug()<<"TFx2Usb object was deleted.";
}
bool TFx2Usb::VendRead(unsigned char *buf, int len, unsigned char req_code,unsigned char value,unsigned char index)
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

bool TFx2Usb::VendWrite(unsigned char *buf, int len, unsigned char req_code, unsigned char value,unsigned char index)
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

bool TFx2Usb::BulkWrite(unsigned char * buf,unsigned int buflen)
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

bool TFx2Usb::BulkRead(unsigned char * buf,unsigned int buflen)
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
bool TFx2Usb::BulkClearRead(void)
{
  qDebug()<<"BulkClearRead";
  if(usb_handle) {
    libusb_clear_halt(usb_handle,0x86);
    return(true);
  }
  else
    return(false);
}
bool TFx2Usb::BulkClearWrite(void)
{
  qDebug()<<"BulkClearWrite";
  if(usb_handle) {
    libusb_clear_halt(usb_handle,0x02);
    return(true);
  }
  else
    return(false);
}

bool TFx2Usb::Open(void)
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

bool TFx2Usb::isOpen(void)
{
  return(usb_handle!=0);
}

bool TFx2Usb::Close(void)
{
  if(usb_handle) {
    libusb_release_interface(usb_handle,0);
    libusb_close(usb_handle);
    usb_handle=0;
    return true;
  }else
    return false;
}


int TFx2Usb::hotplug_callback(struct libusb_context *ctx, struct libusb_device *dev,
                     libusb_hotplug_event event, void *user_data)
{
 // static libusb_device_handle *dev_handle = NULL;
  using namespace usbfx2obj;
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
   //usbComm->arrive=true;

   emit usbfx2->sendHotplugArrive();
  } else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event) {
      //usbComm->Close();
      //usbComm->left=true;
      //usbComm->usb_handle=0;
   emit usbfx2->sendHotplugLeft();

    //if (dev_handle) {
    //  libusb_close(dev_handle);
    //  dev_handle = NULL;
    //}
  } else {
    printf("Unhandled event %d\n", event);
  }
  //count++;
  //qDebug()<<"callback"<<usbComm->left<<usbComm->arrive;
  return 0;
}
//return 1 if device left 2 if device arrive 0 if not changes
int TFx2Usb::leftArriveDev(void)
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
void TFx2Usb::getHotPlugArrive(void)
{
  printf("SLOT FX2 Device arrive\n");
  Open();
  arrive=true;
}
void TFx2Usb::getHotPlugLeft(void)
{
  Close();
  left=true;
  printf("SLOT FX2 device left\n");
}
