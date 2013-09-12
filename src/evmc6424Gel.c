/* ------------------------------------------------------------------------ *
 *                                                                          *
 *  evmc6424.gel                                                            *
 *  Version 1.50                                                            *
 *                                                                          *
 *  This GEL file is designed to be used in conjunction with                *
 *  CCStudio 3.2+ and the C6424 based EVM.                                  *
 *                                                                          *
 * ------------------------------------------------------------------------ */

 // Version History
 //
 // 1.50 Change DDR2 Timings, fixes DDR2 to 162 MHz
 //

#define hotmenu void 

hotmenu Setup_Cache( );
hotmenu Setup_Pin_Mux( );
hotmenu Setup_Psc_All_On( );
hotmenu Setup_PLL1_594_MHz_OscIn( );
hotmenu Setup_PLL2_DDR_162_MHz_OscIn( );
hotmenu Setup_DDR_162_MHz( );
hotmenu Setup_EMIF_CS2_NorFlash_16Bit( );
hotmenu Setup_EMIF_CS3_FPGA_16Bit( );
hotmenu Setup_EMIF_CS4_FPGA_16Bit( );

void GEL_TextOut(char *str)
{
  printf(str);
}
 

/* ------------------------------------------------------------------------ *
 *                                                                          *
 *  OnTargetConnect( )                                                      *
 *      Setup PinMux, Power, PLLs, DDR, & EMIF                              *
 *                                                                          *
 * ------------------------------------------------------------------------ */
initDSP( )
{
    GEL_TextOut( "\nEVM6424 Startup Sequence\n\n" );

    Setup_Cache( );                     // Setup L1P/L1D Cache
    Setup_Pin_Mux( );                   // Setup Pin Mux
    Setup_Psc_All_On( );                // Setup All Power Domains

    //Setup_PLL1_594_MHz_OscIn( );        // Setup Pll1 [DSP @ 594 MHz][1.20V]
    //Setup_PLL2_DDR_162_MHz_OscIn( );    // Setup Pll2 [DDR @ 162 MHz]
    //Setup_DDR_162_MHz( );               // Setup DDR2 [162 MHz]

  //Reset_EMIF_16Bit_Bus( );            // Reset Async-EMIF [16-bit bus]
    Setup_EMIF_CS2_NorFlash_16Bit( );   // Setup NOR Flash
  //Setup_EMIF_CS2_SRAM_16Bit( );       // Setup SRAM
  //Setup_EMIF_CS2_NandFlash_8Bit( );   // Setup NAND Flash

    Setup_EMIF_CS3_FPGA_16Bit( );
	Setup_EMIF_CS4_FPGA_16Bit( );
    CheckPCI( );
    Disable_EDMA( );                // Disable EDMA
        
//    Setup_DDR_162_MHz( );           // Setup DDR
    
    GEL_TextOut( "\nStartup Complete.\n\n" );
}

/* ------------------------------------------------------------------------ *
 *                                                                          *
 *  Setup_Cache( )                                                          *
 *      Invalidate old cache and setup cache for operation                  *
 *                                                                          *
 * ------------------------------------------------------------------------ */
hotmenu
Setup_Cache( )
{
    int l1p, l1d, l2;

    GEL_TextOut( "Setup Cache " );
    #define CACHE_L2CFG         *( unsigned int* )( 0x01840000 )
    #define CACHE_L2INV         *( unsigned int* )( 0x01845008 )
    #define CACHE_L1PCFG        *( unsigned int* )( 0x01840020 )
    #define CACHE_L1PINV        *( unsigned int* )( 0x01845028 )
    #define CACHE_L1DCFG        *( unsigned int* )( 0x01840040 )
    #define CACHE_L1DINV        *( unsigned int* )( 0x01845048 )

    CACHE_L1PINV = 1;           // L1P invalidated
    //CACHE_L1PCFG = 7;           // L1P on, MAX size
    CACHE_L1DINV = 1;           // L1D invalidated
    //CACHE_L1DCFG = 7;           // L1D on, MAX size
    CACHE_L2INV  = 1;           // L2 invalidated
    CACHE_L2CFG  = 0;           // L2 off, use as RAM

    l1p = CACHE_L1PCFG;
    if ( l1p == 0 )
        GEL_TextOut( "(L1P = 0K) + " );
    if ( l1p == 1 )
        GEL_TextOut( "(L1P = 4K) + " );
    if ( l1p == 2 )
        GEL_TextOut( "(L1P = 8K) + " );
    if ( l1p == 3 )GEL_TextOut( "(L1P = 16K) + " );
    if ( l1p >= 4 )
        GEL_TextOut( "(L1P = 32K) + " );

    l1d = CACHE_L1DCFG;
    if ( l1d == 0 )
        GEL_TextOut( "(L1D = 0K) + " );
    if ( l1d == 1 )
        GEL_TextOut( "(L1D = 4K) + " );
    if ( l1d == 2 )
        GEL_TextOut( "(L1D = 8K) + " );
    if ( l1d == 3 )
        GEL_TextOut( "(L1D = 16K) + " );
    if ( l1d >= 4 )
        GEL_TextOut( "(L1D = 32K) + " );

    l2 = CACHE_L2CFG;
    if ( l2 == 0 )
        GEL_TextOut( "(L2 = ALL SRAM)... " );
    else if ( l2 == 1 )
        GEL_TextOut( "(L2 = 31/32 SRAM)... " );
    else if ( l2 == 2 )
        GEL_TextOut( "(L2 = 15/16 SRAM)... " );
    else if ( l2 == 3 )
        GEL_TextOut( "(L2 = 7/8 SRAM)... " );
    else if ( l2 == 7 )
        GEL_TextOut( "(L2 = 3/4 SRAM)... " );

    GEL_TextOut( "[Done]\n" );
}

