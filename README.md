# ESP32-2432S028 Heart Rate Monitor
Heart rate monitor for the ESP32-2432S028 "Cheap Yellow Display" and a Polar H10 for my workouts

This is an Arduino sketch made to run on an ESP32-2432S028, AKA CYD ("Cheap Yellow Display"), all-in-one board. This board combines an ESP-32 Wroom32 (BlueTooth/WiFi) module, a 320x240px backlit color TFT screen (with touch), a TF-card slot, audio (without speaker: this is optional) an RGB LED (unfortunately on the back), an ambient light sensor (LDR) and u USB-to-UART chip (also for uploading code). The board is not much bigger than the display. The sensor used is o Polar H10.
It should be possible to run the code on any BLE capable ESP32 with minor modifications.

An RTC module is added to have the current time. Perhaps in the future I will make it possible to sync the RTC with an NTP server over WiFi. A battery (or single cell if voltage is sufficient) is used to power the board.

The software reads the heart rate from the H10, as well as the interval between heart beats (in milliseconds) and displays the current heart rate (in beats per minute, BPM) on the screen and in a running graph. Soon, functionality will be added to save the total history on a TF card and/or a flash chip on the board (for which space is reserved but the part is not added).
