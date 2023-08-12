#define ONEWIREPIN 4
#define DEFEATRELAYPIN 23
#define SETRELAYPIN 22
#define AUXRELAYPIN 21
#define VREF12PIN 35
#define CURRAPIN 36
#define CURRBPIN 39
#define LINEPIN 34
#define BATPIN 32
typedef struct {
  float refSlope;
  float refOffset;
  float batSlope;
  float batOffset;
  float lineSlope;
  float lineOffset;
  float currSlope;
  float currOffset;
} calibration_t;
typedef struct {
  uint8_t min;
  uint8_t hr;
  uint8_t dow;
  uint8_t freq;
} cront_t;
/* Cron frequency conversion
  0 = null
  1 = weekly
  2 = Semi Monthly
  3 = Monthly
  4 = Quarterly
  5 = Anually
*/
