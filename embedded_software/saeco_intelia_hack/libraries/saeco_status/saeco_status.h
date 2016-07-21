/******************************************************************************
 * @file    saeco_status.h
 * @author  Rémi Pincent - INRIA
 * @date    01/07/2016 
 *
 * @brief Saeco status library header 
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
  WATER_CHANGE,
  WEAK_COFFEE,
  MEDIUM_COFFEE,
  STRONG_COFFEE,
  SPOON_COFFEE,
  DOOR_OPEN,
  COFFEE_GROUND_OPEN,
  NO_COFFEE_WARNING,
  NO_COFFEE_ERROR,
  UNKNWOWN_OK,
  UNKNWOWN_WARNING,
  UNKNWOWN_ERROR,
  OUT_OF_ENUM_SAECO_STATUS
}ESaecoStatus;
