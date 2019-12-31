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

  //libUsbOpen=false;
  arrive=false;
  left=false;
//vendorID=0xc2e;productID=0x200;
  vendorID=0x0;productID=0x0;
  libusb_hotplug_register_callback(NULL,  (libusb_hotplug_event )(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED |
                                      LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT), (libusb_hotplug_flag) 0,
                                      LIBUSB_HOTPLUG_MATCH_ANY  , LIBUSB_HOTPLUG_MATCH_ANY  ,
                                      LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL,
                                      &callback_handle);
  usbHid=this;

  QObject::connect(this,SIGNAL(sendHotplugArrive()),this,SLOT(getHotPlugArrive()));
  QObject::connect(this,SIGNAL(sendHotplugLeft()),this,SLOT(getHotPlugLeft()));

}

THidUsb::~THidUsb()
{
  libusb_hotplug_deregister_callback(NULL, callback_handle);
  if(usb_handle) {
    hid_close(usb_handle);
  }
  hid_exit();

  //syslog( LOG_INFO, "TUsb object delete.");
  qDebug()<<"THidUsb object delete.";
}
void THidUsb::run(void)
{

  QEventLoop *loop ;
  loop= new QEventLoop();
  hid_init();
  getInfo();
  Open();
  int res;
  unsigned char buf[256];
  QByteArray byte_array;
  QString barcode;
  uint16_t  scancode;
  char* c_ptr;
  bool shiftKey=false;

  while(1){
    loop->processEvents();
    msleep(1000);
    res = 0;
    if(isOpen()){
      while(!(res = hid_read_timeout(usb_handle, buf, 16,100))) {
        loop->processEvents();msleep(100);
      }
    }
    if(res<0) Close();
    if(!isOpen()) continue ;
    byte_array.clear();
    barcode.clear();
    while (res) {
      byte_array.append((char*)buf,res);
      res = hid_read_timeout(usb_handle, buf, 16,10);
      if(res<0) { Close(); break; }
    }
    if(!isOpen()) continue ;
    for(int i=0; i<byte_array.size()/sizeof(scancode); i++) {
      c_ptr = (char*) &scancode;
      for(int n=0; n<sizeof(scancode); n++) *(c_ptr++) = byte_array.at(i*sizeof(scancode) + n);
      if(!(i%8)) shiftKey=false;

      switch(scancode){
        case KEY_0: barcode += "0"; break;
        case KEY_1: barcode += "1"; break;
        case KEY_2: barcode += "2"; break;
        case KEY_3: barcode += "3"; break;
        case KEY_4: barcode += "4"; break;
        case KEY_5: barcode += "5"; break;
        case KEY_6: barcode += "6"; break;
        case KEY_7: barcode += "7"; break;
        case KEY_8: barcode += "8"; break;
        case KEY_9: barcode += "9"; break;

        case KEY_A: barcode += "A"; break;
        case KEY_B: barcode += "B"; break;
        case KEY_C: barcode += "C"; break;
        case KEY_D: barcode += "D"; break;
        case KEY_E: barcode += "E"; break;

        case KEY_F: barcode += "F"; break;
        case KEY_G: barcode += "G"; break;
        case KEY_H: barcode += "H"; break;
        case KEY_I: barcode += "I"; break;
        case KEY_J: barcode += "J"; break;

        case KEY_K: barcode += "K"; break;
        case KEY_L: barcode += "L"; break;
        case KEY_M: barcode += "M"; break;
        case KEY_N: barcode += "N"; break;
        case KEY_O: barcode += "O"; break;

        case KEY_P: barcode += "P"; break;
        case KEY_Q: barcode += "Q"; break;
        case KEY_R: barcode += "R"; break;
        case KEY_S: barcode += "S"; break;
        case KEY_T: barcode += "T"; break;

        case KEY_U: barcode += "U"; break;
        case KEY_V: barcode += "V"; break;
        case KEY_W: barcode += "W"; break;
        case KEY_X: barcode += "X"; break;
        case KEY_Y: barcode += "Y"; break;
        case KEY_Z: barcode += "Z"; break;

        case KEY_SLASH: if(shiftKey) barcode +="_"; else barcode +="-";
        case KEY_MOD_LSHIFT: shiftKey=true; break;
        case KEY_ENTER: emit sendBarCode(barcode); break;
      }
    }
//    qDebug()<<"BC="<<barcode;
  }
  qDebug()<<"Run USBHID finished.";
}

void THidUsb::getInfo(void)
{
  struct hid_device_info *devs, *cur_dev;
  vendorID=0;productID=0;
  devs = hid_enumerate(0x0, 0x0);
  cur_dev = devs;
  QRegExp rx1("\\b(Honeywell)\\b",Qt::CaseInsensitive),rx2("\\b|\\b(Scanner)\\b",Qt::CaseInsensitive);
  while (cur_dev) {
    qDebug()<<"Device Found"<<hex<<cur_dev->vendor_id<<cur_dev->product_id<<cur_dev->path<<cur_dev->serial_number;
    printf("\n");
    printf("  Manufacturer: %ls\n", cur_dev->manufacturer_string);
    printf("  Product:      %ls\n", cur_dev->product_string);
    printf("  Release:      %hx\n", cur_dev->release_number);
    printf("  Interface:    %d\n",  cur_dev->interface_number);
    printf("\n");
    QString str1=QString::fromWCharArray(cur_dev->manufacturer_string);
    QString str2=QString::fromWCharArray(cur_dev->product_string);
    if(str1.contains(rx1)&&str2.contains(rx2))
    {
      vendorID=cur_dev->vendor_id; productID=cur_dev->product_id;
      break;
    }
    cur_dev = cur_dev->next;
  }
  hid_free_enumeration(devs);
}

bool THidUsb::Open(void)
{
  if(usb_handle) return true;
  qDebug()<<"Open HID device with vID pID"<<hex<<vendorID<<productID ;
  if((vendorID!=0)&&(productID!=0))
    usb_handle=hid_open(vendorID, productID, NULL);
  else
    usb_handle=0;
  return(usb_handle!=0);
}

bool THidUsb::isOpen(void)
{
  return(usb_handle!=0);
}

bool THidUsb::Close(void)
{
  if(usb_handle) {
    hid_close(usb_handle);
    usb_handle=0;
    return true;
  }else
    return false;
}


int THidUsb::hotplug_callback(struct libusb_context *ctx, struct libusb_device *dev,
                     libusb_hotplug_event event, void *user_data)
{
  using namespace usbhidobj;
  if (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == event) {
    emit usbHid->sendHotplugArrive();
  }
  else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event) {
    emit usbHid->sendHotplugLeft();
  }
  return 0;
}

void THidUsb::getHotPlugArrive(void)
{
  qDebug()<<"USB Device arrive";
  arrive=true;
  getInfo();
  Open();
}
void THidUsb::getHotPlugLeft(void)
{
  Close();
  left=true;
  qDebug()<<"USB Device left";
}