/* ------------------------------------------------------------------------ *
 *                                                                          *
 *  Disable_EDMA( )                                                         *
 *      Disabe EDMA events and interrupts, clear any pending events         *
 *                                                                          *
 * ------------------------------------------------------------------------ */
Disable_EDMA( )
{
    #define EDMA_3CC_IECRH          *( int* )( 0x01C0105C )
    #define EDMA_3CC_EECRH          *( int* )( 0x01C0102C )
    #define EDMA_3CC_ICRH           *( int* )( 0x01C01074 )
    #define EDMA_3CC_ECRH           *( int* )( 0x01C0100C )

    #define EDMA_3CC_IECR           *( int* )( 0x01C01058 )
    #define EDMA_3CC_EECR           *( int* )( 0x01C01028 )
    #define EDMA_3CC_ICR            *( int* )( 0x01C01070 )
    #define EDMA_3CC_ECR            *( int* )( 0x01C01008 )

    GEL_TextOut( "Disable EDMA events\n" );
    EDMA_3CC_IECRH  = 0xFFFFFFFF;   // IERH ( disable high interrupts )
    EDMA_3CC_EECRH  = 0xFFFFFFFF;   // EERH ( disable high events )
    EDMA_3CC_ICRH   = 0xFFFFFFFF;   // ICRH ( clear high interrupts )
    EDMA_3CC_ECRH   = 0xFFFFFFFF;   // ICRH ( clear high events )

    EDMA_3CC_IECR   = 0xFFFFFFFF;   // IER  ( disable low interrupts )
    EDMA_3CC_EECR   = 0xFFFFFFFF;   // EER  ( disable low events )
    EDMA_3CC_ICR    = 0xFFFFFFFF;   // ICR  ( clear low interrupts )
    EDMA_3CC_ECR    = 0xFFFFFFFF;   // ICRH ( clear low events )
}

/* ------------------------------------------------------------------------ *
 *                                                                          *
 *  Setup_Pin_Mux( )                                                        *
 *      Configure Pin Multiplexing for the C6424 EVM in normal operation.   *
 *                                                                          *
 * ------------------------------------------------------------------------ */
hotmenu
Setup_Pin_Mux( )
{
    GEL_TextOut( "Setup PinMux... " );
    #define BOOTCFG             *( unsigned int* )( 0x01c40014 )
    #define PINMUX0             *( unsigned int* )( 0x01c40000 )
    #define PINMUX1             *( unsigned int* )( 0x01c40004 )
    #define VDD3P3V_PWDN        *( unsigned int* )( 0x01c40048 )

    /*
     *  PinMux settings for:
     *      [Normal Operation]
     *          8-bit EMIFA (full address mode)
     *          CS3-CS5 disabled
     *          McBSP0, McBSP1 enabled
     *          UART1 enabled
     *          Timer 0 enabled
     *          PWM1 disabled, I2C_INT on GIO004
     *          VLYNQ enabled
     *          MII/MDIO enabled
     *          PCI disabled
     */
    if ( ( BOOTCFG & 0x00020000 ) == 0 )
    {
        GEL_TextOut( "(Normal operation)... " );
        PINMUX0 = 0x00148002;
              /*| ( 0 << 30 )   // CI10SEL   - No CI[1:0]
                | ( 0 << 28 )   // CI32SEL   - No CI[3:2]
                | ( 0 << 26 )   // CI54SEL   - No CI[5:4]
                | ( 0 << 25 )   // CI76SEL   - No CI[7:6]
                | ( 0 << 24 )   // CFLDSEL   - No C_FIELD
                | ( 0 << 23 )   // CWENSEL   - No C_WEN
                | ( 0 << 22 )   // HDVSEL    - No CCDC HD and VD
                | ( 1 << 20 )   // CCDCSEL   - CCDC PCLK, YI[7:0] enabled
                | ( 4 << 16 )   // AEAW      - EMIFA full address mode
                | ( 1 << 15 )   // VPBECLK   - VPBECLK enabled
                | ( 0 << 12 )   // RGBSEL    - No digital outputs
                | ( 0 << 10 )   // CS3SEL    - LCD_OE/EM_CS3 disabled
                | ( 0 <<  8 )   // CS4SEL    - CS4/VSYNC disabled
                | ( 0 <<  6 )   // CS5SEL    - CS5/HSYNC disabled
                | ( 0 <<  4 )   // VENCSEL   - Video encoder outputs disabled
                | ( 2 <<  0 );  */// AEM     - 16-bit EMIFA
    }

    /*
     *  PINMUX settings for PCI operation
     */
    if ( ( BOOTCFG & 0x00020000 ) != 0 )
    {
        GEL_TextOut( "(PCI operation)... " );
        PINMUX0 = 0x00148000;
              /*| ( 0 << 30 )   // CI10SEL   - No CI[1:0]
                | ( 0 << 28 )   // CI32SEL   - No CI[3:2]
                | ( 0 << 26 )   // CI54SEL   - No CI[5:4]
                | ( 0 << 25 )   // CI76SEL   - No CI[7:6]
                | ( 0 << 24 )   // CFLDSEL   - No C_FIELD
                | ( 0 << 23 )   // CWENSEL   - No C_WEN
                | ( 0 << 22 )   // HDVSEL    - No CCDC HD and VD
                | ( 1 << 20 )   // CCDCSEL   - CCDC PCLK, YI[7:0] enabled
                | ( 4 << 16 )   // AEAW      - EMIFA full address mode
                | ( 1 << 15 )   // VPBECLK   - VPBECLK enabled
                | ( 0 << 12 )   // RGBSEL    - No digital outputs
                | ( 0 << 10 )   // CS3SEL    - LCD_OE/EM_CS3 disabled
                | ( 0 <<  8 )   // CS4SEL    - CS4/VSYNC disabled
                | ( 0 <<  6 )   // CS5SEL    - CS5/HSYNC disabled
                | ( 0 <<  4 )   // VENCSEL   - Video encoder outputs disabled
                | ( 0 <<  0 );*/// AEM       - N/A
    }
    PINMUX0 |=(1<<8)|(1<<10);
    PINMUX1 = 0x01618530;
              /*| ( 1 << 24 )   // SPBK1     - McBSP1 enabled
                | ( 1 << 22 )   // SPBK0     - McBSP0 enabled
                | ( 2 << 20 )   // TIM1BK    - UART1 enabled, Timer1 disabled
                | ( 1 << 16 )   // TIM0BK    - Timer0 enabled
                | ( 2 << 14 )   // CKOBK     - CLKOUT disabled, PWM2 enabled
                | ( 0 << 12 )   // PWM1BK    - PWM1 disabled, GIO84 enabled
                | ( 1 << 10 )   // UR0FCBK   - UART0 HW flow control enabled
                | ( 1 <<  8 )   // UR0DBK    - UART0 data enabled
                | ( 3 <<  4 )   // HOSTBK    - VLYNQ + MII + MDIO Mode
                | ( 0 <<  0 );*/// PCIEN     - PCI disabled

    VDD3P3V_PWDN = 0x00000000;  // Everything on
    GEL_TextOut( "[Done]\n" );
}

