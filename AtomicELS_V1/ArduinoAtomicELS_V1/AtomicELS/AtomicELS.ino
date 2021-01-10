// Copyright (c) 2020 Jon Bryan
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// Jan 2021 Mark Naylor
// Added diagnostic code to monitor encoder pulses and output results to Serial monitor

//================================================================================
//
//  ELECTRONIC LEADSCREW
//  Jon R. Bryan
//  2020
//
//  This runs on an Arduino Mega (ATmega2560).
//
//  The target lathe in this case is a 1946 10" Logan Model 200, Serial #38108
//
//  The spindle encoder, which is mounted to the upper arm of the banjo, tucked in under
//  the reversing lever, and driven 1:1 by a modified 60-tooth timing pulley mounted
//  in place of the stud gear via a 6mm wide 220-2GT belt, outputs 800 counts per revolution.
//  The CL57Y stepper controller/driver from omc-stepperonline.com is set to 400 counts per
//  revolution, and controls a 3.5Nm Nema 24 motor mounted under the lower banjo arm.
//  The stepper drives the leadscrew through 8:1 (15t/120t) HTD3M timing pulleys and
//  a 16mm HTD420-3M belt (for 3200 steps per leadscrew revolution).
//  Everything fits (barely) with no permanent modifications to the lathe.
//  The leadscrew has a pitch of 8tpi (25600 steps per inch).
//  The carriage feeds at a rate of 39.0625 microinches per step.
//  It takes 256 steps to move the carriage or cross feed 0.010".
//
//  Pin change interrupts are used for spindle quadrature decoding and counting.
//  A timer/counter in PWM mode outputs the appropriate number of leadscrew steps per spindle tick.
//  Leadscrew steps are counted by a timer/counter overflow interrupt.
//
//  This will handle spindle speeds over 1500rpm up to one step for every spindle tick.
//  With 800-count spindle encoder resolution that is an interrupt every 50us (20khz).
//  The maximum rate that the stepper controller can handle is 200khz.
//  The absolute theoretical maximum would be 10 step pulses per spindle count,
//  which would translate to a nonsensical 0.250" per revolution feed at 1500rpm.
//  As implemented, the output period is adjusted and the maximum spindle RPM recalculated
//  for each feed rate to keep the maximum lead screw rate approximately constant
//  over the lathe's speed range, resulting in a speed limit of 70rpm at the maximum
//  rate of 4tpi (56rpm at 6.5mm).
//
//  A "Nextion" color LCD touchscreen, a 20ppr encoder knob and a
//  MOM-OFF-MOM toggle switch provide the user interface.
//  A great deal of functionality is bound up in the Nextion
//  display programming, so refer to the Nextion configuration.
//  There is no error checking in the Nextion communications, so there's a bit of risk there.
//  The display is configured to return data only on failures.
//  No problem as long as there's never a glitch.  An error-checking wrapper would be possible?
//
//================================================================================

#include "configuration.h"
#include "tables.h"

// M Naylor
#include "EncoderDiagnostics.h"

//Fixed synchronization bug 04/21/2020:jrb
//Added RPM blanking according to feed rate 04/22/2020:jrb


//================================================================================
// Setup
//================================================================================

void setup(void) {

  //Configure the input pins
  pinMode(SPINDLE_A, INPUT);          //Spindle encoder quadrature inputs
  pinMode(SPINDLE_B, INPUT);          //Put 2k pullups on these inputs to avoid spurious interrupts
  pinMode(ALARM, INPUT_PULLUP);       //ALM+ open collector output from controller (not implemented yet)
  pinMode(LEFT_MOM, INPUT_PULLUP);    //MOM-OFF-MOM toggle switch for controlling feed direction
  pinMode(RIGHT_MOM, INPUT_PULLUP);

  Serial.begin(38400);               //Keep the default port for debugging
  Serial2.begin(38400);               //Use USART2 for the Nextion display

  /*
  * Call Diagnostic setup code - MJMN Jan 2021
  */
  DiagnosticsSetup ();

  nextionInit();                      //Initialize the Nextion display
  nextionLeft();                      //Defaults to feeding toward the headstock

  feedSelect(inch_feed);              //Default to INCH mode

  feedFill(inch[pitchFind("  40")].steps);  //Initialize the lookup table for 40tpi (1 step per spindle tick).

  tc3Init();                          //Initialize the timers
  tc4Init();

  pcint4Enab();                       //Enable timer interrupt

  tc3Enab();                          //Enable timers
  tc4Enab();

  zeroSet(!waitSerial2);              //Zero the leadscrew and clear the limits.

  //Falling edges are sharper, and generally provide better noise margin
  attachInterrupt(digitalPinToInterrupt(KNOB_A), knob, FALLING);
  attachInterrupt(digitalPinToInterrupt(SPINDLE_A), spindle, FALLING);

  steps = 0;

}



