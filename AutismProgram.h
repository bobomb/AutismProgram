#include "cc430x613x.h"
#include "RadioControl.h"
#include "Helpers.h"
#include "Button.h"

/*******************
 * Variable Definition
 */
#define PAIRING_MODE_TIMEOUT_MS 10000
//time to wait between pairing retries
#define PAIRING_RESPONSE_RETRY_MS 2000
//first byte of the mobile send pairing request command
#define PACKET_MOBILE_PAIRING_REQUEST	0x10
//first byte of the base send pairing response command
#define PACKET_BASE_PAIRING_RESPONSE	0x1F
//test data packet
#define PACKET_MOBILE_TEST_DATA			0x2D
//accelerometer data packet
#define PACKET_MOBILE_ACCELEROMETER_DATA 0x3D

#define ACCELEROMETER_MOVEMENT_THRESHOLD	50
/*******************
 * State enums
 */
//Overall program state
enum  {ProgramInitializing, ProgramIdle, ProgramPairing, ProgramRunning, ProgramPairAcknowledged, ProgramTestMode} programState;
//Overall program role (is this a base unit or a mobile unit?)
enum  {BaseUnit, MobileUnit} programRole;
//Pairing state
enum  {PairingInitializing, PairingWaitingForResponse, PairingComplete} pairingState;
//radio transmit buffer. all radio transmit operations will fill this buffer with relevant data before calling transmit
unsigned char TxBuffer[PACKET_LEN]= {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
//stores the two byte random identifier
//the mobile unit will generate it's own and send to the base to pair
//after pairing the base will also store its identifier in here
unsigned char unitIdentifier[2];
//holds the button state for the pairing button on P1.7
TButtonInfo g_pairingButton = {Unpressed,0,0,0,0,0,0,0,0};
//holds the current number of milliseconds (roughly) since start
volatile unsigned long g_currentTimeMS = 0;
//holds the previous value for currenttime
volatile unsigned long g_lastTimeMS = 0;
//holds the time that pairing started
unsigned long g_pairingModeStartTime = 0;
//counter for handling the led blink in pairing mode
unsigned long g_ledBlinkCounter = 0;
//keeps track of the time between each iteration of the main loop
unsigned long g_deltaTime = 0;
//keeps track of the time to send ADC data
unsigned int g_dataTransmitCounter = 0;
//holds the accelerometer values read in by the ADC
volatile unsigned int accelerometerValues[3] = {0,0,0};
//holds the "normal" or resting values for the accelerometer
unsigned int normalAccelerometerValues[3] = {0,0,0};
//stores the random byte from the ADC
unsigned int randomByteFromAdc = 0;

/*******************
 * Function Definition
 */
//initializes buttons and ports for io
void InitPorts(void);
//initializes the timer peripheral for the millisecond counter
void InitMillisecondTimer();
//initializes the ADC to read the accelerometer correctly
void InitAdcForAccelerometer();
//called when mobile unit enters pairing mode
void MobileUnit_SetAccelerometerNormalValues();
void MobileUnit_SendPairingRequest();
//test function for debugging radio comms
void MobileUnit_SendTestData(unsigned char byte1,unsigned char byte2,unsigned char byte3);
//called after the base unit gets a pairing request from the mobile unit
void BaseUnit_SendPairingResponse();
//state handler routines for main loop, fairly self explanatory
void State_ProgramPairing_MobileUnit();
void State_ProgramPairing_BaseUnit();
void State_ProgramPairAcknowledged();
void State_ProgramRunning_MobileUnit();
void State_ProgramRunning_BaseUnit();
//checks to see if the identifier fields in a packet are correct
char CheckPacketIdentifier(unsigned char * packet);
//uses the ADC to read some noise and a generate random bit
char GetRandomBit(void);
//uses the ADC to read some noise and generate random a random byte
char GetRandomByte(void);
