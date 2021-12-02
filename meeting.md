# MEETING 26 NOV 2021


## Deadlines

- 18 Nov:
    - [x] Introduction
- 25 Nov:
    - [] Literature Review (2/3)

## Problems

- Literature Review consume more time than expected 
- Timing issues on loiTruck 
    - response time is undeterministic (not Real Time)
    - not support PDO (Bjoen's request)

## Todos

- Continue working on the Literature Review on weekend
- Adapt CANopenNode --> solve support PDO
- Implement a timing mechanism to accurately estimate time --> Real Time problem

## Findings

- Timing mechanism
    - Use Timer to count clocks cycle 
    - Use Portenta Debug interface: DWT_CYCCNT

- CANopenNode
    - fully implement CANopen
    - available CANopenEditor for auto create CANopen Object Dictionary
    - More time deterministic (using multithread)


# Cortex M4 DWT register map

- address range : 0xE0001000
- M4_DWT_CYCCNT : 0x004

# Cortex M7 DWT register map

- address range : 0xE00010000 on the AHBD
- M7_DWT_CYCCNT : 0x004













