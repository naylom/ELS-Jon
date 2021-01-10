#ifndef __ENCODERDIAGNOSTICS_H
#define __ENCODERDIAGNOSTICS_H
const static char  sInput[] =			"Input mode";
const static char  sInputPullup[] =		"Input with pullup mode ";
const static char  sOutput[] =			"Ouput Mode";
const static char  sUnknownMode[] =		"Unknown mode";

// Configuration items
#define COUNT_PER_REV             800UL                         // Number of encoder signals per revolution per A or B channel
#define ACHANNEL_PIN              SPINDLE_A
#define ACHANNEL_MODE             INPUT
#define BCHANNEL_PIN              SPINDLE_B
#define BCHANNEL_MODE             INPUT
#define ZCHANNEL_PIN              9								// will use Pin Change Interrupt to monitor this pin
#define ZCHANNEL_MODE             INPUT_PULLUP
#define BAUD_RATE                 115200
#define LAST_N_REVS               10                            // How many rev times used at start to get an average
#define ACCEPTABLE_MARGIN_PERCENT 10                            // sets the variation allowed in interrupt timings
#define SETTLE_AVG_MARGIN         5UL                           // percentage of average all settle readings must be within
#define STEPPER_CYCLE_TIME        5                             // seconds for each stepper cycle time
#define VER                       1

enum     LOG_LEVEL { MINIMAL, MEDIUM, ALL };                    // Valid values for a LOG_LEVEL variable
enum  LOG_LEVEL eLogLevel = MEDIUM;                             // Configured logging Level wanted

volatile bool           bRevRateDetermined = false;             // true when we have determined the inital average time of a rev
volatile unsigned long  ulAvgZTime = 0UL;                       // Estimated time for each Z signal based on the average of previous signals that are all within a set % of their mean.
volatile unsigned long  ulZMargin = 0UL;                        // max micros difference between latest Z signal time and expected average time before Z flagged as unstable
volatile          long  lAUnderOver = 0UL;                      // number of microseconds over/under expected for time between interrupts
volatile bool           bAChannelErr = false;                   // true if err seen on channel A timings
volatile unsigned long  ulAChannelCount = 0UL;                  // Increments each time the A Channel interrupts
volatile unsigned long  ulBChannelCount = 0UL;                  // Increments each time the B Channel interrupts
volatile unsigned long  ulAChannelCountErr = 0UL;               // Snapshot of count of A Channel interrupts when a A channel signal is seen outside of expected timings
volatile unsigned long  ulBChannelCountErr = 0UL;               // Snapshot of count of B Channel interrupts when a B channel signal is seen outside of expected timings
volatile unsigned long  ulAChannelInts = 0UL;                   // Holds the number of A channel signals seen during last Z channel rev (i.e. between successive Z Channel interrupts)
volatile unsigned long  ulBChannelInts = 0UL;                   // Holds the number of B channel signals seen during last Z channel rev (i.e. between successive Z Channel interrupts)
volatile          long  lBUnderOver = 0UL;                      // number of microseconds over/under expected for time between interrupts
volatile bool           bBChannelErr = false;                   // true if err seen on channel A timings
volatile bool           bZUnstable = false;                     // true if Z Channel signals at point outside of (expected time +/- error margin)
volatile unsigned long  ulZUnstableRevTime = 0UL;               // Time, in microseconds, between prior Z Channel signal and Z Signal deemed in error
volatile bool           bZResultProcessed = true;               // Boolean flag used to synchronize ISR and loop processing. If true ISR can update with any new data, else wait for loop to process and clear
volatile long           lZRevDeviation = 0L;                    // % that Z channel interrupt time is away from expected time. Stored at 100 times too large. Needs to be scaled during output. Saves using float
volatile unsigned long  ulAChannelGoodInterruptCount = 0UL;     // count of A channel interupts within expected timings
volatile unsigned long  ulBChannelGoodInterruptCount = 0UL;     // count of B channel interupts within expected timings
volatile unsigned long  ulTimeofRevInterrupt[LAST_N_REVS];      // this used a circular array of times the Z channel interrupted         

unsigned long  ulABInterval = 0UL;                              // How often A&B channel should interrupt
unsigned long  ulABMargin = 0UL;                                // how much deviation from iABInterval is acceptable
#endif