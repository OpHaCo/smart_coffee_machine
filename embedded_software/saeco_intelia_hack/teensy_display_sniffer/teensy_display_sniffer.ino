/******************************************************************************
 * @file    teensy_display_sniffer.ino
 * @author  Rémi Pincent - INRIA
 * @date    26 févr. 2014
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
}ECoffeeMachineStatuses;
/**************************************************************************
 * Global Variables
 **************************************************************************/

/**************************************************************************
 * Macros
 **************************************************************************/
//The number of integers per data packet
#define dataLength  500U

/**************************************************************************
 * Static Variables
 **************************************************************************/
//Initialize T3SPI class as SPI_SLAVE
static T3SPI SPI_SLAVE;

//Initialize the arrays for incoming data
static volatile uint8_t data[dataLength] = {0};
//bit field of statuses - bit position got from ECoffeeMachineStatuses
static volatile uint32_t detectedStatuses = 0;

static volatile bool seq1 = false;
static volatile bool seq2 = false;

static uint8_t offSeq[] = {15, 7, 7};
static uint8_t calcCleanSeq[] = {63, 63, 63, 255};

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

  Serial.println("Start COG SPI sniffer");
}

void loop(){  
  if(detectedStatuses)
  {
    if(detectedStatuses & (1 << OFF))
    {
      Serial.println("OFF");
      detectedStatuses &= ~(1 << OFF);
    }
    if(detectedStatuses & (1 << CALC_CLEAN))
    {
      Serial.println("CALC_CLEAN");
      detectedStatuses &= ~(1 << CALC_CLEAN);
    }
  }
}

bool detectSeq(uint8_t* seq, uint8_t length)
{
  int currIndex = 0;
  for(int i = 0; i < length ; i++)
  {
    if((SPI_SLAVE.dataPointer - 1) < (i + length - 1))
      currIndex = dataLength - (SPI_SLAVE.dataPointer - 1) - (i + length - 1);
    else
      currIndex = (SPI_SLAVE.dataPointer - 1) - (i + length - 1);
        
    if(seq[i] != data[currIndex])
      return false;
  }
  return true;
}

//Interrupt Service Routine to handle incoming data
void spi0_isr(void)
{
  SPI_SLAVE.rx8 (data, dataLength);
  detectedStatuses |= detectSeq(offSeq, sizeof(offSeq))             << OFF;  
  detectedStatuses |= detectSeq(calcCleanSeq, sizeof(calcCleanSeq)) << CALC_CLEAN;   
}
