## nRF51822
```
    48      37
   +----------+
  1| *        |36
   |          |
   |          |
 12|          |25
   +----------+
    13      24
```

## Used pins:
```
 1 - VDD (Power supply)
 6 - P0.02/AIN3 (General purpose I/O pin. ADC/LPCOMP input 3)
 7 - P0.03/AIN4 (General purpose I/O pin. ADC/LPCOMP input 4)
12 - VDD (Power supply)

13 - VSS (Ground)
23 - SWDIO/nRESET (System reset (active low). Also hardware debug and flash programming I/O)
24 - SWDCLK (Hardware debug and flash programming I/O)

29 - DEC2 (Power supply decoupling)
30 - VDD_PA (Power supply output (+1.6 V) for on-chip RF power amp)
31 - ANT1 (Differential antenna connection (TX and RX))
32 - ANT2 (Differential antenna connection (TX and RX))
35 - AVDD (Analog power supply (Radio))
36 - AVDD (Analog power supply (Radio))

37 - XC1 (Connection for 16/32 MHz crystal or external 16 MHz clock reference)
38 - XC2 (Connection for 16/32 MHz crystal)
39 - DEC1 (Power supply decoupling)
45 - P0.26/AIN0/XL2 (General purpose I/O pin. ADC/LPCOMP input 0. Connection for 32.768 kHz crystal)
46 - P0.27/AIN1/XL1 (General purpose I/O pin. ADC/LPCOMP input 1. Connection for 32.768 kHz crystal or external 32.768 kHz clock reference)
```

