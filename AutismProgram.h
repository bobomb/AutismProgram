#include "cc430x613x.h"
#include "RadioControl.h"
#include "Helpers.h"
#include "Button.h"

/*******************
 * Variable Definition
 */
#define PAIRING_MODE_TIMEOUT_MS 10000
//first byte of the mobile send pairing request command
#define PACKET_MOBILE_PAIRING_REQUEST	0x10
//first byte of the base send pairing response command
#define PACKET_BASE_PAIRING_RESPONSE	0x1F

/*******************
 * State enums
 */
enum  {ProgramInitializing, ProgramIdle, ProgramPairing, ProgramRunning, ProgramPairAcknowledged} programState;
enum  {BaseUnit, MobileUnit} programRole;
enum  {PairingInitializing, PairingWaitingForResponse, PairingComplete} pairingState;

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
unsigned long ledBlinkCounter = 0;

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
