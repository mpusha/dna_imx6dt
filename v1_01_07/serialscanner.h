#ifndef SERIALSCANNER_H
#define SERIALSCANNER_H

#include <QSerialPort>
#include <QDebug>

class TSerialScanner: public QSerialPort
{
public:
  TSerialScanner(QString devN="/dev/ttymxc2", bool useDev=false);
  ~TSerialScanner(void);
  bool startScan(void);
  bool readScanData(QString &data);
private:
  bool use;
};
#endif // SERIALSCANNER_H

