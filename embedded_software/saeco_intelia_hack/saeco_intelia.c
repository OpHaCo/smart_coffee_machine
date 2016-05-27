/******************************************************************************
 * @file    saeco_intelia.c
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

/**************************************************************************
 * Include Files
 **************************************************************************/
#include "saeco_intelia.h"
#include <Arduino.h>
#include <stdio.h>
 
/**************************************************************************
 * Macros
 **************************************************************************/
#define NB_COFFEE_BUTTONS    6U
 
/**************************************************************************
 * Manifest Constants
 **************************************************************************/


/**************************************************************************
 * Type Definitions
 **************************************************************************/
 static char* _au8_coffeePinNames[NB_COFFEE_BUTTONS] = {
  "SMALL_CUP",
  "BIG_CUP",
  "TEA_CUP",
  "POWER",
  "COFFEE_BREW",
  "HIDDEN"
};

typedef enum 
{
  /** button hacks - simulate buttons */
  SMALL_CUP_BTN = 0,
  BIG_CUP_BTN,
  TEA_CUP_BTN,
  POWER_BTN,
  COFFEE_BREW_BTN,
  HIDDEN_BTN,
  
  /** button hacks - get button state */
  ON_SMALL_CUP_BTN,
  ON_BIG_CUP_BTN,
  ON_TEA_CUP_BTN,
  ON_POWER_BTN,
  ON_COFFEE_BREW_BTN,
  ON_HIDDEN_BTN,
}ECoffeeButtonsId;

typedef void (*TfonButtonStateChanged)(void);
/**************************************************************************
 * Global Variables
 **************************************************************************/

/**************************************************************************
 * Static Variables
 **************************************************************************/
 static uint8_t _au8_coffeePins[NB_COFFEE_BUTTONS*2] = {0};
 static volatile long int _a_li_pressDur[NB_COFFEE_BUTTONS] = {0};
 static TfonButtonStateChanged _apf_onButtonStateChangedCbs[NB_COFFEE_BUTTONS];
 static TfOnButtonPress _apf_buttonPressCbs[NB_COFFEE_BUTTONS] = {NULL};
 /* flag indicating button has been pressed in order to handle it
      in a non interrupted context */
 static volatile uint32_t _u32_buttPress = 0;

/**************************************************************************
 * Local Functions Declarations
 **************************************************************************/
 static void saecoIntelia_emulShortPress(ECoffeeButtonsId arg_e_buttonId);

/** IT */
 static void onSmallCupBtnChanged(void);
/** IT */
 static void onBigCupBtnChanged(void);
/** IT */
 static void onTeaCupBtnChanged(void);
/** IT */
 static void onPowerBtnChanged(void);
/** IT */
 static void onCoffeeBrewBtnChanged(void);
/** IT */
 static void onHiddenBtnChanged(void);

/** IT */
 static void onBtnChanged(ECoffeeButtonsId arg_e_buttonId);


 extern void __attachInterrupt(uint8_t pin, void(*handler)(void), int mode);

/**************************************************************************
 * Global Functions Defintions
 **************************************************************************/
