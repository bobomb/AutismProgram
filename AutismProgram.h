#include "cc430x613x.h"
#include "RadioControl.h"
#include "Helpers.h"
#include "Button.h"

/*******************
 * Variable Definition
 */
#define PAIRING_MODE_TIMEOUT_MS 10000
//first byte of the mobile send pairing request command
#define PACKET_COMMAND_PAIR	0x10
enum  {ProgramInitializing, ProgramIdle, ProgramPairing, ProgramRunning} programState;
enum  {BaseUnit, MobileUnit} programRole;
enum  {PairingInitializing, PairingWaitingForResponse} pairingState;

unsigned char TxBuffer[PACKET_LEN]= {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

unsigned char packetReceived;
unsigned char packetTransmit;
unsigned char baseUnitIdentifier[2];

TButtonInfo pairingButton = {Unpressed,0,0,0,0,0,0,0};

//holds the current number of milliseconds (roughly) since start
volatile unsigned long currentTimeMS = 0;
volatile unsigned long lastTimeMS = 0;
//holds the time that pairing started
unsigned long pairingModeStartTime = 0;

/*******************
 * Function Definition
 */
void InitButtonLeds(void);
void InitMillisecondTimer();
void MobileUnitSendPairingRequest();
void BaseUnitSendPairingResponse();
char GetRandomByte(void);
char GetRandomBit(void);


/*
    while(1)
    {
        unsigned long deltaTime = currentTimeMS - lastTimeMS;
        lastTimeMS = currentTimeMS;

		__disable_interrupt();
		ButtonUpdate(&pairingButton, deltaTime, P1IN & BIT7);
		__bis_SR_register( GIE );

		if(!RadioIsTransmitting())
		{
			RadioEnableReceive();
		}
		if(pairingButton.pressEvent)
		{
			RadioDisableReceive();
			RadioTransmitData( (unsigned char*)TxBuffer, sizeof TxBuffer);
			while(RadioIsTransmitting())
				__no_operation();
			pairingButton.pressEvent = 0;
		}

		if(RadioIsDataAvailable())
		{
			INVERTBIT(P1OUT, BIT0);
		}
    }
*/
