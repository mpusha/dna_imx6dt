/*
 * SimpleGPIO.h
 *
 */

#ifndef HW_H
#define HW_H
#include <syslog.h>
//#include <QApplication>
#include <QtCore>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <syslog.h>



/*#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
*/
/****************************************************************
* Constants
****************************************************************/

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define POLL_TIMEOUT (3 * 1000) /* 3 seconds */
#define MAX_BUF 64

enum PIN_DIRECTION{
   INPUT_PIN=0,
   OUTPUT_PIN=1
};

enum PIN_VALUE{
   LOW=0,
   HIGH=1
};

/****************************************************************
* simple gpios functions from /sys/class/gpio
****************************************************************/
int gpio_export(unsigned int gpio);
int gpio_unexport(unsigned int gpio);
int gpio_set_dir(unsigned int gpio, PIN_DIRECTION out_flag);
int gpio_set_value(unsigned int gpio, PIN_VALUE value);
int gpio_get_value(unsigned int gpio, unsigned int *value);
int gpio_set_edge(unsigned int gpio, char *edge);
int gpio_fd_open(unsigned int gpio);
int gpio_fd_close(int fd);
void gpio_set_backlight(char *val);

// Dt gpio pins definitions
enum PIN_GPIOS{
  GPIO0=16,    // Sound On/Off GPIO1_16 ((1-1)*32+16=16)
  GPIO1=17,    // Usb On/Off GPIO1_17
  GPIO2=19,    // Usb standard/Dt96 mode GPIO1_19
  GPIO3=21,    // J3B.7 GPIO1_21
  GPIO4=20,    // J3B.5 GPIO1_20
  GPIO5=18,    // J3B.4 GPIO1_18
  GPIO6=174,   // J3B.3 GPIO6_14
  GPIO7=175,   // J3B.2 GPIO6_15

  SLP=161,     // J3A.2 GPIO6_1
  ACT=14      // J3A.3 GPIO1_14
};

/****************************************************************
*  Dt gpio function
****************************************************************/
class TGpio: public QThread
{
  Q_OBJECT
  public:

    TGpio();
    ~TGpio();

    void m_sleep(qint64 msec)
    {
        QEventLoop loop;
        QTimer::singleShot(msec, &loop, SLOT(quit()));
        loop.exec();
    }

    int initalHW(void);
    // host GPIO function
    int pins_export(void);
    int pins_unexport(void);
    //int pins_set_dir(void);
    int init_gpios(void);
    void close_gpios(void);
    unsigned int polling_gpios(int timePolling);
// Host GPIO function
    int setGPIO(PIN_GPIOS GPIO,unsigned int val);
    int getGPIO(PIN_GPIOS GPIO,unsigned int *val);
    int setDirGPIO(PIN_GPIOS GPIO,unsigned int dir);
public slots:
    void setBackLight(char *);
protected:

    void run();
  private:
    bool abort;
    QWaitCondition condition;
    int epfd;
    int fd_d0, fd_d1;
    struct epoll_event ev_d0, ev_d1;
    struct epoll_event events[10];


};



#endif /* HW_H */
