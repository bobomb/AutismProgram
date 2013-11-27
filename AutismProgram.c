/******************************************************************************
* Notes: To change packet size make sure to change the packet size in RfRegSettings.c in both 868 mhz and 900 mhz
******************************************************************************/
//mode 0 = mobile
//mode 1 = base
#define PROGRAM_MODE 1

#include "AutismProgram.h"


void main( void )
{  
	// Stop watchdog timer to prevent time out reset
	WDTCTL = WDTPW + WDTHOLD;

	//set program mode
    if(PROGRAM_MODE)
    	programRole = BaseUnit;
    else
    	programRole = MobileUnit;
    //setup radio, initialize ports and the timers
	RadioSetup();
	InitPorts();
    InitMillisecondTimer();

    programState = ProgramIdle;
    //enable interrupts
    __bis_SR_register( GIE );

    while (1)
    {
    	//calculate time since last run to millisecond accuracy
        g_deltaTime = g_currentTimeMS - g_lastTimeMS;
        g_lastTimeMS = g_currentTimeMS;
        //update button state and radio state
		ButtonUpdate(&g_pairingButton, g_deltaTime, P1IN & BIT7);
		RadioUpdate();

		switch(programState)
		{
		case ProgramTestMode:
			break;
		case ProgramIdle:
			if(g_pairingButton.holdEvent)
			{
				g_pairingButton.holdEvent = 0;
				g_pairingModeStartTime = g_currentTimeMS;
				programState = ProgramPairing;
				pairingState = PairingInitializing;
				SETBIT(P1OUT, BIT0);
				RadioEnableReceive();
			}

			if(programRole == MobileUnit)
			{
				if(g_pairingButton.pressEvent)
				{
					g_pairingButton.pressEvent = 0;
					MobileUnit_SetAccelerometerNormalValues();
				}
			}
			break;
		case ProgramPairing:
			if(g_currentTimeMS - g_pairingModeStartTime > PAIRING_MODE_TIMEOUT_MS)
			{
				//pairing mode timeout
				CLEARBIT(P1OUT, BIT0);
				programState = ProgramIdle;
				pairingState = PairingInitializing;
				__no_operation();
			}
			else
			{
				if(programRole == BaseUnit)
					State_ProgramPairing_BaseUnit();
				else if(programRole == MobileUnit)
					State_ProgramPairing_MobileUnit();
			}
			break;
		case ProgramPairAcknowledged: //blink LED
			State_ProgramPairAcknowledged();
			break;
		case ProgramRunning:
			if(programRole == BaseUnit)
				State_ProgramRunning_BaseUnit();
			else if (programRole == MobileUnit)
				State_ProgramRunning_MobileUnit();

			break;
		}

		if(!RadioIsTransmitting())
		{
			RadioEnableReceive();
		}
    }

}