/* ------------------------------------------------------------------------ *
 *                                                                          *
 *  CheckPCI( )                                                             *
 *      Do PCI specific configuration.                                      *
 *                                                                          *
 * ------------------------------------------------------------------------ */
CheckPCI( )
{
    #define BOOTCFG             *( unsigned int* )( 0x01c40014 )
    #define PCISLVCNTRL         *( unsigned int* )( 0x01c1a180 )
    #define PCICFGDONE          *( unsigned int* )( 0x01c1a3ac )

    /* Check for PCI boot mode.  If so setup for PCI */
    if ( ( BOOTCFG & 0x00020000 ) != 0 )
    {
        PCICFGDONE |= 1;
        PCISLVCNTRL |= 1;
    }
}

/* ------------------------------------------------------------------------ *
 *                                                                          *
 *  Setup_Psc_All_On( )                                                     *
 *      Enable all PSC modules on ALWAYSON and DSP power dominas.           *
 *                                                                          *
 * ------------------------------------------------------------------------ */
hotmenu
Setup_Psc_All_On( )
{
    int i;
    GEL_TextOut( "Setup Power Modules (All on)... " );

    /*
     *  Enable all non-reserved power modules
     */
    for ( i = 2 ; i <= 9 ; i++ )
        psc_change_state( i, 3 );
    for ( i = 11 ; i <= 15 ; i++ )
        psc_change_state( i, 3 );
    for ( i = 17 ; i <= 20 ; i++ )
        psc_change_state( i, 3 );
    for ( i = 23 ; i <= 29 ; i++ )
        psc_change_state( i, 3 );
    i = 39;
        psc_change_state( i, 3 );

    GEL_TextOut( "[Done]\n" );
}

/* ------------------------------------------------------------------------ *
 *                                                                          *
 *  psc_change_state( id, state )                                           *
 *      id    = Domain #ID                                                  *
 *      state = ( ENABLE, DISABLE, SYNCRESET, RESET )                       *
 *              (   =3  ,   =2   ,    =1    ,   =0  )                       *
 *                                                                          *
 * ------------------------------------------------------------------------ */
psc_change_state( int id, int state )
{
    #define PSC_PTCMD           *( volatile unsigned int* )( 0x01c41120 )
    #define PSC_PTSTAT          *( volatile unsigned int* )( 0x01c41128 )
    volatile unsigned int* mdstat = ( unsigned int* )( 0x01c41800 + ( 4 * id ) );
    volatile unsigned int* mdctl  = ( unsigned int* )( 0x01c41a00 + ( 4 * id ) );

    /*
     *  Step 0 - Ignore request if the state is already set as is
     */
    if ( ( *mdstat & 0x1f ) == state )
        return;

    /*
     *  Step 1 - Wait for PTSTAT.GOSTAT to clear
     */
    while( PSC_PTSTAT & 1 );

    /*
     *  Step 2 - Set MDCTLx.NEXT to new state
     */
    *mdctl &= ~0x1f;
    *mdctl |= state;

    /*
     *  Step 3 - Start power transition ( set PTCMD.GO to 1 )
     */
    PSC_PTCMD = 1;

    /*
     *  Step 4 - Wait for PTSTAT.GOSTAT to clear
     */
    while( PSC_PTSTAT & 1 );

    /*
     *  Step 5 - Verify state changed
     */
    while( ( *mdstat & 0x1f ) != state );
}

