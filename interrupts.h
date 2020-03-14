

#include "globals.h"
#include <avr/interrupt.h>
#include "atmel.h"
#include <util/atomic.h>

#ifndef INTERRUPTS_H
#define INTERRUPTS_H


// Note: Even variables only READ during an interrupt still need to be volatile!!
// https://stackoverflow.com/questions/55278198/is-volatile-needed-when-variable-is-only-read-during-interrupt
// I probably want to use atomic_block actually... For all these volatile variables!!!
// https://www.avrfreaks.net/forum/volatile-variable-access?name=PNphpBB2&file=viewtopic&t=80112
// Otherwise, the compiler can reorder sei/cli!!
// http://www.nongnu.org/avr-libc/user-manual/group__util__atomic.html#gaaaea265b31dabcfb3098bec7685c39e4


/************************************/
/* Timer2 Interrpts: PWM Interrupts */
/************************************/

void pwm_on();
void pwm_off();
inline void set_pwm_to_on_non_atomic() {
    pwm_ptr = &pwm_on;
}
// Equivelent setting ZL to pwm_wdr: in simonk, but we aren't yet using a watchdog.
inline void set_pwm_to_nop_non_atomic() {
    pwm_ptr = &do_nothing;
}
inline void set_pwm_to_off_non_atomic() {
    pwm_ptr = &pwm_off;
}

// Inline functions should have the same addr.
// https://stackoverflow.com/questions/19134124/are-two-function-pointers-to-the-same-function-always-equal
inline bool is_pwm_set_to_nop_non_atomic() {
    return pwm_ptr == &do_nothing;
}


// Copying the comment from simonk.
/*****************************************************************************/
/* ; Timer2 overflow interrupt (output PWM) -- the interrupt vector actually */
/* ; "ijmp"s to Z, which should point to one of these entry points.	     */
/* ;									     */
/* ; We try to avoid clobbering (and thus needing to save/restore) flags;    */
/* ; in, out, mov, ldi, cpse, etc. do not modify any flags, while dec does.  */
/* ;									     */
/* ; We used to check the comparator (ACSR) here to help starting, since PWM */
/* ; switching is what introduces noise that affects the comparator result.  */
/* ; However, timing of this is very sensitive to FET characteristics, and   */
/* ; would work well on some boards but not at all on others without waiting */
/* ; another 100-200ns, which was enough to break other boards. So, instead, */
/* ; we do all of the ACSR sampling outside of the interrupt and do digital  */
/* ; filtering. The AVR interrupt overhead also helps to shield the noise.   */
/* ;									     */
/* ; We reload TCNT2 as the very last step so as to reduce PWM dead areas    */
/* ; between the reti and the next interrupt vector execution, which still   */
/* ; takes a good 4 (reti) + 4 (interrupt call) + 2 (ijmp) cycles. We also   */
/* ; try to keep the switch on close to the start of pwm_on and switch off   */
/* ; close to the end of pwm_aff to minimize the power bump at full power.   */
/* ;									     */
/* ; pwm_*_high and pwm_again are called when the particular on/off cycle    */
/* ; is longer than will fit in 8 bits. This is tracked in tcnt2h.	     */
/*****************************************************************************/

inline void pwm_a_on() {
    if (HIGH_SIDE_PWM) {
	ApFetOn();
    } else {
	AnFetOn();
    }
}
inline void pwm_b_on() {
    if (HIGH_SIDE_PWM) {
	BpFetOn();
    } else {
	BnFetOn();
    }
}
inline void pwm_c_on() {
    if (HIGH_SIDE_PWM) {
	CpFetOn();
    } else {
	CnFetOn();
    }
}

inline void pwm_a_off() {
    if (HIGH_SIDE_PWM) {
	ApFetOff();
    } else {
	AnFetOff();
    }
}
inline void pwm_b_off() {
    if (HIGH_SIDE_PWM) {
	BpFetOff();
    } else {
	BnFetOff();
    }
}
inline void pwm_c_off() {
    if (HIGH_SIDE_PWM) {
	CpFetOff();
    } else {
	CnFetOff();
    }
}

inline void pwm_on_high() {
    --tcnt2h;
    if ( tcnt2h != 0) {
	return;
    }
    set_pwm_to_on_non_atomic();
    return;
}


inline void pwm_on_fast_high() {
    pwm_on_high();
    return;
}

inline void pwm_again() {
    --tcnt2h;
    return;
}

inline void pwm_on_fast() {
    if (a_fet) {
	pwm_a_on();
    }
    if (b_fet) {
	pwm_b_on();
    }

    if (c_fet) {
	pwm_c_on();
    }
    set_pwm_to_off_non_atomic();
    // Now reset the '16' bit timer2 to duty
    // H is the high byte
    tcnt2h = ((0xFF00u & duty) >> 8);
    // L is the low byte.
    TCNT2 = (0xFFu & duty);
    return;
}

inline void pwm_on() {
    pwm_on_fast();
    return;
}

inline void pwm_off() {
    if ( tcnt2h != 0 ) {
	pwm_again();
	return;
    }
    if ( full_power ) {
	pwm_on(); // If full power, simply jump to keeping PWM_ON.
	return;
    }
    pwm_ptr = pwm_on_ptr;
    tcnt2h = ((off_duty & 0xFF00) >> 8);

    // Turn fets off now.
    // Offset by a few cycles, but should be equal on time.
    if (a_fet) {
	pwm_a_off();
    }
    if (b_fet) {
	pwm_b_off();
    }
    if (c_fet) {
	pwm_c_off();
    }
    TCNT2 = (off_duty & 0xFF);
    // Only COMP_PWM stuff beyond this point!
    return;
}


// Disable PWM interrupts and turn off all FETS.
inline void switchPowerOff() {
    disablePWMInterrupts();
    clearPendingPwmInterrupts();
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
	set_pwm_to_nop_non_atomic();
    }
    // Switch all fets off.
    // Turn off all pFets
    ApFetOff();
    BpFetOff();
    CpFetOff();
    // Turn off all nFets
    AnFetOff();
    BnFetOff();
    CnFetOff();
}

#endif
