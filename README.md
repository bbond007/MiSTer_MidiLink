# MiSTer_MidiLink
This is a daemon for the MiSTer DE10-nano FPGA to allow ALSA supported USB MIDI adapters to be used with the Minimig and ao486 cores.

MidiLink 2.0 is now included in MiSTer general and incudes MUNT and
FluidSynth support running on the HPS ARM core!

Uartmode : MIDI : Local / MUNT   

Use MUNT softSynth.

Speed:

      Default : 31250 BPS
      ao486   : 38400 BPS

Roland MT-32/CM-32 roms need to be placed in '/media/fat/mt32-rom-data'

Additional MUNT parameters can be specified in MidiLink.INI
       
      MUNT_OPTIONS = 

Uartmode : MIDI : Local / FSYNTH 

Use FluidSynth softSynth.

Speed:
      
      Default : 31250 BPS
      ao486   : 38400 BPS

FluidSynth soundfont is defined in MidiLink.INI

      FSYNTH_SOUNDFONT   = /media/fat/SOUNDFONT/default.sf2

UartMODE : MIDI : Remote / TCP    

Modem mode supporting a small subset of common Hayes 'AT' Commands.
Default speed for this mode is 115200 BPS

Uartmode : MIDI : Remote / UDP    

Direct connect to another MiSTer or other computer.

Default speed:

      Default : 31250 BPS
      ao486   : 38400 BPS

Many Amiga applications and most games don’t require any additional drivers for MIDI. Some “newer” applications may require the CAMD driver.
      
      http://aminet.net/package/mus/edit/camd
      
While some sequencer applications may support MIDI on the serial port, DOS games typically require a MPU-401 interface which ao486 unfortunately lacks. In lieu of hardware MPU-401 capability the SoftMPU TSR can be used with a good degree of success.

      http://bjt42.github.io/softmpu/
      
SoftMPU requires the QEMM memory manager be installed. For testing QEMM 8.03 was used. QEMM “stealth” option seems to be incompatible with ao486 so it is advisable to skip that part of the optimize process. It’s a good idea to run the QEMM optimize application again after installing SoftMPU (in the AUTOEXEC.BAT) to get as much of the lower 640K conventional RAM free as possible.

Although less common, some DOS games and applications require MPU-401 interrupts. This option can break compatibility with others software not requiring interrupts.

Starting SoftMPU without MPU-401 interrupts:
      
      SOFTMPU.EXE /MPU:330 /OUTPUT:COM1

Starting SoftMPU with MPU-401 interrupts:

      SOFTMPU.EXE /SB:220 /IRQ:5 /MPU:330 /OUTPUT:COM1  

The Rev.0 Roland MT-32 used in testing required the ‘DELAYSYSEX’ switch to prevent buffer overflow for certain games but made Sierra games upload sysex commands excessively slowly.

*** I now recommend using the "DELAYSYSEX" in the MidiLink.INI instead of the
SoftMPU option ***
      
      SOFTMPU.EXE /MPU:330 /DELAYSYSEX /OUTPUT:COM1 

The 'midilink' daemon currently supports following switches / options:

      TESTMIDI - this option sends a middle 'c' note to the MIDI device 
                 once the daemon is started. 

      QUIET    - this option suppresses MIDI debug output.  

      MUNT     - Use MUNT SoftSynth (no USB MIDI adapter 
                 required)

      FSYNTH   - Use FluidSynth SoftSynth (no USB MIDI adapter 
                 required)

      UDP      - Send MIDI to UDP Port (INI setting MIDI_SERVER /
                 MIDI_SERVER_PORT)

      TCP      - Works like a modem with AT commands: 
                 (ATDT, ATBAUD, ATIPADDR, +++ATH)

      MENU     - starts based on /tmp/ML_MUNT, /tmp/ML_UDP, 
                 /tmp/ML_FSYNTH, /tmp/ML_TCP used with MiSTer
                 Menu to set mode of operation. 