_wait2( int delay )
{
    int i;
    for( i = 0 ; i < delay ; i++ ){}
}


/* ------------------------------------------------------------------------ *
 *                                                                          *
 *  setup_pll_1( )                                                          *
 *                                                                          *
 *      clock_source    <- 0: Onchip Oscillator                             *
 *                         1: External Clock                                *
 *                                                                          *
 *      pll_mult        <- 21: 22x Multiplier * 27MHz Clk = 594 MHz         *
 *                                                                          *
 * ------------------------------------------------------------------------ */
setup_pll_1( int clock_source, int pll_mult )
{
    unsigned int* pll_ctl       = ( unsigned int* )( 0x01c40900 );
    unsigned int* pll_pllm      = ( unsigned int* )( 0x01c40910 );
    unsigned int* pll_cmd       = ( unsigned int* )( 0x01c40938 );
    unsigned int* pll_stat      = ( unsigned int* )( 0x01c4093c );
    unsigned int* pll_div1      = ( unsigned int* )( 0x01c40918 );
    unsigned int* pll_div2      = ( unsigned int* )( 0x01c4091c );
    unsigned int* pll_div3      = ( unsigned int* )( 0x01c40920 );
    unsigned int* pll_bpdiv     = ( unsigned int* )( 0x01c4092c );

    int pll1_freq = 27 * ( pll_mult + 1 );
    int div1 = 0;
    int div2 = 2;
    int div3 = 5;
    int bypass_div = 0;
    int power_up_pll = ( *pll_ctl & 0x0002 ) >> 1;

    GEL_TextOut( "Setup PLL1 " );

    /*
     *  Step 0 - Ignore request if the PLL is already set as is
     */
    if ( ( ( *pll_ctl & 0x0100 ) >> 8 ) == clock_source )
    {
        if ( ( *pll_pllm & 0x3f ) == ( pll_mult & 0x3f ) )
        {
            if (   ( ( *pll_div1 & 0x1f ) == div1 )
                || ( ( *pll_div2 & 0x1f ) == div2 )
                || ( ( *pll_div3 & 0x1f ) == div3 ) )
            {
//                GEL_TextOut( "(DSP = %d MHz + ",,,,, pll1_freq / ( div1 + 1 ) );
  //              GEL_TextOut( "SYSCLK2 = %d MHz + ",,,,, pll1_freq / ( div2 + 1 ) );
    //            GEL_TextOut( "SYSCLK3 = %d MHz + ",,,,, pll1_freq / ( div3 + 1 ) );
                if ( clock_source == 0 )
                    GEL_TextOut( "Onchip Oscillator)... " );
                else
                    GEL_TextOut( "External Clock)... " );
                GEL_TextOut( "[Already Set]\n" );
                return;
            }
        }
    }

    /*
     *  Step 1 - Set clock mode
     */
    if ( power_up_pll == 1 )
    {
        GEL_TextOut( "(Powering up PLL)... " );
        if ( clock_source == 0 )
            *pll_ctl &= ~0x0100;    // Onchip Oscillator
        else
            *pll_ctl |= 0x0100;     // External Clock
    }

    /*
     *  Step 2 - Set PLL to bypass
     *         - Wait for PLL to stabilize
     */
    *pll_ctl &= ~0x0021;
    _wait2( 150 );

    /*
     *  Step 3 - Reset PLL
     */
    *pll_ctl &= ~0x0008;

    /*
     *  Step 4 - Disable PLL
     *  Step 5 - Powerup PLL
     *  Step 6 - Enable PLL
     *  Step 7 - Wait for PLL to stabilize
     */
    if ( power_up_pll == 1 )
    {
        *pll_ctl |= 0x0010;         // Disable PLL
        *pll_ctl &= ~0x0002;        // Power up PLL
        *pll_ctl &= ~0x0010;        // Enable PLL
        _wait2( 150 );               // Wait for PLL to stabilize
    }
    else
        *pll_ctl &= ~0x0010;        // Enable PLL

    /*
     *  Step 8 - Load PLL multiplier
     */
    *pll_pllm = pll_mult & 0x3f;

    /*
     *  Step 9 - Load PLL dividers ( must be in a 1/3/6 ratio )
     *           1:DSP, 2:SCR,EMDA, 3:Peripherals
     */
    *pll_bpdiv = 0x8000 | bypass_div; // Bypass divider
    *pll_div1 = 0x8000 | div1;      // Divide-by-1 or Divide-by-2
    *pll_div2 = 0x8000 | div2;      // Divide-by-3 or Divide-by-6
    *pll_div3 = 0x8000 | div3;      // Divide-by-6 or Divide-by-12
    *pll_cmd |= 0x0001;             // Set phase alignment
    while( ( *pll_stat & 1 ) != 0 );// Wait for phase alignment

    /*
     *  Step 10 - Wait for PLL to reset ( 2000 cycles )
     *  Step 11 - Release from reset
     */
    _wait2( 2000 );
    *pll_ctl |= 0x0008;

    /*
     *  Step 12 - Wait for PLL to re-lock ( 2000 cycles )
     *  Step 13 - Switch out of BYPASS mode
     */
    _wait2( 2000 );
    *pll_ctl |= 0x0001;

    pll1_freq = 27 * ( ( *pll_pllm & 0x3f ) + 1 );
    div1 = ( *pll_div1 & 0x1f );
    div2 = ( *pll_div2 & 0x1f );
    div3 = ( *pll_div3 & 0x1f );

//    GEL_TextOut( "(DSP = %d MHz + ",,,,, pll1_freq / ( div1 + 1 ) );
//    GEL_TextOut( "SYSCLK2 = %d MHz + ",,,,, pll1_freq / ( div2 + 1 ) );
//    GEL_TextOut( "SYSCLK3 = %d MHz + ",,,,, pll1_freq / ( div3 + 1 ) );

    if ( clock_source == 0 )
        GEL_TextOut( "Onchip Oscillator)... " );
    else
        GEL_TextOut( "External Clock)... " );

    GEL_TextOut( "[Done]\n" );
}

