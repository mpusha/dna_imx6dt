/*
 * can_comm.h
 * interface to CAN communication functions
 */
#ifndef CANCOMM_HH
#define CANCOMM_HH

#include "can_ids.h"

#ifdef __cplusplus
extern "C" {
#endif
int CAN_init(void (*cmdfunc)(int,unsigned char*),
             void (*cmdfunc_reply)(int,unsigned char*));
int CAN_exit(void);
int CAN_SendString(char chan,unsigned char *msg, char mode);
int CAN_RecvString(char chan, unsigned char *msg, int blen);
signed char CAN_OpenChan(char nodeid);
signed char CAN_CloseChan(char chan);
signed char CAN_ResetChan(char chan);
signed char CAN_CkChan(char *chan);
signed char CAN_ProcessChan(char chan,char *buf,int blen);
int CAN_ClientChan(char chan, char *buf, char len, int tout);
int CAN_ServerChan(void (*cmdfunc)(char,char*),void (*cmdfunc_reply)(char,char*));
int CAN_SendEvent(char evid);
int CAN_SendEventN(char evid, unsigned char node);
extern int (*CAN_ServerEvent)(unsigned char evid, unsigned char src); //extern
int CAN_GetCID(char chan);

// call c function from c++
//http://docs.oracle.com/cd/E19059-01/wrkshp50/805-4956/bajdcjch/index.html
//http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0203j/Cacjfjei.html
#ifdef __cplusplus
}
#endif

#endif
