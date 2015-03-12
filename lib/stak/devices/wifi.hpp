#ifndef STAK_DEVICES_WIFI_HPP
#define STAK_DEVICES_WIFI_HPP
#include <stak/stak.hpp>
#include <stak/types.hpp>

STAK_EXPORT void stakWifiEnable();
STAK_EXPORT void stakWifiDisable();
STAK_EXPORT uint32_t stakWifiIsEnabled();
STAK_EXPORT const char* stakWifiSsid();
STAK_EXPORT void stakWifiSetSsid(const char* ssid);
#endif//STAK_DEVICES_WIFI_HPP