hotmenu
Setup_PLL1_594_MHz_OscIn( )
{
    setup_pll_1( 0, 21 );               // DSP @ 594 MHz w/ Onchip Oscillator
}

/* ------------------------------------------------------------------------ *
 *                                                                          *
 *  setup_pll_2( )                                                          *
 *                                                                          *
 *      clock_source    <- 0: Onchip Oscillator                             *
 *                         1: External Clock                                *
 *                                                                          *
 *      pll_mult        <- PLL Multiplier                                   *
 *                         23: 24x Multiplier * 27MHz Clk = 648 MHz         *
 *                                                                          *
 *      ddr2_div        <- DDR2 divider ( For PLL2 )                        *
 *                         1: 648 MHz Clk / (2*2)x Divider = 162 MHz        *
 *                                                                          *
 * ------------------------------------------------------------------------ */
setup_pll_2( int clock_source, int pll_mult, int ddr2_div )
{
    unsigned int* pll_ctl       = ( unsigned int* )( 0x01c40d00 );
    unsigned int* pll_pllm      = ( unsigned int* )( 0x01c40d10 );
    unsigned int* pll_cmd       = ( unsigned int* )( 0x01c40d38 );
    unsigned int* pll_stat      = ( unsigned int* )( 0x01c40d3c );
    unsigned int* pll_div1      = ( unsigned int* )( 0x01c40d18 );
    unsigned int* pll_div2      = ( unsigned int* )( 0x01c40d1c );
    unsigned int* pll_bpdiv     = ( unsigned int* )( 0x01c40d2c );

    int pll2_freq = 27 * ( pll_mult + 1 );
    int ddr2_freq = pll2_freq / ( 2 * ( ddr2_div + 1 ) );
    int bypass_div = 1;
    int power_up_pll = ( *pll_ctl & 0x0002 ) >> 1;

    GEL_TextOut( "Setup PLL2 " );

    /*
     *  Step 0 - Ignore request if the PLL is already set as is
     */
    if ( ( ( *pll_ctl & 0x0100 ) >> 8 ) == clock_source )
    {
        if ( ( *pll_pllm & 0x3f ) == ( pll_mult & 0x3f ) )
        {
            if ( ( *pll_div1 & 0x1f ) == ( ddr2_div & 0x1f ) )
            {
         //       GEL_TextOut( "Setup PLL2, DDR2 Phy = %d MHz + ",,,,, ddr2_freq );
                if ( clock_source == 0 )
                    GEL_TextOut( "Onchip Oscillator)... " );
                else
                    GEL_TextOut( "External Clock)... " );

                GEL_TextOut( "[Already Set]\n" );
                return;
            }
        }
    }

    /*
     *  Step 0 - Stop all peripheral operations
     */

    /*
     *  Step 1 - Set clock mode
     */
    if ( power_up_pll == 1 )
    {
        GEL_TextOut( "(Powering up PLL)... " );
        if ( clock_source == 0 )
            *pll_ctl &= ~0x0100;    // Onchip Oscillator
        else
            *pll_ctl |= 0x0100;     // External Clock
    }

    /*
     *  Step 2 - Set PLL to bypass
     *         - Wait for PLL to stabilize
     */
    *pll_ctl &= ~0x0021;
    _wait2( 150 );

    /*
     *  Step 3 - Reset PLL
     */
    *pll_ctl &= ~0x0008;

    /*
     *  Step 4 - Disable PLL
     *  Step 5 - Powerup PLL
     *  Step 6 - Enable PLL
     *  Step 7 - Wait for PLL to stabilize
     */
    if ( power_up_pll == 1 )
    {
        *pll_ctl |= 0x0010;         // Disable PLL
        *pll_ctl &= ~0x0002;        // Power up PLL
        *pll_ctl &= ~0x0010;        // Enable PLL
        _wait2( 150 );               // Wait for PLL to stabilize
    }
    else
        *pll_ctl &= ~0x0010;        // Enable PLL

    /*
     *  Step 8 - Load PLL multiplier
     */
    *pll_pllm = pll_mult & 0x3f;

    /*
     *  Step 9 - Load PLL dividers ( must be in a 1/3/6 ratio )
     *           1:DDR2
     */
    *pll_bpdiv = 0x8000 | bypass_div;
    *pll_div1 = 0x8000 | ( ddr2_div & 0x1f );
    *pll_cmd |= 0x0001;             // Set phase alignment
    while( ( *pll_stat & 1 ) != 0 );// Wait for phase alignment

    /*
     *  Step 10 - Wait for PLL to reset ( 2000 cycles )
     *  Step 11 - Release from reset
     */
    _wait2( 2000 );
    *pll_ctl |= 0x0008;

    /*
     *  Step 12 - Wait for PLL to re-lock ( 2000 cycles )
     *  Step 13 - Switch out of BYPASS mode
     */
    _wait2( 2000 );
    *pll_ctl |= 0x0001;

    pll2_freq = 27 * ( ( *pll_pllm & 0x3f ) + 1 );
    ddr2_freq = pll2_freq / ( 2 * ( ( *pll_div1 & 0x1f ) + 1 ) );

 //   GEL_TextOut( "(DDR2 Phy = %d MHz + ",,,,, ddr2_freq );

    if ( clock_source == 0 )
        GEL_TextOut( "Onchip Oscillator)... " );
    else
        GEL_TextOut( "External Clock)... " );

    GEL_TextOut( "[Done]\n" );
}