void InitPorts(void)
{
  // Set up the button as interruptible 
  //set P1.7 as input
  CLEARBIT(P1DIR, BIT7);
  //set P1.7 input resister enabled
  SETBIT(P1REN, BIT7);
  //set P1.7 interrupt edge on rising edge
  SETBIT(P1IES, BIT7);
  //clear all port 1 interrupt flags
  P1IFG = 0;
  //Set P1.7 to output 1 (why I am unsure, since we already configured it as input...);
  SETBIT(P1OUT, BIT7);
  //Set P1.7 interrupt enable on
  SETBIT(P1IE, BIT7);

  // Initialize Port J
  PJOUT = 0x00;
  PJDIR = 0xFF; 

  // Set up LEDs (on the board)
  P1OUT &= ~BIT0;
  P1DIR |= BIT0;
  P3OUT &= ~BIT6;
  P3DIR |= BIT6;

  if(programRole == BaseUnit)
  {
	  //set port 3.2, 3.3, 3.4 to output mode
	  SETBIT(P3DIR, BIT2);
	  SETBIT(P3DIR, BIT5);
	  SETBIT(P3DIR, BIT4);
	  // Set up LED output pins for RGB LED
	  // set P3.2, P3.3, P3.4 to use the timer output
	  SETBIT(P3SEL, BIT2);
	  SETBIT(P3SEL, BIT5);
	  SETBIT(P3SEL, BIT4);

	  //set the timer output for CCR0
	  //realize that in UP mode, the timer will count up to CCR0
	  TA0CTL = TASSEL_2 + MC_1;
	  TA0CCR0 = 4096;

	  TA0CCR1 = 0; //b
	  TA0CCR4 = 0; //g
	  TA0CCR3 = 0; //b
	  //set output mode to 7 for PWM
	  TA0CCTL1 = OUTMOD_7;
	  TA0CCTL4 = OUTMOD_7;
	  TA0CCTL3 = OUTMOD_7;
  }
  else if (programRole == MobileUnit)
  {
	  //set up accelerometer outputs
	  SETBIT(P2DIR, BIT0); //P2.0 output, G select
	  SETBIT(P2DIR, BIT1); //P2.1 output,  Sleep mode (active low, so set to HIGH to activate)

	  CLEARBIT(P2OUT, BIT0); //P2.0 G-select mode1 6g or 206 mV/G
	  SETBIT(P2OUT, BIT1); //P2.1 sleep mode, is active low (so set high to enable)
	  //set up accelerometer inputs

	  CLEARBIT(P2DIR, BIT2); //Z
 	  CLEARBIT(P2DIR, BIT4); //Y
	  CLEARBIT(P2DIR, BIT6); //X

	  SETBIT(P2SEL, BIT2);                            // Enable A/D channel inputs
	  SETBIT(P2SEL, BIT4);
	  SETBIT(P2SEL, BIT6);
	  InitAdcForAccelerometer();


  }

}

void InitAdcForAccelerometer()
{
	  ADC12CTL0 = ADC12ON+ADC12MSC + ADC12SHT0_8; // Turn on ADC12_A, extend sampling time
	                                            // to avoid overflow of results
	  ADC12CTL1 = ADC12SHP+ADC12CONSEQ_3;       // Use sampling timer, repeated sequence
	  //ADC12CTL2 = ADC12PDIV;					//set predivider to divide ADC clock by 4
	  ADC12MCTL0 = ADC12INCH_2;                 // ref+=AVcc, channel = A0
	  ADC12MCTL1 = ADC12INCH_4;                 // ref+=AVcc, channel = A1
	  ADC12MCTL2 = ADC12INCH_6+ADC12EOS;       // ref+=AVcc, channel = A2, end sequence

	  SETBIT(ADC12IE, BIT2);                      // Enable ADC12IFG.6
	  ADC12CTL0 |= ADC12ENC;                    // Enable conversions
	  ADC12CTL0 |= ADC12SC;                     // Start conversion - software trigger
}
void InitMillisecondTimer()
{
	TA1CCR0 = 1000-1;          // CCR0 period set to 999 cycles
	TA1CCTL0 = CCIE;            // Timer A1 CCTL0 Enable capture compare interrupt
	TA1CTL = TASSEL_2 + MC_1;   // SMCLK, up mode...SMCLK is ~1MHZ so divide by 1000 ~ 1khz
}

#pragma vector=TIMER1_A0_VECTOR
__interrupt void TIMER1_A0_ISR(void)
{
	g_currentTimeMS++;
}

#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR(void)
{
  switch(__even_in_range(P1IV, 16))
  {
    case  0: break;
    case  2: break;                         // P1.0 IFG
    case  4: break;                         // P1.1 IFG
    case  6: break;                         // P1.2 IFG
    case  8: break;                         // P1.3 IFG
    case 10: break;                         // P1.4 IFG
    case 12: break;                         // P1.5 IFG
    case 14: break;                         // P1.6 IFG
    case 16:                                // P1.7 IFG
      CLEARBIT(P1IFG, BIT7); //clear the interrupt
      //set the interrupt event for the pairing button
      g_pairingButton.interruptEvent = 1;
      __bic_SR_register_on_exit(LPM3_bits); // Exit active mode
      break;
  }
}

