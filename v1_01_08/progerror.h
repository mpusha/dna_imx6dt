#ifndef PROGERROR_H
#define PROGERROR_H

#include <QString>
#include <QStringList>
#include <QObject>
#include <QDebug>
#include <bitset>

// Error 16 bits word
// bits set 15-TFT, 14-Motor,13-Temp,12-Optical,11-USB if 1 then error
// simple error for server answer
// this is value! not a bits!
enum CodeErrors
{
  NONE_ERR = 0,        // 0
  INITIALISE_ERR,      // 1  ошибка инициализации (возможно в командной строке неверно указан номер прибора)
  USB_ERR,             // 2  ошибка USB
  CAN_ERR,             // 3  ошибка на CAN канале
  USBBULK_ERR,         // 4  ошибка записи чтения bulk USB
  NOTREADY_ERR,        // 5  прибор не готов к работе, требуется подождать завершнния инициализации
  DEVHW_ERR,           // 6  ошибка работы с аппаратурой внутри прибора (необходимо выключение прибора)
  OPENINRUN_ERR,       // 7  попытка открытия термоблока в момент выполнения температурной программы
  MOTORALARM_ERR,      // 8  ошибка привода
  STARTONOPEN_ERR,     // 9  попытка выполнения температурной программы при открытом термоблоке
  PRERUNFAULT_ERR,     // 10 ошибка при попытке выполнить подготовку к запуску программы
  PRERUNSEMIFAULT_ERR, // 11 была ошибка при попытке выполнить подготовку к запуску, но она была устранена
  DATAFRAME_ERR,       // 12 ошибка в принятии блока данных, число полученных данных не соответствует ожидаемому
  LEDSSETUP_ERR,       // 13 ошибка позиционирования колеса с фильтрами, колесо не провернулось
  MEASHT_ERR,          // 14 ошибка измерения высоты пробирки
  GETAMPPROG_ERR,      // 15 ошибка при попытке считывания программы амплификации в простом режиме
  UNKNOWN_ERR          // 16
};

//this setup bits!
enum DevErrors
{
  USB_CONNECTION_ERROR=0,     // 0
  OPEN_OPT_ERROR,             // 1
  OPEN_AMP_ERROR,             // 2
  OPEN_MOT_ERROR,             // 3
  OPEN_TFT_ERROR,             // 4
  RW_OPT_ERROR,               // 5
  RW_AMP_ERROR,               // 6
  RW_MOT_ERROR,               // 7
  RW_TFT_ERROR,               // 8
  TOUT_OPT_ERROR,             // 9
  TOUT_AMP_ERROR,             // 10
  TOUT_MOT_ERROR,             // 11
  TOUT_TFT_ERROR,             // 12
  INCORRECT_CAN_DEVICE_ERROR, // 13
  RW_USBBULK_ERROR,           // 14
  CMD_USBBULK_ERROR,          // 16 error in process CAN comamnd (but don't CAN error) may be HW don't work property (SD cadr fex)
  MOTORDRVLED_ERROR,          // 17 motor driver of LED error
  GETFRAME_ERROR,             // 18 error in input data counts don't correspond of request for device type
  PRERUNFAULT_ERROR,          // 19 test on run is bad
  PRERUNSEMIFAULT_ERROR,      // 20 1 time test on run is bad but second is OK
  PRERUNMEMTEST_ERROR,        // 21 memory test in prerun error
  PRERUNANALYSE_ERROR,        // 20 analyse get picture data un prerun error
  OPENINRUN_ERROR,            // 22 try open cover in run time
  MOTORALARM_ERROR,           // 23 error of motor
  STARTWITHOPENCOVER_ERROR,   // 24 try start run on with open cover
  MEASHIGHTUBE_ERROR,         // 25 high of tube meas error
  GETAMPPROG_ERROR            // 26 get amplification program error
};
// Extended error of device
class TDevErrors : public QObject
{
  Q_OBJECT
public:
  explicit TDevErrors(QObject *parent = 0);
  ~TDevErrors();


  void clearDevError(void){ bitError.reset(); map=0;}
  void clearCnt(void){ cntOptErr=cntAmpErr=cntMotErr=cntTftErr=cntBulkErr=cntNWErr=0;}
  int readOptErr(void) { return cntOptErr; }
  int readAmpErr(void) { return cntAmpErr; }
  int readMotErr(void) { return cntMotErr; }
  int readTftErr(void) { return cntTftErr; }
  int readUsbErr(void) { return cntBulkErr;}
  int readNWErr(void)  { return  cntNWErr; }
  void setDevError(DevErrors err);
 // unsigned int readDevError(void){return (unsigned int) bitError.to_ulong();}
  short int analyseError(void);
  QStringList readDevTextErrorList(void);
  QStringList devErrorsText;

private:
  std::bitset<32> bitError;
  int cntOptErr,cntAmpErr,cntMotErr,cntTftErr,cntBulkErr,cntNWErr;
  short int map;
};
#endif // PROGERROR_H
