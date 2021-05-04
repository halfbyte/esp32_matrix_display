# ESP32 Matrix Display

This is a fun little project to drive a MAX72xx based matrix display with an ESP32

The idea is to use this as a multi purpose display device outside of my studio, to annouce recording sessions, display arbitrary information based on some home automation etc.

Right now this is little more than an integrated example of the MD_MAX72XX esp example and an IOTAppStory example.

Here's a rough todo-list:

- [ ] Implement a simple server that will drive the display which should pull the info in regular intervals.
- [ ] Implement the pulling code and use IOTAppStory for the configuration (URL of the text server)
- [ ] Allow regular texts and alerts, so that the regular messages can be overridden.

Since this uses IOTAppStory, if you want to play around with this, you'll probably need to get an account there as well.

## Hardware

My hardware is based on the FC16 module as described [here](https://majicdesigns.github.io/MD_MAX72XX/page_f_c16.html). I got mine from AZDelivery (No paid promo, just good prices) in fixed blocks of four and 
soldered 3 of them together to get a 8x96 dot matrix. I'm using a D1 mini ESP32 board which is a thin wrapper around the ESP-WROOM32 board. Power is delivered via USB. I also 3D printed some mounting brackets,
all of which I'm planning on documenting here in greater detail later.

## Credits / Dependencies

This builds on top of the following projects:

- Platformio
- Espressif's ESP32 Arduino core
- IOTAppStory for updating and wifi config by Andreas Spiess and Onno Dirkzwager
- (And thus, as a transient dependency AsyncWebServer and AsyncTCP by me-no-dev
- MD_MAX72XX by MajicDesigns (currently vendored as library is not registered as a PlatformIO library)

## LICENSE

AGPL 3.0. See [LICENSE.txt](LICENSE.txt)