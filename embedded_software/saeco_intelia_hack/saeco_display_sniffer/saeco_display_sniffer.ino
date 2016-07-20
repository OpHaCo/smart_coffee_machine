/******************************************************************************
 * @file    saeco_display_sniffer.ino
 * @author  Rémi Pincent - INRIA
 * @date    10/07/2016 
 *
 * @brief Snif Chip On Glass SPI data lines.
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

/**************************************************************************
 * Global Variables
 **************************************************************************/

/**************************************************************************
 * Macros
 **************************************************************************/
//The number of integers per data packet
#define dataLength  2000U

/**************************************************************************
 * Static Variables
 **************************************************************************/
//Initialize T3SPI class as SPI_SLAVE
static T3SPI SPI_SLAVE;

//Initialize the arrays for incoming data
static volatile uint8_t data[dataLength] = {0};

static volatile bool full = false;

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

  Serial.println("Start Saeco SPI sniffer");
}

void loop(){  
  if(full)
  {
    Serial.println("new buffer");
    for(int i = 0; i < dataLength; i++)
    {
      Serial.println(data[i]);
    }
    SPI_SLAVE.dataPointer = 0;
    full = false;
  }
}

//Interrupt Service Routine to handle incoming data
void spi0_isr(void)
{
  if(SPI_SLAVE.dataPointer == dataLength-1)
  {
    full = true;
    uint8_t trash = SPI0_POPR;
    SPI0_SR |= SPI_SR_RFDF;
  }
  else
  {
    SPI_SLAVE.rx8 (data, dataLength);
  }
}
