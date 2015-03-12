#include <stak/io/buzzer.hpp>
#include <iostream>


// buzzer functionality is essentially a wrapper around pwm functionality with
// some naming changed to accomodate the idea that someone working with a piezo
// buzzer will be more focused on generating audio rather than dealing with the
// specifics of how pwm translates to audible output

//
STAK_EXPORT void stakBuzzerEnable( uint32_t pwmChannel ) {
  std::cout << "Enabling buzzer on pwm channel "
            << pwmChannel
            << std::endl;
}

//
STAK_EXPORT void stakBuzzerDisable( uint32_t pwmChannel ) {
  std::cout << "Disabling buzzer on pwm channel "
            << pwmChannel
            << std::endl;
}

//
STAK_EXPORT void stakBuzzerStart( uint32_t pwmChannel ) {
  std::cout << "Starting buzzer on pwm channel "
            << pwmChannel
            << std::endl;
}

//
STAK_EXPORT void stakBuzzerStop( uint32_t pwmChannel ) {
  std::cout << "Stoping buzzer on pwm channel "
            << pwmChannel
            << std::endl;
}

//
STAK_EXPORT void stakBuzzerChangeFrequency( uint32_t pwmChannel, uint32_t frequency ) {
  std::cout << "Setting buzzer frequency to "
            << frequency
            << "Hz on pwm channel "
            << pwmChannel
            << std::endl;
}


// halfSteps should be a signed integer of how many steps the note is relative
// to middle c
// TODO(Wynter: Should this be relative to A440?

STAK_EXPORT void stakBuzzerChangeNote( uint32_t pwmChannel, int32_t halfSteps ) {
  std::cout << "Setting buzzer note to Middle C"
            << halfSteps
            << " steps on pwm channel "
            << pwmChannel
            << std::endl;
}

//
STAK_EXPORT void stakBuzzerChangeVolume( uint32_t pwmChannel, float volume ) {
  std::cout << "Setting buzzer volume to"
            << volume
            << " on pwm channel "
            << pwmChannel
            << std::endl;
}
