#include "usbscanner.h"
namespace usbhidobj
{
  THidUsb *usbHid;
}
THidUsb::THidUsb(void)
{
  using namespace usbhidobj;
  //vendorID=DT_VENDOR_ID;
  //productID=DT_PRODUCT_ID;
  //syslog( LOG_INFO, "TUsb object initialisation start. vID=%x pID=%x",vendorID,productID);
  //qDebug()<<"TUsb object initialisation start with pID vID"<<hex<<vendorID<<productID;
  usb_handle=0;
  usb_tout=5000;
  //libUsbOpen=false;
  arrive=false;
  left=false;
//vendorID=0xc2e;productID=0x200;
//vendorID=0x0;productID=0x0;
  libusb_hotplug_register_callback(NULL,  (libusb_hotplug_event )(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED |
                                      LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT), (libusb_hotplug_flag) 0,
                                      LIBUSB_HOTPLUG_MATCH_ANY  , LIBUSB_HOTPLUG_MATCH_ANY  ,
                                      LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL,
                                      &callback_handle);
  usbHid=this;

  QObject::connect(this,SIGNAL(sendHotplugArrive()),this,SLOT(getHotPlugArrive()));
  QObject::connect(this,SIGNAL(sendHotplugLeft()),this,SLOT(getHotPlugLeft()));
  qDebug()<<"libusb was initialised";

}

THidUsb::~THidUsb()
{
  libusb_hotplug_deregister_callback(NULL, callback_handle);
  if(usb_handle) {
    libusb_close(usb_handle);
  }

  //syslog( LOG_INFO, "TUsb object delete.");
  qDebug()<<"THidUsb object delete.";
}
void THidUsb::run(void)
{
  QEventLoop *loop ;
  loop= new QEventLoop();
  while(1){
    loop->processEvents();
    msleep(100);
  }
}

void THidUsb::getInfo(void)
{
  int list = libusb_get_device_list(0, &devs);

  struct libusb_device_descriptor desc;
      struct libusb_config_descriptor *config;
      const struct libusb_interface *inter;
      const struct libusb_interface_descriptor *interdesc;
      const struct libusb_endpoint_descriptor *endpointdesc;
  for(int iter = 0; iter < list; iter++)
  {
      /* print devices specs */




int ret;
int i,j,k;

ret = libusb_get_device_descriptor(devs[iter], & desc);
if(ret < 0)
{
    fprintf(stderr, "error in getting device descriptor\n");
    return;
}

printf("Number of possible configs is %d\n",desc.bNumConfigurations);
printf("Vendor ID  : 0x%x\n", desc.idVendor);
printf("Product ID : 0x%x\n", desc.idProduct);
printf("Device class %d subclass %d\n",desc.bDeviceClass,desc.bDeviceSubClass);
char buf[100];
int man_i=desc.iManufacturer,prod_i=desc.iProduct;
usb_handle=libusb_open_device_with_vid_pid(0,desc.idVendor,desc.idProduct);
if(usb_handle){
if(libusb_claim_interface(usb_handle,0)==0){
libusb_get_string_descriptor_ascii(usb_handle,man_i,(unsigned char*)buf,100);
QString s(buf);
qDebug()<<"man"<<s;
s.clear();
libusb_get_string_descriptor_ascii(usb_handle,prod_i,(unsigned char*)buf,100);
QString s1(buf);
qDebug()<<"prod"<<s1;
s1.clear();
libusb_get_descriptor(usb_handle,LIBUSB_DT_INTERFACE  ,1,(unsigned char*)buf,100);
QString s2(buf);
qDebug()<<"xx"<<s2;
s2.clear();
libusb_release_interface(usb_handle,0);
}

libusb_close(usb_handle);
usb_handle=0;
}


libusb_get_config_descriptor(devs[iter], 0, &config);
printf("Interface %d\n", config->bNumInterfaces);

/*for(i=0; i < config->bNumInterfaces; i++)
{
    inter = &config->interface[i];
    printf("Number of alternate settings : %d\n", inter->num_altsetting);
    for(j=0; j < inter->num_altsetting; j++)
    {
        interdesc = &inter->altsetting[j];
        printf("   Interface number : %d, ", interdesc->bInterfaceNumber);
        printf("   Num of endpoints : %d\n", interdesc->bNumEndpoints);
        for(k=0; k < interdesc->bNumEndpoints; k++)
        {
            endpointdesc = &interdesc->endpoint[k];
            printf("     Desc type : %d ",endpointdesc->bDescriptorType);
            printf("      EP Addr: %d\n", endpointdesc->bEndpointAddress);
        }
    }
}*/
  }
printf("\n\n");
libusb_free_config_descriptor(config);
}

bool THidUsb::Open(void)
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
    //serNum=QString::fromLocal8Bit((const char *)buf);

  }
  return(usb_handle!=0);
}

bool THidUsb::isOpen(void)
{
  return(usb_handle!=0);
}

bool THidUsb::Close(void)
{
  if(usb_handle) {
    libusb_release_interface(usb_handle,0);
    libusb_close(usb_handle);
    usb_handle=0;
    return true;
  }else
    return false;
}


int THidUsb::hotplug_callback(struct libusb_context *ctx, struct libusb_device *dev,
                     libusb_hotplug_event event, void *user_data)
{
 // static libusb_device_handle *dev_handle = NULL;
  using namespace usbhidobj;
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
   emit usbHid->sendHotplugArrive();
  } else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event) {
emit usbHid->sendHotplugLeft();
      //usbComm->usb_handle=0;

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
int THidUsb::leftArriveDev(void)
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
void THidUsb::getHotPlugArrive(void)
{
  printf("SLOT Scanner Device arrive\n");
  left=true;
}
void THidUsb::getHotPlugLeft(void)
{
  //Close();
  left=true;
  printf("Scanner device SLOT LEFT\n");
}
