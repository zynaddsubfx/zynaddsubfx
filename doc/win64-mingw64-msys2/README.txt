How to compile ZynAddSubFX on Windows MinGW64 MSYS2:

Tested on Windows 10 64-bit at commit 4b95ad25aebe87c79edfcfde86bd03ac46031d48 on 2016-10-02

This is an unofficial way to get ZynAddSubFX running as a 64-bit VST plugin 
on Windows. Following this method will modify the Zyn source code in ways 
which may break Linux compatibility. Or not, I didn't test it on Linux 
after making the changes.

The VST plugin doesn't have a UI, but we can build the zynaddsubfx-ext-gui,
which can be used to connect to your running Zyn VST instances, providing 
that familiar Zyn UI. It works quite well, although it would be better if 
the VST plugins could just open their own UI instead.


Download and install MSYS2 64-bit ( https://msys2.github.io )

Open an MSYS2 shell.


Install some dependencies with pacman:

pacman -S --force mingw-w64-x86_64-toolchain git make gcc mingw-w64-x86_64-fftw diffutils mingw-w64-i686-toolchain mingw-w64-x86_64-fltk


Make a folder to hold your downloads:

mkdir -p ~/Downloads


Close your MSYS2 shell and open a MinGW64 MSYS2 shell.

cd ~/Downloads

Enable multithreaded compiling:

export MAKE="make -j"

Install some source dependencies:

git clone https://github.com/sharpee/mxml  
cd mxml  
./configure --build=x86_64-w64-mingw32 --host=x86_64-w64-mingw32 --prefix=/mingw64 CFLAGS=-D__CRT__NO_INLINE  
make -D__CRT__NO_INLINE  
make install  

cd ~/Downloads  
git clone https://github.com/guysherman/MINGW-packages.git  
cd MINGW-packages  
git checkout guypkgs  
cd mingw-w64-liblo && makepkg-mingw --install && cd ..  


Add the MinGW and MSYS2 bin dirs to your Windows PATH:

Open System Properties, click Environment Variables,
Then edit the system Path variable. Add these two paths in this order:

C:\msys64\mingw64\bin  
C:\msys64\usr\bin  


Clone the Zyn git repo:

cd ~/Downloads  
git clone https://github.com/zynaddsubfx/zynaddsubfx.git  
cd zynaddsubfx  
git submodule update --init --recursive  


Fix some of Zyn's CMake build scripts:

sed -i "s/string(STRIP \${FLTK_LDFLAGS} FLTK_LIBRARIES)/string(STRIP \"\${FLTK_LDFLAGS}\" FLTK_LIBRARIES)/g" src/CMakeLists.txt  
sed -i "s/target_link_libraries(zynaddsubfx_gui \${FLTK_LIBRARIES})/target_link_libraries(zynaddsubfx_gui \${FLTK_BASE_LIBRARY})/g" src/UI/CMakeLists.txt  
sed -i "s/install(TARGETS ZynAddSubFX_vst LIBRARY/install(TARGETS ZynAddSubFX_vst RUNTIME/g" src/Plugin/ZynAddSubFX/CMakeLists.txt  
cat src/Plugin/ZynAddSubFX/CMakeLists.txt | tr '\n' '\r' | sed -e "s/elseif(FltkGui)\r\r# UI Enabled using FLTK: external only\radd_library(ZynAddSubFX_lv2 SHARED\r    \${CMAKE_SOURCE_DIR}\/src\/globals.cpp\r    \${CMAKE_SOURCE_DIR}\/src\/UI\/ConnectionDummy.cpp\r    \${CMAKE_SOURCE_DIR}\/DPF\/distrho\/DistrhoPluginMain.cpp\r    ZynAddSubFX.cpp)/elseif(FltkGui)\r\r# UI Enabled using FLTK: external only\r#\[\[\radd_library(ZynAddSubFX_lv2 SHARED\r    \${CMAKE_SOURCE_DIR}\/src\/globals.cpp\r    \${CMAKE_SOURCE_DIR}\/src\/UI\/ConnectionDummy.cpp\r    \${CMAKE_SOURCE_DIR}\/DPF\/distrho\/DistrhoPluginMain.cpp\r    ZynAddSubFX.cpp)\r\]\]\r/g" | tr '\r' '\n' > src/Plugin/ZynAddSubFX/CMakeLists.txt  
cat src/Plugin/ZynAddSubFX/CMakeLists.txt | tr '\n' '\r' | sed -e "s/set_target_properties(ZynAddSubFX_lv2 PROPERTIES COMPILE_DEFINITIONS \"DISTRHO_PLUGIN_TARGET_LV2\")\rset_target_properties(ZynAddSubFX_lv2 PROPERTIES LIBRARY_OUTPUT_DIRECTORY \"lv2\")\rset_target_properties(ZynAddSubFX_lv2 PROPERTIES OUTPUT_NAME \"ZynAddSubFX\")\rset_target_properties(ZynAddSubFX_lv2 PROPERTIES PREFIX \"\")/#\[\[\rset_target_properties(ZynAddSubFX_lv2 PROPERTIES COMPILE_DEFINITIONS \"DISTRHO_PLUGIN_TARGET_LV2\")\rset_target_properties(ZynAddSubFX_lv2 PROPERTIES LIBRARY_OUTPUT_DIRECTORY \"lv2\")\rset_target_properties(ZynAddSubFX_lv2 PROPERTIES OUTPUT_NAME \"ZynAddSubFX\")\rset_target_properties(ZynAddSubFX_lv2 PROPERTIES PREFIX \"\")\r\]\]/g" | tr '\r' '\n' > src/Plugin/ZynAddSubFX/CMakeLists.txt  
cat src/Plugin/ZynAddSubFX/CMakeLists.txt | tr '\n' '\r' | sed -e "s/target_link_libraries(ZynAddSubFX_lv2 zynaddsubfx_core \${OS_LIBRARIES} \${ZYN_LIBLO_LIBRARIES}\r    \${PLATFORM_LIBRARIES})/#\[\[\rtarget_link_libraries(ZynAddSubFX_lv2 zynaddsubfx_core \${OS_LIBRARIES} \${ZYN_LIBLO_LIBRARIES}\r    \${PLATFORM_LIBRARIES})\r\]\]/g" | tr '\r' '\n' > src/Plugin/ZynAddSubFX/CMakeLists.txt  
sed -i "s/install(TARGETS ZynAddSubFX_lv2 LIBRARY DESTINATION \${PluginLibDir}\/lv2\/ZynAddSubFX.lv2\/)/#install(TARGETS ZynAddSubFX_lv2 LIBRARY DESTINATION \${PluginLibDir}\/lv2\/ZynAddSubFX.lv2\/)/g" src/Plugin/ZynAddSubFX/CMakeLists.txt  
cat src/Plugin/ZynAddSubFX/CMakeLists.txt | tr '\n' '\r' | sed -e "s/add_library(ZynAddSubFX_lv2_ui SHARED\r    \${CMAKE_SOURCE_DIR}\/DPF\/distrho\/DistrhoUIMain.cpp\r    ZynAddSubFX-UI.cpp)/#\[\[\radd_library(ZynAddSubFX_lv2_ui SHARED\r    \${CMAKE_SOURCE_DIR}\/DPF\/distrho\/DistrhoUIMain.cpp\r    ZynAddSubFX-UI.cpp)\r\]\]/g" | tr '\r' '\n' > src/Plugin/ZynAddSubFX/CMakeLists.txt  



Fix some other files which cause the Zyn build to fail on MinGW:

sed -i "s/lstat/stat/g" src/Misc/BankDb.cpp  
sed -i "s/if(ret != -1)/\/\/if(ret != -1)/g" src/Misc/BankDb.cpp  
sed -i "s/time = st.st_mtim.tv_sec;/\/\/time = st.st_mtim.tv_sec;/g" src/Misc/BankDb.cpp  
sed -i "s/printf(\"desired mask =  %x\\\n\", mask);/\/\/printf(\"desired mask =  %x\\\n\", mask);/g" src/Misc/MiddleWare.cpp  
sed -i "s/#include <err.h>/\/\/#include <err.h>/g" src/main.cpp  
cat src/UI/MasterUI.fl | tr '\n' '\r' | sed -e "s/decl {\\\#include <stdlib.h>} {public local\r}/decl {\\\#undef WIN32\r\\\#include <FL\/x.H>\r\\\#define WIN32} {public local\r}\r\rdecl {\\\#include <stdlib.h>} {public local\r}\r/g" | tr '\r' '\n' > src/UI/MasterUI.fl  
sed -i "s/index/strchr/g" src/UI/Fl_Osc_Tree.H  


Make a build folder:

mkdir -p build  
cd build  


Download and install CMake 64-bit ( https://cmake.org/download/ )

During the install, choose "Add CMake to the system PATH for all users",
and "Create CMake Desktop Icon".

Open CMake-gui, set the source code location to:

C:\msys64\home\<your username>\Downloads\zynaddsubfx

Set the build location to:

C:\msys64\home\<your username>\Downloads\zynaddsubfx\build

Click Configure, then choose the MinGW Makefiles generator, click Finish.

It will give an error about sh.exe being in your path. That's okay,
Click Configure again. It should succeed and say "Configuring done" at the 
bottom.

Uncheck OssEnable and change GuiModule to "fltk" if it isn't set on that.

Click Configure. If it fails, fix any errors and then repeat until it 
succeeds.

Click Generate.


Compile and install ZynAddSubFX:

MSYSTEM=''  
mingw32-make  


It will error out with:
C:/msys64-zyn/mingw64/bin/../lib/gcc/x86_64-w64-mingw32/6.1.0/../../../../x86_64-w64-mingw32/bin/ld.exe: cannot find -llo

So we will patch some CMake output files (there must be a better way, but I
couldn't find where all the -llo were coming from):

sed -i "s/-llo/-l:liblo.dll.a/g" src/Plugin/ZynAddSubFX/CMakeFiles/ZynAddSubFX_lv2.dir/linklibs.rsp  
sed -i "s/-llo/-l:liblo.dll.a/g" src/Plugin/ZynAddSubFX/CMakeFiles/ZynAddSubFX_vst.dir/linklibs.rsp  
sed -i "s/-llo/-l:liblo.dll.a/g" src/CMakeFiles/zynaddsubfx.dir/linklibs.rsp  
sed -i "s/-llo/-l:liblo.dll.a/g" src/UI/CMakeFiles/zynaddsubfx-ext-gui.dir/linklibs.rsp  

Add a missing folder and compile again:

mkdir -p src/Plugin/ZynAddSubFX/lv2  
mingw32-make  

If the build failed, fix any errors until it succeeds (and please tell me
about them), and then copy your shiny new Zyn VST plugin dll into the spot 
where your DAW wants it. For Ardour:

copy C:\msys64\home\<your username>\Downloads\zynaddsubfx\build\src\Plugin\ZynAddSubFX\ZynAddSubFX.dll  
to  
C:\Users\<your username>\Documents\Plugins\VST  


Make a symlink to the instruments that come with Zyn, so it can find them:

cd ~  
ln -s ~/Downloads/zynaddsubfx/instruments/banks  


If it all worked, OMG, You can use Zyn on Windows now! But how to use it?:

I've only tried it in Ardour, so I can't speak for other DAWs, but to use
it in Ardour, you need to launch Ardour from a terminal, make a new session,
add a MIDI track, then open Preferences.

Go to Plugins -> VST, check "Scan for [new] VST Plugins on Application Start"
and make sure Verbose Plugin Scan is checked.

Now click "Scan for Plugins", then go back to the Editor or Mixer view and
you should be able to add your new ZynAddSubFX VST plugin to the MIDI track.

Once added, you should be able to play the default sine wave synth and hear
the audio from it. But you'll probably notice there's no UI. We need the UI!


Enter zynaddsubfx-ext-gui. This is a familiar FLTK frontend to Zyn which 
can connect to a Zyn VST instance. To launch it, do this in the MinGW64
MSYS2 shell:

cd ~/Downloads/zynaddsubfx/build/src/UI

Now take a look at the terminal that you're running your DAW in, and look
for some output that looks like "lo server running on <some number>".

You will see two lines like that which are from when you added the Zyn VST
plugin to your MIDI track. We need the number from the second line.

Now launch the ext-gui pointing at your VST instance:

./zynaddsubfx-ext-gui.exe osc.udp://localhost:<the second number>

The GUI should open, and if you got the number right, some of the knobs
in the GUI should be turned up, and then you'll be able to control the VST
with the GUI, and also the GUI with the VST. 

If the number was wrong, all the knobs in the GUI will be turned down, and
everything will be set to empty or disabled values. In that case, check the
terminal output again, and try a different number.

In Ardour, sometimes the line with the correct number won't show until the 
next time you save your session after adding a VST instance of Zyn.

You can totally use as many Zyn VST instances as you want and attach GUIs
to each of them. The order of the numbers printed in the terminal should 
reflect either the order the plugins were added in, or maybe the order of 
the tracks they are in, remember though that the first line with the number
which shows in the terminal is not valid, so connect your GUIs starting 
from the second number onwards.


Yay, Zyn 64-bit VST in Windows! That was hard! Let me know how it goes, and 
if this process breaks in the future, I'll happily accept pull requests to
fix it.

~Jeremy Carter <Jeremy@JeremyCarter.ca> 2016