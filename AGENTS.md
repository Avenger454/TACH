# TACH development

This is the TACH application repository for the 91 Firebird display project.

- Development is authorized here. The freeze applies only to Avenger454/Base-4Inch.
- Started from frozen Base snapshot bfc78dfdddea16ad112cc4416c5b3398052af079.
- Preserve the hardware-validated 720 x 720 display initialization, DSI compatibility wrapper and linker flags.
- Keep the optimized red RPM pointer, gear readout and slow demonstration sweep.
- Do not modify Base-4Inch as part of TACH work.

## Default sweep style for future gauge designs

- Use a narrow, opaque bright red (#FF0000) radial needle spanning the space between the inner and outer blue arcs.
- Default width is 7 px on a 720 x 720 face; scale proportionally on other displays.
- Calibrate needle endpoints to each face's arcs and its angle mapping to the scale; do not reuse another face's radii blindly.
- Prefer this over a moving dash or animated filled arc unless the user requests another style.
- Preserve bounded old/new needle invalidation, local point coordinates, unchanged-value skips, and static backgrounds.
- Keep the slow 30-second up/down demo sweep unless requested otherwise.
