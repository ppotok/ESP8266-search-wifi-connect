ESP8266 arduino AP mode, NTP time
ESP tries to connect to wifi AP, if it fail, start AP mode where you can choose wifi to connect. After submit module reboots and tries again to connect to that wifi.

In this sketch its added NTP time (time.h) where it gets synchronized time.
ESP8266 serves time on /time, and date on /date

In further development I want to add support of reloading div in page without reloading whole site.
