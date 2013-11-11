/*
 * RadioControl.c
 *
 *  Created on: Oct 17, 2013
 *      Author: Mofidul I. Jamal
 *      		Florida Atlantic University
 */

#include "RadioControl.h"

extern RF_SETTINGS rfSettings;

unsigned char RxBuffer[PACKET_LEN+2];
unsigned char RxBufferLength = 0;
unsigned char transmitting = 0;
unsigned char receiving = 0;
unsigned char data_available = 0;

void RadioSetup()
{
	  // Increase PMMCOREV level to 2 for proper radio operation
	  SetVCore(2);

	  ResetRadioCore();
	  RadioPowerSetup();
}

void RadioPowerSetup(void)
{
  // Set the High-Power Mode Request Enable bit so LPM3 can be entered
  // with active radio enabled
  PMMCTL0_H = 0xA5;
  PMMCTL0_L |= PMMHPMRE_L;
  PMMCTL0_H = 0x00;

  WriteRfSettings(&rfSettings);

  WriteSinglePATable(PATABLE_VAL);
}

void RadioTransmitData(unsigned char *buffer, unsigned char length)
{
  RF1AIES |= BIT9;
  RF1AIFG &= ~BIT9;                         // Clear pending interrupts
  RF1AIE |= BIT9;                           // Enable TX end-of-packet interrupt

  WriteBurstReg(RF_TXFIFOWR, buffer, length);

  Strobe( RF_STX );                         // Strobe STX
  //enable transmit flag
  transmitting = 1;
}

void RadioEnableReceive(void)
{
  RF1AIES |= BIT9;                          // Falling edge of RFIFG9
  RF1AIFG &= ~BIT9;                         // Clear a pending interrupt
  RF1AIE  |= BIT9;                          // Enable the interrupt

  // Radio is in IDLE following a TX, so strobe SRX to enter Receive Mode
  Strobe( RF_SRX );
  //set receiving flat to true
  receiving = 1;
}

void RadioDisableReceive(void)
{
  RF1AIE &= ~BIT9;                          // Disable RX interrupts
  RF1AIFG &= ~BIT9;                         // Clear pending IFG

  // It is possible that ReceiveOff is called while radio is receiving a packet.
  // Therefore, it is necessary to flush the RX FIFO after issuing IDLE strobe
  // such that the RXFIFO is empty prior to receiving a packet.
  Strobe( RF_SIDLE );
  Strobe( RF_SFRX  );
  //set receiving flag to false
  receiving = 0;
}

unsigned char * RadioGetRxBuffer()
{
	if(data_available)
	{
		data_available = 0;
		return RxBuffer;
	}
	else
		return 0;
}

unsigned char RadioIsTransmitting()
{
	return transmitting;
}

unsigned char RadioIsReceiving()
{
	return receiving;
}

unsigned char RadioIsDataAvailable()
{
	return data_available;
}

#pragma vector=CC1101_VECTOR
__interrupt void CC1101_ISR(void)
{
  switch(__even_in_range(RF1AIV,32))        // Prioritizing Radio Core Interrupt
  {
    case  0: break;                         // No RF core interrupt pending
    case  2: break;                         // RFIFG0
    case  4: break;                         // RFIFG1
    case  6: break;                         // RFIFG2
    case  8: break;                         // RFIFG3
    case 10: break;                         // RFIFG4
    case 12: break;                         // RFIFG5
    case 14: break;                         // RFIFG6
    case 16: break;                         // RFIFG7
    case 18: break;                         // RFIFG8
    case 20:                                // RFIFG9
      if(receiving)			    // RX end of packet
      {
        // Read the length byte from the FIFO
        RxBufferLength = ReadSingleReg( RXBYTES );
        ReadBurstReg(RF_RXFIFORD, RxBuffer, RxBufferLength);

        // Stop here to see contents of RxBuffer
        __no_operation();

        // Check the CRC results
        if(RxBuffer[CRC_LQI_IDX] & CRC_OK)
        {
          //P1OUT ^= BIT0;                    // Toggle LED1
          data_available = RxBufferLength;
        }
      }
      else if(transmitting)		    // TX end of packet
      {
        RF1AIE &= ~BIT9;                    // Disable TX end-of-packet interrupt
        P3OUT &= ~BIT6;                     // Turn off LED after Transmit
        transmitting = 0;
      }
      else while(1); 			    // trap
      break;
    case 22: break;                         // RFIFG10
    case 24: break;                         // RFIFG11
    case 26: break;                         // RFIFG12
    case 28: break;                         // RFIFG13
    case 30: break;                         // RFIFG14
    case 32: break;                         // RFIFG15
  }
  __bic_SR_register_on_exit(LPM3_bits); //Go back to low-power mode
}
