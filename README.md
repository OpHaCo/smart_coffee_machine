# Coffee machine Hack

Project has been done in Amiqual4Home Equipex Creativity Lab - https://amiqual4home.inria.fr/

<center><img src="http://www.machineacafe.net/wp-content/uploads/2015/09/Saeco-HD8751-11.jpg" width="200"></center>
<center>To hack : Saeco Intelia Coffee Machine</center>



## Description
**You purchased a Saeco Intelia coffee machine. You are very satisfied with it, but you would like to make your coffee from your bedroom just after you wake up...** 

Some other products can do it for you : http://smarter.am/coffee/ . But with Smarter coffee machine you will have to change Coffee filter each time you get a coffee. With an "automatic" coffee machine such as Intelia, coffee grounds are pushed to a compartment : you can do several coffee only by pushing buttons a putting a cup of coffee.

With this project you will :
 *  **make your automatic coffee machine smarter adding means to control it from anywhere**  e.g. : 
    *  prepare coffee for tomorrow morning at 8AM 
    *  give a vocal command to your smartphone that will make your coffee
    *  using a camera (refer 'Used in projects' section) to deliver coffee to identified users (refer example of smiling users ;- ) 
 * **get coffee machine statistics such as number of coffee per day...**
 
 * **improve some existing parts**
    * add a bigger coffee compartment (TODO)
    * add cup detection (TODO)
    * cup distribution (TODO)

## Prerequisities
 * **saeco intelia coffee machine** : https://www.amazon.com/Philips-Saeco-Intelia-Automatic-Espresso/dp/B00B5M3G26
 * some screws to open coffee machine
 * An external board connected to Coffee Machine front side daugther board : 
   * a fablab access to mill PCB, print 3d parts
   * **ESP8266** 
   * **Teensy 3.1/3.2 (optional)**
   * other electronic components refer hardware part

 * A linux machine hosting an MQTT broker an hosting needed software for your use case

### Hardware hacking
On paper front side with its buttons and LCD sceen gives all elements to hack coffee machine :

<center><img src="https://raw.githubusercontent.com/OpHaCo/smart_coffee_machine/master/img/saeco_front_side.jpg" width="350"></center>
<center>Coffee machine front side control panel</center>

Two things have to be done :
 * control machine from external MCU by sending some commands such as "Small Coffee", "Calc Clean"...
 * get coffee machine current status


#### First analysis
From daughterboard PCB :
<center><img src="https://raw.githubusercontent.com/OpHaCo/smart_coffee_machine/master/img/front_panel_daughter_board_annoted.jpg" width="500"></center>
<center>Coffee machine front side control components</center>

Behind display, no MCU :
<center><img src="https://raw.githubusercontent.com/OpHaCo/smart_coffee_machine/master/img/front_panel_daughter_board_zoom.jpg" width="500"></center>

**We can see** :
 * NO MCU!
 * 16 lines ribbon cable connecting motherboard and daughter board
 * 7 push buttons (PB1 to PB7) pulled up to 5V, debounced using an RC filter, (C25...) 
   * these buttons are either connected to a line of 16 line ribbon cable (PB5, , PB4, PB6) or both combined (threw resistor network) and connected to 16 line ribbon cable (PB1/PB2, PB3/PB7)
 * 18 lines display connector
 * a ChipOnGlass display - no info, datasheet can be found with COG references, except this thread : http://www.forum-raspberrypi.de/Thread-unbekanntes-display-identifizieren  

<center><img src="https://raw.githubusercontent.com/OpHaCo/smart_coffee_machine/master/img/cog_display_ref.jpg" width="200"></center>
<center>COG screen</center>

From this first analysis, we can suppose hacking can be done only from front side daughter board :
 * **hacking existing push buttons to control coffee machine** 
 * **sniffing display lines to get coffee machine display content and so getting current status**

Coffee machine main mother board (on rear side) won't been hacked. 

#### Buttons hack
<center><img src="https://raw.githubusercontent.com/OpHaCo/smart_coffee_machine/master/img/front_panel_buttons.png" width="500"></center>
<center>Front panel buttons identification.</center>

We want to keep coffee machine functional, i.e. :
**coffee machine can be both controlled from buttons and remotely using and external MCU**

On several line of 16 lines ribbon cable some buttons signals are combined. In order to hack it we will not connect bipolar transistors at cable level but at button level. 

In order to achieve this, a simple solution is to use a bipolar transistor in saturation (switch). It simulates button press. It is controlled from an external MCU :  

<img src="https://raw.githubusercontent.com/OpHaCo/smart_coffee_machine/master/img/button_hack_schematic.png" width="450">

#### Machine status
LCD can be either red, yellow, black or green. These backlight colors are set from 3 wires (Red - White - Green) near display connector. At 16 line cable, two signals control it :
 * at pin 14 : green control ( logical 1 => green)
 * at pin 15 : red control (logical 1 => red)
Screen yellow when (pin 14, pin16) =( 1, 1)

Reading on these lines using a digital input can be useful to get coffee machine error status.
In order to get more info, LCD screen must be analyzed.

#### Chip On Glass hack
Coffee machine status is displayed on LCD...  If we hack LCD to get displayed content we will get coffee machine status.

No COG datasheet can be found from display references. Reading some other COG datasheet gave some useful info : most of the time it is controlled using SPI / I2C.
Most of the LCD lines are connected to GND threw a capacitor. But other lines are interesting...
Using logic analyser on display connector lines gives some useful information. It looks like SPI with these characteristics :
 * 2.5MHz clock
 * CPOL = 1
 * CPHA = 1

