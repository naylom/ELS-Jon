void DiagnosticsSetup ()
{
    Serial.begin (BAUD_RATE);
    delay (1000);                   // Let comms settle before output
    // Print config state for this run
    Serial.print (F ("Encoder Quality Check Ver. "));
    Serial.println (VER);
    Serial.print (F ("Serial output running at "));
    Serial.println (BAUD_RATE);
    Serial.print (F ("Channel A expected on Digital Pin "));
    Serial.print (ACHANNEL_PIN);
    Serial.print (F (", connected to interrupt "));
    Serial.print (digitalPinToInterrupt (ACHANNEL_PIN));
    Serial.print (F (", mode is "));
    Serial.println (ModetoString (ACHANNEL_MODE));
    Serial.print (F ("Channel B expected on Digital Pin "));
    Serial.print (BCHANNEL_PIN);
    Serial.print (F (", connected to interrupt "));
    Serial.print (digitalPinToInterrupt (BCHANNEL_PIN));
    Serial.print (F (", mode is "));
    Serial.println (ModetoString (BCHANNEL_MODE));
    Serial.print (F ("Channel Z expected on Digital Pin "));
    Serial.print (ZCHANNEL_PIN);
    Serial.print (F (", connected to interrupt "));
    Serial.print (digitalPinToInterrupt (ZCHANNEL_PIN));
    Serial.print (F (", mode is "));
    Serial.println (ModetoString (ZCHANNEL_MODE));
    Serial.println ();

    // Sanity check config before implementing it
    if (ACHANNEL_PIN == BCHANNEL_PIN || ACHANNEL_PIN == ZCHANNEL_PIN || BCHANNEL_PIN == ZCHANNEL_PIN)
    {
        TerminateProgram (F ("Invalid Config - expected three input different pins for the A,B and Z channels"));
    }
    else
    {
        if (digitalPinToInterrupt (ACHANNEL_PIN) == -1 || digitalPinToInterrupt (BCHANNEL_PIN) == -1 /*|| digitalPinToInterrupt (ZCHANNEL_PIN) == -1*/)
        {
            TerminateProgram (F ("Invalid Config - not all pins selected for channels support interrupts"));
        }
        else
        {
            if (strcmp (ModetoString (ACHANNEL_MODE), sUnknownMode) == 0 || strcmp (ModetoString (BCHANNEL_MODE), sUnknownMode) == 0 || strcmp (ModetoString (ZCHANNEL_MODE), sUnknownMode) == 0)
            {
                TerminateProgram (F ("Invalid Config - channel pins should be set to one of: INPUT / INPUT_PULLUP / OUTPUT"));
            }
        }
    }
    // Don't change A and B channel pins as ELS code is managing these
    pinMode (ZCHANNEL_PIN, ZCHANNEL_MODE);          // internal pullup input pin

    // Setting up interrupt for Z channel

    //Enable pin change interrupts for digital pin 12 (ATmega 2560 pin PB6/PCINT6)
    PCICR |= (1 << PCIE0);                          // enable group interrupts on PCINT[7:0] to any already setup
    PCMSK0 |= (1 << PCINT6);                        // specifically enable interrupt digital pin 12 (PCINT6) to any set up
                                                    // this will cause ISR (PCINT0_vect) to execute on any pin state change

    // Wait for stable revs to occur
    do
    {
        delay (100);
    } 
    while (bRevRateDetermined == false);
    
    // Z has settled, set up channel B ISR (Channel A controlled by ELS code)
    
    // B falling signal from encoder will invoke BChannelISR().
    attachInterrupt ( digitalPinToInterrupt ( BCHANNEL_PIN ), BChannelISR, FALLING );

    // Now show calculated rev speed
    Serial.print ( F ( "Recent revs are taking an average of " ) );
    Serial.print (ulAvgZTime);
    Serial.print  ( F ( " microseconds, each A or B channel signal should occur every " ) );
    Serial.print ( ulABInterval );
    Serial.print ( F ( " microseconds. Will allow  " ) );
    Serial.print ( ulABMargin );
    Serial.println ( F ( " microseconds either side" ) );
    Serial.print ( F ( "This implies a rpm of " ) );
    Serial.println ( ( 1000000L * 60L ) / ulAvgZTime );

    Serial.println ();
    Serial.print ( F ( "Rev times used for avg : " ) );
    for ( int i = 0; i < LAST_N_REVS; i++ )
    {
        if ( i > 0 )
        {
            Serial.print ( F ( ", " ) );
        }
        Serial.print ( ulTimeofRevInterrupt [ i ] );
    }
    Serial.println ();
}

