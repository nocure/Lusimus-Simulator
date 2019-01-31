# Lusimus
A fork of SkyEye 1.2.8 rc1. Lusimus is a total system simulator for Windows 7/8/10

look!!! It's real!!!

<img src='images/shot_in_action_1.png' width=960>

There are pre-built binaries in **bin** directory, you can run **bin\Lusimus runs ELF.bat** to load a ARM demo program into Lusimus.  
The demo program **Unarmed** is here at [nocure / unarmed](https://github.com/nocure/unarmed) too.

Currently stable simulating an s3c2440 demo board only, the one that I am sure of to work as expected. As you can see in the image above, it simulates an LCD, an UART console via TELNET protocol. It is equipped with a GDB server, capable of source-level single-step debugging with breakpoint, variable and memory access in Eclipse CDT environment with GNU MCU (Previously GNU ARM) Eclipse plugin. 

This ancient work has been lying in hard disk for years. Since it was taken from open source community, I might as well I put it up for anyone needed.

SkyEye 1.2.8 rc2 is the only version I can make it work, don't bother with later versions, the simulating routing are strangely commented out in later versions. It seems that they were made intentionally not to work, and there were changes in about half of source files. Since my version worked so I did'nt care about newer ones. The SkyEye project on SourceForge is deserted anyway, it hasn't been updated for years. 

The original SkyEye can be obtained here <a>https://sourceforge.net/projects/skyeye/</a>.  
Said on the project page, It can simulate series ARM, Coldfire, Mips, Powerpc, Sparc, x86, TI DSP and Blackfin DSP Processor. I believe the creator of SkyEye took a bit of Armulator, a bit of this, a bit of that, and more bits of something from somewhere, then assembled them together like...like... Frankenstein. It seems all there alright, A lot of unfinished, unworkable codes in every components in every code file that makes it impossible to do any of them at all. The only configuration that I was able to bring up is s3c2410, but still it randomly crashed after runing few instructions every time I try to launch anything.

There was no instruction about how to compile it and how to setup the environment for the job, so when I tried to compile it for the first time, results were thousands of errors and warnings. I fixed them one by one, every single one of them, took me 7 days and 7 nights to get it done, literally!!! Unused and set but unused variables were easy parts, just went through each source file(about 1 thousand) to take them out, more were ambiguous expressions, oblivious wrong mathematic calculations, confusing condition expresions, missing parentheses, symbols redefined with different values...(the list is quite long, so I'll just stop here).

Linking was another big challenge too, thousands of conflicts and undefined calls with system libs and its own ones. That took me another sleepless 7 nights and days, finally, I was reborn. You can be reborn too, just take original source package and try it out.

You know, <s>I thought I have seen Linux Kernel source codes,</s> I thought I have seen enough in 30 years as a professional, Life never stops to scare me from time to time. I failed to imagine how and why a <s>good</s> average programmer can endure such huge amount of compiler warnings, and yet still consider the state is a release to be. I have come through the long and winding road, so you don't have to be in hell and burned for 14 days (at least).

Enough complains, now get to the point.

## Environment Setup

 *I like to work in a GUI environment, it's more efficient. Since codes I provided are projects in Eclipse, if you are a CLI enthusiast, Well...Hmmm...TBD...* 

 *I put them together some time ago, Newer versions of following components should be applicable if there are any.*

 * Windows 7/8/10 (I am on 10)
 

 * Java Runtime Environment (Required by Eclipse, I use version 8u172)
 
 
 * GNU MCU Eclipse IDE for C/C++ Developers Oxygen.3a 20180419 [download page](https://github.com/gnu-mcu-eclipse/org.eclipse.epp.packages/releases/tag/v4.3.3.20180419-o3a "Download GNU MCU Eclipse")  
	The Eclipse Oxygen.3a that come with GNU MCU pre-installed.  
	File name: **20180419-1818-gnumcueclipse-4.3.3-oxygen-3a-win32.win32.x86(_64).zip**
	
	
 * GNU Arm Embedded Toolchain [download page](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads/ "Download GNU Arm Embedded Toolchain")  (For ARM).  
 	File name: **gcc-arm-none-eabi-7-2018-q2-update-win32-sha2.exe**
 	
 	
 * MinGW [download page](https://sourceforge.net/projects/mingw/files/ "download MinGW") (For PC).  
 	Just hit the green download button, get the file mingw-get-setup.exe, this is a package installer, not the actual MinGW. For what packages to install explain later.
 	
 	
 * EGit, the Git integration for Eclipse 4.6.0 (Optional).  
 	Install it from Eclipse Marketplace.  
 	You can use any other git client (i.e. TortoiseGit) will do the same job.
 	 
 	
##### MinGW packages needed
Basic Setup:  
 * mingw-developer-toolkit
 * mingw32-base
 * mingw32-gcc-g++
 * msys-base
 
All Packages:  
 * MinGW -> MinGW Base System -> mingw32-binutils-dev

And the missing lib "iberty", download it manually:
 * libiberty [download page](https://sourceforge.net/projects/mingw/files/MinGW/Base/binutils/binutils-2.28/ "Download libiberty") on SourceForge.  
	File name: **libiberty-2.28-1-mingw32-dev.tar.xz**  
	Extract content into mingw root directory (e.g. C:\MinGW).


***

When the environment is properly set up, after geting complete project files from github, you can create a workspace in the projects root directory within Eclipse, import each projects in the workspace, and build all lib projects, build main project last, then you are done.
 
Well, That's all for now.
 