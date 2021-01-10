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


#ifndef __TABLES_H
#define __TABLES_H



// Change Log:
//  Added British Association threads to the Metric table 01/28/2020:JRB
//  Added British "Cycle Engineering Institute" 62tpi 04/04/2020:JRB

//================================================================================
// Lookup tables
//================================================================================

struct FEED_TABLE {
  unsigned int steps;     //Encoder steps per spindle revolution for a given pitch
  char rate[7];           //Feed rate in inches or millimeters
  char pitch[5];          //Threads per inch or special designations like "10BA"
};

// INCH mode steps per revolution = leadscrew steps per inch / pitch
#define INCH_STEPS(tpi) ((unsigned int) round((float)LSPI/(tpi)))

// METRIC mode steps per revolution = rate_mm * leadscrew steps per inch / 25.4
#define MM_STEPS(mm) ((unsigned int) round((float)(mm)*LSPI/25.4))

// DIAMETRAL mode steps per revolution = pi * leadscrew steps per inch / diametral pitch
#define DIAM_STEPS(dpi) ((unsigned int) round(PI*(float)LSPI/(dpi)))

// MODULE mode steps per revolution = pi * rate_mm * LSPI / 25.4
#define MOD_STEPS(mm) ((unsigned int) round(PI*(float)(mm)*LSPI/25.4))

//***********************************************************************************
// Following are the lookup tables for the number of steps per spindle tick.        *
// Steps per spindle tick for a given rate/pitch are precalculated for efficiency.  *
//***********************************************************************************

const int INCHES = 76;    //Number of entries in the inch feed table

