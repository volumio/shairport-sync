# Working with PulseAudio or PipeWire
Many Linux systems have [PulseAudio](https://www.freedesktop.org/wiki/Software/PulseAudio/) or [PipeWire](https://pipewire.org) installed as [sound servers](https://en.wikipedia.org/wiki/Sound_server), typically providing
audio facilities for the system's GUI. Unfortunately, they cause problems for Shairport Sync. As you'll see, these problems can be worked around.

The following remarks apply to PulseAudio and PipeWire, so, for simplicity, let's refer to PulseAudio only.

To understand the problems, first consider a  Linux system that _does not_ have PulseAudio installed.
1. Sound is managed by the Advanced Linux Sound Architecture ("[ALSA](https://www.alsa-project.org/wiki/Main_Page)") subsystem.
2. The ALSA subsystem is loaded and becomes available at system startup.
3. The ALSA `"default"` output device is mapped to the system's audio output DAC.

Shairport Sync loads when system startup is complete and provides its service, routing audio to the ALSA default device.

Now, consider a Linux system with a Graphical User Interface (GUI) that has PulseAudio installed.
1. As before, sound is managed by the ALSA subsystem.
2. As before, The ALSA subsystem is loaded and becomes available at system startup.
3. PulseAudio loads and becomes a client of ALSA.
4. The PulseAudio service becomes available after a user has logged in through the GUI. Importantly, if you don't log in through the GUI, you won't have a PulseAudio service.
5. The ALSA `"default"` device no longer connects to a real output device. Instead audio sent to the `"default"` device is routed into PulseAudio. Importantly, if you have no PulseAudio service, then the ALSA default device either doesn't exist at all or goes nowhere.

# The Problem
When Shairport Sync is installed as a system service, it starts after the system has booted up.
It runs under a low-priviliged user called `shairport-sync`. 
Unfortunately, per (4) above, since it was not logged in through the GUI, it won't have a PulseAudio service. If you are using the `"pa"` backend, Shairport Sync may well crash.
Per (5) above, the ALSA `"default"` device either won't exist or -- if it does exist -- won't work. Hence, using the `"alsa"` backend, Shairport Sync will either terminate or remain silent.

# The Fixes
There are three ways to address this problem:
1. Stop using Shairport Sync as a system service. Instead, launch it from a terminal window after log-in through the GUI.
Alternatively, create a user service startup script so that it will be launched automatically after the user has logged in. This means that the system can not run without user intervention.
2. If you are using the `"alsa"` backend, set the output device to a real ALSA hardware output device. Use `$ shairport-sync -h` (or, better, [`sps-alsa-explore`](https://github.com/mikebrady/sps-alsa-explore)) to get a list of output devices, and use the configuration file to set the device. There is a possibility this might prevent PulseAudio from working properly. 
# The Best Fix
3. The best of all fixes is, if possible, to avoid using a system containing PulseAudio altogether. Linux operating systems typically have a "server", "lite" or "headless" version that has no GUI and no PulseAudio.