void DiagnosticsLoop ()
{
    enum eSteppermode { STOP1 = 0, CLOCKWISE, STOP2, COUNTERCLOCKWISE };

    static unsigned long ulTime = millis ();
    static eSteppermode eStepperMode = STOP1;


    // check for a rev outside of expected tolerance; if found tell user and quit
    if (bZUnstable == true)
    {
        Serial.print (F ("Last revolution took "));
        Serial.print (ulZUnstableRevTime);
        Serial.print (F (" microseconds, expected "));
        Serial.print (ulAvgZTime);
        Serial.print (F (" with a tolerance of "));
        Serial.println (ulZMargin);
        Serial.print (F ("During run had "));
        Serial.print (ulAChannelGoodInterruptCount);
        Serial.println (F (" A Channel interrupts within tolerance"));
        Serial.print (F ("During run had "));
        Serial.print (ulBChannelGoodInterruptCount);
        Serial.println (F (" B Channel interrupts within tolerance"));
        // Rev rate on has changed, expected timings on A and B are no longer reliable
        TerminateProgram (F ("Rev rate has changed, results now unreliable, halting program"));
    }
    else
    {
        // See if we have a new good result to show
        if (bZResultProcessed == false)
        {
            Serial.print (F ("Z deviation from average "));
            if (lZRevDeviation < 0)
            {
                Serial.print (F ("-"));
            }
            Serial.print (lZRevDeviation / 100);
            Serial.print (F ("."));
            Serial.print (abs (lZRevDeviation % 100));
            Serial.print (F ("%, A Channel Interrupts this rev "));
            Serial.print (ulAChannelInts);
            Serial.print (F (", B Channel Interrupts this rev "));
            Serial.println (ulBChannelInts);
            bZResultProcessed = true;
        }
    }
    // look for channnel A interrupt outside its time tolerance
    if (bAChannelErr == true)
    {
        // A channel has interrupted oustide expected time
        Serial.print (F ("A Channel has interrupted outside of expected timings. "));
        Serial.print (lAUnderOver);
        Serial.print (F (" microseconds away from expected time of "));
        Serial.print (ulABMargin);
        Serial.print (F (" at  "));
        Serial.print (ulAChannelCountErr % COUNT_PER_REV);
        Serial.println (F (" signals into rev"));
        bAChannelErr = false;
    }
    // look for channnel B interrupt outside its time tolerance
    if (bBChannelErr == true)
    {
        // A channel has interrupted oustide expected time
        Serial.print (F ("B Channel has interrupted outside of expected timings. "));
        Serial.print (lBUnderOver);
        Serial.print (F (" microseconds away from expected time of "));
        Serial.print (ulABMargin);
        Serial.print (F (" at "));
        Serial.print (ulBChannelCountErr % COUNT_PER_REV);
        Serial.println (F (" signals into rev"));
        bBChannelErr = false;
    }

}

const char* ModetoString (int iMode)
{
    const static char* pResult;
    switch (iMode)
    {
    case INPUT:
        pResult = sInput;
        break;

    case INPUT_PULLUP:
        pResult = sInputPullup;
        break;

    case OUTPUT:
        pResult = sOutput;
        break;

    default:
        pResult = sUnknownMode;
        break;
    }
    return pResult;
}