//================================================================================
// Loop
//================================================================================

void loop(void) {

  int i;

  for (i = 0; i < 4; i++) {
    knobCheck();        //Has the encoder knob been turned?
    toggleCheck();      //Feed switch activated?
    nextionCheck();     //Has the display sent anything?
    nextionDirection(); //Correctly reflect the feed direction
    nextionLead();      //Display the leadscrew position.
    delay(25);          //No point in updating too fast
  }

  nextionRPM();         //Display less often so the flicker isn't so distracting.

  /*
  * Call diagnostic loop code - MJMN Jan 2021
  */
  DiagnosticsLoop ();
}


/**************************************************************
***************************************************************
***                                                         ***
***   Functions that communicate with the Nextion display   ***
***                                                         ***
***************************************************************
**************************************************************/

void nextionInit(void) {
  //Initialize the state of the display buttons, etc.

  //I dislike the "magic numbers" for the colors, but I'm not enough of a
  //C programmer to work around them without more effort than it seems to be worth here.
  //F() puts the string constants in flash memory, at least.

  //The display design is done in the Nextion editor.
  //Refer to the Nextion .HMI display configuration file for the object names and id's.

  Serial2.print(F("page start\xFF\xFF\xFF"));
  Serial2.print(F("inch_btn.pco=WHITE\xFF\xFF\xFF"));               //INCH text white
  Serial2.print(F("metric_btn.pco=BLACK\xFF\xFF\xFF"));             //METRIC black
  Serial2.print(F("diam_btn.pco=BLACK\xFF\xFF\xFF"));               // etc...
  Serial2.print(F("module_btn.pco=BLACK\xFF\xFF\xFF"));
  Serial2.print(F("inch_btn.bco=1024\xFF\xFF\xFF"));                //INCH background toggle
  Serial2.print(F("inch_btn.bco=44415\xFF\xFF\xFF"));               //44415 = pale blue
  Serial2.print(F("shoulder.left_lim.txt=\"-----\"\xFF\xFF\xFF"));
  Serial2.print(F("shoulder.right_lim.txt=\"-----\"\xFF\xFF\xFF"));
  Serial2.print(F("shoulder.left_lim.bco=33816\xFF\xFF\xFF"));      //33816 = dark blue
  Serial2.print(F("shoulder.right_lim.bco=33816\xFF\xFF\xFF"));     //33816 = dark blue
  //Serial2.print(F("thup=1\xFF\xFF\xFF"));                         //Auto wake on touch
  //Serial2.print(F("thsp=600\xFF\xFF\xFF"));                       //Sleep on no touch after 10 minutes
}

void nextionRPM(void) {
  // Sends the spindle rpm value to the display.

  char str[30];
  static unsigned int spin;
  int rpm;

  //If stopped, spin_rate is set to 0xFFFF by TC3 overflow interrupt.
  if ((spin = spinAvg(spin_rate)) < RPM20) {
    if (fault) {
      //A fault means that an encoder interrupt occurred before all the steps were output,
      //indicating that the lathe is running too fast for the feed rate.
      //Make RPM background red.  Might need more for a color-blind operator.
      sprintf(str, "%s%s", "rpm.bco=RED", NX_END);
      Serial2.print(str);
    }
    rpm = T3CPM / spin / SCPR;
    sprintf(str, "%s%d%s", "rpm.txt=\"", rpm, QTNX_END);
  } else {
    sprintf(str, "%s%s", "rpm.txt=\"0", QTNX_END);
  }
  Serial2.print(str);
}

void nextionLead(void) {
  //Output the leadscrew position to the display

  char leadstr[20];
  char str[50];

  sprintf(str, "%s%s%s", "shoulder.leadscrew.txt=\"", leadStr(leadscrew, leadstr), QTNX_END);
  Serial2.print(str);
}

