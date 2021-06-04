# ESP32 Matrix Display

This is a fun little project to drive a MAX72xx based matrix display with an ESP32

The idea is to use this as a multi purpose display device outside of my studio, to annouce recording sessions, display arbitrary information based on some home automation etc.

Right now this is little more than an integrated example of the MD_MAX72XX esp example and an IOTAppStory example.

I have settled on trying to make this as simple as possible - for that I have decided to not let the device poll for its text but push it from the outside. This means that the server component
which updates the display has to live inside my LAN, but given that I have an IOTStack docker server running on a Pi4 anyway, this should be fairly easy to set up.

Currently, I'm planning to have the following URL structure:

- `GET /` -> Will display a bit of information and probably also the currently set text
- `POST /update` -> Will update the text via a raw post body. The text will simply be scrolled forever.
- `POST /warning` -> Ability to set a very short text (< length of display) that will flash slowly on the display. (e.g. "!!RECORDING!!")
- `DELETE /warning` -> Disable warning again.

Since this uses IOTAppStory, if you want to play around with this, you'll probably need to get an account there as well.

## Hardware

My hardware is based on the FC16 module as described [here](https://majicdesigns.github.io/MD_MAX72XX/page_f_c16.html). I got mine from AZDelivery (No paid promo, just good prices) in fixed blocks of four and 
soldered 3 of them together to get a 8x96 dot matrix. I'm using a D1 mini ESP32 board which is a thin wrapper around the ESP-WROOM32 board. Power is delivered via USB. I also 3D printed some mounting brackets,
all of which I'm planning on documenting here in greater detail later.

The FC16 module luckily is fine with with taking 3.3V signals on the data lines while being powered by the USB voltage to drive the LEDs. The module runs relatively warm, but not too much, I think. Long term tests are still on my todo list tho.

## Credits / Dependencies

This builds on top of the following projects:

- Platformio
- Espressif's ESP32 Arduino core
- IOTAppStory for updating and wifi config by Andreas Spiess and Onno Dirkzwager
- (And thus, as a transient dependency AsyncWebServer and AsyncTCP by me-no-dev
- MD_MAX72XX by MajicDesigns (currently vendored as library is not registered as a PlatformIO library)

## LICENSE

AGPL 3.0. See [LICENSE.txt](LICENSE.txt)