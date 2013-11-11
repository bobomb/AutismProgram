/*
 * RadioControl.h
 *
 *  Created on: Oct 17, 2013
 *      Author: Mo
 */

#ifndef RADIOCONTROL_H_
#define RADIOCONTROL_H_

#include "cc430x613x.h"
#include "RF1A.h"
#include "hal_pmm.h"

#define  PACKET_LEN         (0x06)			// PACKET_LEN <= 61
#define  RSSI_IDX           (PACKET_LEN)    // Index of appended RSSI
#define  CRC_LQI_IDX        (PACKET_LEN+1)  // Index of appended LQI, checksum
#define  CRC_OK             (BIT7)          // CRC_OK bit
#define  PATABLE_VAL        (0x51)          // 0 dBm output

void RadioSetup();
void RadioPowerSetup();
void RadioTransmitData(unsigned char *buffer, unsigned char length);
void RadioEnableReceive();
void RadioDisableReceive();

unsigned char * RadioGetRxBuffer();

//status checks
unsigned char RadioIsTransmitting();
unsigned char RadioIsReceiving();
unsigned char RadioIsDataAvailable();

#endif /* RADIOCONTROL_H_ */
