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


#ifndef __CONFIGURATION_H
#define __CONFIGURATION_H



//Added RPM blanking 04/22/2020:jrb

//================================================================================
// Output Pins
//================================================================================

#define PUL_N         PORTH3  //Stepper PUL- is on Pin 6, active low, with PUL+ tied to +5V
#define DIR_N         PORTH4  //Stepper DIR- is on Pin 7, active low, with DIR+ tied to +5V



//================================================================================
// Nextion miscellaneous
//================================================================================

#define nxPBLUE "44415"   //Pale blue
#define nxDBLUE "33816"   //Dark blue
#define nxDGREEN "1024"   //Dark green

//Nextion display closing quote and three-character terminator
#define QTNX_END "\"\xFF\xFF\xFF"
//Without the closing quote
#define NX_END "\xFF\xFF\xFF"

//Component ID's for the Nextion "buttons"
enum nextionID {
  inch_btn = 1,
  metric_btn,
  diametral_btn,
  module_btn,
  left_btn,
  right_btn,
  back_btn,
  lset_btn,
  zset_btn,
  rset_btn,
  lclr_btn = 27,    //Too much trouble to edit the display to keep them all in sequence
  rclr_btn
};

//ID's for the RPM text boxes
//These are placed in the display with the ID's in order of descending RPM
char nxrpmID[12][4] {
  "t3", "t4",  "t5",  "t6",  "t7",  "t8",
  "t9", "t10", "t11", "t12", "t13", "t14"
};


//================================================================================
//Input Pins
//================================================================================

// Wired to PE4/INT4 and PE5/INT5 on Mega2560 but Arduino remaps it for compatibility
#define SPINDLE_B   2   //Encoder Phase B pin (PORTE4) pulled up with 2k
#define SPINDLE_A   3   //Encoder Phase A pin (PORTE5) pulled up with 2k

#define LEFT_MOM    18  //Direction toggle switch
#define RIGHT_MOM   19  //Direction toggle switch
#define KNOB_B      20  //Knob Phase B pin (PORTD1) through 1k/0.1uf RC filter
#define KNOB_A      21  //Knob Phase A pin (PORTD0) through 1k/0.1uf RC filter
#define ALARM       13  //Microstepper controller ALARM open collector (not yet implemented)

//================================================================================
// Scaler magic numbers
//================================================================================

#define SCPR        800     //Spindle encoder Counts Per Revolution
#define MICROSTEPS  400     //Driver microsteps per revolution
#define STEP_RATIO  8       //Stepper:Leadscrew ratio
#define LTPI        8       //Leadscrew Threads Per Inch
#define LSPI        25600   //Leadscrew Steps Per Inch (LTPI*MICROSTEPS*STEP_RATIO)
#define LSPM10      256     //Leadscrew Steps Per Mil (0.001") * 10
#define LSPMM10     10079   //Leadscrew Steps Per MilliMeter (LSPI / 25.4 * 10)



//================================================================================
// Timing
//================================================================================

#define STP_MIN   60      //30us period minimum to accommodate jitter (2Mhz clock)
#define PUL_MIN   6       //3us pulse minimum for stepper drive
#define RPM20     60000   //16Mhz clock ticks at 20rpm

// Timer 3 Counts Per Minute for calculating spindle RPM
#define T3CPM     (16000000L * 60L)     //TC3 counts per minute, 16Mhz * 60 seconds



//================================================================================
// Timer Parameters for Jogging
//================================================================================

#define ICR4_MAX    0x8000    //Slow enough to adjust by thousandths
#define ICR4_MIN    0x100     //Any faster can cause the microstepper to error out
#define ICR4_ACCEL  0x200     //Initial increment for acceleration



//================================================================================
// "Slowest possible" value to indicate that the spindle is stopped
//================================================================================

#define SPINDLE_STOPPED   0xFFFF    //Set when the spindle isn't moving



//================================================================================
// Steps per minute to spin the stepper motor at a "conservative" 1500rpm.
// That doesn't mean that you can't get away with running a little faster.
// I settled on 1500rpm because it provides some margin, and people will 
// always push the speed limit, don't you know. The torque curve on the
// 3.5Nm Nema 24 motor that I'm using is plotted up to 2000rpm.
// It's really starting to whistle at that speed, though.
//================================================================================

#define STEPPER_LIMIT     600000L



//================================================================================
// Flags
//================================================================================

bool feed_left = true;        //Feed toward the headstock
bool spin_ccw = true;         //Spindle direction
bool fault = false;           //Step overrun flag
bool jogging = false;         //Flag to control timer interrupt behavior
bool right_limited = false;   //Flag to control feed limits
bool left_limited = false;    //Flag to control feed limits
bool synced = true;           //Flag for synchronizing the leadscrew with the spindle
bool waitSerial2 = true;      //For zeroSet()

