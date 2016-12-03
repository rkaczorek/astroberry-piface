# astroberry-piface
Astroberry PiFace provides a set of INDI drivers for PiFace addon boards to Raspberry Pi:
- PiFace Relay Plus,
- PiFace Motor Extra,
- PiFace Control and Display.

The project provides INDI drivers using above addons:
- PiFace Relay (indi support for 4 relays 20V/1A with status LEDs, with Motor Extra supports 2 DC or stepper motors),
- PiFace Control and Display (indi support for 2x16 LCD display + 5 buttons + nav button)
- PiFace Focuser (indi focuser providing absolute and relative position control),

Starting from version 2.0.1 the driver supports up to 8 relays (requires 2 PiFace Relay Plus addon modules) and 2 stepper focusers.

# So what's the relation between Astroberry PiFace and Astroberry project?
Astroberry PiFace is a successor of [Astroberry project](https://sourceforge.net/projects/astroberry/). Astroberry project was based on custom addon board, which required understanding of electronics and soldering skills to build. The new project was created to make things easier and let anybody use Raspberry Pi as a INDI server with support to all of these nice features of PiFace addon boards. Astroberry PiFace is based on hardware already available on market, so all you need is to put the tiles together, grab astroberry-piface software and you're ready to go.

# How to start?
First, you need to download and install INDI server and libraries. See [INDI site](http://indilib.org/download.html) for more details.
In most cases it's enough to run:
```
sudo apt-add-repository ppa:mutlaqja/ppa
sudo apt-get update
sudo apt-get install libindi1
```
Second, download and install astroberry-piface.

Compiling from source:
```
git clone https://github.com/rkaczorek/astroberry-piface.git
cd astroberry-piface
mkdir build
cd build
cmake ..
make
make install
```
Installing from binaries:
```
wget astroberry-piface-*.deb
dpkg -i astroberry-piface-*.deb
```

NOTE: you need to have [libmcp23s17](https://github.com/piface/libmcp23s17) and [libpifacecad](https://github.com/piface/libpifacecad) libraries installed on your system to compile and run Astroberry PiFace drivers. Read INSTALL file for details and/or download and install binary packages from "binaries" directory:
```
wget https://github.com/rkaczorek/astroberry-piface/binaries/libmcp23s17_0.3.3-1_armhf.deb
wget https://github.com/rkaczorek/astroberry-piface/binaries/libpifacecad_0.2.0-1_armhf.deb
wget https://github.com/rkaczorek/astroberry-piface/binaries/astroberry-piface_2.0.1-1_armhf.deb
dpkg -i libmcp23s17_0.3.3-1_armhf.deb libpifacecad_0.2.0-1_armhf.deb astroberry-piface_2.0.1-1_armhf.deb
```

#How to use it?
Start your INDI server with Astroberry PiFace drivers:

`indiserver -l /var/log/indi -f /var/run/indi -p 7624 indi_piface_relay indi_piface_cad indi_piface_focuser`

Start KStars with Ekos, connect to your INDI server and enjoy!

NOTE: PiFace Relay Plus hardware address MUST be set to 000 for the first addon module and 001 for the second addon module. To do it you need to set JP1, JP2 and JP3 to 1-2 to set hardware address to 000 and JP1 to 2-3 and JP2, JP3 to 1-2 to set hardware address to 001. See PiFace Relay Plus [documentation](https://www.element14.com/community/servlet/JiveServlet/downloadBody/72070-102-2-303814/Getting%20Started%20-%20Relay.pdf) for details.
