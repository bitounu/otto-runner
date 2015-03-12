#ifndef STAK_IO_PWM_HPP
#define STAK_IO_PWM_HPP
#include <stak/stak.hpp>
#include <stak/types.hpp>

// pwm functionality operates on pwm channels rather than gpio pins which
// allows developers to ignore the specifics of pin configuration unless
// they explicitly need to for other reasons

STAK_EXPORT void stakPwmEnable( uint32_t pwmChannel );
STAK_EXPORT void stakPwmDisable( uint32_t pwmChannel );
STAK_EXPORT void stakPwmStart( uint32_t pwmChannel );
STAK_EXPORT void stakPwmStop( uint32_t pwmChannel );
STAK_EXPORT void stakPwmChangeFrequency( uint32_t pwmChannel, uint32_t frequency );
STAK_EXPORT void stakPwmChangeDutyCycle( uint32_t pwmChannel, float dutyCycle );

#endif//STAK_DEVICES_IO_PWM_HPP
