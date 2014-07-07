#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define LEDPIN 13

// ISR (TIMER1_COMPA_vect) {
//   digitalWrite(LEDPIN, !digitalRead(LEDPIN));
// }

// void enable_timer() {
//   cli();

//   // Reset timer and control register
//   TCCR1A = 0;     
//   TCCR1B = 0;     

//   // Set compare register
//   OCR1A = 0xFFFF;

//   // Set clear timer on compare
//   TCCR1B |= (1 << WGM12);
//   // Prescaler 1024
//   TCCR1B |= (1 << CS10) | (1 << CS12);
//   // Set output compare A interrupt
//   TIMSK1 |= (1 << OCIE1A); 
  
//   sei();
// }

// void enable_watchdog() {
//   cli();

//   // Reset watchdog status register flag 
//   MCUSR &= ~(1 << WDRF);
  
//   // Must be set to change prescaler
//   WDTCSR = (1 << WDE) | (1 << WDCE);
//   // Set prescale to 8 sec.
//   WDTCSR = (1 << WDP0) | (1 << WDP3);
//   // Interrupt enable
//   WDTCSR |= (1 << WDIE);
  
//   sei();
// }

// Watchdog interrupt routine
ISR (WDT_vect) {
  wdt_disable();
}

void enable_watchdog() {
  cli();

  wdt_enable(WDTO_500MS);
  // WDT interrupt enable
  WDTCSR |= (1 << WDIE);
  
  sei();
}

void power_down() {
  set_sleep_mode(SLEEP_MODE_PWR_SAVE);
  cli();

  sleep_enable();
  sei();

  sleep_cpu();
  sleep_disable();
}

void disable_peripheral() {
  power_usart0_disable();
  power_spi_disable();
  power_twi_disable();

  power_timer0_disable();
  power_timer1_disable();
  power_timer2_disable();

  // Disable all digital input buffers on ADC pins
  DIDR0 = 0b00111111;
}

void prescale_cpu() {
  // Enable clock prescale changes
  CLKPR = (1 << CLKPCE);

  // Prescale by 16 --> avr runs at 1MHz
  CLKPR = (1 << CLKPS2);
}

// ################################### //
// ############ Arduino ############## //
// ################################### //
void setup() {
  pinMode(13, OUTPUT);

  disable_peripheral();
  enable_watchdog();
  prescale_cpu();
}

void loop() {
  power_down();
  digitalWrite(LEDPIN, !digitalRead(LEDPIN));
  enable_watchdog();
}