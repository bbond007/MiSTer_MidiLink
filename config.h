///////////////////////////////////////////////////////////////////////////////////////
//
// This program is a midi <> serial link for the MiSTer FPGA platform.
// It requires the following kernel changes:
//
// - Alsa Sound support "soundcore"
// - USB sound and midi support "snd-usb-audio"
//
// Thanks to https://ccrma.stanford.edu/~craig/articles/linuxmidi/ for the 
// excellent MIDI programming examples.
// 

char                 * midiLinkINI      = "/media/fat/linux/MidiLink.INI"; 
char                 * midiLinkDIR      = "/media/fat/linux/MidiLink.DIR";
char                 * serialDevice     = "/dev/ttyS1";
char                 * helloStr         = "MiSTer MidiLink 3.2.2 BB7";
char                 * PCMDevice        = "/dev/snd/pcmC0D0p";
int                    CPUMASK          = 1;
static char          * MrAudioDevice    = "/dev/MrAudio";
static unsigned char   test_note[3]     = {0x90, 60, 127};
static char          * midiINDevice     = "/dev/midi2";
static char          * midiDevice       = "/dev/midi1";
static char          * serialDeviceUSB  = "/dev/ttyUSB0";
static char          * tmpSoundfont     = "/tmp/ML_SOUNDFONT";
static char          * tmpBAUD          = "/tmp/ML_BAUD";

char all_notes_off[] = 
{
    0xb0, 0x7b, 0x00, 0xb0, 0x40, 0x00, 0xb1, 0x7b, 0x00, 0xb1, 0x40, 0x00, 0xb2, 0x7b, 0x00, 0xb2, 0x40, 0x00, 
    0xb3, 0x7b, 0x00, 0xb3, 0x40, 0x00, 0xb4, 0x7b, 0x00, 0xb4, 0x40, 0x00, 0xb5, 0x7b, 0x00, 0xb5, 0x40, 0x00, 
    0xb6, 0x7b, 0x00, 0xb6, 0x40, 0x00, 0xb7, 0x7b, 0x00, 0xb7, 0x40, 0x00, 0xb8, 0x7b, 0x00, 0xb8, 0x40, 0x00, 
    0xb9, 0x7b, 0x00, 0xb9, 0x40, 0x00, 0xba, 0x7b, 0x00, 0xba, 0x40, 0x00, 0xbb, 0x7b, 0x00, 0xbb, 0x40, 0x00, 
    0xbc, 0x7b, 0x00, 0xbc, 0x40, 0x00, 0xbd, 0x7b, 0x00, 0xbd, 0x40, 0x00, 0xbe, 0x7b, 0x00, 0xbe, 0x40, 0x00, 
    0xbf, 0x7b, 0x00, 0xbf, 0x40, 0x00
};
    
int sizeof_all_notes_off = sizeof(all_notes_off);