#pragma vector=ADC12_VECTOR
__interrupt void ADC12_ISR (void)
{
  switch(__even_in_range(ADC12IV,34))
  {
  case  0: break;                           // Vector  0:  No interrupt
  case  2: break;                           // Vector  2:  ADC overflow
  case  4: break;                           // Vector  4:  ADC timing overflow
  case  6: break;                           // Vector  6:  ADC12IFG0
  case  8: break;                           // Vector  8:  ADC12IFG1
  case 10:                                 // Vector 10:  ADC12IFG2
	  accelerometerValues[0] = ADC12MEM0;           // Move A2 results, IFG is cleared
	  accelerometerValues[1] = ADC12MEM1;           // Move A4 results, IFG is cleared
	  accelerometerValues[2] = ADC12MEM2;           // Move A6 results, IFG is cleared
	  __no_operation();
  case 12: break;                           // Vector 12:  ADC12IFG3
  case 14: break;                           // Vector 14:  ADC12IFG4
  case 16: break;                           // Vector 16:  ADC12IFG5
  case 18: break;							// Vector 18:  ADC12IFG6
  case 20: break;                           // Vector 20:  ADC12IFG7
  case 22: break;                           // Vector 22:  ADC12IFG8
  case 24: break;                           // Vector 24:  ADC12IFG9
  case 26: break;                           // Vector 26:  ADC12IFG10
  case 28: break;                           // Vector 28:  ADC12IFG11
  case 30: break;                           // Vector 30:  ADC12IFG12
  case 32: break;                           // Vector 32:  ADC12IFG13
  case 34: break;                           // Vector 34:  ADC12IFG14
  default: break;
  }
}

char GetRandomBit(void)
{
	char returnValue = 0;
	SETBIT(P2SEL, BIT7);

	ADC12CTL1 = ADC12SHP;
	ADC12CTL0 |= ADC12REF2_5V_L + ADC12SHT0_2 + REFON + ADC12ON;
	ADC12CTL0 |= ADC12ENC + ADC12SC;
	ADC12MCTL0 = ADC12INCH_7;
	while((ADC12CTL1 & ADC12BUSY) == 1);

	returnValue = ADC12MEM0 & 0x01;
	return returnValue;
}

char GetRandomByte(void)
{
	unsigned char randomNum = 0;
	char i = 0;
	for (i = 0; i < 8; i++)
	{
		 randomNum |= GetRandomBit() << i;
	}
	InitAdcForAccelerometer();
	return randomNum;
}

char CheckPacketIdentifier( unsigned char * packet)
{
	if((packet[1] == unitIdentifier[0]) && (packet[2] == unitIdentifier[1]))
		return 1;
	else
		return 0;
}

//Sends the pairing request to the base station
void BaseUnit_SendPairingResponse()
{
	if(RadioIsReceiving())
		RadioDisableReceive();

	TxBuffer[0] = PACKET_BASE_PAIRING_RESPONSE;
	TxBuffer[1] = unitIdentifier[0];
	TxBuffer[2] = unitIdentifier[1];
	TxBuffer[3] = 0xFF;
	TxBuffer[4] = 0xFF;
	TxBuffer[5] = 0xFF;

	RadioTransmitData( (unsigned char*)TxBuffer, sizeof TxBuffer);

	while(RadioIsTransmitting())
		__no_operation();

	RadioEnableReceive();
}

void MobileUnit_SetAccelerometerNormalValues()
{
	normalAccelerometerValues[0] = accelerometerValues[0];
	normalAccelerometerValues[1] = accelerometerValues[1];
	normalAccelerometerValues[2] = accelerometerValues[2];
}

