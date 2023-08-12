#GenSmart
An ESP32 based IOT enhancement for my home generator.

The board intersects some of the control signals to the generator built-in controller and controlls the excercise time so the weekly excercise can be cut down to monthly, Quaterly or Annually, etc. It also adds a simulate power fail mode.  It monitors current out, line voltage and battery voltage.  It sends an email/SMS on power fail and power resume.

This uses my iotfw library for most of the work.