void nextionCheck(void) {
  //Handler for touch events from the display

  char buf[20];

  //Things get a little complicated here.
  //Wait for an event from the Nextion display.
  //The mode buttons only send a "press" event.
  //The left/right and limit buttons also send a "release" event
  //which is used in the called functions to implement delays and spin checks.
  if (Serial2.available()) {
    Serial2.readBytes(buf, 7);
    //The mode-select ID's 1 through 4 appear on all three Nextion pages.
    //It took some care to make sure that the ID's were the same on each page.
    //ID's 5 and 6 are the left and right feed select "arrows", 7 is the "BACK" button,
    //and 8, 9 and 10 are the "LEFT", "ZERO" and "RIGHT" limit buttons.
    //            touch              page 0            page 1            page 2             press
    if (buf[0] == 0x65 && (buf[1] == 0x00 || buf[1] == 0x01 || buf[1] == 0x02) && buf[3] == 0x01) {
      //Figure out which button press came from the touchscreen:
      switch (buf[2]) {   // Component ID
        case inch_btn:
          feed_mode = inch_feed;
          break;
        case metric_btn:
          feed_mode = metric_feed;
          break;
        case diametral_btn:
          feed_mode = diametral_feed;
          break;
        case module_btn:
          feed_mode = module_feed;
          break;
        case left_btn:
          //Left arrow
          jogCheck(true);
          break;
        case right_btn:
          //Right arrow
          jogCheck(false);
          break;
        case lset_btn:
          leftSet();
          break;
        case zset_btn:
          zeroSet(waitSerial2);
          break;
        case rset_btn:
          rightSet();
          break;
        case lclr_btn:
          leftClr(waitSerial2);
          break;
        case rclr_btn:
          rightClr(waitSerial2);
          break;
      }
    }
    feedSelect(feed_mode);
  }
}

void leftSet(void) {
  //Set the left limit and update the display

  char lead[10];
  char str[30];

  left_limited = true;
  left_limit = leadscrew;
  sprintf(str, "%s%s%s", "shoulder.left_lim.bco=", nxDGREEN, NX_END);
  Serial2.print(str);
  sprintf(str, "%s%s%s", "shoulder.left_lim.txt=\"", leadStr(left_limit, lead), QTNX_END);
  Serial2.print(str);
}

void rightSet(void) {
  //Set the right limit and update the display

  char lead[10];
  char str[30];

  right_limited = true;
  right_limit = leadscrew;
  sprintf(str, "%s%s%s", "shoulder.right_lim.bco=", nxDGREEN, NX_END);
  Serial2.print(str);
  sprintf(str, "%s%s%s", "shoulder.right_lim.txt=\"", leadStr(right_limit, lead), QTNX_END);
  Serial2.print(str);
}

void leftClr(bool wait) {
  //Clear the limit and update the display

  if (spin_rate != SPINDLE_STOPPED) {
    //Refuse if the spindle is turning, for safety's sake
    Serial2.print(F("shoulder.lclr_btn.bco2=RED\xFF\xFF\xFF"));
  } else {
    left_limited = false;
    Serial2.print(F("shoulder.left_lim.bco=33816\xFF\xFF\xFF"));
    Serial2.print(F("shoulder.left_lim.txt=\"-----\"\xFF\xFF\xFF"));
  }
  if (wait) {
    while (!Serial2.available()); //Wait for the release event
    Serial2.print(F("shoulder.lclr_btn.bco2=1024\xFF\xFF\xFF"));
  }
}

void rightClr(bool wait) {
  //Clear the limit and update the display

  if (spin_rate != SPINDLE_STOPPED) {
    //Refuse if the spindle is turning, for safety's sake
    Serial2.print(F("shoulder.rclr_btn.bco2=RED\xFF\xFF\xFF"));
  } else {
    right_limited = false;
    Serial2.print(F("shoulder.right_lim.bco=33816\xFF\xFF\xFF"));
    Serial2.print(F("shoulder.right_lim.txt=\"-----\"\xFF\xFF\xFF"));
  }
  if (wait) {
    while (!Serial2.available()); //Wait for the release event
    Serial2.print(F("shoulder.rclr_btn.bco2=1024\xFF\xFF\xFF"));
  }
}