The MidiLink.INI file:

      #This is a sample MidiLink.INI FILE  _[DESCRIPTION]_ 

      MIDILINK_PRIORITY  = -20          --> Sets the task priority of MidiLink
      
      MUNT_OPTIONS       =              --> Optional parameters for MUNT

      MP3_VOLUME         = 100          --> Volume for MP3 Player (0 - 100)

      MUNT_VOLUME        = 90           --> Volume for MUNT (0 - 100)  
      
      FSYNTH_VOLUME      = 100          --> Volume for FluidSynth (0 - 100)
      
      FSYNTH_SOUNDFONT   = /media/fat/SOUNDFONT/default.sf2
                                        --> This is the soundfont For 
                                            FluidSynth.
      MIXER_CONTROL      = Master       --> Name of Mixer control (leave
                                            Master!)

      UDP_SERVER         = 192.168.1.52 --> MIDI server for UDP 
      
      UDP_SERVER_PORT    = 1999         --> Port for UDP SERVER and local
                                            listening port
      
      UDP_BAUD           = 2400         --> Set a different BPS for socket
                                            connection (this is useful 
                                            this for serial connection
                                            for gaming (Populous)). This 
                                            overrides arg '38400' (UDP only) 

      UDP_FLOW           = 0            --> This sets the default Flow
                                            Control for UDP option
                                            0 = none 3 = CTS/RTS 
                                            4 = XON/XOFF

      UDP_SERVER_FILTER  = FALSE        --> Only accept Socket data from 
                                            MIDI_SERVER IP (UDP only)

      TCP_SERVER_PORT    = 23           --> Port for TCP Listener

      TCP_TERM_ROWS      = 23           --> Number of default rows for 
                                            file picker.

      
                                        --> Directories for:

      TCP_TERM_UPLOAD    = /media/fat/UPLOAD  --> TCP 'ATRZ' Zmodem Uploads 
                  
      TCP_TERM_DOWNLOAD  = /media/fat/        --> TCP 'ATSZ' Zmodem Downloads 
      
      TCP_TERM_MP3       = /mdia/fat/MP3      --> TCP 'ATMP3' MP3 player
      
      TCP_TERM_MIDI      = /media/fat/MIDI    --> TCP 'ATMID' MIDI player
                           
      TCP_TERM_SYNTH     = FluidSynth   --> Default synth for TCP 'ATMID'

      TCP_BAUD           = 9600         --> Set a different BPS for socket
                                            connection (this is useful 
                                            this for serial connection
                                            for gaming (Populous)). This 
                                            overrides arg '38400' but only 
                                            when used with arg 'TCP'

      MIDI_BAUD          = 38400        --> Set MIDI baud to 38400 
                                            this is for [A0486] 

      TCP_FLOW           = 0            --> This sets the default Flow
                                            Control for TCP option
                                            0 = none 3 = CTS/RTS 
                                            4 = XON/XOFF

      TCP_SOUND          = TRUE         --> TCP option:
                                            Play /media/fat/SOUNDS/dial.wav
                                            Play /media/fat/SOUNDS/connect.wav

      #TCP_SOUND_RING    =              --> optional WAV file for TCP modem 
                                            ring

      #TCP_SOUND_DIAL    =              --> optional WAV file for TCP modem 
                                            dial

      #TCP_SOUND_CONNET  =              --> optional WAV file for TCP modem 
                                            connect

      DELAYSYSEX         = TRUE         --> This option fixes "Buffer Overflow" 
                                            error on Roland MT-32 Rev0.
      
      MT32_LCD_MSG       = MiSTer MIDI! --> This shows a custom message on
                                            the MT-32 LCD Screen. Max 20!
      

MidiLink : TCP
The TCP option works like a WiFi232 adapter supporting a small subset of the Hayes "AT" command set and some additionl stuff
      
      AT       - Attention
      ATBAUD#  - Set baud rate (can't auto-detect yet) --> ATBAUD9600 or ATBAUD6 (#6 from menu)
      ATBAUD   - Show baud rate menu.
      ATDIR    - Show dialing Directory (/media/fat/linux/MidiLink.DIR)
      ATDT     - Dial --> ATDT192.168.1.131:23 or ATDTBBS.DOMAIN.COM:31337 ( '*' can also be used in place of ':')
      +++ATH   - Hang-up 
      ATINI    - Show MidiLink.INI (/media/fat/linux/MidiLink.INI)
      ATIP     - Show IP address --> ATIP
      AT&K0    - Disable local flow control
      AT&K3    - RTS/CTS bidirectional hardware flow control
      AT&K4    - XON/XOFF bidirectional software flow control
      ATMID1   - Switch soft-synth to FluidSynth
      ATMID2   - Switch soft-synth to MUNT
      ATMID    - Play MIDI file (see file picker)
      ATMIDSF  - Select FluidSynth SoundFont (Change MidiLink.INI)
      ATMID!   - Stop currently playing MIDI
      ATM0     - Enable modem sounds
      ATM1     - Disable modem sounds 
      ATMP3    - Play MP3 file (see file picker)
      ATMP3!   - Stop currently playing MP3 File
      ATROWS   - Do terminal row test
      ATROWS## - Set number of terminal rows for MIDI, MP3 and Zmodem file picker (0 for continious list - no pause between p
      ATRZ     - Receive a file using Zmodem
      ATSZ     - Send a file via Zmodem (see file Picker)
      ATTEL0   - Disable basic telnet negotiation 
      ATTEL1   - Enable basic telnet negotiation (default)
      ATVER    - Show MidiLink version

Navigation within the file picker - ATSZ, ATMIDI and ATMP3

      [RETURN] / [SPACE] - Next page of results
      "P" - Return to pervious page of results
      "Q" - Quit without making selection
      "-" - change to parent directory 

File picker prompts:
     
      MORE ##? --> Indicates at least one additional page of results 
      END ##?  --> Indicateds end of the list

