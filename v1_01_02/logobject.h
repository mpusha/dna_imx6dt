#ifndef LOGOBJECT_H
#define LOGOBJECT_H

#include <QObject>
#include <QFile>
#include <QTextStream>

#define MAX_LOG_SIZE   100000
#define ARCHIVE_LENGTH      9

class TLogObject : public QObject
{
  Q_OBJECT
public:
  explicit TLogObject(QString logFileName,int logFileSize=MAX_LOG_SIZE,int arch_length=ARCHIVE_LENGTH);
  ~TLogObject();
  void setLogEnable(bool en){logEnable=en;}
signals:
  void logScrMessage(QString);
public slots:
  void logMessage(QString message);
private:
  QString FileName;
  bool logEnable;
  QFile *log;
  QTextStream log_out;
  int max_log_size;
  int archive_lenght;
  void archive(void);
};

#endif // LOGOBJECT_H