void zeroSet(bool wait) {
  //Zero the leadscrew and clear the limits.
  //Optionally wait for a button release event from the display.

  if (spin_rate != SPINDLE_STOPPED) {
    //Refuse if the spindle is turning, for safety's sake,
    //because having the spindle suddenly start moving could be bad.
    Serial2.print(F("shoulder.zset_btn.bco2=RED\xFF\xFF\xFF"));
  } else {
    spin_count = 0;
    leadscrew = 0L;
    left_limit = 0L;
    right_limit = 0L;
    leftClr(!waitSerial2);
    rightClr(!waitSerial2);
  }
  if (wait) {
    while (!Serial2.available()); //Wait for the release event
    Serial2.print(F("shoulder.zset_btn.bco2=1024\xFF\xFF\xFF"));
  }
}

void jogCheck(bool feed) {
  //Decide whether it's safe to jog

  unsigned long timer = millis();
  long lead;

  while (!Serial2.available()) {
    //The button hasn't been released.
    //Don't jog unless the Nextion button is held for 500ms.
    //This needs to be extended to also jog from the toggle switch.
    if ((millis() - timer) > 500UL) {
      if (spin_rate != SPINDLE_STOPPED) {
        //Refuse if the spindle is turning.
        //I'm considering allowing creeping up on a shoulder and nudging the limit. It's on the list.
        noJog(feed);
      } else {
        //Save the leadscrew position
        lead = leadscrew;
        //Spin the leadscrew
        detachInterrupt(digitalPinToInterrupt(SPINDLE_A));    //No spindle interrupts
        nextionJog(feed);
        //Compensate the leadscrew value by the distance jogged
        jogAdjust(leadscrew - lead);
        attachInterrupt(digitalPinToInterrupt(SPINDLE_A), spindle, FALLING);
      }
    }
  }
  feed_left = feed;
}

void nextionRamp(void) {
  //Accelerate until the loop finishes at ICR4_MIN or the button is released
  //This needs a little more work to handle parameter changes better

  unsigned int accel = ICR4_ACCEL;
  unsigned int i, j;

  //Smoothly accelerate
  for ( j = ICR4_MAX ; j != ICR4_MIN ; /* decrement j index in inner loop */ ) {
    for ( i = 32 ; i > 0 ; i--, j -= accel ) {
      _icr4 = j;  //ICR4 updates from this in the interrupt handler for smoothness
      if ( Serial2.available() ) break ;  //Direction button release message received
      nextionLead();
      delay( 50 );  //Determines the time to ramp up
    }
    if ( Serial2.available() ) break ;
    accel = accel / 2;
  }
  while ( !Serial2.available() ) {
    //The jog button hasn't been released
    //Still jogging at max speed, but need to keep the position updated
    nextionLead();
    delay( 25 );
  }
}

void noJog(bool feed) {
  //Change the direction button background RED

  char str[30];

  //Turn left or right button RED
  //Then wait until the button is released
  if (feed) {
    //You're on a page with a left_btn, so no need for the page name
    sprintf(str, "%s%s%s", "left_btn.bco2=", "RED", NX_END);
    Serial2.print(str);
    while (!Serial2.available());
    sprintf(str, "%s%s%s", "left_btn.bco2=", nxDGREEN, NX_END);
    Serial2.print(str);
  } else {
    sprintf(str, "%s%s", "right_btn.bco2=RED", NX_END);
    Serial2.print(str);
    while (!Serial2.available());
    sprintf(str, "%s%s%s", "right_btn.bco2=", nxDGREEN, NX_END);
    Serial2.print(str);
  }
}

void nextionFeed(FEED_TABLE *table, int i) {
  //Display the rate and pitch values

  char nxstr[50];
  char rate[20];

  //Changing the rate always clears the fault
  fault = false;

  sprintf(nxstr, "%s%s%s", "rpm.bco=", nxDBLUE, NX_END);
  Serial2.print(nxstr);

  sprintf(rate, "%s%s", table[i].rate, QTNX_END);

  //Unfortunately, this has to be sent to all three pages
  sprintf(nxstr, "%s%s", "start.rate.txt=\"", rate);
  Serial2.print(nxstr);
  sprintf(nxstr, "%s%s", "shoulder.rate.txt=\"", rate);
  Serial2.print(nxstr);
  sprintf(nxstr, "%s%s", "setup.rate.txt=\"", rate);
  Serial2.print(nxstr);

  sprintf(rate, "%s%s", table[i].pitch, QTNX_END);

  sprintf(nxstr, "%s%s", "start.pitch.txt=\"", rate);
  Serial2.print(nxstr);
  sprintf(nxstr, "%s%s", "shoulder.pitch.txt=\"", rate);
  Serial2.print(nxstr);
  sprintf(nxstr, "%s%s", "setup.pitch.txt=\"", rate);
  Serial2.print(nxstr);
}

