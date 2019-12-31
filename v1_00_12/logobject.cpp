#include <QDebug>
#include "logobject.h"
#include <QDateTime>

//-----------------------------------------------------------------------------
//--- Constructor
//-----------------------------------------------------------------------------
TLogObject::TLogObject(QString logFileName,int logFileSize,int arch_length)
{  
  FileName=logFileName+"_";
  log=new QFile(FileName+"0.log");
  logEnable=false;
  max_log_size = logFileSize;
  archive_lenght = arch_length;
  log->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append);
  log_out.setDevice(log);
  log_out << "---------------------------------------- "+QDateTime::currentDateTime().toString("dd/MM/yyyy ") +"----------------------------------------\n" ;
  log_out.flush();
  log->close();
}

//-----------------------------------------------------------------------------
//--- Destructor
//-----------------------------------------------------------------------------
TLogObject::~TLogObject()
{
  log->close();
  if(log) { delete log; log=0;}
}

//-----------------------------------------------------------------------------
//--- print log message on screen or into file
//-----------------------------------------------------------------------------
void TLogObject::logMessage(QString message)
{
  if(logEnable){
    if(!log->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) return;
    log_out.setDevice(log);
    log_out << QDateTime::currentDateTime().toString("hh:mm:ss ") +message+"\n" ;
    log_out.flush();
    log->close();
    if(log->size() > max_log_size) archive();
    //qDebug()<<"LOG "<<message;
  }
}

//-----------------------------------------------------------------------------
//--- archive files. File with name _0.log all time old
//-----------------------------------------------------------------------------
void TLogObject::archive(void)
{
  QFile file;
  file.setFileName(FileName +QString::number(archive_lenght) + ".log");
  file.remove();
  for(int n=archive_lenght; n>0; n--)
  {
    file.setFileName(FileName+ QString::number(n-1) + ".log");
    file.rename(FileName + QString::number(n) + ".log");
  }
  log->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append);
  log_out.setDevice(log);
  log_out << "---------------------------------------- "+QDateTime::currentDateTime().toString("dd/MM/yyyy ") +"----------------------------------------\n" ;
  log_out.flush();
  log->close();
}