Some capture are in ./hardware/logic_capture/

Some SPI bursts are sent at fixed interval :

<center><img src="https://raw.githubusercontent.com/OpHaCo/smart_coffee_machine/master/img/spi_burst.png" width="900"></center>
<center>SPI burst</center>
<center><img src="https://raw.githubusercontent.com/OpHaCo/smart_coffee_machine/master/img/spi_burst_byte.png" width="900"></center>
<center>SPI byte transmission in a burst</center>

**How to identify a coffee machine status from some SPI bytes?**

An intuitive solution is to find some unique patterns in SPI bytes when coffee machine displays a specific content. 

**How can my external MCU sniff SPI traffic?**
 * it must be configured as an SPI slave and read incoming bytes
 * incoming bytes are computed to identify some given patterns

 * on arduino uno : SPI slave in not supported using Arduino delivered libraries. But Nick Gammon release an SPI library supporting SPI slave http://www.gammon.com.au/forum/?id=10892 
Find some simple patterns from SPI slave bytes can be done... but it is too slow. A simple test is to echo a rising edge/falling edge when a byte is received. It is done by manipulating directly GPIO register e.g.:
    
        // set pin 4 to HIGH
        PORTD |= 1 << 4;
        // set pin 4 to LOW
        PORTD &= ~(1 << 4);

such a simple echo takes 5.4µs between two consecutive edges. We won't be able to identify precisely some pattern from SPI bytes. Too many data are lost.    
 * on an ESP8266 : SPI slave conflicts with flash memory, HSPI must be used http://bbs.espressif.com/viewtopic.php?t=77 . Not tested.
 * on Teensy 3.1/3.2, SPI slave library is included in Arduino Core. After testing, it is possible to compute SPI bytes to identify predefined patterns.

##### Pattern identification
In order to find unique patterns in SPI data for a given LCD displayed status :
 * some logic captures are done for each LCD displayed status using Salae logic analyzer
 * these captures ared decoded using SPI analyser and exported to CSV, some captures are available under : 
     
        ./hardware/logic_capture/SPI_decoded
 * SPI decoded files are computed using :
     * a bash script (very slow - given as exemple) located in 
        
                ./hardware/logic_capture/tools/get_pattern.sh 

     * or a python script, bash script was too slow

                ./hardware/logic_capture/tools/get_pattern.py

Te find unique pattern of minimum 2 bytes only present when a middle amount of coffee is displayed on screen :

        ./hardware/logic_capture/tools/get_pattern.py ./hardware/logic_capture/SPI_decoded/off.txt ./hardware/logic_capture/SPI_decoded/*.txt 2
        Try to find a unique pattern for a pattern size of 2
        new pattern [240 224 ] is unique - nb occurences = 162
        new pattern [128 128 ] is unique - nb occurences = 216
        new pattern [15 15 ] is unique - nb occurences = 378
 
It can be confirmed using : 

        pcregrep -c -M  '15\n.*15\n' ./hardware/logic_capture/SPI_decoded/*.txt

### software 
Add library description

## Setup

### hardware 

## Commands
Coffee machine controlled using MQTT protocol. 

### MQTT topics 
Here are MQTT topics for example given in : 

    ./embedded_software/saeco_intelia_hack/saeco_hack_example/    

- **"/amiqual4home/machine_place/small_cup"** : sending any byte to this value will simulate a short press on smallCoffee button

- **"/amiqual4home/machine_place/big_cup"**   : sending any byte to this value will simulate a short press on coffee button

- **"/amiqual4home/machine_place/power"**     : sending any byte to this value will simulate a short press on power button

- **"/amiqual4home/machine_place/brew"**      : sending any byte to this value will simulate a short press on brew button

- **"/amiqual4home/machine_place/tea"**       : sending any byte to this value will simulate a short press on tea button

- **"/amiqual4home/machine_place/clean"**     : sending any byte to this value will simulate a short press on clean button

Example :

    mosquitto_pub -h MQTT_BROKER_IP -t /amiqual4home/machine_place/saeco-intelia/on_button_press/smallCoffee -m 1

## Events
Buttons press events are send threw MQTT. Here are events MQTT topic for example given in :

    ./embedded_software/saeco_intelia_hack/saeco_hack_example/   

Button press topics messages payload contains press duration in ms. 

- **"/amiqual4home/machine_place/_on_button_press/powerButton"**        : power button pressed
 
- **"/amiqual4home/machine_place/_on_button_press/Coffee"**             : coffee button pressed
 
- **"/amiqual4home/machine_place/_on_button_press/smallCoffee"**        : small coffee button pressed
 
- **"/amiqual4home/machine_place/_on_button_press/brew"**               : brew coffee button pressed
 
Example to subscribe all button press events :

        mosquitto_sub -v -h MQTT_BROKER_IP -p 1883 -t /#

## Existing applications using this project
 * refer gateway_software/examples
 * coffee_machine_metrics : get coffee machine usage (TODO add picture)

## References
http://www.gammon.com.au/forum/?id=10892

## TODO
galère avec SPI
Description du hacking
Ajouter capture arduino et SPI
Add grafana capture
Detection et mise en place tasse
Detection debordement eau
Hack mecanique : plus grands compartiments

## Thanks to
Fablab group
