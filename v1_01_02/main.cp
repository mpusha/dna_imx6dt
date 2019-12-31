#include <stdlib.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>

#include <stdint.h>

#include <QtCore/QCoreApplication>
#include <QDebug>

#include "commobject.h"
#include "usb_hid_keys.h"
#define EV_MAKE   1  // when key pressed

void print_devices(libusb_device *dev)
{
    struct libusb_device_descriptor desc;
    struct libusb_config_descriptor *config;
    const struct libusb_interface *inter;
    const struct libusb_interface_descriptor *interdesc;
    const struct libusb_endpoint_descriptor *endpointdesc;

int ret;
int i,j,k;

ret = libusb_get_device_descriptor(dev, & desc);
if(ret < 0)
{
    fprintf(stderr, "error in getting device descriptor\n");
    return;
}

printf("Number of possible configs is %d\n",desc.bNumConfigurations);
printf("Vendor ID  : 0x%x\n", desc.idVendor);
printf("Product ID : 0x%x\n", desc.idProduct);

libusb_get_config_descriptor(dev, 0, &config);

printf("Interface %d\n", config->bNumInterfaces);

for(i=0; i < config->bNumInterfaces; i++)
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
}
printf("\n\n");
libusb_free_config_descriptor(config);
}



int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
  int opt=1;

  libusb_device **devs; //pointer to pointer of device, used to retrieve a list of devices
     libusb_context *context = NULL; //a libusb session
     libusb_device_handle *dev_handle; //a device handle

 size_t list;
 size_t iter;
 int retVal;
 int kernelDriverDetached     = 1;  /* Set to 1 if kernel driver detached */

 retVal = libusb_init(NULL);//libusb_init(&context);
 if(retVal < 0)
 {
     perror("libusb_init");
     exit(1);
 }

 //libusb_set_debug(context, 3); //set verbosity level to 3, as suggested in the documentation

 list = libusb_get_device_list(/*context*/0, &devs);
 if(list < 0){
     fprintf(stderr, "Error in getting device list\n");
     libusb_free_device_list(devs, 1);
     libusb_exit(context);
     exit(1);
 }

 printf("There are %d devices found\n",list);

 for(iter = 0; iter < list; iter++)
 {
     /* print devices specs */
     print_devices(devs[iter]);
 }

 dev_handle = libusb_open_device_with_vid_pid(/*context*/0,  0x0c2e, 0x0200 /*0x1d6b,0x0002*/); //these are vendorID and productID I found for my usb device
     if(dev_handle == NULL) {
     fprintf(stderr, "Unable to open device.\n");
         return 1;

 }

 /* Check whether a kernel driver is attached to interface #0. If so, we'll
  * need to detach it.
  */
 if (libusb_kernel_driver_active(dev_handle, 0))
 {
         retVal = libusb_detach_kernel_driver(dev_handle, 0);
         if (retVal == 0)
         {
             kernelDriverDetached = 1;
         }
         else
         {
             fprintf(stderr, "Error detaching kernel driver.\n");
             return 1;
         }
 }

 /* Claim interface #0. */
 retVal = libusb_claim_interface(dev_handle, 0);
 if (retVal != 0)
 {
         fprintf(stderr, "Error claiming interface.\n");
      //   return 1;
 }

 printf("Scanner Device Opened\n");

   //  struct libusb_transfer *transfer = libusb_alloc_transfer(0);
 unsigned char buf[16];
 int actualbytes;
 unsigned char code[32];
 uint16_t  event;
 char* c_ptr;
 qDebug()<<"EV size="<<sizeof(event);
 QByteArray byte_array;
 libusb_interrupt_transfer(dev_handle, 0x81/*(1 | LIBUSB_ENDPOINT_IN)*/, buf, 16, &actualbytes, 0);

 while(actualbytes!=0){
   byte_array.append((char*)buf,actualbytes);
   libusb_interrupt_transfer(dev_handle, 0x81/*(1 | LIBUSB_ENDPOINT_IN)*/, buf, 16, &actualbytes, 100);
 }
 QString barcode;
 bool shiftKey=false;
 qDebug()<<byte_array.size();
 for(int i=0; i<byte_array.size()/sizeof(event); i++) {
   c_ptr = (char*) &event;
   for(int n=0; n<sizeof(event); n++) *(c_ptr++) = byte_array.at(i*sizeof(event) + n);
   if(!(i%8)) shiftKey=false;

   switch(event){
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
   }

//   if (event.type == EV_KEY && event.value == EV_MAKE) {
  // c_ptr = (char*) &event;

//   }
 }
       qDebug()<<"event"<<barcode;
 exit(0);
 int k=0,j=0,n;
 while(n<10){
 for(j = 0; j < 32; j++) code[j]=30;
 j=0;
 retVal = libusb_interrupt_transfer(dev_handle, 0x81/*(1 | LIBUSB_ENDPOINT_IN)*/, buf, 16, &actualbytes, 0);
 for(k=0;k<16;k++){
   if(buf[k]==KEY_0) {code[j++]='0';}//printf("0");}
   else if(buf[k]==KEY_1) {code[j++]='1';}//printf("1");}
   else if(buf[k]==KEY_2) {code[j++]='2';}//printf("2");}
   else if(buf[k]==KEY_3) {code[j++]='3';}//printf("3");}
   else if(buf[k]==KEY_4) {code[j++]='4';}//printf("4");}
   else if(buf[k]==KEY_5) {code[j++]='5';}//printf("5");}
   else if(buf[k]==KEY_6) {code[j++]='6';}//printf("6");}
   else if(buf[k]==KEY_7) {code[j++]='7';}//printf("7");}
   else if(buf[k]==KEY_8) {code[j++]='8';}//printf("8");}
   else if(buf[k]==KEY_9) {code[j++]='9';}//printf("9");}
   else if(buf[k]==KEY_MINUS) code[j++]='_';
   else if(buf[k]==KEY_SLASH) code[j++]='/';
  // printf("Data[%d] = %x\n",k,buf[k]);
 }
 while(actualbytes!=0){
   libusb_interrupt_transfer(dev_handle, 0x81/*(1 | LIBUSB_ENDPOINT_IN)*/, buf, 16, &actualbytes, 100);
   for(k=0;k<actualbytes;k++){
     if(buf[k]==KEY_0) {code[j++]='0';}//printf("0");}
     else if(buf[k]==KEY_1) {code[j++]='1';}//printf("1");}
     else if(buf[k]==KEY_2) {code[j++]='2';}//printf("2");}
     else if(buf[k]==KEY_3) {code[j++]='3';}//printf("3");}
     else if(buf[k]==KEY_4) {code[j++]='4';}//printf("4");}
     else if(buf[k]==KEY_5) {code[j++]='5';}//printf("5");}
     else if(buf[k]==KEY_6) {code[j++]='6';}//printf("6");}
     else if(buf[k]==KEY_7) {code[j++]='7';}//printf("7");}
     else if(buf[k]==KEY_8) {code[j++]='8';}//printf("8");}
     else if(buf[k]==KEY_9) {code[j++]='9';}//printf("9");}
     else if(buf[k]==KEY_MINUS) code[j++]='_';
     else if(buf[k]==KEY_SLASH) code[j++]='/';
     //printf("Data[%d] = %x\n",k,buf[k]);
   }
   //qDebug()<<retVal<<actualbytes;
 }
 //qDebug()<<retVal<<actualbytes;

 j=0;
/* while(buf[k]!=KEY_ENTER){
   if(buf[k]==KEY_0) {code[j++]='0';printf("1");}
   else if(buf[k]==KEY_1) {code[j++]='1';printf("1");}
   else if(buf[k]==KEY_2) {code[j++]='2';printf("2");}
   else if(buf[k]==KEY_3) {code[j++]='3';printf("3");}
   else if(buf[k]==KEY_4) {code[j++]='4';printf("4");}
   else if(buf[k]==KEY_5) {code[j++]='5';printf("5");}
   else if(buf[k]==KEY_6) {code[j++]='6';printf("6");}
   else if(buf[k]==KEY_7) {code[j++]='7';printf("7");}
   else if(buf[k]==KEY_8) {code[j++]='8';printf("8");}
   else if(buf[k]==KEY_9) {code[j++]='9';printf("9");}
   k++;
 }*/
 if(retVal == 0) {
     printf("\nReceived %d bytes %d\n",actualbytes,j);
 }
 else
 {
     fprintf(stderr, "Error Receiving message. retVal : %d, Actual : %d\n",retVal,actualbytes);
 }
 for(j = 0; j < 32; j++){
     printf("%c",code[j]);
 }
 printf("\n");
 }
 //for(iter = 0; iter < actualbytes; iter++){
     printf("Data[%d] = %d\n",iter,buf[iter]);
 //}

 /* Release interface #0. */
 retVal = libusb_release_interface(dev_handle, 0);
 if (0 != retVal)
 {
         fprintf(stderr, "Error releasing interface.\n");
 }

 /* If we detached a kernel driver from interface #0 earlier, we'll now
  * need to attach it again.  */
 if (kernelDriverDetached)
 {
         libusb_attach_kernel_driver(dev_handle, 0);
 }

 libusb_close(dev_handle);
 /* Shutdown libusb. */
 libusb_free_device_list(devs, 1); //free the list, unref the devices in it
 libusb_exit(/*context*/0);


 return 0;
 }