FEED_TABLE inch[INCHES] = {
  {INCH_STEPS(2000),  "0.0005", "----"}, /* 0  */
  {INCH_STEPS(1000),  "0.001 ", "----"}, /* 1  */
  {INCH_STEPS(667),   "0.0015", "----"}, /* 2  */
  {INCH_STEPS(500),   "0.002 ", "----"}, /* 3  */
  {INCH_STEPS(400),   "0.0025", "----"}, /* 4  */
  {INCH_STEPS(333),   "0.003 ", "----"}, /* 5  */
  {INCH_STEPS(286),   "0.0035", "----"}, /* 6  */
  {INCH_STEPS(250),   "0.004 ", "----"}, /* 7  */
  {INCH_STEPS(240),   "0.0042", " 240"}, /* 8  */
  {INCH_STEPS(224),   "0.0045", " 224"}, /* 9  */
  {INCH_STEPS(216),   "0.0046", " 216"}, /* 10 */
  {INCH_STEPS(208),   "0.0048", " 208"}, /* 11 */
  {INCH_STEPS(192),   "0.0052", " 192"}, /* 12 */
  {INCH_STEPS(184),   "0.0054", " 184"}, /* 13 */
  {INCH_STEPS(176),   "0.0057", " 176"}, /* 14 */
  {INCH_STEPS(160),   "0.0062", " 160"}, /* 15 */
  {INCH_STEPS(144),   "0.0069", " 144"}, /* 16 */
  {INCH_STEPS(128),   "0.0078", " 128"}, /* 17 */
  {INCH_STEPS(120),   "0.0083", " 120"}, /* 18 */
  {INCH_STEPS(112),   "0.0089", " 112"}, /* 19 */
  {INCH_STEPS(108),   "0.0093", " 108"}, /* 20 */
  {INCH_STEPS(104),   "0.0096", " 104"}, /* 21 */
  {INCH_STEPS(100),   "0.010 ", " 100"}, /* 22 */
  {INCH_STEPS(96),    "0.0104", "  96"}, /* 23 */
  {INCH_STEPS(92),    "0.0109", "  92"}, /* 24 */
  {INCH_STEPS(90),    "0.0111", "  90"}, /* 25 */
  {INCH_STEPS(88),    "0.0114", "  88"}, /* 26 */
  {INCH_STEPS(80),    "0.0125", "  80"}, /* 27 */
  {INCH_STEPS(72),    "0.0139", "  72"}, /* 28 */
  {INCH_STEPS(70),    "0.0143", "  70"}, /* 29 */
  {INCH_STEPS(64),    "0.0156", "  64"}, /* 30 */
  {INCH_STEPS(62),    "0.0161", "  62"}, /* 31 */     //CEI thread (old spoke nipples mostly, I guess)
  {INCH_STEPS(60),    "0.0167", "  60"}, /* 32 */
  {INCH_STEPS(56),    "0.0179", "  56"}, /* 33 */
  {INCH_STEPS(54),    "0.0185", "  54"}, /* 34 */
  {INCH_STEPS(52),    "0.0192", "  52"}, /* 35 */
  {INCH_STEPS(50),    "0.020 ", "  50"}, /* 36 */
  {INCH_STEPS(48),    "0.0208", "  48"}, /* 37 */
  {INCH_STEPS(46),    "0.0217", "  46"}, /* 38 */
  {INCH_STEPS(44),    "0.0227", "  44"}, /* 39 */
  {INCH_STEPS(40),    "0.025 ", "  40"}, /* 40 */
  {INCH_STEPS(36),    "0.0278", "  36"}, /* 41 */
  {INCH_STEPS(32),    "0.0312", "  32"}, /* 42 */
  {INCH_STEPS(30),    "0.0333", "  30"}, /* 43 */
  {INCH_STEPS(28),    "0.0357", "  28"}, /* 44 */
  {INCH_STEPS(27),    "0.037 ", "  27"}, /* 45 */
  {INCH_STEPS(26),    "0.0385", "  26"}, /* 46 */
  {INCH_STEPS(25),    "0.040 ", "  25"}, /* 47 */
  {INCH_STEPS(24),    "0.0417", "  24"}, /* 48 */
  {INCH_STEPS(23),    "0.0434", "  23"}, /* 49 */
  {INCH_STEPS(22),    "0.0454", "  22"}, /* 50 */
  {INCH_STEPS(20),    "0.050 ", "  20"}, /* 51 */
  {INCH_STEPS(19),    "0.0526", "  19"}, /* 52 */      // British Standard Pipe or "G" thread
  {INCH_STEPS(18),    "0.0555", "  18"}, /* 53 */
  {INCH_STEPS(17.5),  "0.0571", " 17\xBD"}, /* 54 */   // xBD is the character for "1/2"
  {INCH_STEPS(16),    "0.0625", "  16"}, /* 55 */
  {INCH_STEPS(15),    "0.0667", "  15"}, /* 56 */
  {INCH_STEPS(14),    "0.0714", "  14"}, /* 57 */
  {INCH_STEPS(13.5),  "0.0741", " 13\xBD"}, /* 58 */
  {INCH_STEPS(13),    "0.0769", "  13"   }, /* 59 */
  {INCH_STEPS(12),    "0.0833", "  12"   }, /* 60 */
  {INCH_STEPS(11.5),  "0.087 ", " 11\xBD"}, /* 61 */
  {INCH_STEPS(11),    "0.0909", "  11"   }, /* 62 */
  {INCH_STEPS(10),    "0.100 ", "  10"   }, /* 63 */
  {INCH_STEPS(9),     "0.1111", "   9"   }, /* 64 */
  {INCH_STEPS(8),     "0.125 ", "   8"   }, /* 65 */
  {INCH_STEPS(7.5),   "0.1333", "  7\xBD"}, /* 66 */
  {INCH_STEPS(7),     "0.1429", "   7"   }, /* 67 */
  {INCH_STEPS(6.75),  "0.1481", "  6\xBE"}, /* 68 */   // xBE is the character for "3/4"
  {INCH_STEPS(6.5),   "0.1538", "  6\xBD"}, /* 69 */
  {INCH_STEPS(6),     "0.1667", "   6"   }, /* 70 */
  {INCH_STEPS(5.75),  "0.1739", "  5\xBE"}, /* 71 */
  {INCH_STEPS(5.5),   "0.1818", "  5\xBD"}, /* 72 */
  {INCH_STEPS(5),     "0.200 ", "   5"   }, /* 73 */
  {INCH_STEPS(4.5),   "0.2222", "  4\xBD"}, /* 74 */
  {INCH_STEPS(4),     "0.250 ", "   4"   }  /* 75 */
};

const int METRICS = 56 ;