void MobileUnit_SendPairingRequest()
{
	if(RadioIsReceiving())
		RadioDisableReceive();

	TxBuffer[0] = PACKET_MOBILE_PAIRING_REQUEST;
	unitIdentifier[0] = TxBuffer[1] = GetRandomByte();
	unitIdentifier[1] = TxBuffer[2] = GetRandomByte();
	TxBuffer[3] = 0x0;
	TxBuffer[4] = 0x0;
	TxBuffer[5] = 0x0;
	RadioTransmitData( (unsigned char*)TxBuffer, sizeof TxBuffer);
	//wait for transmission before proceeding
	while(RadioIsTransmitting())
		__no_operation();
	RadioEnableReceive();
}
void MobileUnit_SendTestData(unsigned char byte1,unsigned char byte2,unsigned char byte3)
{
	if(RadioIsReceiving())
		RadioDisableReceive();

	TxBuffer[0] = PACKET_MOBILE_TEST_DATA;
	TxBuffer[1] = unitIdentifier[0];
	TxBuffer[2] = unitIdentifier[1];
	TxBuffer[3] = byte1;
	TxBuffer[4] = byte2;
	TxBuffer[5] = byte3;
	RadioTransmitData( (unsigned char*)TxBuffer, sizeof TxBuffer);

	while(RadioIsTransmitting())
		__no_operation();

	RadioEnableReceive();
}

void MobileUnit_SendAccelerometerData()
{
	unsigned int accelerometerDifferences[3] = {0,0,0};
	unsigned int i = 0;
	for(i = 0; i < 3; i++)
	{
		if(normalAccelerometerValues[i] >=accelerometerValues[i] )
			accelerometerDifferences[i] = normalAccelerometerValues[i] - accelerometerValues[i];
		else
			accelerometerDifferences[i] = accelerometerValues[i] - normalAccelerometerValues[i];

		if(accelerometerDifferences[i] < ACCELEROMETER_MOVEMENT_THRESHOLD)
			accelerometerDifferences[i] = 0;
	}

	if(RadioIsReceiving())
		RadioDisableReceive();
	//DATA IS SENT X, Y, Z with most significant 8 bits followed by least significant 8 bits
	TxBuffer[0] = PACKET_MOBILE_ACCELEROMETER_DATA;
	TxBuffer[1] = unitIdentifier[0];
	TxBuffer[2] = unitIdentifier[1];
	TxBuffer[3] = accelerometerDifferences[0] >> 8;
	TxBuffer[4] = accelerometerDifferences[0] & 0xFF;
	TxBuffer[5] = accelerometerDifferences[1] >> 8;
	TxBuffer[6] = accelerometerDifferences[1] & 0xFF;
	TxBuffer[7] = accelerometerDifferences[2] >> 8;
	TxBuffer[8] = accelerometerDifferences[2] & 0xFF;
	TxBuffer[9] = 0xC0;
	TxBuffer[10] = 0xFF;
	TxBuffer[11] = 0xEE;
	RadioTransmitData( (unsigned char*)TxBuffer, sizeof TxBuffer);

	while(RadioIsTransmitting())
		__no_operation();

	RadioEnableReceive();
}

//checks a given packet to see if the identifier matches the one stored in memory. used by the base unit
void State_ProgramPairing_MobileUnit()
{
	switch(pairingState)
	{
	case PairingInitializing:
		//send pairing data
		MobileUnit_SendPairingRequest();
		pairingState = PairingWaitingForResponse;
		break;
	case PairingWaitingForResponse:
		//check to see if we got response to our pairing request
		if(RadioIsDataAvailable())
		{
			//check to see if we got a pair request
			const char bytesAvailable = RadioIsDataAvailable();
			if(bytesAvailable)
			{
				unsigned char * rxData = RadioGetRxBuffer();
				//check to see if pairing command
				if(rxData[0] == PACKET_BASE_PAIRING_RESPONSE)
				{
					//verify base unit identifier is the same
					if((unitIdentifier[0] == rxData[1]) && (unitIdentifier[1] == rxData[2]))
					{
						programState = ProgramPairAcknowledged;
						pairingState = PairingComplete;
					}
				}
			}
		}

		break;
	}
}

