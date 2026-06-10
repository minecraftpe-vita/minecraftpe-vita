# Minecraft PE for PlayStation Vita & Symbian^3

This repository contains a port of Minecraft Pocket Edition 0.6.1 to the following systems:
 - PlayStation Vita
 - PlayStation TV
 - Symbian^3

# Build instructions
to build it, make sure you have the latest VitaSDK from vitasdk.org

```
cmake -DPLATFORM=vita -DPUBLISH=on -B build -S handheld/project
cd build
make -j$(nproc)
```

If you want to build the demo; you can add the ``-DDEMO=on`` flag to the cmake line.

for the symbian version; see ``/handheld/project/symbian/readme.textile`` file;

# Compatbility
the games should be network-compatible and binarty-compatible with eachother, 
meaning you can join a world created on MCPE 0.6.1 IPA, or APK; using your PSVita, 

and they should also be able to join you- world files created on one;
can be transferred to the vita version (and vice-versa)

# Credits
   - Olebeck, (graphics, sound, networking)
   - Li, (controls, fixing options, recreating menus)
   - Koutsie, (original options menu, from MCPE Desire)
   - PVR_PSP2, developers (GrapheneCt) 
   - Julia, Symbian^3 port
   
# Other information:
 - VPK is just install-and-play, no additional setup should be required.
 - due to the vita having very slow IO speeds, world generation can take awhile on the first time
 - savedata (worlds, options , etc) are stored in ux0:/data/minecraftpe/
 - as this is a source-port and cannot be easily adapted to any other version