void TerminateProgram (const __FlashStringHelper* pErrMsg)
{
    Serial.println (pErrMsg);
    Serial.flush ();
    exit (0);
}
// This code is called from ELS SPINDLE_A ISR
void AChannelISR ()
{
    // check how long since last interrupt
    static unsigned long ulALastTime = 0UL;
    ulAChannelCount++;

    if (CheckElapsedTime (&ulALastTime, &lAUnderOver) == false)
    {
        ulAChannelCountErr = ulAChannelCount;
        bAChannelErr = true;
    }
    else
    {
        ulAChannelGoodInterruptCount++;
    }
}

// This is run directly as ELS code does not set up an interrupt for Channel B from encoder
void BChannelISR ()
{
    // check how long since last interrupt
    static unsigned long ulBLastTime = 0UL;
    ulBChannelCount++;

    if (CheckElapsedTime (&ulBLastTime, &lBUnderOver) == false)
    {
        ulBChannelCountErr = ulBChannelCount;
        bBChannelErr = true;
    }
    else
    {
        ulBChannelGoodInterruptCount++;
    }
}


// Pin Chnage interrupt routine
ISR (PCINT0_vect)
{
    // Check if falling signal
    if ( digitalRead (ZCHANNEL_PIN) == LOW )
    {
        ZChannelISR();
    }
}

void ZChannelISR ()
{
    enum    bZState { BEGIN_CALC_REV_RATE, MONITOR_REV_RATE };         // valid states for Z Channel logic
    static  bZState           ZRevState = BEGIN_CALC_REV_RATE;                        // current state of Z channel logic
    static  unsigned long     ulLastRevTime = 0UL;                                    // Time, in microseconds, that last Z channel interrupted
    static  unsigned long     ulLastAChannelInts = 0UL;
    static  unsigned long     ulLastBChannelInts = 0UL;

    switch (ZRevState)
    {
    case BEGIN_CALC_REV_RATE:
        ulLastRevTime = micros ();
        if (ZSettled (ulLastRevTime, &ulAvgZTime) == true)
        {
            // Z has settled down so set the expexted values of each rev and times of A and B channel interrupts
            ulZMargin = (ulAvgZTime * ACCEPTABLE_MARGIN_PERCENT) / 100UL;           // acceptable percent of rev time
            ulABInterval = ulAvgZTime / COUNT_PER_REV;
            ulABMargin = ulZMargin / COUNT_PER_REV;
            bRevRateDetermined = true;
            ZRevState = MONITOR_REV_RATE;
        }
        break;

    case MONITOR_REV_RATE:
        // keep an eye on rev rate, if it changes the checks will be wrong
        unsigned long ulZTime = micros ();
        if (bZUnstable == false)
        {
            // see how many A and B channel interrupts occured in this rev

            ulAChannelInts = ulAChannelCount - ulLastAChannelInts;
            ulLastAChannelInts = ulAChannelCount;
            ulBChannelInts = ulBChannelCount - ulLastBChannelInts;
            ulLastBChannelInts = ulBChannelCount;
            // check how long this last rev took
            ulZUnstableRevTime = ulZTime - ulLastRevTime;               // Time last rev took
            long lThisDeviation = ulZUnstableRevTime - ulAvgZTime;       // deviation (in micros) from expected time
            if (abs (lThisDeviation) > ulZMargin)
            {
                // revs speed has changed by greater than margin
                bZUnstable = true;
            }
            else
            {
                // within acceptable bounds
                // if last result has been processed update with this one
                if (bZResultProcessed == true)
                {
                    // calc deviation as +/- percentage from expected average as 100 times too big - reduce on display
                    lZRevDeviation = (lThisDeviation * 10000L) / (long)ulAvgZTime;
                    bZResultProcessed = false;
                }
            }
        }
        ulLastRevTime = ulZTime;
        break;
    }
}

