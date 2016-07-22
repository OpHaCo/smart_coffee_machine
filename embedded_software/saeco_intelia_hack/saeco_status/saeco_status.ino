/******************************************************************************
 * @file    saeco_status.ino
 * @author  Rémi Pincent - INRIA
 * @date    01/07/2016 
 *
 * @brief Get Saeco Intelia status sniffing Chip On Glass SPI data lines. 
 * 
 * @details Saeco intelia has a COG display but I could not find any datasheet
 * neither doc (refer README.md). After sniffing 16 lines flex connector I 
 * found display was controlled over SPI @2.5M. Sniffing SPI MOSI done using  
 * an Arduino mini. ESP8266 SPI slave documentation not clear. After decoding
 * some data, I found a functional way to get coffee machine status from SPI
 * lines. 
 * 
 *
 * Project : smart_coffee
 * Contact:  Rémi Pincent - remi.pincent@inria.fr
 *
 * Revision History:
 * https://github.com/Lahorde/smart_coffee.git
 * 
 * LICENSE :
 * smart_coffee (c) by Rémi Pincent
 * smart_coffee is licensed under a
 * Creative Commons Attribution-NonCommercial 3.0 Unported License.
 *
 * You should have received a copy of the license along with this
 * work.  If not, see <http://creativecommons.org/licenses/by-nc/3.0/>.

/**************************************************************************
 * Include Files
 **************************************************************************/
#include <t3spi.h>
#include <saeco_status.h>

/**************************************************************************
 * Manifest Constants
 **************************************************************************/

/**************************************************************************
 * Type Definitions
 **************************************************************************/
typedef enum{
  UINT32,
  UINT64,
  ARRAY
}EPatternType;

 typedef enum {
    OK_STATUS = 0,  // Green backlight
    WARNING, // Yellow backlight
    ERROR,    // Red backlight
    POWER_OFF,// No backlight
    UNKNOWN, 
    OUT_OF_ENUM_STATUS_TYPE
 }ELCDBacklightStatus;
  
typedef struct{
  const ESaecoStatus lcdStatus;
  const ELCDBacklightStatus lcdStatusType;
  const union{
    uint32_t pattern32;
    uint64_t pattern64;
    uint8_t patternArray[];
  }pattern;
  const EPatternType patternType;
  const char* name;
}TsLCDSPIPattern;

/**************************************************************************
 * Global Variables
 **************************************************************************/

/**************************************************************************
 * Macros
 **************************************************************************/
//The number of integers per data packet
#define SPI_BUFF_SIZE  200U

#define PATTERN_32(byte0, byte1, byte2, byte3)\
        (uint32_t)(byte0 | byte1 << 8 | byte2 << 16 | byte3 << 24) 

#define PATTERN_64(byte0, byte1, byte2, byte3, byte4, byte5, byte6, byte7)\
        (uint64_t)(PATTERN_32(byte0, byte1, byte2, byte3) \
                | (uint64_t)PATTERN_32(byte4, byte5, byte6, byte7) << 32 )

#define GREEN_BL_PIN 23U
#define RED_BL_PIN   22U

#define FRAME_START_BYTES (uint16_t) 0x00FF

/** when no state change - current state sent at this interval */
#define SEND_INTERVAL 10000
/**************************************************************************
 * Static Variables
 **************************************************************************/
//Initialize T3SPI class as SPI_SLAVE
static T3SPI SPI_SLAVE;

//Initialize the arrays for incoming data
static volatile uint8_t _spiData[SPI_BUFF_SIZE] = {0};
//bit field of statuses - bit position got from ELCDStatuses
static volatile uint32_t _detectedStatuses = 0;

static volatile uint8_t _buffInit = 0;
static volatile uint64_t _spiPatternDataBuff = 0;

