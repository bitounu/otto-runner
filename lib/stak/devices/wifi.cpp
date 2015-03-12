#include <stak/devices/wifi.hpp>
#include <string.h>
#include <stdlib.h>

static uint32_t __stakWifiStatus = 1;
static char* __stakWifiSsid = 0;

//
STAK_EXPORT void stakWifiEnable() {
  __stakWifiStatus = 1;
}

//
STAK_EXPORT void stakWifiDisable() {
  __stakWifiStatus = 0;
}

//
STAK_EXPORT uint32_t stakWifiIsEnabled() {
  return __stakWifiStatus;
}

//
STAK_EXPORT const char* stakWifiSsid() {
  if( !__stakWifiSsid ) {
    return "";
  }
  return __stakWifiSsid;
}

//
STAK_EXPORT void stakWifiSetSsid(const char* ssid) {
  if( __stakWifiSsid ) {
    free( __stakWifiSsid );
  }
  size_t length = strlen( ssid );
  __stakWifiSsid = static_cast<char*>( malloc( length + 1 ) );

  // TODO(Wynter): research security concerns regarding memcpy vs strcpy when
  // string length is already known.
  memcpy( __stakWifiSsid, ssid, length + 1 );
}
