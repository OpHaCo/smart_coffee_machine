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

/**************************************************************************
 * Manifest Constants
 **************************************************************************/

/**************************************************************************
 * Type Definitions
 **************************************************************************/
typedef enum{
  OFF = 0,
  CALC_CLEAN,
  NO_WATER,
  WATER_HEATING,
  WEAK_COFFEE,
  MEDIUM_COFFEE,
  STRONG_COFFEE,
  SPOON_COFFEE,
  DOOR_OPEN,
  COFFEE_GROUND_OPEN,
  MEDIUM_COFFEE_ON_GOING,
  NO_COFFEE_RED,
  NO_COFFEE_YELLOW,
  OUT_OF_ENUM_STATUS
}ECoffeeMachineStatuses;

typedef enum{
  UINT32,
  UINT64,
  ARRAY
}EPatternType;

typedef struct{
  const ECoffeeMachineStatuses status;
  const union{
    uint32_t pattern32;
    uint64_t pattern64;
    uint8_t patternArray[];
  }pattern;
  const EPatternType patternType;
  const char* name;
}TsPattern;
/**************************************************************************
 * Global Variables
 **************************************************************************/

/**************************************************************************
 * Macros
 **************************************************************************/
//The number of integers per data packet
#define dataLength  200U

#define PATTERN_32(byte0, byte1, byte2, byte3)\
        (uint32_t)(byte0 | byte1 << 8 | byte2 << 16 | byte3 << 24) 

#define PATTERN_64(byte0, byte1, byte2, byte3, byte4, byte5, byte6, byte7)\
        (uint64_t)(PATTERN_32(byte0, byte1, byte2, byte3) \
                | (uint64_t)PATTERN_32(byte4, byte5, byte6, byte7) << 32 )
/**************************************************************************
 * Static Variables
 **************************************************************************/
//Initialize T3SPI class as SPI_SLAVE
static T3SPI SPI_SLAVE;

//Initialize the arrays for incoming data
static volatile uint8_t data[dataLength] = {0};
//bit field of statuses - bit position got from ECoffeeMachineStatuses
static volatile uint32_t detectedStatuses = 0;

static volatile uint8_t buffInit = 0;
static volatile uint64_t spiDataBuff = 0;

TsPattern patterns[] = {
  /** First block : 32 bits patterns */
  {OFF,             {.pattern32 = PATTERN_32(128, 128,   128,   0)}, UINT32, "OFF"},
  {NO_WATER,        {.pattern32 = PATTERN_32(224, 240, 252, 254)}, UINT32, "NO_WATER"},
  {WATER_HEATING,   {.pattern32 = PATTERN_32(2  ,   2,   2,   2)}, UINT32, "WATER_HEATING"},
  {WEAK_COFFEE,     {.pattern32 = PATTERN_32(192,   0,  15, 120)}, UINT32, "WEAK_COFFEE"},
  {STRONG_COFFEE,   {.pattern32 = PATTERN_32(31,   15,  0,  192)}, UINT32, "STRONG_COFFEE"},
  {SPOON_COFFEE,    {.pattern32 = PATTERN_32( 3,    3,  3,    3)}, UINT32, "SPOON_COFFEE"},
  {DOOR_OPEN,       {.pattern32 = PATTERN_32(240,  248, 124,   62)}, UINT32, "DOOR_OPEN"},
  {COFFEE_GROUND_OPEN,       {.pattern32 = PATTERN_32(224,  224, 224,   224)}, UINT32, "COFFEE_GROUND_OPEN"},
  {NO_COFFEE_RED,       {.pattern32 = PATTERN_32(255,  248, 224,   192)}, UINT32, "NO_COFFEE_RED"},
  {NO_COFFEE_YELLOW,       {.pattern32 = PATTERN_32(248,  248, 224,   192)}, UINT32, "NO_COFFEE_YELLOW"},

  /** Second block : 64 bits patterns */
  {MEDIUM_COFFEE,   {.pattern64 = PATTERN_64(14, 56, 48, 24, 15, 0, 192, 240)}, UINT64, "MEDIUM_COFFEE"},

  /** Third block : array patterns */
};

static uint8_t offSeq[] = {15, 7, 7};
static uint32_t offPattern = PATTERN_32(128, 128, 0, 0);
static uint32_t noWaterPattern = PATTERN_32(252, 254, 255, 127);
static uint32_t waterHeatingPattern = PATTERN_32(2, 2, 2, 2);
 
static uint8_t calcCleanSeq[] = {63, 63, 63, 255};
static uint32_t calcCleanPattern = PATTERN_32(63, 63, 63, 255);

static ECoffeeMachineStatuses currentStatus = OUT_OF_ENUM_STATUS;

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
  
  // default PINS 
  SPI_SLAVE.begin_SLAVE(SCK, MOSI, MISO, CS0);
  
  // CPOL1 CPHA1 
  SPI_SLAVE.setCTAR_SLAVE(8, SPI_MODE3);
  
  //Enable the SPI0 Interrupt
  NVIC_ENABLE_IRQ(IRQ_SPI0);

  Serial.println("Start Saeco status getter");
}

void loop(){  
  if(detectedStatuses)
  {
    for(int i = 0; i < sizeof(patterns) / sizeof(TsPattern); i++)
    {
      if(detectedStatuses & (1 << patterns[i].status))
      {
        if(currentStatus != patterns[i].status)
        {
          Serial.println(patterns[i].name);
          currentStatus = patterns[i].status;
        }
        detectedStatuses &= ~(1 << patterns[i].status);
      }
    }
  }
}

//Interrupt Service Routine to handle incoming data
void spi0_isr(void)
{
  SPI_SLAVE.rx8 (data, dataLength);
  spiDataBuff = spiDataBuff << 8 | data[SPI_SLAVE.dataPointer - 1];
  
  if(buffInit < sizeof(spiDataBuff))
  {
    buffInit++;
  }
  else
  {
    if(spiDataBuff == 0)
      return;
      
    uint32_t buff32 = spiDataBuff >> 32;

    for(int i = 0; i < sizeof(patterns) / sizeof(TsPattern); i++)
    {
      if(patterns[i].patternType == UINT64)
      {
        if(spiDataBuff == patterns[i].pattern.pattern64)
        {
          detectedStatuses |= 1 << patterns[i].status;
          return;
        }
      }
      else if(patterns[i].patternType == UINT32)
      {
        if(buff32 == patterns[i].pattern.pattern32)
        {
          detectedStatuses |= 1 << patterns[i].status;
          return;
        }
      }
    }
  }
}
