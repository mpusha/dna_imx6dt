#ifndef CODE_ERRORS_H
#define CODE_ERRORS_H

namespace CODEERR{
//#define    C_NONE              0
//#define    C_INITIALISE_ERROR  1
//#define    C_USB_ERROR         2
//#define    C_OPEN_CAN_ERROR    3
//#define    C_RW_CAN_ERROR      4
//#define    C_USBBULK_ERROR     5
//#define    C_NOTREADY          6

// Error 16 bits word
// bits set 15-TFT, 14-Motor,13-Temp,12-Optical,11-USB if 1 then error
enum TCodeErrors
{
  NONE = 0,              // 0
  INITIALISE_ERROR,      // 1  ������ ������������� (�������� � ��������� ������ ������� ������ ����� �������)
  USB_ERROR,             // 2  ������ USB
  CAN_ERROR,             // 3  ������ �� CAN ������
  USBBULK_ERROR,         // 4  ������ ������ ������ bulk USB
  NOTREADY,              // 5  ������ �� ����� � ������, ��������� ��������� ���������� �������������
  DEVHW_ERROR,           // 6  ������ ������ � ����������� ������ ������� (���������� ���������� �������)
  OPENINRUN,             // 7  ������� �������� ���������� � ������ ���������� ������������� ���������
  MOTORALARM,            // 8  ������ �������
  STARTONOPEN,           // 9  ������� ���������� ������������� ��������� ��� �������� ����������
  PRERUNFAULT_ERROR,     // 10 ������ ��� ������� ��������� ���������� � ������� ���������
  PRERUNSEMIFAULT_ERROR, // 11 ���� ������ ��� ������� ��������� ���������� � �������, �� ��� ���� ���������
  DATAFRAME_ERROR,       // 12 ������ � �������� ����� ������, ����� ���������� ������ �� ������������� ����������
  LEDSSETUP_ERROR,       // 13 ������ ���������������� ������ � ���������, ������ �� ������������
  UNKNOWN_ERROR          // 14
};

}

#endif // CODE_ERRORS_H
