/******************************************************************************
 * @file    saeco_intelia.h
 * @author  Rémi Pincent - INRIA
 * @date    25/05/2016
 *
 * @brief Control Saeco intelia threw ESP8266 
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
 *****************************************************************************/
#ifndef SAECO_INTELIA
#define SAECO_INTELIA

#ifdef __cplusplus
extern "C"
{
#endif
/**************************************************************************
 * Include Files
 **************************************************************************/
#include <stdint.h>
#include <stdbool.h>
 
/**************************************************************************
 * Manifest Constants
 **************************************************************************/

/**************************************************************************
 * Type Definitions
 **************************************************************************/
typedef struct {
  /** button hacks - simulate buttons */
  uint8_t _u8_smallCupBtnPin;
  uint8_t _u8_bigCupBtnPin;
  uint8_t _u8_teaCupBtnPin;
  uint8_t _u8_powerBtnPin;
  uint8_t _u8_coffeeBrewBtnPin;
  uint8_t _u8_hiddenBtnPin;
  /** button hacks - get button state */
  uint8_t _u8_onSmallCupBtnPin;
  uint8_t _u8_onBigCupBtnPin;
  uint8_t _u8_onTeaCupBtnPin;
  uint8_t _u8_onPowerBtnPin;
  uint8_t _u8_onCoffeeBrewBtnPin;
  uint8_t _u8_onHiddenBtnPin;
}TsCoffeeBtnPins;

typedef void(*TfOnButtonPress)(uint32_t arg_u32_pressDurationMs);

/** register cb on button presses */
typedef struct {
  TfOnButtonPress _pf_onSmallCupBtnPress;
  TfOnButtonPress _pf_onBigCupBtnPress;
  TfOnButtonPress _pf_onTeaCupBtnPress;
  TfOnButtonPress _pf_onPowerBtnPress;
  TfOnButtonPress _pf_onCoffeeBrewBtnPress;
  TfOnButtonPress _pf_onHiddenBtnPress;
}TsButtonPressCb;

/**************************************************************************
 * Global variables
 **************************************************************************/

/**************************************************************************
 * Macros
 **************************************************************************/
#define NOT_USED_COFFEE_PIN 0xFFU

/**************************************************************************
 * Global Functions Declarations
 **************************************************************************/
extern void saecoIntelia_init(TsCoffeeBtnPins* arg_ps_buttonPins, TsButtonPressCb* arg_ps_buttonPressCbs); 

extern void saecoIntelia_power(void); 
extern bool saecoIntelia_isPowered(void);

extern void saecoIntelia_smallCup(void); 

extern void saecoIntelia_bigCup(void); 

/** must be called in order to handle button presses cb */
extern void saecoIntelia_update(void);

#ifdef __cplusplus
}
#endif

#endif /* SAECO_INTELIA */