int knob_count;               //Knob counts (really just direction)

int spin_count = 0;           //Value from 0-799 (counts per rev)

int sync_count = 0;           //On-the-fly spindle count for synchronizing
int lsync_count = 0;          //Spindle count upon reaching the left limit
int rsync_count = 0;          //Likewise for the right

int steps_per_rev;            //Steps per spindle revolution for the current pitch

long leadscrew;               //Leadscrew counts
long left_limit = 0L;         //Leadscrew value for left limit when enabled
long right_limit = 0L;        //Leadscrew value for right limit

unsigned int spin_rate = SPINDLE_STOPPED;
unsigned int last_spin;



//================================================================================
// Feed direction logic
//================================================================================

#define FEEDING_LEFT  ((spin_ccw && feed_left) || (!spin_ccw && !feed_left))



//================================================================================
// Feed modes
//================================================================================

enum {
  inch_feed,
  metric_feed,
  diametral_feed,
  module_feed
} feed_mode = inch_feed ;



//================================================================================
// Timers
//================================================================================

union {
  // Used to extend the resolution of TC3
  unsigned long count;
  unsigned int low, high;
} tc3 ;

unsigned long last_tc3 = 0;   //This is used to calculate spindle RPM.

unsigned int last_step = 0;   //For determining pulse rate

unsigned int _icr4;           //Buffered value for ICR4 used during jogging

byte steps;                   //Steps per spindle tick
byte step_table[SCPR];        //Lookup table for precalculated feed rate
int max_steps;                //Maximum steps per spindle tick, used in several ways

//The measured spindle speeds on my lathe with a 1720rpm motor
unsigned int rpm_table[12] { 1430, 812, 648, 463, 368, 238, 210, 135, 108, 77, 61, 34 };

//        "Official" values: 1450, 780, 620, 420, 334, 244, 179, 131, 104, 70, 56, 30

// The highest-speed entry in the Nextion display has an ID of "t3",
// and the lowest is "t14", and they are in order from highest to lowest RPM.
// To blank the highest entry send "setup.t3.pco=33816" to turn the text blue,
// then to unblank it send "setup.t3.pco=WHITE" to turn it white again.

// This chart is reorganized from the Model 200 manual to reflect the physical belt position.
// Eventually I want this chart to display/highlight RPM's that are valid for the feed rate.
//
// --------------------------------------------------------------------|
// MOTOR    |                   SPINDLE BELT POSITION                  |
// BELT     |----------------------------------------------------------|
// POSITION |     DIRECT BELT DRIVE       |       BACK GEAR DRIVE      |
// ---------|-----------------------------|----------------------------|
//   HIGH   |  1450   |   780   |   420   |   244   |   131   |   70   |
// ---------|---------|---------|---------|---------|---------|--------|
//   LOW    |   620   |   334   |   179   |   104   |    56   |   30   |
// --------------------------------------------------------------------|


//For completeness I tried to include every tap pitch available from McMaster-Carr,
//and every pitch that I could identify in pictures of old lathes.
//I even included metric "British Association" pitches once I knew about them.
//The maximum feed rate in each mode is limited to about 8000 steps per rev.
//There are practical limits on spindle speed for given feed rates.
//It worked well to scale the RPM speed steps directly as the steps per spindle tick increases.
//A rate of 0.025" per rev moves the carriage 0.6" per second at 1450rpm, a fast clip.
//That's the 1:1 rate, when a step is output for every spindle tick.
//A 0.050" feed at 780rpm is moving 0.65" per second, 0.075" at 620rpm is 0.775" etc.
//At the max inch rate of 0.25" per rev at 70rpm, that's fallen off to about 0.29" per rev.
//That seems sensible, since the forces go up with the deeper corresponding cut.
//We'll see how it works out in practice.

//I experimented with dynamically varying the step period, but it just got too fussy.
//Maximum steps per spindle tick is 11, a carriage movement of 0.00043" (fine enough for me).
// Period is roughly scaled to feed rate, for smoothing and to mitigate resonances.
// 2Mhz / ((RPM*SCPR)/60) / max_steps
//int period_list[12] { STP_MIN, STP_MIN, 96, 81, 90, 90, 69, 120, 143, 160, 214, 244 };
// 80% of 2Mhz / ((RPM*SCPR)/60) / max_steps

//So after all that I wound up looking at it on the oscilloscope and picking arbitrary values.
//Maybe I'll revisit this at a later date:
int period_list[12] {
  STP_MIN, STP_MIN, STP_MIN, STP_MIN, STP_MIN, STP_MIN,
  STP_MIN, STP_MIN,      83,     104,     155,     194 };


#endif // __CONFIGURATION_H
