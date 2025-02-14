/* SPDX-License-Identifier: (GPL-2.0 OR MIT) */
/*
 * Copyright (C) 2024 TO BE DETERMINED
 */

#define CLK_CMNPLL_VCO		0
#define CLK_CMNPLL_POSTDIV	1

#define CLK_DDRPLL_POSTDIV	2
// useless
// #define CLK_DDRPLL_4PHASE	3

// note: FOUT0 is useless, FOUT1~3=pll_pcie_clk[4-6]

#define CLK_PCIEPLL_VCO		3
#define CLK_PCIEPLL_FOUT0	4
#define CLK_PCIEPLL_FOUT1	5
#define CLK_PCIEPLL_FOUT2	6
#define CLK_ETH_REF_P		CLK_PCIEPLL_FOUT2
#define CLK_PCIEPLL_FOUT3	7

#define CLK_CPU			8
#define CLK_PIC			9
#define CLK_AXI			10
#define CLK_AHB			11
#define CLK_APB			12
#define CLK_UART		13
#define CLK_IRAM		14
#define CLK_NPU			15
#define CLK_DDRPHY_REF		16
#define CLK_DDR_BYPASS		17
#define CLK_ETHTSU		18
#define CLK_GMAC_BYP_REF	19

#define CLK_USB			20
#define CLK_USBPHY		21
#define CLK_SERDES_CSR		22
#define CLK_CRYPT_CSR		23
#define CLK_CRYPT_APP		24
#define CLK_IROM		25

// mux only
#define CLK_BOOT		26

// div only
#define CLK_PVT			27
#define CLK_PLL_TEST		28

// gate_only
#define CLK_PCIE_REFN		29
#define CLK_PCIE_REFP		30

#define CLK_MAX			31