void nextionLeft(void) {
  //"Click" the left direction button

  Serial2.print(F("click left_btn,1\xFF\xFF\xFF"));
  Serial2.print(F("click left_btn,0\xFF\xFF\xFF"));
}

void nextionRight(void) {
  //Click the right direction button

  Serial2.print(F("click right_btn,1\xFF\xFF\xFF"));
  Serial2.print(F("click right_btn,0\xFF\xFF\xFF"));
}

void nextionDirection(void) {
  //Update the display left/right feed buttons when feed changes direction.
  //Selected feed direction stays "sticky".

  static bool last_spin;

  if (last_spin != spin_ccw) {
    if (FEEDING_LEFT)
      nextionLeft();
    else
      nextionRight();
  }
  last_spin = spin_ccw;
}

void nextionUseRPM(void) {
  //Blank the RPM values on the SETUP screen that are too high for the current feed rate
  //"Too high" is defined as driving the stepper faster than 1500rpm.

  int i;

  for ( i=0 ; i<12 ; i++ ) {
    if (steps_per_rev * (long)rpm_table[i] >= STEPPER_LIMIT) {
      rpmHide(i);
    } else {
      rpmShow(i);
    }
  }
}

void rpmHide(int i) {
  char str[30];

  sprintf(str, "%s%s%s%s", "setup.", nxrpmID[i], ".pco=33816", NX_END);
  Serial2.print(str);
}

void rpmShow(int i) {
  char str[30];

  sprintf(str, "%s%s%s%s", "setup.", nxrpmID[i], ".pco=WHITE", NX_END);
  Serial2.print(str);
}


/**************************************
***************************************
***                                 ***
***        Support Functions        ***
***                                 ***
***************************************
**************************************/

int pitchFind(const char *pitch) {
  //Search the feed table for the pitch string and return the index.
  //I got tired of doing this manually every time I changed the table.

  int i;

  for (i = 0 ; i < INCHES ; i++) {
    if (strcmp(inch[i].pitch, pitch) == 0) return i;
  }
}

int rateFind(const char *rate){
  //Search the feed table for the rate string and return the index.

  int i;

  for (i = 0 ; i < METRICS ; i++) {
    if (strcmp(metric[i].rate, rate) == 0) return i;
  }
}

void feedSelect(int fmode) {
  //Fill the step lookup table according to mode and feed rate, and update the display.

  //Default to 40tpi, 0.5mm, and lowest diametral and module.
  static int i[4] = {pitchFind("  40"), rateFind("  0.5 "), 0, 0};

  switch (fmode) {
    case inch_feed:
      i[fmode] = knobCount(i[fmode], INCHES);   //Update and remember the rate
      feedFill(inch[i[fmode]].steps);           //Update the lookup table
      //Serial.println(inch[i[fmode]].steps);
      nextionFeed(inch, i[fmode]);              //Update the display
      nextionUseRPM();
      break;
    case metric_feed:
      i[fmode] = knobCount(i[fmode], METRICS);
      feedFill(metric[i[fmode]].steps);
      //Serial.println(metric[i[fmode]].steps);
      nextionFeed(metric, i[fmode]);
      nextionUseRPM();
      break;
    case diametral_feed:
      i[fmode] = knobCount(i[fmode], DIAMETRALS);
      feedFill(diametral[i[fmode]].steps);
      //Serial.println(diametral[i[fmode]].steps);
      nextionFeed(diametral, i[fmode]);
      nextionUseRPM();
      break;
    case module_feed:
      i[fmode] = knobCount(i[fmode], MODULES);
      feedFill(module[i[fmode]].steps);
      //Serial.println(module[i[fmode]].steps);
      nextionFeed(module, i[fmode]);
      nextionUseRPM();
      break;
  }
}