FEED_TABLE metric[METRICS] = {
  {MM_STEPS(0.01),  "  0.01", "----"}, /* 0  */
  {MM_STEPS(0.02),  "  0.02", "----"}, /* 1  */
  {MM_STEPS(0.03),  "  0.03", "----"}, /* 2  */
  {MM_STEPS(0.04),  "  0.04", "----"}, /* 3  */
  {MM_STEPS(0.05),  "  0.05", "----"}, /* 4  */
  {MM_STEPS(0.06),  "  0.06", "----"}, /* 5  */
  {MM_STEPS(0.07),  "  0.07", "----"}, /* 6  */
  {MM_STEPS(0.08),  "  0.08", "----"}, /* 7  */
  {MM_STEPS(0.09),  "  0.09", "----"}, /* 8  */
  {MM_STEPS(0.10),  "  0.1 ", "----"}, /* 9  */
  {MM_STEPS(0.12),  "  0.12", "----"}, /* 10 */
  {MM_STEPS(0.15),  "  0.15", "----"}, /* 11 */
  {MM_STEPS(0.20),  "  0.2 ", "----"}, /* 12 */
  {MM_STEPS(0.225), " 0.225", "----"}, /* 13 */
  {MM_STEPS(0.25),  "  0.25", "----"}, /* 14 */
  {MM_STEPS(0.30),  "  0.3 ", "----"}, /* 15 */
  {MM_STEPS(0.35),  "  0.35", "10BA"}, /* 16 */   // British Association Thread
  {MM_STEPS(0.39),  "  0.39", " 9BA"}, /* 17 */
  {MM_STEPS(0.40),  "  0.4 ", "----"}, /* 18 */
  {MM_STEPS(0.43),  "  0.43", " 8BA"}, /* 19 */
  {MM_STEPS(0.45),  "  0.45", "----"}, /* 20 */
  {MM_STEPS(0.48),  "  0.48", " 7BA"}, /* 21 */
  {MM_STEPS(0.50),  "  0.5 ", "----"}, /* 22 */
  {MM_STEPS(0.53),  "  0.53", " 6BA"}, /* 23 */
  {MM_STEPS(0.55),  "  0.55", "----"}, /* 24 */
  {MM_STEPS(0.59),  "  0.59", " 5BA"}, /* 25 */
  {MM_STEPS(0.60),  "  0.6 ", "----"}, /* 26 */
  {MM_STEPS(0.65),  "  0.65", "----"}, /* 27 */
  {MM_STEPS(0.66),  "  0.66", " 4BA"}, /* 28 */
  {MM_STEPS(0.70),  "  0.7 ", "----"}, /* 29 */
  {MM_STEPS(0.73),  "  0.73", " 3BA"}, /* 30 */
  {MM_STEPS(0.75),  "  0.75", "----"}, /* 31 */
  {MM_STEPS(0.80),  "  0.8 ", "----"}, /* 32 */
  {MM_STEPS(0.81),  "  0.81", " 2BA"}, /* 33 */
  {MM_STEPS(0.90),  "  0.9 ", " 1BA"}, /* 34 */
  {MM_STEPS(1.00),  "  1.0 ", " 0BA"}, /* 35 */
  {MM_STEPS(1.10),  "  1.1 ", "----"}, /* 36 */
  {MM_STEPS(1.20),  "  1.2 ", "----"}, /* 37 */
  {MM_STEPS(1.25),  "  1.25", "----"}, /* 38 */
  {MM_STEPS(1.30),  "  1.3 ", "----"}, /* 39 */
  {MM_STEPS(1.40),  "  1.4 ", "----"}, /* 40 */
  {MM_STEPS(1.50),  "  1.5 ", "----"}, /* 41 */
  {MM_STEPS(1.75),  "  1.75", "----"}, /* 42 */
  {MM_STEPS(2.00),  "  2.0 ", "----"}, /* 43 */
  {MM_STEPS(2.25),  "  2.25", "----"}, /* 44 */
  {MM_STEPS(2.50),  "  2.5 ", "----"}, /* 45 */
  {MM_STEPS(2.75),  "  2.75", "----"}, /* 46 */
  {MM_STEPS(3.00),  "  3.0 ", "----"}, /* 47 */
  {MM_STEPS(3.25),  "  3.25", "----"}, /* 48 */
  {MM_STEPS(3.50),  "  3.5 ", "----"}, /* 49 */
  {MM_STEPS(4.00),  "  4.0 ", "----"}, /* 50 */
  {MM_STEPS(4.50),  "  4.5 ", "----"}, /* 51 */
  {MM_STEPS(5.00),  "  5.0 ", "----"}, /* 52 */
  {MM_STEPS(5.50),  "  5.5 ", "----"}, /* 53 */
  {MM_STEPS(6.00),  "  6.0 ", "----"}, /* 54 */
  {MM_STEPS(6.50),  "  6.5 ", "----"}  /* 55 */
};

const int DIAMETRALS = 38;