void saecoIntelia_init(TsCoffeeBtnPins* arg_ps_buttonPins, TsButtonPressCb* arg_ps_buttonPressCbs)
{
  uint8_t loc_u8_index = 0;
  
  if(!arg_ps_buttonPins)
  {
    printf("No Btn pins given - exit");
    return;
  }
  _au8_coffeePins[SMALL_CUP_BTN]        = arg_ps_buttonPins->_u8_smallCupBtnPin;
  _au8_coffeePins[BIG_CUP_BTN]          = arg_ps_buttonPins->_u8_bigCupBtnPin;
  _au8_coffeePins[TEA_CUP_BTN]          = arg_ps_buttonPins->_u8_teaCupBtnPin;
  _au8_coffeePins[POWER_BTN]            = arg_ps_buttonPins->_u8_powerBtnPin;
  _au8_coffeePins[COFFEE_BREW_BTN]      = arg_ps_buttonPins->_u8_coffeeBrewBtnPin;
  _au8_coffeePins[HIDDEN_BTN]           = arg_ps_buttonPins->_u8_hiddenBtnPin;
  _au8_coffeePins[ON_SMALL_CUP_BTN]     = arg_ps_buttonPins->_u8_onSmallCupBtnPin;
  _au8_coffeePins[ON_BIG_CUP_BTN]       = arg_ps_buttonPins->_u8_onBigCupBtnPin;
  _au8_coffeePins[ON_TEA_CUP_BTN]       = arg_ps_buttonPins->_u8_onTeaCupBtnPin;
  _au8_coffeePins[ON_POWER_BTN]         = arg_ps_buttonPins->_u8_onPowerBtnPin;
  _au8_coffeePins[ON_COFFEE_BREW_BTN]   = arg_ps_buttonPins->_u8_onCoffeeBrewBtnPin;
  _au8_coffeePins[ON_HIDDEN_BTN]        = arg_ps_buttonPins->_u8_onHiddenBtnPin;
  
  if(arg_ps_buttonPressCbs)
  {
    _apf_buttonPressCbs[SMALL_CUP_BTN]    = arg_ps_buttonPressCbs->_pf_onSmallCupBtnPress;
    _apf_buttonPressCbs[BIG_CUP_BTN]      = arg_ps_buttonPressCbs->_pf_onBigCupBtnPress;
    _apf_buttonPressCbs[TEA_CUP_BTN]      = arg_ps_buttonPressCbs->_pf_onTeaCupBtnPress;
    _apf_buttonPressCbs[POWER_BTN]        = arg_ps_buttonPressCbs->_pf_onPowerBtnPress;
    _apf_buttonPressCbs[COFFEE_BREW_BTN]  = arg_ps_buttonPressCbs->_pf_onCoffeeBrewBtnPress;
    _apf_buttonPressCbs[HIDDEN_BTN]       = arg_ps_buttonPressCbs->_pf_onHiddenBtnPress;
  }
  
  /** set local cb in IT functions */
  _apf_onButtonStateChangedCbs[SMALL_CUP_BTN]    = &onSmallCupBtnChanged;
  _apf_onButtonStateChangedCbs[BIG_CUP_BTN]      = &onBigCupBtnChanged;
  _apf_onButtonStateChangedCbs[TEA_CUP_BTN]      = &onTeaCupBtnChanged;
  _apf_onButtonStateChangedCbs[POWER_BTN]        = &onPowerBtnChanged;
  _apf_onButtonStateChangedCbs[COFFEE_BREW_BTN]  = &onCoffeeBrewBtnChanged;
  _apf_onButtonStateChangedCbs[HIDDEN_BTN]       = &onHiddenBtnChanged;

  /** configure buttons used to emulate user button press */
  for(loc_u8_index = 0; loc_u8_index < NB_COFFEE_BUTTONS; loc_u8_index++)
  {
    if(_au8_coffeePins[loc_u8_index] == NOT_USED_COFFEE_PIN)
    {
      printf("!no config for button hack pin : %s\n", _au8_coffeePinNames[loc_u8_index]);
    }
    else
    {
      pinMode(_au8_coffeePins[loc_u8_index], OUTPUT);
    }
  }
  
    /** configure input used to get button statuses & press durations */
  for(loc_u8_index = 0; loc_u8_index < NB_COFFEE_BUTTONS; loc_u8_index++)
  {
    if(_au8_coffeePins[loc_u8_index + NB_COFFEE_BUTTONS] == NOT_USED_COFFEE_PIN)
    {
      printf("!no config for button status : %s\n", _au8_coffeePinNames[loc_u8_index]);
    }
    /** do not register IT if no cb registered */
    else if(_apf_buttonPressCbs[loc_u8_index])
    {
      pinMode(_au8_coffeePins[loc_u8_index + NB_COFFEE_BUTTONS], INPUT);
      attachInterrupt(_au8_coffeePins[loc_u8_index + NB_COFFEE_BUTTONS], 
                      _apf_onButtonStateChangedCbs[loc_u8_index],
                      CHANGE);
    }
    else
    {
      printf("no callback given for button status :  %s\n", _au8_coffeePinNames[loc_u8_index]);
    }
  }
}