TsLCDSPIPattern _patterns[] = {
  /** First block : 32 bits _patterns */
  {OFF,                POWER_OFF , {.pattern32 = PATTERN_32(128, 128,   128,   0)}, UINT32, "OFF"},
  {NO_WATER,           ERROR     , {.pattern32 = PATTERN_32(224, 240, 252, 254)}, UINT32, "NO_WATER"},
  {WATER_CHANGE,       WARNING   , {.pattern32 = PATTERN_32(2  ,   2,   2,   2)}, UINT32, "WATER_CHANGE"},
  {WEAK_COFFEE,        UNKNOWN   , {.pattern32 = PATTERN_32(192,   0,  15, 120)}, UINT32, "WEAK_COFFEE"},
  {STRONG_COFFEE,      UNKNOWN   , {.pattern32 = PATTERN_32(31,   15,  0,  192)}, UINT32, "STRONG_COFFEE"},
  {SPOON_COFFEE,       OK_STATUS , {.pattern32 = PATTERN_32( 3,    3,  3,    3)}, UINT32, "SPOON_COFFEE"},
  {DOOR_OPEN,          ERROR     , {.pattern32 = PATTERN_32(240,  248, 124,   62)}, UINT32, "DOOR_OPEN"},
  {COFFEE_GROUND_OPEN, ERROR     , {.pattern32 = PATTERN_32(248,  248, 248,   0)}, UINT32, "COFFEE_GROUND_OPEN"},
  {COFFEE_GROUND_FULL, ERROR     , {.pattern32 = PATTERN_32(249,  248, 248,   240)}, UINT32, "COFFEE_GROUND_FULL"},

  /** Second block : 64 bits _patterns */
  {MEDIUM_COFFEE,      UNKNOWN  , {.pattern64 = PATTERN_64(14, 56, 48, 24, 15, 0, 192, 240)}, UINT64, "MEDIUM_COFFEE"}
  /** Third block : array _patterns */
};

static TsLCDSPIPattern* _p_currentPattern = NULL;
static ELCDBacklightStatus _currentStatusType = OUT_OF_ENUM_STATUS_TYPE;
static unsigned long _lastSendTime = 0;

/**************************************************************************
 * Local Functions Declarations
 **************************************************************************/

/**************************************************************************
 * Global Functions Defintions
 **************************************************************************/
 
 /**************************************************************************
 * Local Functions Definitions
 **************************************************************************/
void setup(){
  
  Serial.begin(115200);
  //Serial1.begin(115200);

  pinMode(GREEN_BL_PIN, INPUT_PULLUP);
  pinMode(RED_BL_PIN, INPUT_PULLUP);
  
  // default PINS 
  SPI_SLAVE.begin_SLAVE(SCK, MOSI, MISO, CS0);
  
  // CPOL1 CPHA1 
  SPI_SLAVE.setCTAR_SLAVE(8, SPI_MODE3);
  
  //Enable the SPI0 Interrupt
  NVIC_ENABLE_IRQ(IRQ_SPI0);

  Serial.println("Start Saeco status getter");
}

void loop(){  
  ELCDBacklightStatus newStatusType = getStatusType();
  ESaecoStatus newStatus = OUT_OF_ENUM_SAECO_STATUS;
  
  if(_detectedStatuses)
  {
    for(int i = 0; i < sizeof(_patterns) / sizeof(TsLCDSPIPattern); i++)
    {
      if(_detectedStatuses & (1 << _patterns[i].lcdStatus))
      {
        if(_p_currentPattern != &_patterns[i])
        {
          newStatus = _patterns[i].lcdStatus;
          Serial.println(_patterns[i].name);
          Serial.flush();
          _p_currentPattern = &_patterns[i];    
        }

        /** clear status that has been set in interrupted context */
        _detectedStatuses &= ~(1 << _patterns[i].lcdStatus);
      }
    }
  }

  if((newStatusType != OUT_OF_ENUM_STATUS_TYPE && _currentStatusType != newStatusType)
       || (newStatus != OUT_OF_ENUM_SAECO_STATUS && newStatus != _p_currentPattern->lcdStatus)
       || (millis() - _lastSendTime > SEND_INTERVAL))
  {
    _currentStatusType = newStatusType;
    ESaecoStatus saecoStatus = computeStatus();

    if(saecoStatus != OUT_OF_ENUM_SAECO_STATUS)
      sendStatus(saecoStatus);
    else
      Serial.println("Error - invalid status "); 
  }
}

/**
 * Compute status from actual status and status type
 */
