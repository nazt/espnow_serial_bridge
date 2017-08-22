#ifndef CMMCBlink_h
#define CMMCBlink_h
#include <Arduino.h>
#include <Ticker.h>

class CMMC_Blink
{
  public:

    CMMC_Blink init() {
      this->_ticker = new Ticker();
      _initialized = true;
      return *this;
    }

    void setPin(uint8_t pin) {
      pinMode(_ledPin, OUTPUT);
      digitalWrite(_ledPin, LOW);
      _ledPin = pin;
    }

    void blink(int ms, uint8_t pin) {
      this->setPin(pin);
      this->blink(ms);
    }

    void detach() {
      this->_ticker->detach();
    }

    void blink(int ms) {
      if (!_initialized) return;
      if (_ledPin == 254) return;
      static int _pin = this->_ledPin;
      this->_ticker->detach();
      auto lambda = []() {
        int state = digitalRead(_pin);  // get the current state of GPIOpin pin
        digitalWrite(_pin, !state);     // set pin to the opposite state
      };
      this->_ticker->attach_ms(ms, lambda);
    }

  private:
    unsigned int _ledPin = 254;
    Ticker *_ticker;
    bool _initialized = false;

};


#endif