void State_ProgramPairing_BaseUnit()
{
	//check to see if we got a pair request
	const char bytesAvailable = RadioIsDataAvailable();
	if(bytesAvailable)
	{
		unsigned char * rxData = RadioGetRxBuffer();
		//check to see if pairing command
		if(rxData[0] == PACKET_MOBILE_PAIRING_REQUEST)
		{
			//save the base unit identifier
			unitIdentifier[0] = rxData[1];
			unitIdentifier[1] = rxData[2];
			//send a response packet (make the assumption that it gets the response)
			BaseUnit_SendPairingResponse();
			programState = ProgramPairAcknowledged;
			pairingState = PairingComplete;
		}
	}
}

void State_ProgramPairAcknowledged()
{
	//counter is less than 5 seconds
	if(g_ledBlinkCounter < 5000)
	{
		//add the elapsed time since the last call
		g_ledBlinkCounter+=g_deltaTime;
		//step function, divide by 250 and check to see if its even, if so then turn light on, if not then turn it off
		//since division returns the integer part this acts as a step function
		if((g_ledBlinkCounter / 250) % 2 == 0)
		{
			SETBIT(P1OUT, BIT0);
		}
		else
			CLEARBIT(P1OUT,BIT0);
	}
	else //timer has elapsed
	{
		//turn off LED
		CLEARBIT(P1OUT,BIT0);
		//reset blink counter
		g_ledBlinkCounter = 0;
		programState = ProgramRunning;
	}
}

void State_ProgramRunning_MobileUnit()
{
	//in case we want to pair again
	if(g_pairingButton.holdEvent)
	{
		g_pairingButton.holdEvent = 0;
		g_pairingModeStartTime = g_currentTimeMS;
		programState = ProgramPairing;
		pairingState = PairingInitializing;
		SETBIT(P1OUT, BIT0);
		RadioEnableReceive();
	}
	//sets normal values
	if(g_pairingButton.pressEvent)
	{
		g_pairingButton.pressEvent = 0;
		MobileUnit_SetAccelerometerNormalValues();
	}

	g_dataTransmitCounter+=g_deltaTime;
	if(g_dataTransmitCounter >= 100)
	{
		__disable_interrupt();
		MobileUnit_SendAccelerometerData();
		__enable_interrupt();
		g_dataTransmitCounter = 0;
	}
}
void State_ProgramRunning_BaseUnit()
{
	//in case we want to pair again
	if(g_pairingButton.holdEvent)
	{
		g_pairingButton.holdEvent = 0;
		g_pairingModeStartTime = g_currentTimeMS;
		programState = ProgramPairing;
		pairingState = PairingInitializing;
		SETBIT(P1OUT, BIT0);
		RadioEnableReceive();
	}

	const char bytesAvailable = RadioIsDataAvailable();
	if(bytesAvailable)
	{
		unsigned char * rxData = RadioGetRxBuffer();
		//check to see if test command
		if(rxData[0] == PACKET_MOBILE_ACCELEROMETER_DATA)
		{
			if(CheckPacketIdentifier(rxData))
			{
				accelerometerValues[0] = rxData[3] << 8;
				accelerometerValues[0] |= rxData[4];
				accelerometerValues[1] = rxData[5] << 8;
				accelerometerValues[1] |= rxData[6];
				accelerometerValues[2] = rxData[7] << 8;
				accelerometerValues[2] |= rxData[8];

				TA0CCR3 = accelerometerValues[0]; //b
				TA0CCR4 = accelerometerValues[1]; //g
				TA0CCR1 = accelerometerValues[2]; //r
				INVERTBIT(P1OUT, BIT0);
			}
		}
	}
}
