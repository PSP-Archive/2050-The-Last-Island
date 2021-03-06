/*
Copyright (C) 2007 Peter Mackay and Chris Swindle.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#define CONSOLE_DEBUG 0
#include <stdexcept>
#include <vector>

#include <pspdisplay.h>
#include <pspkernel.h>
#include <psppower.h>
#include <psprtc.h>

extern "C"
{
#include "../quakedef.h"
}

#include "battery.hpp"
#include "system.hpp"


// Running a dedicated server?
qboolean isDedicated = qfalse;

#ifdef PROFILE
extern "C" void gprof_cleanup(void);
#endif


namespace quake
{
	namespace main
	{
#ifdef PSP_SOFTWARE_VIDEO
		// Clock speeds.
		extern const int		cpuClockSpeed	= scePowerGetCpuClockFrequencyInt();
		extern const int		ramClockSpeed	= cpuClockSpeed;
		extern const int		busClockSpeed	= scePowerGetBusClockFrequencyInt();
#endif

		// Should the main loop stop running?
		static volatile bool	quit			= false;

		// Is the PSP in suspend mode?
		static volatile bool	suspended		= false;

		static int exitCallback(int arg1, int arg2, void* common)
		{
			// Signal the main thread to stop.
			quit = true;
			return 0;
		}

		static int powerCallback(int unknown, int powerInfo, void* common)
		{
			if (powerInfo & PSP_POWER_CB_POWER_SWITCH || powerInfo & PSP_POWER_CB_SUSPENDING)
			{
				suspended = true;
			}
			else if (powerInfo & PSP_POWER_CB_RESUMING)
			{
			}
			else if (powerInfo & PSP_POWER_CB_RESUME_COMPLETE)
			{
				suspended = false;
			}

			return 0;
		}

		static int callback_thread(SceSize args, void *argp)
		{
			// Register the exit callback.
			const SceUID exitCallbackID = sceKernelCreateCallback("exitCallback", exitCallback, NULL);
			sceKernelRegisterExitCallback(exitCallbackID);

			// Register the power callback.
			const SceUID powerCallbackID = sceKernelCreateCallback("powerCallback", powerCallback, NULL);
			scePowerRegisterCallback(0, powerCallbackID);

			// Sleep and handle callbacks.
			sceKernelSleepThreadCB();
			return 0;
		}

		static int setUpCallbackThread(void)
		{
			const int thid = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, PSP_THREAD_ATTR_USER, 0);
			if (thid >= 0)
				sceKernelStartThread(thid, 0, 0);
			return thid;
		}

		static void disableFloatingPointExceptions()
		{
#ifndef _WIN32
			asm(
				".set push\n"
				".set noreorder\n"
				"cfc1    $2, $31\n"		// Get the FP Status Register.
				"lui     $8, 0x80\n"	// (?)
				"and     $8, $2, $8\n"	// Mask off all bits except for 23 of FCR. (? - where does the 0x80 come in?)
				"ctc1    $8, $31\n"		// Set the FP Status Register.
				".set pop\n"
				:						// No inputs.
				:						// No outputs.
				: "$2", "$8"			// Clobbered registers.
				);
#endif
		}
	}
}

using namespace quake;
using namespace quake::main;

int main(int argc, char *argv[])
{
	// Catch exceptions from here.
	try
	{
		// Set up the callback thread.
		setUpCallbackThread();


		scePowerSetClockFrequency(333, 333, 166);

		// Disable floating point exceptions.
		// If this isn't done, Quake crashes from (presumably) divide by zero
		// operations.
		disableFloatingPointExceptions();

		// Initialise the Common module.
#if CONSOLE_DEBUG
		char* args[] =
		{
			argv[0], "-condebug"
		};
		COM_InitArgv(sizeof(args) / sizeof(args[0]), args);
#else
		COM_InitArgv(0, NULL);
#endif

		// Allocate the heap.
		std::vector<unsigned char>	heap(12 * 1024 * 1024, 0);

		// Initialise the Host module.
		quakeparms_t parms;
		memset(&parms, 0, sizeof(parms));
		parms.argc		= com_argc;
		parms.argv		= com_argv;
		parms.basedir	= ".";
		parms.memsize	= heap.size();
		parms.membase	= &heap.at(0);
		Host_Init(&parms);

		// Precalculate the tick rate.
		const float oneOverTickRate = 1.0f / sceRtcGetTickResolution();

		// Record the time that the main loop started.
		u64 lastTicks;
		sceRtcGetCurrentTick(&lastTicks);

		// Enter the main loop.
		while (!quit)
		{
			// Handle suspend & resume.
			if (suspended)
			{
				// Suspend.
				S_ClearBuffer();
				quake::system::suspend();

				// Wait for resume.
				while (suspended && !quit)
				{
					sceDisplayWaitVblankStart();
				}

				// Resume.
				//quake::system::resume();

				// Reset the clock.
				sceRtcGetCurrentTick(&lastTicks);
			}

			// What is the time now?
			u64 ticks;
			sceRtcGetCurrentTick(&ticks);

			// How much time has elapsed?
			const unsigned int	deltaTicks		= ticks - lastTicks;
			const float			deltaSeconds	= deltaTicks * oneOverTickRate;


			// Run the frame.
			Host_Frame(deltaSeconds);

			// Remember the time for next frame.
			lastTicks = ticks;
		}

#if PROFILE
		gprof_cleanup();
#endif

	}

	// Quit.
	Sys_Quit();
	return 0;
}


							