char *leadStr(long lead, char *leadstr) {
  //Format and return the leadscrew position string

  char sign;
  int whole, fraction;

  if (lead < 0)
    sign = '-';
  else
    sign = ' ';
  if (feed_mode == inch_feed || feed_mode == diametral_feed) {
    //Format in inches
    whole = abs(lead) / LSPI;
    fraction = (abs(lead) * 10 / LSPM10) % 1000;          //Scale to 0.001"
    sprintf(leadstr, "%c%d.%03d", sign, whole, fraction );
  } else {
    //Format in millimeters with extra precision
    whole = abs(lead) * 10 / LSPMM10;  //Lead is a long, so the long calculation is cast to int
    fraction = (abs(lead) * 10 % LSPMM10) * 100 / LSPMM10;  //Scale to .01mm
    sprintf(leadstr, "%c%d.%02d", sign, whole, fraction );
  }
  return (leadstr);
}

void feedFill(int steps_per) {
  //Distribute the steps evenly around one spindle rotation

  int i;
  int spsc;           //Steps per spindle count
  int rem;            //Remainder
  int sum = 0;        //For accumulating remainders

  steps_per_rev = steps_per;

  spsc = steps_per / SCPR;
  rem = steps_per % SCPR;

  max_steps = spsc;

  if (rem)
    max_steps++;

  for (i = 0; i < SCPR; i++) {
    step_table[i] = spsc;
    sum += rem;
    //Sum the remainders and carry the one
    if (sum >= SCPR) {
      step_table[i]++;
      sum -= SCPR;
    }
  }
  pwmPeriodSet();
}

int spinAvg(unsigned int rate) {
  //Average 16 spindle rate readings for RPM calculation.

  static unsigned int spin[16];
  unsigned long sum = 0L;
  static int i = 0;
  int j;

  spin[i++] = rate;
  if (i == 16)
    i = 0;
  for (j = 0; j < 16; j++)
    sum += spin[j];
  return sum / 16;
}

void spinModulus(bool ccw) {
  //Keep track of the spindle position for synchronization
  //Counts between 0 and SCPR-1

  if (ccw) {
    if (spin_count < (SCPR - 1)) {
      spin_count++;
    } else {
      spin_count = 0;
    }
  } else {
    if (spin_count > 0) {
      spin_count--;
    } else {
      spin_count = SCPR - 1;
    }
  }
}

void jogAdjust(long jog_count) {
  //Adjust spin_count as if the leadscrew movement had been
  //generated by the spindle turning rather than jogging.

  int jog_adjust;

  //A few counts can get lost to truncation on each jog, but saving them would be complicated.
  //Because jog_count is long, the calculation is performed with long precision.
  //Jogging toward the head produces a negative jog count and jog_adjust value.

  jog_adjust = (jog_count * SCPR / steps_per_rev) % SCPR;

  if ((spin_count -= jog_adjust) < 0) {
    spin_count += SCPR;
  }
}

void nextionJog(bool feed) {
  //While the direction button is held, advance the leadscrew with smooth acceleration

  //Save the timer registers
  volatile int icr4 = ICR4;
  volatile int tcnt4 = TCNT4;

  jogging = true;

  pwmOn(feed);

  nextionRamp();

  pwmOff();
  jogging = false;

  TCNT4 = tcnt4;    //Restore the timer registers
  ICR4 = icr4;
}

void knobCheck(void) {
  //Check for encoder knob ticks

  if (knob_count) {
    feedSelect(feed_mode);
    nextionUseRPM();
  }
}

int knobCount(int i, int table_size) {
  //Knob count doesn't really matter, only direction and table limits

  if (knob_count > 0) {
    if (i < table_size - 1)
      ++i;
  } else if (knob_count < 0) {
    if (i > 0)
      --i;
  }
  knob_count = 0;
  return i;
}

void toggleCheck(void) {
  //Check the state of the feed direction switch (active low).

  //I intend to add the jog function to the toggle switch, but haven't gotten to it yet.

  unsigned long timer = millis();
  long lead;

  volatile int this_left = digitalRead(LEFT_MOM);
  volatile int this_right = digitalRead(RIGHT_MOM);
  static int last_left;
  static int last_right;

  if (FEEDING_LEFT && (this_right == LOW)) {   //No need to do anything unless direction changes
    if (last_right == LOW) {   //Very simple debounce
      if (spin_ccw) {
        feed_left = false;
      } else {
        feed_left = true;
      }
      nextionRight();
    }
  } else if (!FEEDING_LEFT && (this_left == LOW)) {
    if (last_left == LOW) {
      if (spin_ccw) {
        feed_left = true;
      } else {
        feed_left = false;
      }
      nextionLeft();
    }
  }

  last_left = this_left;
  last_right = this_right;
}