ESaecoStatus computeStatus(void)
{
  ESaecoStatus ret = OUT_OF_ENUM_SAECO_STATUS;

  if(_currentStatusType == POWER_OFF)
  {
    ret = OFF;
  }
  else if(_p_currentPattern == NULL)
  {
    /** no status captured on SPI */
    if(_currentStatusType == OK_STATUS) ret = UNKNOWN_OK;
    else if(_currentStatusType == WARNING) ret = UNKNOWN_WARNING;
    else if(_currentStatusType == ERROR) ret = UNKNOWN_ERROR;
  }
  else if(_p_currentPattern->lcdStatusType == UNKNOWN)
  {
    if(_p_currentPattern->lcdStatus == WEAK_COFFEE
      || _p_currentPattern->lcdStatus == MEDIUM_COFFEE
      || _p_currentPattern->lcdStatus == STRONG_COFFEE)
    {
      if(_currentStatusType == WARNING)
        ret = NO_COFFEE_WARNING;
      else if(_currentStatusType == ERROR)
        ret = NO_COFFEE_ERROR;
      else if(_currentStatusType == OK_STATUS)
        ret = _p_currentPattern->lcdStatus;
    }
  }
  else if(_p_currentPattern->lcdStatusType != _currentStatusType)
  {
    /** error in status */
    if(_currentStatusType == OK_STATUS) ret = UNKNOWN_OK;
    else if(_currentStatusType == WARNING) ret = UNKNOWN_WARNING;
    else if(_currentStatusType == ERROR) ret = UNKNOWN_ERROR;
  }
  else
  {
    ret = _p_currentPattern->lcdStatus;
  }
  return ret;
}

void sendStatus(ESaecoStatus arg_machineStatus)
{  
  /** Frame sent is 3 bytes length 
   *  Byte    0                          1                            2                     
   *         FRAME_START_BYTE MSByte     FRAME_START_BYTE LSByte      machine status         
   */
   uint8_t frame[] = {
    (FRAME_START_BYTES >> 8 ) & 0xFF, 
    FRAME_START_BYTES         & 0xFF, 
    arg_machineStatus         & 0xFF
    };
  
  Serial1.write(frame, sizeof(frame)); 
  Serial1.flush();
  _lastSendTime = millis();

  Serial.print("sent status : ");
  Serial.print(arg_machineStatus);
  Serial.print(" at ");
  Serial.print((long) _lastSendTime);
  Serial.println("ms");
}

ELCDBacklightStatus getStatusType(void)
{
  bool isRed = digitalRead(RED_BL_PIN);
  bool isGreen = digitalRead(GREEN_BL_PIN);
  ELCDBacklightStatus ret ;
  
  /** when off some spike occur on green wire (even
   *  when configuring input as pull up)
   *  filter it.
   */
  delay(1);
  if(digitalRead(GREEN_BL_PIN) != isGreen || digitalRead(RED_BL_PIN) != isRed)
    return OUT_OF_ENUM_STATUS_TYPE;
  
  if(isRed && isGreen)
    ret = WARNING;
  else if(isRed && !isGreen)
    ret = ERROR;
  else if(!isRed && isGreen)
    ret = OK_STATUS;
  else 
    ret = POWER_OFF;
  return ret;
}

//Interrupt Service Routine to handle incoming data
void spi0_isr(void)
{
  SPI_SLAVE.rx8 (_spiData, SPI_BUFF_SIZE);
  _spiPatternDataBuff = _spiPatternDataBuff << 8 | _spiData[SPI_SLAVE.dataPointer - 1];
  
  if(_buffInit < sizeof(_spiPatternDataBuff))
  {
    _buffInit++;
  }
  else
  {
    if(_spiPatternDataBuff == 0)
      return;
      
    uint32_t buff32 = _spiPatternDataBuff >> 32;

    for(int i = 0; i < sizeof(_patterns) / sizeof(TsLCDSPIPattern); i++)
    {
      if(_patterns[i].patternType == UINT64)
      {
        if(_spiPatternDataBuff == _patterns[i].pattern.pattern64)
        {
          _detectedStatuses |= 1 << _patterns[i].lcdStatus;
        }
      }
      else if(_patterns[i].patternType == UINT32 && buff32 != 0)
      {        
        if(buff32 == _patterns[i].pattern.pattern32)
        {
          _detectedStatuses |= 1 << _patterns[i].lcdStatus;
        }
      }
    }
  }
}
