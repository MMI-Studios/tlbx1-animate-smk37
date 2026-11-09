# Animate mathematical backend

`src/animate_math.c` supplies original freestanding helpers; it imports no donor
engine. Its constant-table interpolation follows the general approach in Melodee's
`dsp.c`. There is no runtime table generation, allocation, mutable shared state or
production dependency on libm. Define `AV_MATH_REFERENCE` to use native libm in
comparison builds. Reference builds require the host math library.

| Helper | Production method | Tested tolerance |
| --- | --- | --- |
| `av_sin` | 257-entry quarter-wave table, linear interpolation | Absolute error <0.000006 for 0–2π; <0.0001 for ±1024 radians |
| `av_tanh` | 257-entry positive table over 0–8, linear interpolation, odd symmetry | Absolute error <0.0001; clamps to ±1 beyond ±8 |
| `av_pow2` | Integer exponent plus seventh-order exponential polynomial | Relative error <0.000002 over exponents −11 through 14 |
| `av_exp` | Base conversion through `av_pow2` | Relative error <0.00002 over −80 through 80 |
| `av_note_hz` | A4 times `av_pow2((note−69)/12)` | Fractional note pitches retained |

The two float tables occupy 2,056 constant bytes before compiler/linker alignment.
Sine expects bounded synthesizer phases; it is not a high-precision huge-angle
reducer. Inputs outside ±1,048,576 radians return zero. `av_pow2` flushes results
below the minimum normal binary32 range to zero and clamps overflow to the largest
finite float. NaNs propagate. These are deliberate differences from full libm.

Use sine/tanh in the sample path. Prepare exp/pow2-derived envelope, frequency and
filter coefficients when parameters or sample rate change; availability of a
freestanding function does not justify recomputing those coefficients per sample.
The voice core must account for the stated approximations in reference comparisons.

`tests/math_test.c` sweeps each range at 200,001 points and checks tuning, monotonic
tanh and exceptional-value behavior. Numerical tests do not establish processor
timing, device polyphony, or parity of an entire synthesized sound. Hardware CPU
measurements remain necessary.