hotmenu
Setup_PLL2_DDR_162_MHz_OscIn( )
{
    /* [DDR @162 MHz] w/ Onchip Oscillator */
    setup_pll_2( 0, 23, 1 );
}

/* ------------------------------------------------------------------------ *
 *                                                                          *
 *  setup_ddr2( )                                                           *
 *      Configure DDR2 to run at specified frequency.                       *
 *                                                                          *
 *      Using 2 x MT47H32M16 - ( 8Mb x 16 bits x 4 banks ) @ 162 MHz        *
 *                                                                          *
 * ------------------------------------------------------------------------ */
setup_ddr2( )
{
    #define DDR_SDBCR           *( unsigned int* )( 0x20000008 )
    #define DDR_SDRCR           *( unsigned int* )( 0x2000000c )
    #define DDR_SDTIMR          *( unsigned int* )( 0x20000010 )
    #define DDR_SDTIMR2         *( unsigned int* )( 0x20000014 )
    #define DDR_DDRPHYCR        *( unsigned int* )( 0x200000e4 )

    #define DDR_VTPIOCR         *( unsigned int* )( 0x200000f0 )
    #define DDR_DDRVTPR         *( unsigned int* )( 0x01c42038 )
    #define DDR_DDRVTPER        *( unsigned int* )( 0x01c4004c )

    int dummy_read;
    int pch_nch;
    int freq = 162;
	
//    GEL_TextOut( "Setup DDR2 (%d MHz + 32-bit bus)... ",,,,, freq );

    /*
     *  Step 1 - Setup PLL2
     *  Step 2 - Enable DDR2 PHY
     */
    psc_change_state( 13, 3 );

    /*
     *  Step 3 - DDR2 Initialization
     */
    DDR_DDRPHYCR = 0x50006405;      // DLL powered, ReadLatency=5
    DDR_SDBCR    = 0x00138822;      // DDR Bank: 32-bit bus, CAS=4,
                                    // 4 banks, 1024-word pg
    DDR_SDTIMR   = 0x22923209;      // DDR Timing
    DDR_SDTIMR2  = 0x0012c722;      // DDR Timing
    DDR_SDBCR    = 0x00130822;      // DDR Bank: cannot modify
    DDR_SDRCR    = freq * 7.8;      // Refresh Control [ 7.8 usec * freq ]

    /*
     *  Step 4 - Dummy Read from DDR2
     */
    dummy_read = *( int* )0x80000000;

    /*
     *  Step 5 - Soft Reset ( SYNCRESET followed by ENABLE ) of DDR2 PHY
     */
    psc_change_state( 13, 1 );
    psc_change_state( 13, 3 );

    /*
     *  Step 6 - Enable VTP calibration
     *  Step 7 - Wait for VTP calibration ( 33 VTP cycles )
     */
    DDR_VTPIOCR = 0x201f;
    DDR_VTPIOCR = 0xa01f;
    _wait2( 1500 );

    /*
     *  Step 8 - Enable access to DDR VTP reg
     *  Step 9 - Reat P & N channels
     *  Step 10 - Set VTP fields PCH & NCH
     */
    DDR_DDRVTPER = 1;
    pch_nch = DDR_DDRVTPR & 0x3ff;
    DDR_VTPIOCR = 0xa000 | pch_nch;

    /*
     *  Step 11 - Disable VTP calibaration
     *          - Disable access to DDR VTP register
     */
    DDR_VTPIOCR &= ~0x2000;
    DDR_DDRVTPER = 0;

    GEL_TextOut( "[Done]\n" );
}

hotmenu
Setup_DDR_162_MHz( )
{
    setup_ddr2( );
}

/* ------------------------------------------------------------------------ *
 *                                                                          *
 *  setup_aemif( )                                                          *
 *      Setup Async-EMIF to Max Wait cycles and specified bus width.        *
 *                                                                          *
 * ------------------------------------------------------------------------ */