FEED_TABLE diametral[DIAMETRALS] = {
  {DIAM_STEPS(120),  "0.0262", " 120"}, /* 0  */
  {DIAM_STEPS(112),  "0.0280", " 112"}, /* 1  */
  {DIAM_STEPS(108),  "0.0291", " 108"}, /* 2  */
  {DIAM_STEPS(104),  "0.0302", " 104"}, /* 3  */
  {DIAM_STEPS( 96),  "0.0327", "  96"}, /* 4  */
  {DIAM_STEPS( 92),  "0.0341", "  92"}, /* 5  */
  {DIAM_STEPS( 88),  "0.0357", "  88"}, /* 6  */
  {DIAM_STEPS( 80),  "0.0393", "  80"}, /* 7  */
  {DIAM_STEPS( 76),  "0.0413", "  76"}, /* 8  */
  {DIAM_STEPS( 72),  "0.0436", "  72"}, /* 9  */
  {DIAM_STEPS( 64),  "0.0491", "  64"}, /* 10 */
  {DIAM_STEPS( 60),  "0.0524", "  60"}, /* 11 */
  {DIAM_STEPS( 56),  "0.0561", "  56"}, /* 12 */
  {DIAM_STEPS( 54),  "0.0582", "  54"}, /* 13 */
  {DIAM_STEPS( 52),  "0.0604", "  52"}, /* 14 */
  {DIAM_STEPS( 48),  "0.0654", "  48"}, /* 15 */
  {DIAM_STEPS( 46),  "0.0683", "  46"}, /* 16 */
  {DIAM_STEPS( 44),  "0.0714", "  44"}, /* 17 */
  {DIAM_STEPS( 40),  "0.0785", "  40"}, /* 18 */
  {DIAM_STEPS( 38),  "0.0827", "  38"}, /* 19 */
  {DIAM_STEPS( 36),  "0.0873", "  36"}, /* 20 */
  {DIAM_STEPS( 32),  "0.0912", "  32"}, /* 21 */
  {DIAM_STEPS( 30),  "0.1041", "  30"}, /* 22 */
  {DIAM_STEPS( 28),  "0.1122", "  28"}, /* 23 */
  {DIAM_STEPS( 27),  "0.1164", "  27"}, /* 24 */
  {DIAM_STEPS( 26),  "0.1208", "  26"}, /* 25 */
  {DIAM_STEPS( 24),  "0.1309", "  24"}, /* 26 */
  {DIAM_STEPS( 23),  "0.1366", "  23"}, /* 27 */
  {DIAM_STEPS( 22),  "0.1428", "  22"}, /* 28 */
  {DIAM_STEPS( 20),  "0.1571", "  20"}, /* 29 */
  {DIAM_STEPS( 19),  "0.1653", "  19"}, /* 30 */
  {DIAM_STEPS( 18),  "0.1745", "  18"}, /* 31 */
  {DIAM_STEPS( 16),  "0.1963", "  16"}, /* 32 */
  {DIAM_STEPS( 15),  "0.2094", "  15"}, /* 33 */
  {DIAM_STEPS( 14),  "0.2244", "  14"}, /* 34 */
  {DIAM_STEPS(13.5), "0.2327", " 13\xBD"}, /* 35 */
  {DIAM_STEPS( 13),  "0.2417", "  13"   }, /* 36 */
  {DIAM_STEPS( 12),  "0.2618", "  12"   }  /* 37 */
};

const int MODULES = 26;

FEED_TABLE module[MODULES] = {
  {MOD_STEPS(0.20), "  0.2 ", "----"}, /* 0  */
  {MOD_STEPS(0.25), "  0.25", "----"}, /* 1  */
  {MOD_STEPS(0.30), "  0.3 ", "----"}, /* 2  */
  {MOD_STEPS(0.35), "  0.35", "----"}, /* 3  */
  {MOD_STEPS(0.40), "  0.4 ", "----"}, /* 4  */
  {MOD_STEPS(0.45), "  0.45", "----"}, /* 5  */
  {MOD_STEPS(0.50), "  0.5 ", "----"}, /* 6  */
  {MOD_STEPS(0.55), "  0.55", "----"}, /* 7  */
  {MOD_STEPS(0.60), "  0.6 ", "----"}, /* 8  */
  {MOD_STEPS(0.65), "  0.65", "----"}, /* 9  */
  {MOD_STEPS(0.70), "  0.7 ", "----"}, /* 10 */
  {MOD_STEPS(0.80), "  0.8 ", "----"}, /* 11 */
  {MOD_STEPS(0.90), "  0.9 ", "----"}, /* 12 */
  {MOD_STEPS(0.95), "  0.95", "----"}, /* 13 */
  {MOD_STEPS(1.00), "  1.0 ", "----"}, /* 14 */
  {MOD_STEPS(1.10), "  1.1 ", "----"}, /* 15 */
  {MOD_STEPS(1.20), "  1.2 ", "----"}, /* 16 */
  {MOD_STEPS(1.25), "  1.25", "----"}, /* 17 */
  {MOD_STEPS(1.30), "  1.3 ", "----"}, /* 18 */
  {MOD_STEPS(1.40), "  1.4 ", "----"}, /* 19 */
  {MOD_STEPS(1.50), "  1.5 ", "----"}, /* 20 */
  {MOD_STEPS(1.60), "  1.6 ", "----"}, /* 21 */
  {MOD_STEPS(1.75), "  1.75", "----"}, /* 22 */
  {MOD_STEPS(1.80), "  1.8 ", "----"}, /* 23 */
  {MOD_STEPS(1.90), "  1.9 ", "----"}, /* 24 */
  {MOD_STEPS(2.00), "  2.0 ", "----"}  /* 25 */
};


#endif // __TABLES_H
