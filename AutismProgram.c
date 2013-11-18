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
	RadioSetup();
	InitButtonLeds();

    InitMillisecondTimer();

    if(PROGRAM_MODE)
    	programRole = BaseUnit;
    else
    	programRole = MobileUnit;

    //programState = ProgramTestMode;
    programState = ProgramIdle;
    //enable interrupts again
    __bis_SR_register( GIE );

    while (1)
    {
        g_deltaTime = g_currentTimeMS - g_lastTimeMS;
        g_lastTimeMS = g_currentTimeMS;

		//__disable_interrupt();
		ButtonUpdate(&pairingButton, g_deltaTime, P1IN & BIT7);
		//__bis_SR_register( GIE );

		switch(programState)
		{
		case ProgramTestMode:
			break;
		case ProgramIdle:
			if(pairingButton.holdEvent)
			{
				pairingButton.holdEvent = 0;
				g_pairingModeStartTime = g_currentTimeMS;
				programState = ProgramPairing;
				pairingState = PairingInitializing;
				SETBIT(P1OUT, BIT0);
				RadioEnableReceive();
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
			//in case we want to pair again
			if(pairingButton.holdEvent)
			{
				pairingButton.holdEvent = 0;
				g_pairingModeStartTime = g_currentTimeMS;
				programState = ProgramPairing;
				pairingState = PairingInitializing;
				SETBIT(P1OUT, BIT0);
				RadioEnableReceive();
			}

			if(programRole == BaseUnit)
				State_ProgramRunning_BaseUnit();
			else if (programRole == MobileUnit)
				State_ProgramRunning_MobileUnit();

			break;
		}
	__no_operation();

	if(!RadioIsTransmitting())
		RadioEnableReceive();
    }

}

void InitButtonLeds(void)
{
  // Set up the button as interruptible 
  //set P1.7 as output
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

  // Set up LEDs 
  P1OUT &= ~BIT0;
  P1DIR |= BIT0;
  P3OUT &= ~BIT6;
  P3DIR |= BIT6;
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
      pairingButton.interruptEvent = 1;
      __bic_SR_register_on_exit(LPM3_bits); // Exit active    
      break;
  }
}

char GetRandomBit(void)
{
	P2SEL = 0x01; // p2 A/D channel inputs

	ADC12CTL1 = ADC12SHP;
	ADC12CTL0 |= ADC12REF2_5V_L + ADC12SHT0_2 + REFON + ADC12ON;
	ADC12CTL0 |= ADC12ENC + ADC12SC;

	while((ADC12CTL1 & ADC12BUSY) == 1);

	return ADC12MEM0 & 0x01;
}

char GetRandomByte(void)
{
	unsigned char randomNum = 0;
	char i = 0;
	for (i = 0; i < 8; i++)
	{
		 randomNum |= GetRandomBit() << i;
	}

	return randomNum;
}

//Sends the pairing request to the base station
void MobileUnitSendPairingRequest()
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
void BaseUnitSendPairingResponse()
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

void MobileUnitSendTestData(unsigned char byte1,unsigned char byte2,unsigned char byte3)
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

char CheckPacketIdentifier( unsigned char * packet)
{
	if((packet[1] == unitIdentifier[0]) && (packet[2] == unitIdentifier[1]))
		return 1;
	else
		return 0;
}

void State_ProgramPairing_MobileUnit()
{
	switch(pairingState)
	{
	case PairingInitializing:
		//disable rcv
		if(RadioIsReceiving())
			RadioDisableReceive();
		//send pairing data
		MobileUnitSendPairingRequest();
		pairingState = PairingWaitingForResponse;
		break;
	case PairingWaitingForResponse:
		//check to se if we got response to our pairing request
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
			BaseUnitSendPairingResponse();
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
	if(pairingButton.pressEvent)
	{
		pairingButton.pressEvent = 0;
		MobileUnitSendTestData(0x55,0x00,0x00);
	}
}
void State_ProgramRunning_BaseUnit()
{
	const char bytesAvailable = RadioIsDataAvailable();
	if(bytesAvailable)
	{
		unsigned char * rxData = RadioGetRxBuffer();
		//check to see if test command
		if(rxData[0] == PACKET_MOBILE_TEST_DATA)
		{
			if(CheckPacketIdentifier(rxData))
			{
				INVERTBIT(P1OUT, BIT0);
				//RadioDisableReceive();
				//RadioEnableReceive();
			}
		}
	}
}
