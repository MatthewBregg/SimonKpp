#include "atmel.h"
#include "beep.h"
// REMEMBER: VARIABLES BEING set/access from an interrupt must be volatile!

// Afro NFet ESCs have an external 16mhz oscillator.
// We are disregarding the bootloader, and will be using USBASP.
// For laziness/convenience, we will be using https://github.com/carlosefr/atmega to flash.
//
// This does not set fuses, we must do so manually if it's a new chip.
//    With external oscillator: avrdude -U lfuse:w:0x3f:m -U hfuse:w:0xca:m
void setup() {
  // put your setup code here, to run once:
    setDefaultRegisterValues();
    // Start off with the beepies!
    beepF1();
    delay(2000);
    beepF2();
    delay(2000);
    beepF3();
    delay(2000);
    beepF4();
    delay(2000);
}

void loop() {
    enableTimerInterrupts();
    sei();
    return;
}

volatile bool oct1_pending = false;
volatile byte ocr1ax = 0; // third byte of OCR1A.
volatile byte tcnt1x = 0; // third byte of TCNT1.
volatile byte rc_timeout = 0;
volatile byte rct_boot = 0; // Counter which increments while rc_timeout is 0 to jump to boot loader (TODO delete?)
volatile byte rct_beacon = 0; // Counter which increments while rc_timeout is 0 to disarm and beep occasionally (TODO delete?)

///////////////////////////////////////////////////////////////////////////////////////
// Interrupt Documentation							     //
//    http://ee-classes.usc.edu/ee459/library/documents/avr_intr_vectors/	     //
//    https://arduino.stackexchange.com/questions/32721/interrupts-on-the-arduino-ng //
// BE VERY CAREFUL ABOUT USING RETURN STATEMENTS in interrupts, see                  //
// https://www.avrfreaks.net/forum/return-statement-isr                              //
///////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Be sure to check interrupt names for the ATMEGA8 here 							     //
// https://www.nongnu.org/avr-libc/user-manual/group__avr__interrupts.html#gad28590624d422cdf30d626e0a506255f	     //
// The LEDs can be used to verify an interrupt works				      				     //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// timer1 output compare interrupt
ISR(TIMER1_COMPA_vect) {
    if (ocr1ax < 1) {
	oct1_pending = false; // Passed OCT1A.
    }
    --ocr1ax;
}

// timer1 overflow interrupt (happens every 4096µs)
ISR(TIMER1_OVF_vect) {
    ++tcnt1x;
    if ( (tcnt1x & B1111 /* 15U */ ) == 0 ) {
	if ( rc_timeout == 0 ) {
	    // rc_timeout hit, increase the beacon.
	    ++rct_boot;
	    ++rct_beacon;
	}
    }
}

volatile bool set_duty = false;
constexpr unsigned short MIN_DUTY = 56 * cpuMhz/16;
constexpr unsigned short POWER_RANGE = 1500U * cpuMhz/16 + MIN_DUTY;
constexpr unsigned short MAX_POWER = POWER_RANGE-1;
constexpr unsigned short PWR_MIN_START = POWER_RANGE/6;
constexpr byte RCP_TOT = 2U; // Number of 65536us periods before considering rc pulse lost
constexpr byte ENOUGH_GOODIES = 6; // This many start cycles without timeout will transition to running mode (tm4 experimental 05-01-18 - stock 12)

volatile byte power_skip = 6U;
volatile byte goodies = 0U;

volatile bool startup = false;
volatile bool aco_edge_high = false;



void wait_pwm_running() {

}

void wait_pwm_enable() {
    if (pwmSetToNop() ) {
	setPwmToOff();
	redLedOff();
    }
    wait_pwm_running();
}

void wait_for_edge0() {

}

void wait_for_edge1() {

}

void set_ocr1a_rel(unsigned short Y) {

}

void wait_for_edge() {
    if (power_skip < 1) { // Are we trying to track a maybe running motor?
	wait_pwm_enable();
	return;
    }
    // We might be trying to track a maybe running motor.
    --power_skip;
    if (!startup) {
	wait_for_edge0();
	return;
    }
    unsigned short Y = (0xFFU * 0x100U);
    set_ocr1a_rel(Y);
    // xl = ZC_CHECK_MIN.
    wait_for_edge1();
    return;
}

void wait_for_low() {
    aco_edge_high = false;
    wait_for_edge();
}

void wait_for_high() {
    aco_edge_high = true;
    wait_for_edge();
}

//  See run1 in simonk source.
void run() {

}

void startFromRunning() {
    switchPowerOff();
    initComparator();
    greenLedOff();
    redLedOff();

    // We could just truncate, https://www.learncpp.com/cpp-tutorial/unsigned-integers-and-why-to-avoid-them/
    // as the above link shows is safe, but avoid a compiler warning by using a bit mask to extract the lower 8 bits
    // of PWR_MIN_START.
    const byte sys_control_l = (0xFU /* Lol */ & PWR_MIN_START);
    set_duty = true;

    // Initialization.
    // TODO: Update this comment to account for new control scheme.
    // Start with a short timeout to stop quickly
    // if we see no further pulses after the first.
    // Do not enable FETs during first cycle to
    // see if motor is running, and align to it.
    // If we can follow without a timeout, do not
    // continue in startup mode (long ZC filtering).
    // Enable PWM (ZL has been set to pwm_wdr)
    //
    // We also set wait_timeout_init, start_delay, start_modulate,
    // and start_fail to 0x00 in simonK, but don't add them here quite yet...
    rc_timeout = RCP_TOT;
    power_skip = 6U;
    goodies = ENOUGH_GOODIES;
    enablePwmInterrupt();

}

// Also encapsulates wait_for_power_*
void restartControl() {
    switchPowerOff();
    set_duty = false;
    greenLedOn();
    redLedOff();
    // Idle beeping happened here in simonk.
    // dib_l/h set here (although not rc_duty?).
    // YL/rc_duty is set however, which seems to correspond to power?
    // After a bunch of puls input validation yadda yadda, we get to start_from_running
    startFromRunning();
}
