### Description

This is https://web.archive.org/web/20220912051936/https://sites.google.com/site/neilsequencer/

### Features 
Updated to python 3 and gtk 3 with partial lv2, vst2 and vst3 support. 

### Notes 
Calf plugin guis do not work - they are gtk2 and incompatible with gtk3 - can't fix. 

DPF plugins guis blankscreen, should fix.

It's linux only atm.

The build is slow and messy, fixing it is in the top ten todo. Proper sidechain support, better automation, CV connections, are nearer the top.  

### Installation

```git clone --recursive https://github.com/catesq/neil```

Copy vst2 sdk to libneil/src/plugins/vstadapter/VST_SDK_2.4

```
scons configure DEBUG=false DESTDIR="/home/me/orwherever/apps/neil.v.0.4" PREFIX=""
scons
scons install
```

Run /home/me/orwherever/apps/neil.v.0.4/bin/neil
