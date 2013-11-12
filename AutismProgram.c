/******************************************************************************
* Notes: To change packet size make sure to change the packet size in RfRegSettings.c in both 868 mhz and 900 mhz
******************************************************************************/
//mode 0 = mobile
//mode 1 = base
#define PROGRAM_MODE 0
//#define PROGRAM_MODE 1
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

    programState = ProgramIdle;
    //enable interrupts again
    __bis_SR_register( GIE );

    while (1)
    {
        unsigned long deltaTime = currentTimeMS - lastTimeMS;
        lastTimeMS = currentTimeMS;

		__disable_interrupt();
		ButtonUpdate(&pairingButton, deltaTime, P1IN & BIT7);
		__bis_SR_register( GIE );

		switch(programState)
		{
		case ProgramIdle:
			if(pairingButton.holdEvent)
			{
				pairingButton.holdEvent = 0;
				pairingModeStartTime = currentTimeMS;
				programState = ProgramPairing;
				pairingState = PairingInitializing;
				SETBIT(P1OUT, BIT0);
				RadioEnableReceive();
			}
			break;
		case ProgramPairing:
			if(currentTimeMS - pairingModeStartTime > PAIRING_MODE_TIMEOUT_MS)
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
							baseUnitIdentifier[0] = rxData[1];
							baseUnitIdentifier[1] = rxData[2];
							//send a response pkt (make the assumption that it gets the response)
							BaseUnitSendPairingResponse();
							programState = ProgramPairAcknowledged;
							pairingState = PairingComplete;
						}
					}
				}
				else if(programRole == MobileUnit)
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
									if((baseUnitIdentifier[0] == rxData[1]) && (baseUnitIdentifier[1] == rxData[2]))
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
			}
			break;
		case ProgramPairAcknowledged: //blink LED
			if(ledBlinkCounter < 5000)
			{
				ledBlinkCounter+=deltaTime;
				if((ledBlinkCounter / 250) % 2 == 0)
				{
					SETBIT(P1OUT, BIT0);
				}
				else
					CLEARBIT(P1OUT,BIT0);
			}
			else
			{
				//turn off LED
				CLEARBIT(P1OUT,BIT0);
				//reset blink counter
				ledBlinkCounter = 0;
				programState = ProgramRunning;
			}

			break;
		case ProgramRunning:
			//in case we want to pair again
			if(pairingButton.holdEvent)
			{
				pairingButton.holdEvent = 0;
				pairingModeStartTime = currentTimeMS;
				programState = ProgramPairing;
				pairingState = PairingInitializing;
				SETBIT(P1OUT, BIT0);
				RadioEnableReceive();
			}

			if(RadioIsDataAvailable())
			{

			}
			break;
		}
	__no_operation();
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
	currentTimeMS++;
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
      //P1IE = 0;                           // Debounce by disabling buttons
      ButtonSetPressed(&pairingButton);
      CLEARBIT(P1IFG, BIT7);
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
	baseUnitIdentifier[0] = TxBuffer[1] = GetRandomByte();
	baseUnitIdentifier[1] = TxBuffer[2] = GetRandomByte();
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
	TxBuffer[1] = baseUnitIdentifier[0];
	TxBuffer[2] = baseUnitIdentifier[1];
	TxBuffer[3] = 0xFF;
	TxBuffer[4] = 0xFF;
	TxBuffer[5] = 0xFF;
	RadioTransmitData( (unsigned char*)TxBuffer, sizeof TxBuffer);

	while(RadioIsTransmitting())
		__no_operation();

	RadioEnableReceive();
}
