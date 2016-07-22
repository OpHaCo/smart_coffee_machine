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
  OFF                     = 0,
  CALC_CLEAN              = 1,
  NO_WATER                = 2,
  WATER_CHANGE            = 3,
  WEAK_COFFEE             = 4,
  MEDIUM_COFFEE           = 5,
  STRONG_COFFEE           = 6,
  SPOON_COFFEE            = 7,
  DOOR_OPEN               = 8,
  COFFEE_GROUND_OPEN      = 9,
  NO_COFFEE_WARNING       = 10,
  NO_COFFEE_ERROR         = 11,
  UNKNOWN_OK              = 12,
  UNKNOWN_WARNING         = 13,
  UNKNOWN_ERROR           = 14,
  COFFEE_GROUND_FULL      = 15,
  OUT_OF_ENUM_SAECO_STATUS
}ESaecoStatus;
