#include "serialscanner.h"

//-------------------------------------------------Bar Code reader--------------------------------------
// Serial scanner device function
TSerialScanner::TSerialScanner(QString devN, bool useDev)
{
  use=useDev;
  //sp=new QSerialPort("/dev/ttymxc2");
  if(use) {
    QSerialPort::setPortName(devN);
    QSerialPort::setBaudRate(QSerialPort::Baud9600);
    QSerialPort::open(QIODevice::ReadWrite);
  }
  qDebug()<<"TScanner object create for Bar Code Reader device"<<devN;
  //startScan();
  //printf("pointer %p\n",sp);
}

TSerialScanner::~TSerialScanner(void)
{
  if(use) QSerialPort::close();
  qDebug()<<"TSerialScanner object was deleted.";
  //if(sp) delete sp;
}

bool TSerialScanner::startScan(void)
{
  if(use) {
    QSerialPort::putChar('E');
  }
  return true;
}
bool TSerialScanner::readScanData(QString &data)
{
  if(! use) return true;
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

