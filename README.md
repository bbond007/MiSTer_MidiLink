# MiSTer_MidiLink
This is a daemon for the MiSTer DE10-nano FPGA to allow ALSA supported USB MIDI adapters to be used with the Minimig and ao486 cores.

MidiLink is now included in MiSTer general!

When running the Minimig and ao486 cores, once a compatible USB MIDI device is attached, two additional ‘UART Connection’ menu options (‘USBMIDI’ and ‘USBMIDI-38K’) will be available in addition to ‘None’, ‘PPP’ and ‘Console’.

‘USBMIDI’ - This option is used with the Amiga / Minimig core. This option sets the UART connection speed to 31250 baud which is the standard MIDI speed.

Many Amiga applications and most games don’t require any additional drivers for MIDI. Some “newer” applications may require the CAMD driver.
      
      http://aminet.net/package/mus/edit/camd
      
‘USBMIDI-38K’ -This option is used with the ao486 core. This option sets the UART Connection speed to 38400 baud. (The MIDI speed of 31250 baud is not a standard speed DOS PC UARTs were capable of doing)

While some sequencer applications may support MIDI on the serial port, DOS games typically require a MPU-401 interface which ao486 unfortunately lacks. In lieu of hardware MPU-401 capability the SoftMPU TSR can be used with a good degree of success.

      http://bjt42.github.io/softmpu/
      
SoftMPU requires the QEMM memory manager be installed. For testing QEMM 8.03 was used. QEMM “stealth” option seems to be incompatible with ao486 so it is advisable to skip that part of the optimize process. It’s a good idea to run the QEMM optimize application again after installing SoftMPU (in the AUTOEXEC.BAT) to get as much of the lower 640K conventional RAM free as possible.

Although less common, some DOS games and applications require MPU-401 interrupts. This option can break compatibility with others software not requiring interrupts.

Starting SoftMPU without MPU-401 interrupts:
      
      SOFTMPU.EXE /MPU:330 /OUTPUT:COM1

Starting SoftMPU with MPU-401 interrupts:

      SOFTMPU.EXE /SB:220 /IRQ:5 /MPU:330 /OUTPUT:COM1  

The Rev.0 Roland MT-32 used in testing required the ‘DELAYSYSEX’ switch to prevent buffer overflow for certain games but made Sierra games upload sysex commands excessively slowly.
      
      SOFTMPU.EXE /MPU:330 /DELAYSYSEX /OUTPUT:COM1

The 'midilink' daemon currently supports following switches / options:

      TESTMIDI - this option sends a middle 'c' note to the MIDI device 
                 once the daemon is started. 

      QUIET    - this option suppresses MIDI debug output.  

      38400    - this option sets the serial speed to 38400 baud 
                 (default is 31200 baud) - used with ao486 core.
      
      MUNT     - Use MUNT SoftSynth (no USB MIDI adapter 
                 required)

      FSYNTH   - Use FluidSynth SoftSynth (no USB MIDI adapter 
                 required)

      UDP      - Send MIDI to UDP Port (INI setting MIDI_SERVER /
                 MIDI_SERVER_PORT)

      AUTO     - starts based on /tmp/ML_MUNT, /tmp/ML_UDP, /tmp/ML_FSYNTH 
                 used with MiSTer Menu to set mode of operation. 

The Midilink INI file:

      #This isa sample MidiLink.INI FILE 

      MIDILINK_PRIORITY = -20          --> Sets the task priority of MidiLink
      MUNT_VOLUME       = 71           --> Volume for MUNT  
      FSYNTH_VOLUME     = 83           --> Volume for FluidSynth
      MIDI_SERVER       = 192.168.1.52 --> MIDI server for UDP 
      MIDI_SERVER_PORT  = 1999         --> Port for MIDI SERVER and local
      					   listening port
      #MIDI_SERVER_BAUD = 2400         --> Set a different BPS for socket
                                           connection (this is useful 
                                           this for serial connection
                                           for gaming (Populous)). This 
                                           overrides arg '38400' but only 
                                           when used with arg 'UDP'
								 
      FSYNTH_SOUNDFONT  = /media/fat/SOUNDFONT/default.sf2

                                       --> This is the soundfont For 
                                           FluidSynth. 