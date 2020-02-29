#ifndef COMMUTATIONS_H
#define COMMUTATIONS_H

struct OrigPortNewPortVal {
private:
    const byte orig;
    const byte new_val;
public:
    byte get_orig() const { return orig; }
    byte get_new() const { return new_val; }
    OrigPortNewPortVal(const byte orig, const byte new_val) : orig(orig), new_val(new_val) {
    }

    bool values_equal() const {
	return get_orig() == get_new();
    }
};

// These *FOCUS* functions do nothing with comp_pwm disabled
void pwm_focus_c_off() {};
void pwm_focus_c_on() {};
void pwm_focus_b_off() {};
void pwm_focus_b_on() {};
void pwm_focus_a_off() {};
void pwm_focus_a_on() {};

// Skip setting A if we didn't change the port in copy_from.
// Otherwise, change a.
void pwm_a_copy(const OrigPortNewPortVal copy_from) {
    if (copy_from.values_equal()) { return; }
    if ( HIGH_SIDE_PWM ) { ApFetOn(); } else { AnFetOn(); }
}
void pwm_b_copy(const OrigPortNewPortVal copy_from) {
    if (copy_from.values_equal()) { return; }
    if ( HIGH_SIDE_PWM ) { BpFetOn(); } else { BnFetOn(); }
}
void pwm_c_copy(const OrigPortNewPortVal copy_from) {
    if (copy_from.values_equal()) { return; }
    if ( HIGH_SIDE_PWM ) { CpFetOn(); } else { CnFetOn(); }
}

OrigPortNewPortVal pwm_a_clear() {
    byte orig_port_value = PWM_A_PORT_IN;
    if ( HIGH_SIDE_PWM ) {
	ApFetOff();
    } else {
	AnFetOff();
    }
    byte new_port_value = PWM_A_PORT_IN;
    return OrigPortNewPortVal(orig_port_value, new_port_value);
}
OrigPortNewPortVal pwm_b_clear() {
    byte orig_port_value = PWM_B_PORT_IN;
    if ( HIGH_SIDE_PWM ) {
	BpFetOff();
    } else {
	BnFetOff();
    }
    byte new_port_value = PWM_B_PORT_IN;
    return OrigPortNewPortVal(orig_port_value, new_port_value);
}

OrigPortNewPortVal pwm_c_clear() {
    byte orig_port_value = PWM_C_PORT_IN;
    if ( HIGH_SIDE_PWM ) {
	CpFetOff();
    } else {
	CnFetOff();
    }
    byte new_port_value = PWM_C_PORT_IN;
    return OrigPortNewPortVal(orig_port_value, new_port_value);
}

void com1com6() {
    set_comp_phase_c();
    cli();
    all_fets = false;
    a_fet = true;
    pwm_focus_c_off();
    pwm_a_copy(pwm_c_clear());
    pwm_focus_a_on();
    sei();
}
void com6com5() {}
void com5com4() {}
void com4com3() {}
void com3com2() {}
void com2com1() {}


#endif