/********************************************************
*********************************************************
***                                                   ***
***              REGISTER MANIPULATION                ***
***                                                   ***
*********************************************************
********************************************************/

void pcint4Enab(void) {
  //Interrupt on PB4 (Pin 21) falling edge from the knob

  EICRB |= _BV(ISC41);
  EIMSK |= _BV(INT4);
}

void tc3Init(void) {
  // Configure TC3 for checking spindle speed

  TCCR3A  = _BV(COM3A0);    //Because the high byte doesn't increment in '00' mode
  TCCR3B  = _BV(CS30);      //Run TCNT3 at 16Mhz to get better RPM resolution at high speed
  tc3.low = TCNT3;          //Initialize to the current value of the counter
}

void tc3Enab(void) {
  //Enable TC3 overflow interrupt

  TIMSK3  = _BV(TOIE3);
}

void tc4Init(void) {
  //TC4 generates the step pulses to drive the leadscrew

  ICR4  = STP_MIN;                                    //Period defaults to minimum
  OCR4A = PUL_MIN;                                    //Pulse width is constant
  TCCR4B = _BV(WGM43) | _BV(WGM42);                   //Set Fast PWM Mode 14 with the clock off
  PORTH |= _BV(DIR_N) | _BV(PUL_N);                   //Set high or it glitches low when enabled
  DDRH  |= _BV(DDH4) | _BV(DDH3);                     //Set timer pins to output
  TCCR4A =  _BV(WGM41) | _BV(COM4A1) | _BV(COM4A0);   //Set on compare match
  TCNT4 = period_list[max_steps] - PUL_MIN;           //Preload the (stopped) counter for immediate pulse
}

void tc4Enab(void) {
  //Enable TC4 overflow interrupt

  TIMSK4 = _BV(OCIE4A);
}

void pwmOn(bool feed) {
  //Fast PWM Mode 14, inverting

  //Start by setting the direction bit
  if (feed) {
    PORTH |= _BV(DIR_N);
  } else {
    PORTH &= ~_BV(DIR_N);
  }
  //TOP in ICR4A, start the 2Mhz clock
  TCCR4B = _BV(WGM43) | _BV(WGM42) | _BV(CS41);
}

void pwmOff(void) {
  //Just turn off the clock

  TCCR4B = _BV(WGM43) | _BV(WGM42);
}

void pwmPeriodSet(void) {
  //Set the period value according to the number of steps per spindle tick.
  //Update ICR4 with interrupts off since it's not double-buffered.

  noInterrupts();
  ICR4 = period_list[max_steps];
  interrupts();
}


/********************************************************
*********************************************************
***                                                   ***
***              INTERRUPT HANDLING                   ***
***                                                   ***
*********************************************************
********************************************************/

