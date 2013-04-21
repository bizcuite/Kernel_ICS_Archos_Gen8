#ifndef __OMAP3_OPP_H_
#define __OMAP3_OPP_H_

#include <mach/omap-pm.h>


static struct omap_opp omap3630_mpu_rate_table[] = {
	{0, 0, 0},
	/*OPP1 (OPP50) - 0.93mV*/
	{S300M, VDD1_OPP1, 0x20, 0x0, 0x0, 0x0},
	/*OPP2 (OPP100) - 1.1V*/
	{S600M, VDD1_OPP2, 0x2D, 0x0, 0x0, 0x0},
	/*OPP3 (OPP130) - 1.26V*/
	{S800M, VDD1_OPP3, 0x38, 0x0, 0x0, 0x0},
	/*OPP4 (OPP-1G) - 1.35V*/
	{S1000M, VDD1_OPP4, 0x38, 0x0, 0x0, 0x0},
	/*OPP5 (OPP-1.3G) - 1.38V*/
	{S1100M, VDD1_OPP5, 0x3E, 0x0, 0x0, 0x0},

};

static struct omap_opp omap3630_l3_rate_table[] = {
	{0, 0, 0},
	/*OPP1 (OPP50) - 0.93V*/
	{S100M, VDD2_OPP1, 0x1E, 0x1b, 0x1b, 0x1b},
	/*OPP2 (OPP100) - 1.1375V*/
	{S200M, VDD2_OPP2, 0x2D, 0x0, 0x0, 0x0},
};

static struct omap_opp omap3630_dsp_rate_table[] = {
	{0, 0, 0},
	/*OPP1 (OPP50) - 0.93V*/
	{S260M, VDD1_OPP1, 0x20, 0x0, 0x0, 0x0},
	/*OPP2 (OPP100) - 1.1V*/
	{S520M, VDD1_OPP2, 0x2D, 0x0, 0x0, 0x0},
	/*OPP3 (OPP130) - 1.26V*/
	{S660M, VDD1_OPP3, 0x38, 0x0, 0x0, 0x0},
	/*OPP4 (OPP-1G) - 1.35V*/
	{S800M, VDD1_OPP4, 0x3C, 0x0, 0x0, 0x0},
	/*OPP5 (OPP-1.3G) - 1.38V*/
	{S800M, VDD1_OPP5, 0x3E, 0x0, 0x0, 0x0},
};






#endif
