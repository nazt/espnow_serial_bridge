#ifndef CMMCBlink_h
#define CMMCBlink_h
#include <Arduino.h>
#include <Ticker.h>

class CMMC_Blink
{
  public:

    CMMC_Blink init() {
      this->_ticker = new Ticker();
      this->state = LOW;
      this->_initialized = true;
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
      static bool *state = &this->state;
      this->_ticker->detach();
      auto lambda = []() {
        *state = !*state;
        digitalWrite(_pin, *state);     // set pin to the opposite state
      };
      this->_ticker->attach_ms(ms, lambda);
    }

  private:
    unsigned int _ledPin = 254;
    bool state;
    bool _initialized = false;
    Ticker *_ticker;
    // auto *callback;
};


#endif