void saecoIntelia_power(void)
{
  saecoIntelia_emulShortPress(POWER_BTN);
}

bool saecoIntelia_isPowered(void){}

void saecoIntelia_smallCup(void)
{
 saecoIntelia_emulShortPress(SMALL_CUP_BTN);
}

void saecoIntelia_bigCup(void){}

void saecoIntelia_update(void)
{
  uint8_t loc_u8_buttonIndex = 0;
  
  /** check button presses in non interrupted context */
  if(_u32_buttPress)
  {
    /** find button pressed */
    for(loc_u8_buttonIndex = 0; loc_u8_buttonIndex < NB_COFFEE_BUTTONS; loc_u8_buttonIndex++)
    {
      if((1 << loc_u8_buttonIndex) & _u32_buttPress)
      {
        _apf_buttonPressCbs[loc_u8_buttonIndex](_a_li_pressDur[loc_u8_buttonIndex]);
        /** reset flag */
        _u32_buttPress &= ~(1<<loc_u8_buttonIndex);
      }
    }
  }
}
 /**************************************************************************
 * Local Functions Definitions
 **************************************************************************/
 static void saecoIntelia_emulShortPress(ECoffeeButtonsId arg_e_buttonId)
 {
  if(_au8_coffeePins[arg_e_buttonId] != NOT_USED_COFFEE_PIN)
  {
    digitalWrite(_au8_coffeePins[arg_e_buttonId], HIGH);
    delay(100);
    digitalWrite(_au8_coffeePins[arg_e_buttonId], LOW);
  }
  else
  {
    printf("!Cannot emulate short press - no config for button hack pin : %s\n", _au8_coffeePinNames[arg_e_buttonId]);
  }
}

/** IT */
static void onSmallCupBtnChanged(void)
{
  /** button debounced on hw */
  onBtnChanged(SMALL_CUP_BTN);
}

/** IT */
static void onBigCupBtnChanged(void)
{
  /** button debounced on hw */
  onBtnChanged(BIG_CUP_BTN);
}

/** IT */
static void onTeaCupBtnChanged(void)
{
  /** button debounced on hw */
  onBtnChanged(TEA_CUP_BTN);
}

/** IT */
static void onPowerBtnChanged(void)
{
  /** button debounced on hw */
  onBtnChanged(POWER_BTN);
}

/** IT */
static void onCoffeeBrewBtnChanged(void)
{
  /** button debounced on hw */
  onBtnChanged(COFFEE_BREW_BTN);
}

/** IT */
static void onHiddenBtnChanged(void)
{
  /** button debounced on hw */
  onBtnChanged(HIDDEN_BTN);
}

/** IT */
static void onBtnChanged(ECoffeeButtonsId arg_e_buttonId)
{ 
  /** button debounced on hw side */
  if(digitalRead(_au8_coffeePins[arg_e_buttonId + NB_COFFEE_BUTTONS]))
  {
    if(_a_li_pressDur[arg_e_buttonId] == 0)
    {
      /** Error : button release but did not get button press */
      return;
    }
    
    if(_apf_buttonPressCbs[arg_e_buttonId])
    {
      
      /** set flag indicating button has been pressed in order to handle it
      in a non interrupted context */
      _u32_buttPress |= 1 << arg_e_buttonId;
      _a_li_pressDur[arg_e_buttonId] = millis() - _a_li_pressDur[arg_e_buttonId];
    }
  }
  else
  {
    /** button press */
    /* start press duration meas */
    _a_li_pressDur[arg_e_buttonId] = millis();
  }
}