void spindle(void) {
  //This interrupt is called on falling edges of SPINDLE_A

  bool feeding_left;
  static bool last_feed = feed_left;  //Just for the first time

  // First check if all steps have been sent from the last time.
  // If steps are left over, the spindle is going too fast for the feed rate.
  // A fault here will cause the displayed RPM to turn red, but nothing stops.
  // Things can get strange with the driver if the difference is too extreme.
  // Several times it has seemed to trip something so that both driver status LED's went out.
  // Turning it off for a couple of minutes has reset it so far, so thermal protection?

  if (steps != 0)
    fault = true;

  // SPINDLE_A brought us here, now determine the direction it's turning.
  // The lathe's reverse lever position isn't known.  I keep it in the latched-down position.
  // Adding a microswitch to the encoder bracket to sense it might be a future enhancement.

  // I do the simplest possible decoding of direction by simply looking at the state of phase B,
  // which was much simpler and faster than full quadrature decoding.
  // I started off with a 200 pulse-per-revolution rotary encoder and full decoding,
  // but switched to an 800p/r encoder and this scheme for the equivalent resolution.
  // Full decoding may have better noise immunity, but I haven't had any problems with this
  // since adding 2k external pull-up resistors to the Arduino inputs to speed up the rising edges.
  // The Arduino's internal pull-ups were just too big and slow.

  if (digitalRead(SPINDLE_B)) {   //Spindle is turning CCW if B is high
    spinModulus(spin_ccw = true);
  } else {
    spinModulus(spin_ccw = false);
  }

  if (feed_left != last_feed) {
    //Changed direction, so remember sync count
    synced = false;
    //Keep the sync value between 0 and SCPR-1
    if (spin_count) {   //Non-zero
      sync_count = SCPR - spin_count;
    } else {
      sync_count = 0;
    }
  }

  //Save a few clock cycles later
  feeding_left = (spin_ccw && feed_left) || (!spin_ccw && !feed_left);

  if (feeding_left) {
    if (synced) {
      //Is feeding left possible?
      if ( !left_limited || (leadscrew > left_limit) ) {
        //Turn on PWM if steps is non-zero.
        if (steps = step_table[spin_count]) {
          pwmOn(feeding_left);
        }
      } else {
        //The left limit was just reached
        synced = false;
        if (spin_count) {
          lsync_count = SCPR - spin_count;
        } else {
          lsync_count = 0;
        }
      }
    } else if (right_limited && (leadscrew >= right_limit)) {
      //Hit the right limit last time, so wait for sync before going left
      if (spin_count == rsync_count) {
        synced = true;
      }
    } else if (!left_limited || (leadscrew > left_limit)) {
      //It was an on-the-fly direction change
      if (spin_count == sync_count) {
        synced = true;
      }
    }
  } else {
    if (synced) {
      if ( !right_limited || (leadscrew < right_limit) ) {
        if (steps = step_table[spin_count]) {
          pwmOn(feeding_left);
        }
      } else {
        synced = false;
        if (spin_count) {
          rsync_count = SCPR - spin_count;
        } else {
          rsync_count = 0;
        }
      }
    } else if (left_limited && (leadscrew <= left_limit)) {
      if (spin_count == lsync_count) {
        synced = true;
      }
    } else if (!right_limited || (leadscrew < right_limit)) {
      if (spin_count == sync_count) {
        synced = true;
      }
    }
  }

  last_feed = feed_left;

  tc3.low = TCNT3;  //tc3.high is handled by the counter overflow interrupt
  spin_rate = tc3.count - last_tc3;
  last_tc3 = tc3.count;
  
  /*
   * Call diagnostic code ISR MJMN Jan 2021
   */
  AChannelISR ();
}

void knob(void) {
  //This interrupt is called by KNOB_A
  //It really just keeps track of the direction of the click.

  if (digitalRead(KNOB_B)) {
    knob_count-- ;
  } else {
    knob_count++ ;
  }
}

ISR(TIMER3_OVF_vect) {
  // TIMER3 provides the master clock for determining spindle speed.
  // This interrupt counts 16-bit timer overflows to extend the precision.
  // It also provides a convenient place to check whether the spindle is moving.

  tc3.count += 0x10000;

  // Detect that the spindle has stopped
  if (spin_rate == last_spin)
    spin_rate = SPINDLE_STOPPED;

  last_spin = spin_rate;
}

ISR(TIMER4_COMPA_vect) {
  // This stops the PWM if all the steps for a spindle tick have been output,
  // resets the counter for the next spindle tick according to feed rate,
  // and keeps track of the carriage movement.

  static int i;

  if (!jogging) {
    if (--steps == 0) {   //Get out fast if there's another step coming
      pwmOff();

      //The clock is stopped, so TCNT4 is no longer incrementing.
      //Now, preload TCNT4 so that the first step pulse will be
      //output immediately when the clock is turned on by pwmOn().
      //This provides more margin after the last step is output.
      TCNT4 = period_list[max_steps] - PUL_MIN;

      //When feeding, the granularity of the leadscrew value
      //is determined by counts per spindle tick.
      if (FEEDING_LEFT) {
        leadscrew -= step_table[spin_count];
      } else {
        leadscrew += step_table[spin_count];
      }
    }
  } else {
    //Synchronously update ICR4 here for smoother acceleration
    ICR4 = _icr4;
    //When jogging, the leadscrew is incremented/decremented by one
    if (PORTH & _BV(DIR_N)) {   //Cheat a little here and read the bit directly
      --leadscrew;
    } else {
      ++leadscrew;
    }
  }
}