/*
 * function to determine if Z has initially stabilised and we can set the expected rev times and by implication the interval between interrupts on the A and B channels
 * this is called for each Z interrupt at the start of the program
 *
 * This fills a circular buffer with the time of the last LAST_N_REVS interrupts and once all entries have a value it calculates the time between the oldest and newest. From this
 * the average interval time is calculated. This is deemed settled if all entries only deviate by +/- SETTLE_AVG_MARGIN % from the calculated average.
 *
 * Parameters
 * ulThisIntTime - time this interrupt occured in microseconds
 * pulAvgRevTime - pointer to var to hold the average time between interrupts once they have settled down
 *
 * Returns true if settled state is reached, pulAvgRevTime will be populated otherwise its value is undefined.
*/
bool ZSettled (unsigned long ulThisIntTime, volatile unsigned long* pulAvgRevTime)
{
    static int            iIndexTimeofRevInterrupt = 0;                       // point to next entry to be used
    static int            iCountInterrupts = 0;                        // count of how many entries we have used so far
    bool                  bSettled = false;                    // true when settled condition met
    // Add latest time to circular array
    ulTimeofRevInterrupt[iIndexTimeofRevInterrupt] = ulThisIntTime;
    iCountInterrupts++;

    // do we have minimum # of interrupt times to start analysis? 
    if (iCountInterrupts >= LAST_N_REVS)
    {
        // calculate time between oldest and newest interrupt. ( Oldest is next entry in circular array )

        unsigned long ulElapsedTime = ulThisIntTime - ulTimeofRevInterrupt[(iIndexTimeofRevInterrupt + 1) % LAST_N_REVS];

        *pulAvgRevTime = ulElapsedTime / (LAST_N_REVS - 1);

        // look at prior entries and check that the elapsed times are within 5% of calcuated average
        int iOldest = (iIndexTimeofRevInterrupt + 1) % LAST_N_REVS;
        unsigned long ulMaxDiff = (*pulAvgRevTime * SETTLE_AVG_MARGIN) / 100UL;
        bSettled = true;                                  // assume good
        for (int i = 0; i < LAST_N_REVS - 1; i++)
        {
            int iNext = (iOldest + 1) % LAST_N_REVS;
            unsigned long ulInterval = ulTimeofRevInterrupt[iNext] - ulTimeofRevInterrupt[iOldest];
            long  ulDiff = ulInterval - *pulAvgRevTime;
            if (abs (ulDiff) > ulMaxDiff)
            {
                // not settled
                bSettled = false;
                break;
            }
            iOldest = iNext;
        }
    }
    // move to next entry in circular buffer
    ++iIndexTimeofRevInterrupt %= LAST_N_REVS;
    return bSettled;
}

/*
* This function looks at the time between signals on the A or B Channel and returns false if it is outside the accepted margin

Parameters
pulLastTime - pointer to the time of the last signal on this channel in microseconds
plOverUnder - pointer to storeage to hold the number of microseconds that at above/below the expected interval
*/
bool CheckElapsedTime (unsigned long* pulLastTime, volatile long* plOverUnder)
{
    bool bResult = true;              // All OK
    unsigned long ulNow = micros ();

    // see if not first time this has run
    if (*pulLastTime > 0UL)
    {
        // ignore this time if micros() has reset
        if (ulNow > *pulLastTime)
        {
            // if we have a target then compare it
            if (ulABMargin > 0UL)
            {
                // see how much time has elapsed since last interrupt
                unsigned long ulElapsed = ulNow - *pulLastTime;
                *plOverUnder = ulElapsed - ulABInterval;
                if ((unsigned long)abs (*plOverUnder) > ulABMargin)
                {
                    // error took too long or too soon
                    bResult = false;
                }
            }
        }
    }
    *pulLastTime = ulNow;
    return bResult;
}