setup_aemif( int bus_width )
{
    #define AEMIF_BASE          0x01e00000
    #define AWCCR               *( unsigned int* )( 0x01e00004 )
    #define A1CR                *( unsigned int* )( 0x01e00010 )
    #define A2CR                *( unsigned int* )( 0x01e00014 )
    #define A3CR                *( unsigned int* )( 0x01e00018 )
    #define A4CR                *( unsigned int* )( 0x01e0001c )
    #define NANDFCR             *( unsigned int* )( 0x01e00060 )

 //   GEL_TextOut( "Setup Asyn Emif (%d-bit bus)... ",,,,, bus_width );

    AWCCR = 0x00000000;         // No extended wait cycles
    if ( bus_width == 8 )       // Setup for 8-bit bus
    {
        A1CR = 0x3ffffffc;      // Wait cycles - Max Wait
        A2CR = 0x3ffffffc;
        A3CR = 0x3ffffffc;
        A4CR = 0x3ffffffc;
    }
    if ( bus_width == 16 )      // Setup for 16-bit bus
    {
        A1CR = 0x3ffffffd;      // Wait cycles - Max Wait
        A2CR = 0x3ffffffd;
        A3CR = 0x3ffffffd;
        A4CR = 0x3ffffffd;
    }
    NANDFCR = 0x00000000;       // NAND controller not used
    GEL_TextOut( "[Done]\n" );
}

/*hotmenu*/
Reset_EMIF_8Bit_Bus( )
{
    setup_aemif( 8 );
}

/*hotmenu*/
Reset_EMIF_16Bit_Bus( )
{
    setup_aemif( 16 );
}

/* ------------------------------------------------------------------------ *
 *                                                                          *
 *  Setup_EMIF_CS2( )                                                       *
 *      Setup Async-EMIF to depending on the memory device.                 *
 *                                                                          *
 * ------------------------------------------------------------------------ */
hotmenu
Setup_EMIF_CS2_NorFlash_16Bit( )
{
    #define AEMIF_BASE              0x01e00000
    #define AEMIF_A1CR              *( unsigned int* )( 0x01e00010 )
    #define AEMIF_NANDFCR           *( unsigned int* )( 0x01e00060 )
    #define EMIF_CS2_PTR            *( unsigned char* )( 0x42000000 )

    GEL_TextOut( "Setup EMIF CS2 - NOR Flash (16-bit bus)... " );
    AEMIF_A1CR = 0x0050043d;        // NOR Flash settings ( @ 99MHz or below )
    AEMIF_NANDFCR &= 0xfffffffe;    // Disable Hw NAND Flash controller
    EMIF_CS2_PTR = 0xf0;            // Reset Flash memory to Read mode
    GEL_TextOut( "[Done]\n" );
}

hotmenu
Setup_EMIF_CS2_SRAM_16Bit( )
{
    #define AEMIF_BASE              0x01e00000
    #define AEMIF_A1CR              *( unsigned int* )( 0x01e00010 )
    #define AEMIF_NANDFCR           *( unsigned int* )( 0x01e00060 )
    #define EMIF_CS2_BASE           ( 0x42000000 )

    GEL_TextOut( "Setup EMIF CS2 - SRAM (16-bit bus)... " );
    AEMIF_A1CR = 0x00200105;        // SRAM settings ( @ 99MHz or below )
    AEMIF_NANDFCR &= 0xfffffffe;    // Disable Hw NAND Flash controller
    GEL_TextOut( "[Done]\n" );
}

hotmenu
Setup_EMIF_CS2_NandFlash_8Bit( )
{
    #define AEMIF_BASE              0x01e00000
    #define AEMIF_A1CR              *( unsigned int* )( 0x01e00010 )
    #define AEMIF_NANDFCR           *( unsigned int* )( 0x01e00060 )
    #define NAND_CLE_PTR            *( unsigned char* )( 0x42000010 )

    GEL_TextOut( "Setup EMIF CS2 - NAND Flash (8-bit bus)... " );
    AEMIF_A1CR = 0x00840328;        // NAND Flash settings ( @ 99MHz or below )
    AEMIF_NANDFCR |= 0x00000001;    // Enable Hw NAND Flash controller
    NAND_CLE_PTR = 0xff;            // Reset Flash memory to Read Mode
    GEL_TextOut( "[Done]\n" );
}


hotmenu
Setup_EMIF_CS3_FPGA_16Bit( )
{
    #define AEMIF_BASE              0x01e00000
    #define AEMIF_A2CR              *( unsigned int* )( 0x01e00014 )
    #define AEMIF_NANDFCR           *( unsigned int* )( 0x01e00060 )
    #define EMIF_CS3_PTR            *( unsigned char* )( 0x44000000 )

    GEL_TextOut( "Setup EMIF CS3 - FPGA (16-bit bus)... " );
    AEMIF_A2CR = 0x00300105;        // 0x00300605
    AEMIF_NANDFCR &= 0xfffffffe;    // Disable Hw NAND Flash controller
    GEL_TextOut( "[Done]\n" );
}

hotmenu
Setup_EMIF_CS4_FPGA_16Bit( )
{
    #define AEMIF_BASE              0x01e00000
    #define AEMIF_A3CR              *( unsigned int* )( 0x01e00018 )
    #define AEMIF_NANDFCR           *( unsigned int* )( 0x01e00060 )
    #define EMIF_CS4_PTR            *( unsigned char* )( 0x46000000 )

    GEL_TextOut( "Setup EMIF CS4 - FPGA (16-bit bus)... " );
    AEMIF_A3CR = 0x00300605;        //
    AEMIF_NANDFCR &= 0xfffffffe;    // Disable Hw NAND Flash controller
    GEL_TextOut( "[Done]\n" );
}

