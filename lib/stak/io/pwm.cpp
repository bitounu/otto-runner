#include <stak/io/pwm.hpp>
#include <iostream>

//
STAK_EXPORT void stakPwmEnable( uint32_t pwmChannel ) {
  std::cout << "Enabling Pwm on pwm channel "
            << pwmChannel
            << std::endl;
}

//
STAK_EXPORT void stakPwmDisable( uint32_t pwmChannel ) {
  std::cout << "Disabling Pwm on pwm channel "
            << pwmChannel
            << std::endl;
}

//
STAK_EXPORT void stakPwmStart( uint32_t pwmChannel ) {
  std::cout << "Starting Pwm on pwm channel "
            << pwmChannel
            << std::endl;
}

//
STAK_EXPORT void stakPwmStop( uint32_t pwmChannel ) {
  std::cout << "Stoping Pwm on pwm channel "
            << pwmChannel
            << std::endl;
}

//
STAK_EXPORT void stakPwmChangeFrequency( uint32_t pwmChannel, uint32_t frequency ) {
  std::cout << "Setting Pwm frequency to "
            << frequency
            << "Hz on pwm channel "
            << pwmChannel
            << std::endl;
}

//
STAK_EXPORT void stakPwmChangeDutyCycle( uint32_t pwmChannel, float dutyCycle ) {
  std::cout << "Setting Pwm cycle to "
            << dutyCycle
            << " steps on pwm channel "
            << pwmChannel
            << std::endl;
}
