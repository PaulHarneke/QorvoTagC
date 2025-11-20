# QorvoTagC

Standalone Zephyr application for the DWM3001CDK (nRF52833 + DW3110) that boots
into a FiRa-compatible controlee/tag and automatically starts three parallel
ranging sessions using the Qorvo/Decawave UWB stack.

## Sessions
- Session 1: ID 1, controller MAC `0x10`, channel 9
- Session 2: ID 2, controller MAC `0x11`, channel 9
- Session 3: ID 3, controller MAC `0x12`, channel 9

Common settings for all sessions: device address `0x01`, ranging interval 100 ms,
slot duration 2400 RSTU (2 ms), unicast/multi-node disabled for 1:1 ranging.

## Build
Point west to your nRF Connect SDK environment and build like any Zephyr sample,
for example:

```sh
west build -b decawave_dwm3001cdk
```

The application initializes the UCI backend and FiRa app, retries on failures,
and keeps running without a host PC once powered.