/* ------------------------------------------------------------------------ *
 *                                                                          *
 *  Boot_Mode_Reader( )                                                     *
 *      Read and Print boot mode                                            *
 *                                                                          *
 * ------------------------------------------------------------------------ */
hotmenu
Boot_Mode_Reader( )
{
    #define BOOTCFG             *( unsigned int* )( 0x01c40014 )
    int endian                  = ( BOOTCFG >> 20 ) & 0x1;
    int fast_boot               = ( BOOTCFG >> 19 ) & 0x1;
    int pci_enable              = ( BOOTCFG >> 17 ) & 0x1;
    int emifa_bus_width         = ( BOOTCFG >> 16 ) & 0x1;
    int aeaw                    = ( BOOTCFG >> 12 ) & 0x7;
    int aem                     = ( BOOTCFG >> 8  ) & 0x7;
    int boot_mode               = ( BOOTCFG >> 0  ) & 0xf;

    GEL_TextOut( "\nBoot Mode Reader:\n" );

    if ( boot_mode == 0 )
        GEL_TextOut( "  > [Boot Mode]: No Boot\n" );
    else if ( boot_mode == 1 )
    {
        if ( pci_enable == 0 )
            GEL_TextOut( "  > [Boot Mode]: HPI Boot\n" );
        if ( pci_enable == 1 )
            GEL_TextOut( "  > [Boot Mode]: PCI Boot w/o auto init\n" );
    }
    else if ( boot_mode == 2 )
    {
        if ( pci_enable == 0 )
            GEL_TextOut( "  > [Boot Mode]: HPI Boot\n" );
        if ( pci_enable == 1 )
            GEL_TextOut( "  > [Boot Mode]: PCI Boot w/ auto init\n" );
    }
    else if ( boot_mode == 4 )
    {
        if ( fast_boot == 0 )
            GEL_TextOut( "  > [Boot Mode]: EMIFA ROM Direct Boot\n" );
        if ( fast_boot == 1 )
            GEL_TextOut( "  > [Boot Mode]: EMIFA ROM Fast Boot\n" );
    }
    else if ( boot_mode == 5 )
        GEL_TextOut( "  > [Boot Mode]: I2C Boot\n" );
    else if ( boot_mode == 6 )
        GEL_TextOut( "  > [Boot Mode]: SPI Boot\n" );
    else if ( boot_mode == 7 )
        GEL_TextOut( "  > [Boot Mode]: NAND Boot\n" );
    else if ( boot_mode == 8 )
        GEL_TextOut( "  > [Boot Mode]: UART Boot\n" );
    else if ( boot_mode == 11 )
        GEL_TextOut( "  > [Boot Mode]: EMAC Boot\n" );
    else
        GEL_TextOut( "  >>>>>> ERROR boot option not supported <<<<<<\n" );

    if ( fast_boot == 0 )
        GEL_TextOut( "  > [Fast Boot]: No\n" );
    if ( fast_boot == 1 )
        GEL_TextOut( "  > [Fast Boot]: Yes\n" );

    if ( emifa_bus_width == 0 )
        GEL_TextOut( "  > [Bus Width]: 8-bit\n" );
    if ( emifa_bus_width == 1 )
        GEL_TextOut( "  > [Bus Width]: 16-bit\n" );

    if ( pci_enable == 0 )
        GEL_TextOut( "  > [PCI]      : OFF\n" );
    if ( pci_enable == 1 )
        GEL_TextOut( "  > [PCI]      : ON\n" );

    if ( endian == 0 )
        GEL_TextOut( "  > [Endianess]: Big Endian\n" );
    if ( endian == 1 )
        GEL_TextOut( "  > [Endianess]: Little Endian\n" );
#if 0
    if ( aeaw == 0 )
        GEL_TextOut( "  > [AEAW][%d]  : EMIFA ADDR[12:0]\n",,,,, aeaw );
    else if ( aeaw == 4 )
        GEL_TextOut( "  > [AEAW][%d]  : EMIFA ADDR[20:0]\n",,,,, aeaw );
    else
        GEL_TextOut( "  >>>>>> ERROR AEAW option not supported <<<<<<\n" );

    if ( aem == 0 )
        GEL_TextOut( "  > [AEM] [%d]  : No boot\n",,,,, aem );
    else if ( aem == 1 )
        GEL_TextOut( "  > [AEM] [%d]  : [EMIFA 8-bit][CCDC 16-bit][VENC 8-bit]\n",,,,, aem );
    else if ( aem == 2 )
        GEL_TextOut( "  > [AEM] [%d]  : [EMIFA 16-bit]\n",,,,, aem );
    else if ( aem == 3 )
        GEL_TextOut( "  > [AEM] [%d]  : [EMIFA 8-bit][CCDC 8-bit][VENC 16-bit]\n",,,,, aem );
    else if ( aem == 4 )
        GEL_TextOut( "  > [AEM] [%d]  : [NAND 8-bit][CCDC 8-bit][VENC 16-bit]\n",,,,, aem );
    else if ( aem == 5 )
        GEL_TextOut( "  > [AEM] [%d]  : [NAND 8-bit][CCDC 16-bit][VENC 8-bit]\n",,,,, aem );
    else
        GEL_TextOut( "  >>>>>> ERROR AEM option not supported <<<<<<\n" );
#endif

    GEL_TextOut( "\n" );
}
