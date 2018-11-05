///////////////////////////////////////////////////////////////////////////////////////
//
// This program is a midi <> serial link for the MiSTer FPGA platform.
// It requires the following kernel changes:
//
// - Alsa Sound support "soundcore"
// - USB sound and midi support "snd-usb-audio"
//
// Thanks to https://ccrma.stanford.edu/~craig/articles/linuxmidi/ for the MIDI programming examples.
// 
// /dev/sequencer does not seem to work for input

#define MIDI_INPUT
#define MIDI_DEV

static char          * serialDevice     = "/dev/ttyS1";
static char          * helloStr         = "BinaryBond007 MidiLink 1.0B\n";
static unsigned char   midiDevnum       = 0;
static unsigned char   test_note[3]     = {0x90, 60, 127};
#ifdef MIDI_DEV
static char          * midiDevice       = "/dev/midi";
static char          * midi1Device      = "/dev/midi1";
#else
static char          * midiDevice       = "/dev/sequencer";
#endif