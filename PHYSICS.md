# RC Cars vehicle physics — what's recovered

Two halves: the **tuning data** (recovered in full, usable now) and the
**equations** (located and mapped, not yet transcribed).

Tool: `dump_physics.py` prints every parameter for every car.

## Parameter data — RECOVERED

Six subsystems, one file set per car (1 = Overkill, 2 = Buggy, 3 = Hummer):

| group | parameters |
|-------|-----------|
| `engine` | boostSpeedUp, boostSpeedDown, speedBaseMax, speedBoostMax, splAccelBase.bspl, splAccelBoost.bspl |
| `tires` | CoeffFrontTires, CoeffRearTires, coeffDriftRear, angleDriftOn, speedDriftOn, coeffDeepSand, coeffWater, SpeedAngMaxREL |
| `resistance` | coeffFrictionBearings, coeffAirResistance |
| `springs` | coeffMomentOZ, coeffMomentOX, lenAxe, length, touchPoint, kPos, kSpeed, lenMin, lenMax, dx, dz, angleProcUpMax, angleProcDownMax |
| `steering` | AngleSteer, steering.bspl |
| `cdt` | CdtRadWheel, coeffRadBackWheels, FrontWheelDeltaX, CdtRadUp, CdtDeltaX/Y/Z, CenterMassOY, Corridor |

Each exists twice in `Settings/`:

- **`<group><n>.ini`** — slider DEFINITIONS. `Values min max default`, plus an
  optional `FValues lo hi` giving the float range the integer maps into.
- **`<group><n>.crs`** — TIOX binary with the RUNTIME values.

### .crs format

Same chunk framing as everything else, and like the pack it carries a float
version immediately after the root `0x5210` before the first chunk — miss that
and the walker reads the float as a chunk header and bails.

Strings are obfuscated with a flat **XOR 0x25** (`'NuJV'` -> `kPos`,
`'QJPFMuJLKQ'` -> `touchPoint`). Note this is a different, much simpler scheme
than the pack's mutating key.

Per-parameter chunks inside each `0x0220` entry:

    0x4021 name (obfuscated)   0x3023 min   0x3024 max   0x3025 value

Where the `.ini` supplies `FValues`, the physical quantity is
`lo + (value-min)/(max-min) * (hi-lo)`; otherwise the loader applies its own
scale (see below).

Some values are splines, not scalars: `splAccelBase.bspl`, `splAccelBoost.bspl`
and `steering.bspl` name `.bspl` files. **All 30 are now decoded** — see the
next section.

## Spline curves — RECOVERED

`bspl2csv.py` decodes all 30 `.bspl` files; CSVs in `splines_csv/`.

A `.bspl` is a **miniature scene file**: the same `0x3801` marker as `.sb`, a
`0x4802` comment, sometimes the full `0x9A00` type registry, then a `0x9200`
node tagged `NLPS` (reversed `SPLN` = MOD_SPLINE).

    0x305D       point count
    0x865D [n]   container, n * 0x805B
    0x805B       12 bytes = 3 floats (x, y, z)

**x is 0 in every RC Cars spline**, so these are 2D curves carried in (y, z):
y is the input, z the output.

Two exceptions: `Height2Stiffness` and `ButtonBegSpline` have y = 0 as well, so
they store only output values with the input implied by point index. Treating
them as (y,z) divides by zero.

### The curves that matter

| file | meaning | shape |
|------|---------|-------|
| `splAccelBase1/2/3` | acceleration vs speed, per car | Overkill 9.55 -> 0 over speed 0..25; Buggy 9.60 -> 0 over 0..30; Hummer 8.00 -> 0 over 0..20 |
| `splAccelGrav1/2/3` | gravity / incline response | out -0.7..10 |
| `steering[1/2/3]` | steering input -> degrees | linear, 0..2 -> 0..40 deg |
| `OVERKILL/BUGGY/HUMMER` | per-car response | in 0..32, out 0..9 |
| `*_RESTRICT` | restricted variants | lower limits |
| `FuncWaitAccel1..5[_A]` | AI throttle timing multipliers | in -50..45, out 0.6..1.2 |
| `Height2Stiffness` | suspension stiffness | 0.002 .. 1.005 |

The base acceleration curves corroborate the slider data: `splAccelBase1` hits
zero at speed 25 and car 1's `speedBaseMax` slider reads 27. Top speeds rank
Buggy > Overkill > Hummer, which matches the three cars' character.

The `.spl` text files (18 of them) are a related but distinct format -- a cubic
Bezier chain: a segment count, then 4 control points per segment, each segment
starting where the previous ended. Names like `AngleSteer2RearDrift.spl` mark
them as physics response curves too.

## Code — RECOVERED

All in `RCCars.exe`, clustered at `0x4f7000`–`0x50e000`.

### Loaders (one per subsystem)

| function | size | subsystem |
|----------|------|-----------|
| `FUN_004f79a0` | 1265 | engine (`speedBaseMax`) |
| `FUN_004f7ea0` | 246 | steering (`angleSteer`) |
| `FUN_004f7fa0` | 1114 | tires (`coeffDriftRear`, `CoeffFrontTires`) |
| `FUN_004f8400` | 3074 | collision / centre of mass (`CenterMassOY`) |
| `FUN_004f9080` | 303 | resistance (`coeffAirResistance`) |
| `FUN_004f91b0` | 1840 | springs (`coeffMomentOZ`) |
| `FUN_004fcce0` | 691 | physics subsystem creation |
| `FUN_00504470` | 776 | `SpringLength` |
| `FUN_0050d0d0` | 764 | `shiftCenterMassOY` |

They all share one shape: build the section name (`"RESISTANCE%d"` etc.) for
car *i*, fetch each key as an integer, clamp it against limits, scale by a
constant, and store into a global array with one slot per car. See
`physics.c`.

### The integrator — FOUND

It is **explicit Euler over a 32-float state vector**, and the model around it
is textbook **Baraff rigid-body dynamics** (the SIGGRAPH *Physically Based
Modeling* course notes). Nothing exotic: no RK4, no constraint solver on the
body itself.

Two earlier searches missed it for the same reason. Both looked in
`0x4f7000`–`0x50f000`, which is the **config-loading** region, and in
`0x507000`, which is the **renderer**. The actual vehicle dynamics live in two
places neither search covered:

| region | contents |
|--------|----------|
| `0x4ed000`–`0x4f4000` | vehicle force generation (tires, engine, suspension, contacts) |
| `0x4f5100`–`0x4f5c00` | the ODE: state marshalling, the Euler step, the substep search |
| `0x474900`–`0x476d00` | the **engine's** generic rigid-body layer (inertia, force summation) |

### How it was found

Not by call graphs or sizes — those had already produced two wrong answers.
The tuning globals are never read directly by the physics; they are reached
through six one-line **accessor functions**, each mapping a car index to a slot
in a per-car global array:

| getter | array base | stride | subsystem |
|--------|-----------|--------|-----------|
| `cfgEngineParams` `0x4f73d0` | `0x014c46d0` | `0x4c` | engine |
| `cfgSteeringParams` `0x4f7400` | `0x014c4868` | `0x08` | steering |
| `cfgTireParams` `0x4f7420` | `0x014c4ae8` | `0x34` | tires |
| `cfgResistParams` `0x4f7450` | `0x014c4b90` | `0x08` | resistance |
| `cfgSpringParams` `0x4f74a0` | `0x014c4c58` | `0x38` | springs |
| `cfgCdtParams` `0x4f7470` | `0x014c4d00` | `0xb4` | collision points |

So the earlier note's guess was right in spirit and wrong in mechanism: the
values are not copied into a per-car struct, the struct just stores an **index**
(`phys + 0x54`) and calls a getter. Enumerating the getters' callers lands
directly in the physics module. Two of the entry points sit inside byte ranges
Ghidra never turned into functions at all, which is why nothing referenced
them — `0x4f5400` had to be created by hand.

Also worth keeping: scanning raw `.text` bytes for any 32-bit value inside the
global range beats asking Ghidra for xrefs. It finds base-plus-index accesses
the reference model never materialises. `scanrefs.py`, `callers.py`, `cg.py`.

### The model

State vector, 32 floats (`carGetState` `0x4f5180`, `carSetState` `0x4f5290`):

| index | contents | body offset |
|-------|----------|-------------|
| 0–2 | position of the centre of mass | `+0x90` |
| 3–6 | orientation quaternion `(w,x,y,z)` | `+0x9c` |
| 7–9 | linear momentum **P** | `+0x104` |
| 10–12 | angular momentum **L** | `+0x110` |
| 13–18 | per-wheel suspension compression, 6 corners | wheel `+0x14` |
| 19–24 | per-wheel compression delta | wheel `+0x28` |
| 25 | one scalar | `phys+0x5c6c` |
| 26–31 | per-wheel extra compression | wheel `+0x2c` |

Derivative (`carStateDeriv` `0x4f5400`):

    ydot[0..2]   = v
    ydot[3..6]   = 0.5 * (0, w) (x) q       Hamilton product, quatMul 0x4079f0
    ydot[7..9]   = F                        force accumulator, body +0x11c
    ydot[10..12] = tau                      torque accumulator, body +0x128
    ydot[13..31] = 0

Step (`rbEulerStep` `0x4f5590`) — the whole integrator, 119 bytes:

    for (i = 0; i < 32; i++)
        yout[i] = y[i] + dt * ydot[i];

Indices 13..31 have zero derivative, so the wheel state rides through the step
unchanged. That is deliberate: it is in the state vector so the **substep
search can rewind it** along with the body when a trial step penetrates.

Scatter (`carSetState`) rebuilds everything derived, in this order:

1. normalise `q` — `quatNormalize 0x4078d0`, and note it takes the norm in two
   steps (`|xyz|`, then hypot with `w`), which does not round the same as one
   four-term `sqrt`;
2. `v = P / m` — `body+0x04` holds **1/mass**, not mass;
3. `Iinv_world = M' * Ibodyinv * M` — `rbUpdateInvInertiaWorld 0x474e00`,
   with `Ibodyinv` at `body+0x50` and the result at `body+0xac`;
4. `w = Iinv_world * L` — `rbMat3MulVec3 0x474910`.

### The rigid body struct

Base is `car->phys + 0x57f4`, i.e. `*(car + 0xE8) + 0x57f4`.

| offset | field |
|--------|-------|
| `+0x00` | mass |
| `+0x04` | 1 / mass |
| `+0x50` | inverse body inertia, 4x4 row-major |
| `+0x90` | position (centre of mass) |
| `+0x9c` | orientation quaternion `(w,x,y,z)` |
| `+0xac` | inverse world inertia, 4x4 |
| `+0xec` | linear velocity (derived) |
| `+0xf8` | angular velocity (derived) |
| `+0x104` | linear momentum **P** (state) |
| `+0x110` | angular momentum **L** (state) |
| `+0x11c` | force accumulator |
| `+0x128` | torque accumulator |

Wheels: 6 records of `0x68` bytes at `phys + 0x5948`, count at `phys + 0x5928`.
Fields used by the suspension: `+0x00` mount point (body space), `+0x0c` `kPos`,
`+0x10` `kSpeed`, `+0x14` compression, `+0x18` free length, `+0x28` compression
delta, `+0x2c` extra compression.

### Matrix and axis conventions — both load-bearing

Matrices are 4x4 **row-major with row-vector semantics**: a point transforms as
`p' = p * M` and translation sits in row 3 (`+0x30`). That is why the world
inertia is assembled as `M' * Ibodyinv * M` rather than `R * Ibodyinv * R'` —
same thing, read in the other convention.

The car's world matrix is at `car + 0xf8`. Its rows are the local axes:

| accessor | row | meaning |
|----------|-----|---------|
| `mat4GetRow0` `0x406640` | `+0x00` | X, right |
| `mat4GetRow1` `0x40cbd0` | `+0x10` | Y, up |
| `mat4GetRow2` `0x40cbf0` | `+0x20` | Z, forward |
| `mat4GetRow3` `0x40cbb0` | `+0x30` | translation |

**Gravity is 10.0 m/s², not 9.81**, applied as `mass * -10.0` along `(0,1,0)`.

### Forces

`carAccumForces 0x4f0910` builds a list of `(point, force)` pairs and reduces it
with `rbSumForcesTorques 0x475100`:

    F   = sum f_i
    tau = sum (p_i - x) x f_i

in that order, with both accumulators reset first — they double as scratch for
the suspension sub-queries, so only the final call's result reaches the
derivative.

Contributions, in the original's order:

1. **Gravity** at the centre of mass: `mass * -10.0 * (0,1,0)`.
2. **Drag**, `carDragForce 0x4ed970`:

       F = -vhat * (coeffAirResistance * |v|^2 + coeffFrictionBearings)

   Quadratic air drag plus a constant bearing/rolling term. Both constants come
   from `cfgResistParams`, confirming the recovered values.
3. **Suspension force**, `carSuspForce 0x4f1000` — the per-corner spring force
   projected onto the contact normal.
4. **Suspension torque**, `carSuspTorque 0x4f0be0`, applied as a force couple
   via `rbTorqueToCouple 0x4749d0`.
5. **Contacts, tires and engine**, `carContactSolve 0x4edac0` — see below.

The spring/damper, `suspSpringDamper 0x4f0f80`:

    spring = -((travel2 + travel - rest) * kPos)
    damp   = kSpeed * dtravel / dt          (zero if dt <= 0.001)
    result = spring - damp, with |damp| clamped to |spring| * 2e6

A corner is loaded only while `travel2 + travel <= rest`. The clamp is a
runaway guard: `dtravel/dt` blows up at small `dt`, so it is capped relative to
the spring term rather than absolutely.

### coeffMomentOX / coeffMomentOZ — independently confirmed

`carSuspTorque` is the best cross-check in the whole effort. It takes the raw
suspension torque, decomposes it onto the car's **own X and Z axes**, scales
each component, and reassembles — discarding the Y component entirely, so
suspension can never yaw the car:

    tau' = Xaxis * (tau . Xaxis) * springs[0]
         + Zaxis * (tau . Zaxis) * springs[1]

`springs[0]` is `0x014c4c58` and `springs[1]` is `0x014c4c5c` — exactly where the
loaders write `coeffMomentOX` and `coeffMomentOZ`. The names, the axes and the
array slots all agree, which is much stronger evidence than any of the three
independently.

### The substep search

`carPhysTick 0x4f5e50` runs at most **4** substeps per frame, alternating two
strategies over the same Euler stepper:

- `carSubstepCCD 0x4f5b60` — **conservative advancement**. Collect the car's
  collision spheres (`carGatherCollSpheres 0x4ef9e0`, packed as `x,y,z,radius`),
  take a full step, and if anything moved too far, shrink `dt` to
  `min over spheres of 0.9 * radius / |v_point|`, where the point velocity comes
  from `rbPointVelocity 0x474ed0` (`v + w x r`).
- `carSubstepContact 0x4f0270` — **time-of-impact bisection**, at most 5 tries.
  Trial time is `t_lo * a + (1 - a) * t_hi` with `a = 0.8` on the first probe and
  after a penetration, `0.5` once a clean step is found. It returns the `dt`
  actually consumed, so the caller loops on the remainder.

Both save state, step, commit, test, and restore from the saved 32-float
vector — which is what those 19 non-integrated wheel slots are for.

### Contacts, tires and engine — TRANSCRIBED

`carFrictionSolve 0x4edac0` and its subtree are now transcribed into
`rccars_vita/contact.c`. The division of labour turns out to be unusually clean:

| what | where | how |
|------|-------|-----|
| normal load | suspension springs | `carSuspForce`, per corner, along the car's up axis |
| longitudinal | `carDriveForces 0x4ee5e0` | engine force at the **two rear** contact points |
| lateral | `carFrictionSolve 0x4edac0` | 2x2 simultaneous friction solve, grip-clamped |
| environment | `carSurfaceDrag 0x4eeea0` | per-wheel quadratic drag in sand or water |

The car is **rear wheel drive**: the drive loop runs over wheel indices 2 and 3
only, with the force halved between them.

#### CORRECTION: 0x4eeea0 is not a tire force

It was provisionally named `carTireForce`. It is `carSurfaceDrag` — per-wheel
quadratic drag using `coeffDeepSand` (tire[6]) on surface class 3, or
`coeffWater` (tire[7]) ramped linearly between 30% and 90% of the wheel radius
submerged. There is no separate cornering-force function: **all lateral grip
comes out of the friction solve.**

#### The friction solve

Two contacts, chosen by `carGatherContacts 0x4ee280`. With one or two wheels
down it takes them in order; with three or four it takes **one wheel from each
axle**, picking the side by the sign of the steering angle. That is why a 2x2
system is enough — the solve always sees one front and one rear contact.

    A[i][j] = t_i . ( t_j / m  +  (Iinv (r_j x t_j)) x r_i )
    b[i]    = -d/dt( v_point_i . t_i )
    solve A f = b, then apply f_i along t_i at contact i

`t` is the lateral direction, `lat = dir x normal`, where `dir` is the wheel's
local +Z rotated about local Y by the steering angle (`mat4RotY 0x40cc60` takes
**degrees**). So the solve resists cornering slip specifically.

The clamp is where the handling lives. Each `b[i]` is limited to `grip*10` at the
rear and `grip*5` at the front — unless the car is drifting, in which case both
axles get the wider window. Past that limit the tire stops holding and the car
slides.

When drive is inhibited (`carDriveForces` returns nonzero, i.e. you are braking
hard enough to reverse direction), `carGatherContacts` **replaces** `lat` with
the normalised full slide direction, so friction opposes all motion rather than
only the lateral part. That is the locked-wheel skid.

#### Tires

`carTireGrip 0x4ee180`:

    grip_front = tire_upgrade[level] * CoeffFrontTires
    grip_rear  = tire_upgrade[level] * CoeffRearTires
                 * coeffDriftRear   (only while drifting)

`carTireDrifting 0x4ee130` is simply **brake held AND `|steer| > angleDriftOn`** —
a handbrake turn. With Overkill's `coeffDriftRear` of 0.218 that cuts rear grip
to a fifth, which is the whole drift mechanic.

`tire_upgrade[0..3]` is `tire[8..11]`, loaded from `UPGRADES.ini` section
`[TIRES]`, key `<Car>Upgrades` — it is an **upgrade** table, not a surface table.

#### Engine

`carEngineAccel 0x4eea50` returns an acceleration in m/s², which
`carDriveForces` clamps to `+-grip_rear * 20` (the traction limit) and multiplies
by mass.

    x = (v_forward * 3.6) / (|throttle| * boost_mult * resonator_speed[lvl])
    a = curve(<CAR>.bspl, x)          zero above the limiter
    a *= 1.2                          if boosting
    a *= resonator_accel[lvl]
    a += |drag|                       see below

Reverse is separate: `-brake * 4` (`* 6` boosted), tapering to nothing between
2.8333 and 3.3333 m/s. Direction changes require `|v| <= 2.0`; above that the
drive is blocked instead, which is what triggers the skid path above.

`engine[11..14]` / `engine[15..18]` are the `[RESONATORS]` `<Car>Upgrades` and
`<Car>Upgrades_SPL_OY` tables — a speed scale and an acceleration scale.

The `<CAR>_RESTRICT.bspl` curve comes back through the second out-parameter and
becomes engine braking, applied along the rolling direction only when the car is
rolling against it.

#### The curves are NET acceleration

The last line of `carEngineAccel` **adds the positive drag magnitude back**:

    if (a > 0.001) return a + |drag|;

`carDragForce` subtracts drag at the centre of mass every step, so adding it here
cancels it. The acceleration curves therefore describe *net* acceleration versus
speed, which is why they cross zero exactly at each car's top speed.

(`carDragForce 0x4ed970` returns that magnitude in `ST0` — visible only in the
disassembly at 0x4eda87, not in the decompiler output, which types the function
`void`. With a NULL out-parameter it is used purely as a drag-magnitude query.
Note the original leaves `ST0` undefined when speed <= 0.0001; the port returns
0.0 there.)

### kPos — the last unresolved key, and it was not a stiffness

`kPos` was one of the two keys whose slider conversion was never recovered.
Both halves are now in hand, and the reason it never made sense as a spring rate
is that **it is not one**. `carSetupSprings 0x4f1930` derives the runtime
stiffness from it:

    wheel.k_pos = (mass * 10) / (nwheels * kPos)

Set the equilibrium condition `nwheels * k_pos * compression = mass * g` and
compression comes out as exactly `kPos`. So **`kPos` is the static suspension sag
in metres**, and the stiffness is whatever achieves it. A designer-friendly
parameterisation, and it explains the implausible magnitude.

The loader conversion (`physLoadSprings 0x4f91b0`) is:

| key | conversion |
|-----|-----------|
| `length` | `(raw-1) * 0.0049495 + 0.01` |
| `lenMin` | `((raw-1) * 0.0080808 + 0.1) * length` |
| `lenMax` | `((raw-1) * 0.0090909 + 1.1) * length` |
| `kPos` | `((raw-1) * 0.0080808 + 0.1) * (length - lenMin)` |
| `kSpeed` | `(raw-1) * 0.0909091 + 1.0` |

**`lenMin` and `lenMax` are fractions of `length`**, not absolute lengths — the
loader seeds each slot from `length` and multiplies. Overkill: `length` 0.2179 m,
`lenMin` 0.0218, `lenMax` 0.2397, `kPos` (sag) 0.1194 m, `kSpeed` 4.182.

So PHYSICS.md's earlier "kPos 1.573" was never recovered from the loaders and is
wrong; and `46 of 46` keys now resolve, not 44.

### CORRECTION: speedBaseMax is km/h, not m/s

This was assumed, never checked, and it is wrong. `carEngineAccel` divides it by
3.6 before comparing against the forward speed in m/s:

    if (speed_base_max * 0.2777778 * resonator_speed[lvl] <= v_forward) a = 0;

Overkill's 27 is therefore **27 km/h = 7.5 m/s**, and with the level-0 resonator
scale of 0.92 the limiter sits at 24.84 km/h = 6.90 m/s. The curve x axis is
km/h as well. These are 1:10 scale radio-controlled cars, so a top speed around
25 km/h is right and 27 m/s (97 km/h) never was.

The driving test confirms it end to end: ten seconds at full throttle settles at
**6.904 m/s** against a computed limiter of **6.900 m/s**.

### Suspension: `travel` is a LENGTH, not a compression

Worth stating plainly because getting it backwards makes the car fall through the
world, and the first transcription did. In the wheel record, `+0x28` (state
`y[13+i]`) is the **current spring length**; `+0x2c` is the free length. The
spring pushes while the length is below the free length and does nothing at or
above it, and the loaded test is `len_extra + len <= len_free`.

### Quirks reproduced deliberately

Three places where the original does something the textbook does not. All three
are verified in the disassembly, not inferred from the decompiler, and all three
are reproduced as-is because they are part of how the game feels.

| where | what | textbook |
|-------|------|----------|
| `rbPointAccelAlong 0x476c50` | builds `r x w` then crosses with `w`, giving `+w^2 r_perp` | `a + alpha x r + w x (w x r)`, i.e. the other sign |
| `carDriveForces 0x4ee8fc` | the suspension-force / rolling-direction dot product omits the Y term | a full three-term dot |
| `carFrictionSolve 0x4edfea`, `0x4ee02f` | the singular-case consistency check compares against `b[0]` in both branches | `b[1]` for the row-1 test |

The first two affect handling continuously; the third only fires when the 2x2 is
singular.

### Collision — TRANSCRIBED

`rccars_vita/collide.c`. This is the last piece, and it holds the single most
surprising fact in the whole model.

#### The suspension is not integrated. It is solved geometrically.

Each wheel is a **sphere on a prismatic joint along the body Y axis**. Every
substep, `carUpdateSuspension 0x4fb340` solves the spring length against the
world: if the wheel sphere is touching, retract until it is clear; if it is
clear, extend it back toward the free length. The spring/damper in `rb.c` then
converts that length into a force.

So the causality runs *geometry → length → force*, not force → length. And that
is the answer to the question the state vector posed back when the integrator was
found: **`len`, `dlen` and `len_extra` sit in the 32-float ODE state with a zero
derivative because they are a collision result the substep search has to be able
to roll back.** Nothing integrates them.

Per wheel, per substep:

| situation | action |
|-----------|--------|
| touching | `suspRetract 0x4fb9e0` — retract until clear, at most `0.95 * radius`, at most 5 passes |
| clear | extend toward `len_free` at `dt * radius`, ten times that once moving |
| barely moving (`< 1.388889 m/s` = 5 km/h) | extend toward `len_free - 0.95 * sag` instead — the parked ride height |
| after either | a segment test from the old wheel centre to the new one rejects a move that tunnelled through geometry |

`suspRetract` retracts by the penetration depth **plus `radius * 0.01`**, so a
resolved wheel ends up slightly clear of the surface. Every failure path —
bottoming out below `lenMin`, needing more than the budget, or exhausting five
passes — restores the original length and reports failure, which the caller
treats as "stuck" and which zeroes every `dlen`.

The extension rate is ramped by `phys+0x08` while it sits in `0..0.5`; that ramp
is set outside this function, so airborne extension starts slow and speeds up.

#### The 6 mm tolerance is load-bearing

Because `suspRetract` deliberately leaves the wheel clear, a zero-tolerance
contact query finds nothing. `carPhysTick` gathers the per-wheel contacts with
**`tol = 0.006`** (verified at the call site, 0x4f5fc9), and `carCheckContact`
uses the same figure. Get this wrong and the car free-falls through its own
suspension while the suspension code reports everything as fine — which is
exactly what happened during transcription before the call site was checked.

#### The collision proxy

`carGatherCollSpheres 0x4ef9e0` builds the whole car out of spheres:

- one sphere per wheel, at the wheel centre, radius `CdtRadWheel` (rear wheels
  scaled by `coeffRadBackWheels`);
- three body spheres on the centre line, at `CdtDeltaX/Y/Z(n)` with radius
  `CdtRadUp(n)`;
- the same three offset `+shiftRoofBaseLefter` and `-shiftRoofBaseLefter`.

That is **13 spheres for a four-wheel car and 15 for the Hummer**. The Buggy
shifts the centre-line trio along Z by `CdtDeltaZ_CENTRAL(n)`.

`carCollide 0x4efe00` queries them and turns hits into contacts:

    normal = normalise(sphereCentre - surfacePoint)     fallback (0,1,0)
    point  = sphereCentre - normal * radius

with three filter modes — 0 wheels only, 1 body only, 2 everything. The substep
search uses mode 2 (penetration); the per-wheel contact gather uses mode 0.

The output is the 0x98-byte wheel record the rest of the physics consumes:
`+0x4c` contact flag, `+0x58` contact point, `+0x70` normal, `+0x94` is-a-wheel.
Those are the offsets `contact.c` was already written against, now confirmed
from the producer side.

#### The wheel frame

`carWheelFrame 0x4ef680` is small but load-bearing:

    centre = (mount.x [+- cdt[15] for the front pair],
              mount.y - len [- len_extra],
              mount.z)  * carMatrix
    roll   = local +Z rotated about local Y by the steer angle, to world
    side   = the same at steer +- 90

The wheel hangs straight down the **body** Y axis, so it leans with the car. And
the Hummer's middle axle steers at **a third** of the front angle — which is why
`RB_MAX_WHEELS` is 6 and the state vector has six slots per wheel field.

#### The cdt array

Base `0x014c4d00`, stride `0xb4` (45 floats), per car:

| index | contents |
|-------|----------|
| 1 | `CdtRadWheel` |
| 3..6 | `coeffRadBackWheels * CdtRadWheel` |
| 11..14 | `CdtRadUp1..4` x 0.1 |
| 15 | front-wheel X offset (`FrontWheelDeltaX`) |
| 18..29 | `CdtDeltaX/Y/Z` for spheres 1..4, x 0.1 |
| 32, 35, 38 | `CdtDeltaZ_CENTRAL1..3` x 0.1 |
| 42 | `CenterMassOY` |
| 43, 44 | `shiftRoofBaseLefter` / `Righter` |

Note the extra **x 0.1** the loader applies to every `CdtDelta` and `CdtRadUp`
key on top of the slider conversion; `dump_physics.py` prints the pre-scaling
value, so `CdtRadUp` 0.51 is really 0.051 m.

#### Where the host comes in

`collide.c` takes three callbacks (`rb_world`), matching the three engine calls
the original makes:

| callback | original |
|----------|----------|
| `sphere` | `worldSphereQuery 0x454970`, query type 4 |
| `segment` | `worldSegmentQuery 0x4557e0` |
| `water` | the water probe, `0x00531b10` |

That is the entire remaining interface between the transcribed physics and the
level. For the Vita port it maps onto the existing `.col` grid.

#### The water probe, and what it actually returns

`0x00531b10` is not a query — it is an **accessor**. It reads a per-wheel record
out of `phys+0x5d40 → +0x0c`, stride `0x28`, which the WaterLOD module fills while
it animates the surface:

| field | out param | meaning |
|---|---|---|
| `+0x10` | return value | nonzero if this wheel has water |
| `+0x14` | `param_7` | **the gap**: wheel centre height above the surface |
| `+0x18` | `param_6` | unread by the physics |
| `+0x1c` | `param_4` | a point; defaults to (0,0,0) |
| `(i*5+5)*8` | `param_5` | a normal; defaults to (0,1,0) |
| `+0x34` | `param_3` | unread by the physics |

`carSurfaceDrag` asks for the return value and the gap only (`0x004eeefa`, four
NULLs), and gates the whole loop body on the wheel record's `+0x4c` active flag —
so **water only affects a wheel that is also touching ground.** Submerged depth is
`radius - gap`, and the drag ramps in over 0.3r to 0.9r of it. `coeffWater` is
tire[7]: 0.192 on the Overkill.

The Vita port has no per-wheel water bookkeeping, so `rb_world.water` takes the
**wheel sphere centre** as an extra argument and `col.c` answers it on the spot
from a water-height grid packed into the `.col` file (**COL3**). That grid is the
only piece of this that is not recovered data; see `pack_col.py`'s WATER note for
how it is built and why its 3 m cell granularity is harmless given the 0.3r
threshold.

Nothing else in the physics reads the water probe. `carSurfaceDrag` is the whole
of water's effect on the car: no buoyancy, no engine cut, and the surface is never
solid.

### Wired into the app

The model is now driving the Vita app, not just passing tests.

| file | role |
|------|------|
| `rccars_vita/col.c` / `col.h` | the `.col` grid, moved out of `main.c`, plus the sphere and segment queries the physics needs. `col_rb_world()` hands back an `rb_world`. |
| `rccars_vita/rbcar.c` / `rbcar.h` | builds an `rb_car` from the recovered data, drives it, exposes the pose |
| `rccars_vita/rb_data.h` | generated per-car tuning, curves and geometry |
| `rccars_re/gen_rb_data.py` | the generator |

`main.c` runs the transcribed model by default; **SELECT** swaps to the
`physics.c` placeholder and back, handing the pose across, so the two can be
compared on the same track.

The car renders from `rb_car.m` via `glMultMatrixf`. That needs no transpose: the
engine's row-major row-vector layout has the same memory layout as OpenGL's
column-major column-vector layout.

#### gen_rb_data.py supersedes gen_physics_header.py

Four things in `physics_data.h` are wrong for this model, and each is a
correction rather than a refinement:

- **the acceleration curve is the wrong curve.** `physics_data.h` uses
  `splAccelBase<n>.bspl`. The engine reads `engine[6]`, which `physLoadEngine`
  fills from `"OVERKILL"` / `"BUGGY"` / `"HUMMER"` — a different spline that
  crosses zero at 32/38/30 km/h, not 25/30/20. `splAccelBase` is never
  evaluated by `carEngineAccel`.
- `susp_k_pos` is the **raw slider value** (64), because the conversion was
  unknown when it was generated. It is the sag, and it is a fraction of
  `length - lenMin`.
- `susp_len_min` / `susp_len_max` are the **fractions** (0.1, 1.1), not lengths.
- the upgrade tables were absent; they come from `Scripts/upgrades.ini`, not the
  `.crs` files.

Two values `gen_rb_data.py` deliberately zeroes rather than guess:
`FrontWheelDeltaX` (no recovered slider scale, and the raw value scaled by 0.1
gives an absurd 1.2 m), and any body sphere whose keys are absent from a car's
`.crs` — a sphere left at the body origin would collide with the ground
permanently.

### carPhysTick's substep order, and why it matters

Reading the call sequence inside carPhysTick's `while` loop settled a question
that numerical experiment could not:

    carSubstepCCD              advance, capped at 0.9 radius of sphere travel
    carUpdateSuspension(0)     re-solve the spring lengths
    carSubstepContact          corrective bisection
    carUpdateSuspension(1)     re-solve again, slow mode

The suspension is re-solved **twice per substep**, not once per frame. That is
load-bearing. The primary advance is limited *only* by sphere travel — it is not
gated on penetration — and any resulting overlap is absorbed by the geometric
suspension solve, which shortens the spring so the wheel stays at the surface
while the body keeps moving. Gate the advance on a solid penetration test instead
and a landing car crawls at a few percent of real time, because the springs never
get the chance to compress.

What stops the car when the suspension runs out of travel is the **`stuck`
return**: once a strut bottoms out at `lenMin`, retract can no longer resolve the
overlap, the solve reports failure, and the substep is refused. carPhysTick
branches on exactly that.

### carPhysTick, read end to end — and the four things the port was missing

This section used to list three untranscribed limitations and blame the port's
porpoise and its floating flipped car on them. All three are resolved and **none of
them was the cause**. `carPhysTick 0x4f5e50` was decompiled in full and its loop
body is, in order:

    FUN_004f5770        clamp |P| and |L|                       -> rb_clamp_momentum
    carGatherWheelContacts(tol 0.006) + FUN_004f5980            -> rb_collide + rest_touch
    carGetState
    carSubstepCCD       ADVANCE, capped at 0.9 sphere radius    -> rb_euler_step
    carCheckPenetration(mode 1)   BODY spheres, solid           -> the contact gate
      if clear: carUpdateSuspension(dt, 0)
                carCheckPenetration(mode 0) if it reported stuck
    on an event: carSubstepContact -> FUN_004f0560 -> FUN_004f0730
    carUpdateSuspension(dt, 1)
    FUN_004fbe60        len_extra = radius * 0.02               -> rb_susp_len_extra

and it opens, before any of that, with `FUN_004f59a0` (four rest timers) and
`FUN_004f6610` (the rest clamp) — and when the clamp fires, **the whole loop is
skipped**.

#### 1. The advance comes FIRST, and that is what fixed the porpoise

The port solved the suspension before the Euler step. `carPhysTick` steps first.
The difference decides what `dlen` means, and `dlen` is the damper's only input:

`rb_susp_spring_damper` computes `damp = k_speed * dlen / dt`, where `dlen` is what
the last suspension pass moved the length by. Solve before the step and the pose has
not changed since the previous substep's second pass already solved it — the length
is already correct, so `dlen` is not the body's motion at all, just the extension
rate limit. A per-frame trace showed it **pinned at `10*dt*radius` = 2.992 mm**
frame after frame. Step first and the derivative uses the `dlen` from the previous
substep's slow pass, which ran right after that substep's advance and does measure
the geometric change it caused.

So a suspension whose paper damping ratio is `4.18 / (2*sqrt(41.86*0.5))` = **0.46**
was running with no velocity feedback, at its own natural frequency
`sqrt(4*41.86/2)` = 9.15 rad/s = **1.46 Hz**. That is the 1.5 Hz limit cycle, and it
is why every earlier experiment came back negative: the forces were right, the
disassembly matched, the cycle was timestep-independent — and the damper could not
see the body move. 131 mm -> **0.5 mm**.

#### 2. The contact solve — sequential impulses, not a projection

`carCheckPenetration(mode 1)` runs before the suspension, and a solid body-sphere
overlap (or a bottomed strut that leaves a wheel overlapping) is a *contact event*.
An event runs two functions:

**`FUN_004f0560`** — tangential friction. Takes the **first non-wheel contact**
whose contact-point velocity has a tangential component and applies an impulse of
`-min(|v_t|, dt*10)` along it, then returns. One contact only.

**`FUN_004f0750`** — the normal solve, a Gauss-Seidel relaxation:

    repeat (max 10 passes, "any" = did anything move):
      for each contact c:
        vrel = pointVelocity(c.point) . c.normal          (FUN_004f0460)
        if vrel > 0.02: continue
        dv = max(0.05 - vrel, 0)
        j  = (dv / k) * c.normal,  k = FUN_004754a0(c)    (FUN_004f0840)
        P += j;  L += r x j;  rederive v, w               (FUN_004756c0)

with `k = 1/m + n . ((Iinv (r x n)) x r)` — the same denominator the tire friction
matrix is built from. The two-body branch (`c+0x40 != 0`, `dv = max(-2*vrel, 0.2)`)
never fires here; this port has no car-vs-car contacts.

`L += r x j` is the part the invented stand-in did not have. It deleted linear
momentum along the normal and never touched angular momentum, so a car on its roof
kept turning; and with no friction its tangential velocity never decayed. Measured
on a flat plane, released inverted from y = 0.40:

| | invented projection | transcribed solve |
|---|---|---|
| Overkill rests at | 0.441 (0.39 m above its roof) | **0.085** |
| Buggy | 0.303 | **0.064** |
| Hummer | −0.061 (under the ground) | **0.110** |
| still drifting after 15 s | 0.04–0.08 m/s | **0.000 m/s** |

**The gate is load-bearing.** Force the solve to run every substep and it drives
every resting wheel to 0.05 m/s of separation and launches the car. `rb_test` pins
that with a mutation.

**One forced divergence.** The recovered normal `normalise(centre - surfacePoint)`
is the separating direction only while the centre is outside the surface. Push the
centre through and the same formula points *into* the ground, and the solve — whose
job is to reach +0.05 m/s along the normal — becomes a pump. Measured on the
inverted Hummer: 0.04 m to **−174 m** at never more than 0.06 m/s. The original
never reaches that state because `carSubstepContact` bisects the substep back to the
touching moment; below-surface centres fall back to world up.

#### 3. `len_extra` is `radius * 0.02`, and nothing wrote it

`FUN_004fbe60`, one line, last call in the loop:

    for i < nwheels:  len_extra[i] = wheel[i].radius * 0.02

`0x5934 + i*0x68` is the **wheel radius** — it is the `radius*0.01` in `suspRetract`,
the `dt*radius` extension rate, the `0.99` cap and the `0.9`/`0.3` in
`carSurfaceDrag`. So `len_extra` is a constant **1.44 mm** on the Overkill,
re-asserted every substep rather than integrated, which is exactly why it sits in
the 32-float ODE state with a zero derivative. It offsets the clearance
`suspRetract` deliberately leaves. Nothing in the port wrote it, so every spring
read 1.44 mm of slack.

#### 4. The engine puts a settled car to SLEEP

`FUN_004f59a0` keeps four timers; any control input resets them all and parks the
no-contact timer at 1999872.0 (`0x49f42400`):

| timer | condition |
|---|---|
| `phys+0x5c74` | time with `max(|v|, |P|)` below **0.3611111 m/s** |
| `phys+0x5c78` | time with `|w|` below **1.0 rad/s** |
| `phys+0x5c7c` | time since a contact was found (`FUN_004f5980` zeroes it) |
| `phys+0x5c80` | time accumulated while `0x5c7c < 0.1` |

`FUN_004f6610` then tests `0x5c74 > 2 && 0x5c78 > 2 && 0x5c80 > 1`, and if it holds
it **zeroes P, L, v and w** and returns nonzero — on which `carPhysTick` skips the
entire physics step, doing nothing but a `tol = 0.03` penetration test to keep the
contact timer alive.

Note the speed test picks the **larger of |v| and |P|**, which for a 2 kg car is
always |P| — so it trips at |v| = 0.18 m/s, not 0.36.

This is why the original can afford a slow extension rate on a parked car, and it is
what `rb_car.frozen` needed to hold the start line.

#### 5. MAX_IMPULSE

`FUN_004f5770`, top of every substep: `|P| <= maxImpLinear * mass`,
`|L| <= maxImpAng`. Loaded at `0x004f777d`/`0x004f7793` from section `MAX_IMPULSE`
(keys `maxImpAng`, `maxImpLinear`, plus `coeffPeople`); the image defaults and the
shipped settings agree on **20.0 m/s** and **1.0**. It does not rederive `v`/`w`, so
they stay one substep stale when it fires — reproduced as written.

#### What is still not transcribed

1. **The shell query.** `carCheckContact` passes `mode8 = 1`, setting a second
   radius of `0.2r` and flags `0x404` — a shell test, "near the surface but not deep
   inside", interpreted inside the engine's collision module. The port substitutes a
   solid test plus tolerances (`RB_CONTACT_TOL` 6 mm, `RB_PENETRATION_SLACK` 1 mm).
   Necessary because `suspRetract` parks each wheel only `radius*0.01` clear, so a
   strict solid test flips on and off under sub-millimetre motion.
2. **`carSubstepContact`'s bisection is not on the port's path**, so the port
   supplies `rb_body_depenetrate` — the positional half only, and only for the single
   deepest overlapping body sphere. This is the one remaining invented piece, and
   instrumented across all ten tracks parked and driving it is called **zero times**.
3. **`FUN_004f6730`** records each contact event into a 20-slot ring at
   `phys+0x4560` (wheel) / `+0x4e24` (body) with the point and its velocity, and
   `carPhysTick` accumulates impact magnitudes at `0x5c8c..0x5ca0` against
   thresholds of 0.8/1.0/2.0 and 3.0/5.0/7.0. That is damage and impact sound, not
   dynamics.
4. **The anti-stuck jiggle.** When a whole frame makes less than 5% progress,
   carPhysTick perturbs the state by `rand * 0.003` (and then by −0.015), retries the
   penetration test, and counts up to 5 before calling `FUN_00508b30(car, -1)`.

#### The ramp collapse — a parked car that never stopped shaking (HISTORY)

Kept because the reasoning is still worth having, but note the conclusion changed:
the ramp was never the cause of anything, and its writer is recovered (item 4 of
"carPhysTick, read end to end" cites the store at `0x004f6fd8`). What actually fixed
the parked shake was the advance ordering plus the rest clamp.

This was written as a missing *feature*. It was a live *bug*, and the
difference is that `carUpdateSuspension` only resets `phys+0x08`; nothing inside
it ever raises the value. Transcribe the resets alone and the ramp falls to 0 on
the first parked frame and stays there for the rest of the session — so the
extension rate is pinned at 1x forever, including at speed, where the
`speed >= SLOW_MPS` branch is meant to select 10x. `rbcar_init`'s 0.5 was being
overwritten one frame later.

The consumer's own range test says what the value is:

    if (t >= 0.0 && t <= 0.5)  rate += 2*t*(fast - rate);   else  rate = fast;

`0` is slow, `0.5` is fast, and **anything outside `[0, 0.5]` is also fast**. It
is a transient window the writer drives the value through, not a resting state —
so "pinned at 0" is not a conservative choice, it is the one value that is always
wrong.

Why slow extension oscillates rather than just lagging. A strut extending at
`radius/240` per substep covers **72 mm/s**. A body pitching at ~1 rad/s moves
its wheel mounts at **~145 mm/s** across the 0.149 m half-wheelbase. `len` cannot
track the geometry, so it lags — and a lagged spring length is a *negative
damping* term. Put that against `suspRetract`, which is fast and effectively
unbounded, and the loop is bang-bang: fast retract, rate-limited extend, around a
parked set point (`lenFree - 0.95*sag`) that sits 6 mm off the force equilibrium
(`lenFree - sag`, where `k_pos` puts it by construction). At the geometric target
the four springs carry 19.0 N against a 20 N weight, so the car sinks, retracts,
over-stiffens, rises, and extends slowly back — forever.

Measured on a tiltable plane, 10 s in, tilt peak-to-peak:

| slope | shipped | fixed |
|---|---|---|
| 0° | 0.00° | 0.00° |
| 1° | **5.16°** | 0.03° |
| 2° | 5.39° | 0.01° |
| 5° | 5.05° | 0.01° |
| 10° | 7.71° | 0.05° |
| 15° | 8.05° | 0.16° |

Past 15° the car rolls down the hill instead of holding — correct behaviour, not
instability: it reaches 3.4 m/s at 16° and 11.8 m/s at 26°, against `tan 15° =
0.27` and a grip of 0.70 plus the 0.202 N bearing term.

**Exactly-level ground is the one case that was always silent** (0.000°), because
all four corners stay in phase there and the cycle degenerates to a fixed point.
That is why the flat-ground verification passed, and why this needed a
parameterised synthetic plane to find: the beach spawn is a 23° slope the car
rolls straight off, so it never sits still there long enough to shake.

A note on the test that missed it. `rb_test.c` bounded the *mean* tail tilt error
and the *whole-run* worst |ω|; a steady oscillation about the correct attitude
passes both, and the suite was printing `settled tilt error = 3.05 deg` next to
"all checks passed". It now also bounds the tail **peak-to-peak** tilt (< 1°) and
the tail |ω| (< 0.5), which fail 7 times on the old behaviour. **A mean and a
global peak are not a stillness test.**

### The Jump action — TRANSCRIBED

`rccars_vita/collide.c`, `rb_car_jump` / `rb_car_reset_upright`.

Jump is one of the game's eight player input actions, not something the port
invented. `FUN_004e3110` binds them by name and index:

| index | 1 | 2 | 3 | 4 | 5 | **6** | 7 | 8 |
|---|---|---|---|---|---|---|---|---|
| action | Boost | Left | Right | Forw | Back | **Jump** | Brake | Reset |

The name strings are at `0x00570e6c`–`0x00570eac`, bound by
`FUN_004e48a0(player, name, device, index)` at `0x004e4183` onward.
`FUN_005029c0` then decodes a control-bit byte into the physics struct:

| bit | field | meaning |
|---|---|---|
| 0x40 | `phys+0x575c` / `+0x5760` | unmapped in the port |
| 0x80 | `phys+0x5764` / `+0x5768` | unmapped in the port |
| 0x01 | `phys+0x576c` / `+0x5770` | accel, throttle amount |
| 0x02 | `phys+0x5774` / `+0x5778` | brake, brake amount |
| **0x04** | **`phys+0x577c`** / `+0x5780` | **jump** |
| 0x08 | `phys+0x57ec` | drive blocked |
| 0x10 | `phys+0x573c` | boost |

#### carJump `0x004f3b80`

Called once per **frame** from `FUN_004f6b20`, which the per-frame car loop
`FUN_004f6ea0` runs *before* the physics tick at `0x004f72f0`. Only for
`phys+0x50 == 0`, the locally controlled player car.

    phys+0x6d4 += dt                          cooldown timer
    if (phys+0x6d4 < 0.5) return              0x554384
    if (!(phys+0x577c & 1)) phys+0x123a0 = 0  release clears the latch
    if (phys+0x123a0 || !(phys+0x577c & 1)) return
    phys+0x123a0 = 1                          latch: one hop per press

    up    = mat4GetRow1(car+0xf8)              the car's OWN up axis
    angle = vec3AngleDeg(up, (0,1,0))          0x00410130

    if (angle > 80.0) {                        0x554958
        if (carCheckPenetration(mode 1, tol 0.006, limit 0x32))
            FUN_00508600(car)                  right it
        phys+0x123a0 = 0                       retry allowed immediately
        return
    }
    if (carCheckPenetration(mode 0, tol 0.006, limit 0x32)) {
        f = (phys+0x54 == 1 ? 3.3000002 : 3.0) * mass   0x5549ac / 0x5543f4
        P    += up * f
        L[1] *= 1.5                            0x554824
        phys+0x6d4 = 0
    }

Notes:

- `phys+0x57f4` is the rigid body base, so `[+0x57f4]` **is the mass** — the
  `fmul DWORD PTR [eax+0x57f4]` at `0x004f3cd6` is not a position term.
- mode 0 is the **wheel** spheres and mode 1 the **body** spheres, so the hop
  needs a wheel down and the reset needs the shell down. Between roughly 80° and
  100° a four-wheel car balances on the outer edges of its wheels with no body
  sphere touching, and the reset declines.
- `vec3AngleDeg` = `acos(clamp(dot/(|a||b|), -1, 1)) * 57.2957763671875`, and
  `0x00410080` returns **0.0** for a degenerate length — so a zero-length axis
  reads as 90°, on the reset side of the test.
- The 80° comparison is `fcomp` + `test ah,0x41`, i.e. **hop when ≤ 80**.

#### carResetUpright `0x00508600`

    save the collision spheres (carGatherCollSpheres)
    zero v, w, P, L, force, torque
    pos = mat4GetRow3(car+0xf8)
    fwd = spline direction if the car has one (FUN_004873c0 / 0x00406660)
          else mat4GetRow2(car+0xf8)
    fwd.y = 0; renormalise in XZ; fall back to (0,0,1) = DAT_0055e9b0
    build a matrix from (pos, up = (0,1,0), fwd)   FUN_00408460
    push it into the node and the rigid body       0x0044c8e0 / 0x0044c8a0 / 0x00474f70

    up to 10 times:
        worldSphereQuery type 4, radius 0.5 at pos     radius field is +0x14
        if it misses, stop
        d = normalise(pos - hitPoint)
        pos += d * ((0.5 - |pos - hitPoint|) + 1e-06)

    re-gather the spheres; if any moved, FUN_004f6af0 (teleport bookkeeping)

The 0.5 m clearance sphere is **larger than the car** (0.42 m long), so on open
ground this lifts the body origin to 0.5 m and drops it — which is what a reset
looks like in the retail game. Ten passes so it also works out of a corner.

`FUN_004f6af0` → `FUN_00508b30` → `FUN_00508b50` is engine bookkeeping, not
dynamics: snap the follow camera (`camUpdate` with a 2e6 dt), re-baseline
`phys+0x5d2c` so the teleport is not counted as distance travelled, and poke the
AI path progress. The port reproduces only the camera snap.

#### The AI has its own auto-right

`FUN_004fe310`, from the AI per-frame path: if the up axis is more than **45°**
from world up *and* a body sphere is touching, accumulate `phys+0x43bc`; after
**2 s** call `FUN_00508600`. Not transcribed — the port has no AI cars.

#### Where 0x577c is also read

`FUN_004f59a0`, the rest-timer function, tests it at `0x004f59d1` alongside brake.
Holding Jump therefore keeps a parked car awake, which matters on every frame where
Jump produces no impulse (cooldown, latch, mid-air).

### The follow camera — TRANSCRIBED

`rccars_vita/cam.c`. The chain:

| address | name | role |
|---------|------|------|
| `0x4f9da0` | `physLoadCamera` | `Settings/Camera.crs` -> defaults at `0x014c4898` |
| `0x5007a0` | `camUpdate` | per frame |
| `0x500840` | `camRefFrame` | the car's position and basis to follow |
| `0x500a80` | `camSetupTargets` | copies the defaults, applies the pull-back |
| `0x471c00` | `camSetFollowers` | installs four descriptors at `cam+0,+7,+0xe,+0x15` |
| `0x471ca0` | `camFollow` | the placement |
| `0x4722c0` | `camFollowStep` | one follower |

Three quantities are tracked **independently**, each by its own follower:
horizontal distance, height, and yaw. That is where the camera's character comes
from — the yaw follower has a **±20° slack window**, so the view swings behind
the car instead of being welded to it.

#### The follower

Each descriptor is 7 floats, and `camFollowStep` returns an *offset*:

| slot | meaning |
|------|---------|
| `[0]` | rest value it is pulled back to |
| `[1]` | deadzone — inside it, no offset at all |
| `[2]`, `[3]` | how far below / above rest it may stray |
| `[4]` | pull-back acceleration |
| `[5]` | rate limit |
| `[6]` | easing exponent on the pull-back, 0 = linear |

    if |current - rest| < dead: no offset
    clamp current into [rest - slack_lo, rest + slack_hi]
    approach it at max_speed
    then pull back toward rest at accel, eased by exponent [6]

The distance follower is loaded with unbounded slack and an effectively infinite
rate (2e6), so it simply snaps to its rest value; the height follower gets
**±75% of defDistY** of give at `accDistY` per second; the yaw follower gets ±20°
at `accAlpha * 200` per second with an ease exponent of **2.0** — or a flat 30°/s
once the car is spinning faster than 180°/s.

#### The numbers, and what they confirm

`Settings/Camera.crs` with `physLoadCamera`'s own clamped affine maps:

| key | raw | value |
|-----|-----|-------|
| `defDistXZ` | 30 | **0.7929 m** behind |
| `defDistY` | 30 | **0.3636 m** above |
| `accDistY` | 1 | 0.1 /s vertical pull-back |
| `accAlpha` | 50 | 0.4949 -> 98.99 °/s yaw pull-back |
| `VisTurn` | 70 | **11.82°** of look-into-turn |

A camera 0.79 m behind and 0.36 m above is another independent confirmation of
the scale story: these are 1:10 models — a 0.5 m car on 0.072 m wheels doing
27 km/h. The port's previous ad-hoc chase camera sat at 3.2 m and 1.3 m, about
four times too far out, which is exactly what made the game look like it was
about full-size cars.

#### The pull-back

`camSetupTargets` widens the distance with speed and throttle:

    speed:    +0.5 * defDistXZ, ramped in between 4 and 8 m/s, nothing below 4,
              smoothed at 1.0/s   (phys+0x6e0)
    throttle: +0.2 * defDistXZ, approached at 0.5/s and released at 0.3/s
              (phys+0x6e4)

So flat out on the throttle the camera sits at `1.7 x defDistXZ` = 1.348 m. The
test harness reproduces that figure exactly.

#### Divergence

The original expresses the car in the camera's **previous** frame and rebuilds
incrementally; the second half of that (the matrix assembly inside `camFollow`)
is not transcribed. `cam.c` works in the car's frame instead, which is equivalent
while the camera trails the car — the case that matters — and is much easier to
read. The follower shape and every constant are the originals.

### Diagnosing three reported symptoms — the geometry was invented

Running it produced three complaints: the car *glided* on surfaces, the body
looked *static*, and the camera *floated* around the car. All three were real,
and two shared one cause.

#### The cause: invented geometry, not the model

`rb_data.h` was carrying made-up numbers for everything the `.crs` files do not
hold — body extents, wheel mount heights, the centre of mass. That is fatal
specifically for the friction solve, because the effective-mass matrix is

    A[i][j] = invMass*(t_i . t_j) + t_i . ((Iinv (r_j x t_j)) x r_i)

and with a lateral `t` the second term reduces to `rz^2*Iyy_inv + ry^2*Izz_inv`.
The **vertical lever arm `ry` from the centre of mass to the contact patch,
squared and multiplied by the roll inertia, dominates it.** Get the COM height or
the inertia wrong and the whole solve goes stiff: a stiff `A` means a small `f`
for the same demanded slip correction, and the car slides.

Measured, with a car shoved sideways at 3 m/s on flat ground:

| geometry | A[0][0] | friction force | result |
|----------|---------|----------------|--------|
| invented (extent 0.19x0.12x0.50, COM at body origin, contacts 0.18 m below) | 5.03 | 1.5 N | still doing 1.5 m/s after 3 s — *glides* |
| mesh-derived, COM on the wheel-centre plane, inertia from the real mass distribution | ~1.4 | 8.2 N | stopped inside 0.5 s, slid 0.83 m |

The same error suppressed the body's attitude response: yaw rate at 25° of steer
went from 0.19 rad/s to **0.93 rad/s** once the geometry was right. The body was
not static, it was over-damped by a wrong inertia tensor and starved of grip.

#### The real geometry, from Car.sb

`car_geom.py` reads it out of `RCCarsDB/Car.sb`. The LOD3 subtree is the one to
use — its wheel nodes carry plain unrotated translations, so no hierarchy
composition is needed. For Overkill:

| quantity | value |
|----------|-------|
| half-track | **0.1410 m** |
| half-wheelbase | **0.1490 m** |
| wheel centre | 0.053 above the model origin |
| body centre | 0.172 above the model origin |
| body extent | **0.140 x 0.106 x 0.422 m** |

The wheel mesh measures 0.136 x 0.144 x 0.074, i.e. a radius of **0.07** —
which matches `CdtRadWheel` = 0.0718 independently. Note `lenAxe` (0.089) is
*narrower* than the mesh half-track (0.141); they are different quantities.

Buggy and Hummer nest their wheel offsets in parent nodes, so their LOD3
translations are near zero and they currently reuse Overkill's figures. Composing
their hierarchies is outstanding work.

#### The centre of mass is the number that matters, and it IS recovered

`CenterMassOY` (cdt[42]) is the height of the rigid body's own origin — the centre
of mass — **above the MODEL origin**. `physLoadCdt` reads it at
`0x004f9010(0, 0.2, 1.0, 100.0, raw)`, a clamped 1..100 → 0..0.2 m remap, and
alone among the cdt keys takes **no extra ×0.1**; it lands at
`DAT_014c4da8 + car*0x2d*4`. Raw is **1 on the Overkill** — the bottom of the
slider, where the `.ini` default is 17, so somebody dragged it to the floor — and
17 on the other two: **0.0000 / 0.0323 / 0.0323 m**.

Two functions settle the meaning and the sign, and they are the original's own
version of the port's `glTranslatef(0, -com_oy, 0)`:

- `FUN_004f1150(phys, out)` hands it out as the vector `(0, CenterMassOY, 0)`.
- `FUN_00474f70(rb, phys+0xf8, v)` **adds** `v` rotated by the model's basis to the
  model matrix's translation and writes the result as the body's position — set
  state, model → body.
- `FUN_00475030(rb, out, v)` **negates** `v` first (`fchs` at `0x475078`,
  `0x475083`, `0x47508c`) and builds a model matrix out of the body — render,
  body → model. `FUN_004f66e0` is the caller that hands the result to
  `FUN_0044c8e0`.

**`phys+0xf8` is the MODEL matrix, not the body's, and that is the trap here.**
`FUN_0044c8e0` is the only writer (`0x44c8f7`) and it goes straight on to copy the
same 16 floats into the render node at `*(phys+0x10)+0xac`. So
`carGatherCollSpheres`, which builds every collision sphere as
`row0*X + row1*Y + row2*Z + pos` out of that matrix with `pos` at `phys+0x128`,
measures the **cdt offsets from the MODEL origin** — the com and the proxy sit on
two different origins with `CenterMassOY` between them, and only the com's moved.

The port acts on it: `gen_rb_data.py` emits `com_oy`, builds
`mount_y = wheel_y - com_oy + (len_free - sag)` so a resting wheel centre in body
space IS the mesh's own wheel node, and converts the body-sphere offsets by
`- com_oy`. `main.c` and `fx.c` draw at `-rbcar_com_oy(car)`. The com is
**57 / 29 / 39 mm lower** than the wheel-centre plane the port used to park it on;
the friction matrix goes as `ry^2 * Izz_inv`, so this is a handling change — see
"What CenterMassOY moved" below.

#### What CenterMassOY moved

Measured against a reference build with `mount_y` and the sphere frame put back,
everything else identical. Every harness passes on both.

Better, and the direction a lower com predicts:

| | before | after |
|---|---|---|
| `rockroll` car 1, roll at full lock | **8.28°** | **3.11°** |
| `allstarts` worst tilt while DRIVING, all ten tracks | 8.62–17.33° | **3.77–13.31°** |
| `allstarts` worst \|w\| while driving | worse on 9 of 10 | better on 9 of 10 |
| `curb` part 4, Overkill over a rock ≤ ½ wheel radius | 11.10° | **5.82°** |
| `curb` part 4, Hummer | 10.11° | **5.77°** |
| `flipped`, deepest shell penetration over ten tracks | 6.3% of a shell | **0.0%** (worst gap +0.018 m) |
| `wetcheck` ford, coasting vs the drag law's closed form | 376 mm/s out | **6 mm/s** |
| steady-state heave / pitch peak-to-peak | 0.5 / 0.30 mm | 0.2–0.4 / 0.11–0.29 mm |

Worse, and both are the same mechanism — an impact at chassis-sphere height now has
a much longer lever arm over a com 15–20 mm off the ground, so it goes into
rotation instead of into stopping the car:

- **The Overkill backflips off a wall.** `curb` part 1, any step ≥ 0.20 m: it used
  to stop dead against it (4.01 → 0.38 m/s in one frame, `w[0]` −0.65, settling
  nose-up at 25°); it now takes `w[0]` **−9.66 rad/s**, keeps 2.35 m/s, and pitches
  through 100° onto its back, ending 3–10 m *behind* where it started. The Buggy and
  the Hummer do not — the Hummer stops cleanly at every wall height.
- **The Buggy wheelies over a 20 mm rock**, `curb` part 4: 5.04° → **36.04°**, a
  clean threshold at 20 mm with a smooth plateau above it (36.04, 35.34, 34.59,
  34.10, 33.35 at 20–24 mm), not a knife edge. Bisected: it is `mount_y`, not the
  inertia term, which is worth about 1° of it. **Disabling that car's chassis body
  sphere removes it entirely** (peak `w[0]` −0.56, tilt 2.7°, and the suspension
  absorbs the rock as it did before), so the mechanism is that sphere and not the
  suspension.

That sphere is the port's own invented clamp: the Buggy's recovered `CdtDeltaY`
converts to **−0.0515 m**, below its own parked ground line, so `gen_rb_data.py`
clamps it to `ground + 1.2 * radius` and it hangs **10.2 mm** off the ground —
low enough to catch every 2 cm obstacle. Neither the clamp nor the single-Z-station
chassis proxy is recovered data, and both are the obvious next thing to measure;
raising them was deliberately NOT done here, because "where should the Buggy's
chassis sphere be" is a different question from "where is the centre of mass".

Cost is unchanged: `colprof` on beach_1 reads 116 sphere and 52 segment queries a
tick either way, mean 34 µs, and the per-position figures at the hardware log's own
worst spots move by less than 5%. The full sweep's single worst cell moved and is
1.5× higher there (190 → 281 µs, at a different position visiting different
geometry).

#### Inertia

The original's source for the inertia tensor is not recovered either. `rbcar_init`
now builds it from the geometry that *is*: half the mass as a box over the body
mesh extents, half as point masses at the wheel centres. Body-box-only — what it
did before — gives about a quarter of the real roll inertia, because it leaves out
wheels sitting at ±0.141 m. With `coeffMomentOZ` amplifying suspension roll torque
by 1.87 on top, that was enough to flip the car on a 10° slope.

#### The camera

Two things, one a genuine bug. `orbit_yaw` and `orbit_pitch` accumulated from the
right stick and were **never recentred**, so any nudge or stick drift left the
camera permanently off-axis — barely noticeable at the old 3.2 m, obvious at the
real 0.79 m. They now spring back to centre when the stick is released.

The rest is the game's own behaviour: the yaw follower has a ±20° slack window
and the height follower ±75% of `defDistY` with a pull-back of only `accDistY` =
0.1/s (raw 1, the minimum of its range). It is a deliberately loose camera. Note
that `camSetupTargets` has a **rigid alternative** — `DAT_0147d254`, toggled by a
debug key (0x78), replaces the descriptors with `distXZ = 1.04432`,
`distY = 0.409`, zero slack and no lag. If the loose one reads badly in motion,
that is the game's own tighter preset to switch to.

#### Still wrong: slope stability

A car placed on a 10° slope still tips over and goes airborne, and the contact
set flickers between diagonal wheel pairs on flat ground (`1010`, `0101`,
alternating). Flat-ground behaviour, grip, drive, top speed and steering response
are all correct; slopes are not. The flicker is the most likely culprit — with
only two contacts feeding the friction solve and *which* two changing every
frame, the lateral force direction is not stable. That is the next thing to fix,
and it is a suspension/contact problem rather than a dynamics one.

### The slope tumble — two spawn/probe bugs, and the real track now works

The symptoms persisted after the geometry fix, so I stopped guessing and drove the
car on the **actual `beach_1.col` grid** (`track.c`). That was the step that
mattered: the synthetic plane in the unit tests was hiding the problem.

What it showed: **the spawn point is a 23° slope** (normal
`-0.309, 0.918, -0.251`). The car tilted to 47°, flipped past 134°, and fell out
of the world. So the slope instability was never a side issue — the spawn *is* a
slope, so the car never had a chance to behave.

Two bugs, both mine, both in the spawn/probe path rather than the dynamics:

#### 1. `rb_wheel_buried` was far too eager, and `embedded` is not per-wheel

`buried.c` printed the probes per wheel at rest on the spawn slope:

    w0  upSeg=0 fatSphere=0  overlap=+0.00000
    w1  upSeg=1 fatSphere=1  overlap=+0.05684     <-- fires
    w2  upSeg=0 fatSphere=0  overlap=+0.00000
    w3  upSeg=0 fatSphere=0  overlap=+0.01208

One wheel 5.7 cm low trips the fat-sphere probe. And `embedded` is **not
per-wheel** — `carUpdateSuspension` applies it to the whole car, putting every
strut on the "push it out" branch instead of resolving contact normally. So a
single slightly-sunk wheel disables the suspension for all four, permanently, on
any slope.

`rb_wheel_buried` now additionally requires the wheel's own sphere to be
overlapped by more than **half a radius** — past what one `suspRetract` pass could
resolve. That keeps the original's intent (the wheel is genuinely inside geometry)
without firing for a wheel merely riding low. Marked as a divergence: the
original's probes go through the engine's collision module, whose semantics are
not transcribed.

#### 2. The spawn placed the car flat on a slope

`rbcar_init` put the car at one ground height with its up axis along world up. On
a 23° slope that buries the uphill wheels and leaves the downhill ones floating —
exactly the state above.

`rb_world` gained an **optional** `ground` callback (highest surface at (x,z) plus
its normal), used only for placement and explicitly not part of the transcribed
model — the original places cars from the track's start markers. `rbcar_init` now
tilts the body to the surface normal and lifts it until no wheel is inside the
ground. `col.c` backs it with the `col_ground_at` the port already had.

#### Result on the real track

| | before | after |
|---|--------|-------|
| spawn tilt | 0° (flat, on a 23° slope) | **23.43°**, matching the slope |
| tilt after 1 s | 47° | 31° |
| tilt after 2 s | 134° — flipped | 20° |
| after 6 s | y = −18, falling away | on the ground, driving |

The car now stays on the terrain, follows it, and drives under throttle.

#### Still not right: a car parked on a sustained incline

`slope.c` parks a car on an infinite 10° plane with no throttle. Even with an
aligned spawn it eventually tumbles. On the real track this does not show up
because the terrain varies and the car is moving, but it is a genuine remaining
weakness and it is the same family as the earlier findings: the contact set feeds
only two wheels into the friction solve and *which* two flickers, so the lateral
force direction is not stable over long steady-state contact.

`track.c`, `slope.c` and `buried.c` are kept in `rccars_re/` as repro harnesses.
`track.c` in particular is the one to reach for first next time — it exercises the
real collision grid, which the unit tests do not.

### The symptoms survived three rounds of physics fixes — because the renderer was never switched over

Worth recording as a process failure, not just a bug.

The reported symptoms ("body static", "glides") persisted through the geometry
fix, the buried-probe fix and the spawn-alignment fix. The physics was measurably
improving each time — the harnesses proved it — and the app looked identical.

The cause: **`main.c` was still drawing the car from `veh.yaw` / `veh.pitch` /
`veh.roll`, and the transcribed path only ever fills `yaw`.** `pitch` and `roll`
stayed zero, so the body rendered dead level no matter how the car was actually
lying. A car whose position moves and whose attitude never changes looks exactly
like something gliding.

It should have been `glMultMatrixf(rb_car.m)`, which I had written and believed
was in place. It was not: the edit was applied with a string replace whose
indentation did not match (8 spaces against the file's 12, inside
`if (car.n_batches)`), so it **silently did nothing**. Two other edits to the same
file failed the same way — the physics-model toggle, and its button binding, which
had also collided with SELECT (already the free-fly camera).

Lessons, both cheap:

- **Verify every edit landed.** A `grep` for the new text costs nothing. Three of
  my `main.c` edits silently no-oped and I did not check any of them.
- **Instrument the target, not just the harness.** The unit tests and the
  `track.c` harness were both green while the app was visibly wrong, because they
  exercise the physics and the app also has a renderer. `main.c` now logs body
  tilt, per-wheel contacts, spring lengths in mm and the embedded flag once a
  second, so the next report arrives with numbers.

Controls are now: TRIANGLE swaps between the transcribed model and the
`physics.c` placeholder (SELECT remains the free-fly camera).

### Why the transcribed model is NOT the app's default

It is off by default. `physics.c`, the arcade placeholder, drives; the transcribed
model is on TRIANGLE. That is a deliberate retreat after four rounds of "still
wrong" from actually running it, and the reasons are specific.

#### What is solid

Measured, in isolation and on flat ground, and all still passing:

- the integrator, state marshalling and force summation
- static ride height landing on the closed-form equilibrium
- top speed 6.94 m/s against a computed limiter of 6.90
- lateral grip stopping a 3 m/s sideways shove in 0.5 s (~5 m/s²)
- yaw response, tire grip, drift cut, engine curves, reverse taper
- the camera, at the recovered 0.79 m / 0.36 m with its ±20° yaw slack

#### What breaks, and where exactly

On the real track the body gets spun up. Instrumenting the torque directly:

    |tau| = 1.530 Nm (constant) -> |alpha| = 62 rad/s^2
    |w| = 0.00 -> 0.86 -> 1.44 -> 2.03 -> ... -> 7.93 rad/s over twelve frames

A torque that never reverses, so no equilibrium. The cause is visible one column
over: `len = 110 98 110 110`, `act = 0100` — **only one wheel in contact.** One
wheel gives a quarter of the support force and a permanent moment about the centre
of mass.

That traces to the spawn. Three attempts, all recorded because each failed
differently:

| attempt | result |
|---------|--------|
| place flat at one ground height | uphill wheels buried, downhill floating — the original symptom |
| align to the normal, lift until no wheel is inside | one wheel touching, three hanging — the runaway torque above |
| align, then set each strut from its own patch of ground | struts at 83/31/218/61 mm, one jammed at lenMax |

The third failure is the informative one: **at this track's spawn the ground under
the four wheels varies by more than the 0.098 m of suspension travel**, so no rigid
placement can load all four. The spawn is a 23° slope beside a pier edge. It is a
genuinely hard spot, chosen originally *because* it showed off body attitude.

The current code takes the average ground height with all struts at rest length —
symmetric, not exact, and the suspension solve is left to absorb the residual.

#### A related fix that did land

`carUpdateSuspension` abandons its whole pass on the first wheel it cannot resolve
(`stuck = 1; break;`). Reproducing that left two of four wheels holding stale
lengths for 150+ frames while the body was thrown between 12° and 176° of tilt.
The port now solves every wheel and still reports `stuck`. Marked as a divergence:
the original can afford the early exit because its body-sphere contact solve
catches what the struts cannot, and that solve is not transcribed.

#### What I would do next

1. **Pick a flat spawn** and confirm the model is stable there. That separates
   "the model is wrong" from "the spawn is impossible", and I never did it.
2. Then transcribe the **body-sphere contact solve** (`carContactSolve`'s normal
   path over the 9 body spheres). It is the backstop the port is missing, and
   three separate problems have now been traced to its absence — the bottomed-out
   normal stop, the substep gating, and the stuck early-exit.
3. ~~Recover **`CenterMassOY`**~~ — **done**, and acted on. See "The centre of
   mass is the number that matters" above and "What CenterMassOY moved" below.

### The real cause: one Euler step per frame is unstable in roll

The planned next step was to instrument which suspension branch each wheel takes
at 1 degree versus 2 degrees of slope. It **disproved the hypothesis** — no
`stuck`, no `embedded`, no segment rejects, every wheel on a healthy
`clear:extend` / `touch:retract-ok` path — but the trace showed the real thing: a
left-right rocking oscillation at about 7.5 Hz that never damps.

Five A/B tests then eliminated every force term in turn, and each removal made it
*worse*: the drive torque, the drive application point, engine braking, the second
suspension pass, the friction force, the damper clamp, the extension rate limit
(faster extension is worse), and applying the corner forces at the wheels instead
of via the COM-plus-couple decomposition (better, still unstable). That pattern —
"take anything away and it degrades" — says the force balance is roughly right and
the **integration** is at fault.

The timestep sweep settled it:

| substep rate | 2 deg slope: worst tilt error | worst \|w\| |
|--------------|------------------------------|-----------|
| 1/60 | **31.1 deg** | **12.3 rad/s** |
| 1/240 | 6.4 deg | 1.8 rad/s |
| 1/960 | 5.7 deg | 1.8 rad/s |

The roll mode is unstable at one explicit Euler step per 60 Hz frame, and bounded
at 240 Hz and finer. `rb_car_tick` capped substeps only by conservative
advancement -- sphere travel -- which never subdivides a slow-moving car, so a
stationary car on a slope took a single 16.7 ms step.

**Exactly-flat ground was always stable because nothing excites roll there.** That
is why the earlier flat-spawn test looked clean and every sloped case failed, and
why "it destabilises above 1 m/s" was wrong: the faster runs simply travelled off
the flat patch sooner.

#### The fix

`RB_MAX_SUBSTEP` (1/240 s) with `RB_MAX_SUBSTEPS` (8). A 60 Hz frame becomes four
substeps, a 30 Hz frame eight. This is a **divergence** from the original, which
caps only by sphere travel and allows four substeps; it is recorded as such in
`rb.h`.

After it, on a perfect plane at 60 Hz:

| slope | worst \|w\| | settled tilt error |
|-------|-----------|--------------------|
| 0.5 deg | 1.63 | 3.05 deg |
| 2 deg | 1.65 | 2.93 deg |
| 5 deg | 4.31 | 1.17 deg |
| 10 deg | 5.60 | 2.47 deg |
| 20 deg | 9.34 | 4.83 deg |

Every angle settles or decays instead of diverging. This is now a regression test
in `rb_test.c` (the harness's plane is tiltable), and `sweep.c` reproduces the
whole table in one run.

#### What is still not right

Real terrain is harsher than a plane. On `beach_1`:

- the verified-flat patch at (-1.64, 40.37): stationary settles to 1.9 deg with
  \|w\| 0.94; under throttle the car drives 2.6 m, meets the 2.3 m wall at
  z = 42.5, and **rests against it stably** (tilt 21 deg, \|w\| 0.04)
- a spot whose normal is 2.5 deg off vertical: still reaches 26 deg of tilt

So the substep fix converted divergence into a bounded wobble, and closed the
synthetic-plane case, but real triangle terrain still excites more than the model
absorbs. The transcribed model therefore stays **off by default**.

#### Two facts about the map worth keeping

`findflat.c` and `profile.c` measured the terrain the car actually drives on:

- the **largest area on beach_1 flatter than 1 degree is about 2 m across.** It is
  a beach — dunes and slopes throughout. A flat spawn is not a workaround.
- 2 m north of the flat patch the ground **steps up 2.3 m vertically** (from 1.200
  to 3.511) — a wall. The port has no wall collision, so the body spheres do
  nothing there; that is a pre-existing known issue, now shown to be what the car
  runs into.

### The camera did point at the car — it just ended up behind the wrong part of it

Reported as "camera doesn't point at the car, especially when turning". Measuring
the angle between the view direction and the eye-to-car vector (`camaim.c`) gave
**0.00 degrees at every frame**: the aim is exact by construction, since the eye is
placed at `car - forward(cam_yaw) * dist` and the view runs along
`forward(cam_yaw)`.

The fault was in *where* behind the car it sat. Through a sustained full-lock turn:

| after | camera yaw | car yaw | trail |
|-------|-----------|---------|-------|
| 1 s | 151.8 | 122.0 | **30 deg** |
| 2 s | 22.5 | -22.5 | **45 deg** |
| 4 s | -109.5 | -178.1 | **69 deg** |
| 5 s | 118.5 | 24.6 | **94 deg** |

The trail grew without bound, so the camera slid round to the car's flank and then
its nose — pointing straight at it the whole way, from entirely the wrong place.

Two causes, both consequences of the car-frame divergence:

1. **The slack window was never enforced.** `camFollowStep` returns an offset
   limited by its own `max_speed` (`accAlpha * 200`, about 99 deg/s), and when the
   value is outside the slack window the pull-back block is skipped entirely, so
   only that rate-limited step applies. In the original that is enough, because
   `camFollow` rebuilds the camera relative to its PREVIOUS frame — the geometry is
   already converging and the follower merely damps it. In the car's frame the
   follower is the only thing that tracks at all, and a car at full lock yaws far
   faster than 99 deg/s (a 0.3 m wheelbase at 6 m/s gives several hundred). The
   +-20 degree slack is the original's own statement of how far the view may
   trail, so it is now applied as a hard bound.
2. **VisTurn had the wrong sign.** Positive steer makes the app-convention yaw
   decrease (measured), so leaning into the corner subtracts. Adding it put the
   11.82 degree lean and the 20 degree slack on the same side of the car: 31.8
   degrees off the tail instead of 8.18.

After both: a steady **8.18 degree** trail through a 7 s full-lock turn (20 - 11.82,
exactly as intended), with a worst aim error of **0.028 degrees**. Regression test
in `rb_test.c`; `camaim.c` reproduces the table.

### The model was rendered backwards, not the camera aimed wrongly

Reported as "ok when the car reverses, looks somewhere else going forward". That
phrasing is the whole diagnosis: a 180 degree error that cancels itself in reverse.

Measured (`convention.c`), after driving forward under throttle:

| quantity | value |
|----------|-------|
| `dot(velocity, body +Z)` | **+0.998** — the car drives along its local +Z |
| mesh nose | +Z (front wheels at z = +0.149, rear at -0.149) |
| camera position | **behind** the +Z end — correct |
| rendered nose **without** the half turn | dot with velocity **+0.999** |
| rendered nose **with** the half turn | dot with velocity **-0.999** |

So the camera was right and the car was drawn facing backwards. In reverse the
visible nose does point the way the car is going, which is why that case looked
fine.

The cause: the rb render path had a `glRotatef(180, Y)` copied from the placeholder
path. **The two models do not share a forward convention.** `vehicle_t` travels
along -Z at heading 0, so its render needs the half turn to line the mesh up; the
rigid body travels along its local +Z, and the mesh nose is +Z as well, so they
already agree and the half turn must not be applied.

Removed. `rb_test.c` now asserts both halves of the convention — that the car
drives along body +Z, and that the front wheels are the +Z pair — so a change to
the wheel layout cannot silently flip it again.

Worth noting this is the third bug in this port traced to blindly reusing the
placeholder's conventions: the earlier ones were the renderer never being switched
over at all, and the camera's VisTurn sign.

### Steering was inverted: +X is the car's LEFT

Reported as "car left/right inverted to camera left/right". Measured
(`handed.c`): a stick input of **+1.0 (right) turned the car LEFT** by 16.9 degrees
over three seconds.

The chain:

- `carWheelFrame` builds the wheel's rolling direction as local +Z rotated about
  local +Y by the steer angle, and a positive rotation about +Y carries +Z toward
  **+X**.
- **+X is the car's LEFT.** The mesh says so directly: `WHEEL_FRONT_LEFT` sits at
  x = +0.141 and `WHEEL_FRONT_RIGHT` at x = -0.141. (Consistently,
  front x left = +Z x +X = +Y, so front/left/up is a right-handed triple, and
  right = front x up = -X.)
- So a positive body steer angle points the wheels left, and feeding the stick
  straight through made the car steer away from it.

Fixed at the input boundary: `rbcar_step` negates the stick to get the body angle.
The body angle's own meaning is untouched, so `carGatherContacts`' outside-wheel
choice on `c->steer >= 0.01` still means what the original meant.

The camera's VisTurn sign then had to go back to **+**: with the stick fixed,
positive stick is a right turn, a right turn increases the app-convention yaw
(forward = `(sin yaw, 0, -cos yaw)`, so yaw 0 is -Z and yaw 90 is +X, and seen from
above increasing yaw sweeps to the right), and leaning into the corner means
leading in that direction. Trail stays at 8.18 degrees, aim error 0.028.

`rb_test.c` now asserts both directions turn the right way and that the body angle
carries the opposite sign to the stick.

### The renderer's yaw convention is MIRRORED, and that was the root of the whole family

Reported as "camera rotation inverts to car turns". This one turned out to be the
cause behind several of the earlier symptoms, and finding it required abandoning an
assumption I had been testing against rather than testing.

The view matrix main.c builds is `Rx(pitch) . Ry(-yaw) . T(-eye)`, and a camera
looks along its own -Z. Working that through GL's actual `glRotatef` matrix gives
the world direction the camera faces:

    F(v) = (-sin v, 0, -cos v)

**Not** `(+sin v, 0, -cos v)`, which is the port's old `vehicle_t` convention and
what `cam.c` had assumed. The two agree only at yaw 0 and 180 -- which is exactly
why the view was fine pointing straight ahead and fell apart in a turn.

Measured by building the real GL matrices (`eyespace.c`) and transforming the car
into eye space, mid-turn:

| | before | after |
|--|--------|-------|
| car eye-space position | `(-0.762, -0.543, +0.402)` — **behind the camera** | `(0.000, 0.000, -1.019)` |
| a marker off the car's right | eye x **-1.236**, i.e. screen-LEFT | eye x **+0.98**, screen-right |

#### Why my earlier "verification" missed it

The previous camera test computed the view direction from an assumed formula
(`vd = (sin y cos p, -sin p, -cos y cos p)`) and reported a **0.028 degree aim
error** for a camera that was putting the car behind itself. The test encoded the
same wrong assumption as the code, so it agreed with it. The test now builds
`Rx . Ry . T` from the same numbers `glRotatef`/`glTranslatef` would and checks
three things directly: the car's eye-space z stays negative, its eye-space x stays
near zero, and a marker off its right stays on screen-right.

#### The fix, and the sign cascade

- `cam.c`'s `yaw_forward` is now `(-sin, 0, -cos)`, and `car_yaw_deg` /
  `rbcar_yaw_deg` return `atan2(-m[8], -m[10])` — the view yaw that faces the
  car's local +Z.
- main.c places the eye at `car - F(h)*dist`, i.e. `car + (sin h, 0, cos h)*dist`.
  Note the **+sin**; the placeholder path keeps the other convention.
- In this convention a right turn **decreases** yaw (v=0 is -Z, v=90 is -X, so
  increasing v swings left), so VisTurn's lean subtracts. Verified rather than
  reasoned: trail back to 8.18 degrees.

Steering was unaffected -- stick right still turns right -- but three test
expectations had been written against the old convention and had to be corrected,
including one that carried its own inline copy of the old `atan2(m[8], -m[10])`.

#### The lesson, since this is now the fourth of its kind

Four bugs in this port have come from reusing the placeholder's conventions
without checking: the renderer never being switched over, the model's half turn,
the camera's VisTurn sign, and this. The through-line is that `vehicle_t` and the
rigid body disagree about which way is forward and which way yaw runs, and every
piece of glue between them is a chance to inherit the wrong one. Where a convention
matters, **construct the actual transform and measure a point through it.**

### Decompiled sources on disk

| file | contents |
|------|----------|
| `rb_core.c` | engine rigid-body layer: inertia, quat/matrix, force couple |
| `rb_helpers.c` | point velocity, matrix multiply, axis accessors |
| `forces.c` | the whole force pipeline including the friction solve |
| `contact_helpers.c` | point accel, 2x2 solve, rotation, surface/water probes |
| `curves.c` .. `curves4.c` | the spline object model and its five evaluators |
| `setup.c` | `carSetupSprings` -- where kPos becomes a stiffness |
| `rest.c` | the move-towards helper used as the rest damper |
| `collision.c` | the sphere proxy, the queries and their three thunks |
| `suspupd.c`, `suspstep.c` | `carUpdateSuspension` and its retract/extend/buried helpers |
| `curves2.c`..`curves4.c` | the curve constructor, bracket search and importers |
| `camera.c`, `camera2.c`, `camera3.c` | the follow camera and its follower |
| `ani.c` | `carAniProc1` and its setup, plus the LOD dispatch around them |
| `wheelspin.c` | `carWheelSpinUpdate`, `carWheelFrame` and the matrix helpers |
| `physics.c` | the config loaders (unchanged) |
| `integrator.c`, `integrator2.c` | the **renderer** cluster; misnamed, kept for the record |

101 functions are named and commented in the Ghidra project.

## The car's rig — RECOVERED

The physics moves a body. The **visible** car is articulated on top of it, and
all of it comes out of state the transcribed model already carries. Transcribed
into `rccars_vita/carani.c`, with the geometry side in `pack_vsc.py --rig`.

`carVisualUpdate` reaches it through a LOD dispatch (`0x00503f00` ->
`0x00504780`), which picks one of three procs by car index:

| car | create | init | update | node table |
|-----|--------|------|--------|------------|
| Overkill | `carCreateAniProc1 0x00504100` | `0x00504470` | **`carAniProc1 0x00504820`** | `0x00572fcc` |
| Buggy | `0x00505150` | `0x005053d0` | `0x00505780` | `0x005731b8` |
| Hummer | — | — | `0x005068e0` | `0x005731b8` |

Only proc 1 is transcribed; the other two drive a different node set
(`AXLE_FRONT_LEFT_UP`/`DOWN`/`WHEEL`, `Spring<n>_<m>`) and the Buggy and Hummer
meshes carry those nodes instead. Note the LOD argument: `2` skips animation
entirely, and at LOD1 the front WHEEL nodes double as the steering knuckles.

### What moves

`carInitAniProc1` snapshots each node's **local** matrix (`node+0xac`) into the
ani context; `carAniProc1` restores that snapshot every frame and pre-multiplies
rotations onto it. Four things move:

| node | rotation | source |
|------|----------|--------|
| `AXLE_FRONT_LEFT/RIGHT_SUPPORT` | `rotY(steer)` | `phys+0x5c6c`, degrees — the same steer the tire forces use |
| `WHEEL_*` | `rotZ(spin)` | wheel record `+0x04`, radians |
| `FRONT_AXLE` / `REAR_AXLE` | `rotZ(-pitch)` then `rotX(±tilt)` | the two wheel-centre heights on that axle |
| `SPRING_*` | aim at a point on its axle, then stretch | the axle's animated matrix |

**The rotate helpers pre-multiply.** `FUN_0040c940` mode 1 is
`mat4Mul(rot, m) -> m`, so a rotation acts in the node's own frame *before* its
rest transform. And their axes, read off the matrix elements each one writes:
`0x0040ca30` = X, `0x0040cab0` = Y, `0x0040cb30` = Z. The wheels turn about **Z**
because their node frames are rolled — `WHEEL_FRONT_LEFT`'s local +Z maps to
model +X, which is the axle line.

### The axle angles

Both wheel heights are taken in **body space**, as `mount.y - len - len_extra`
(`carAniProc1` gets them by calling `carWheelFrame` through the `0x004ef9b0`
thunk and pushing the result back through the car's inverse matrix, which is an
exact round trip to that expression). Then, with `yl` the LEFT wheel:

    tilt  = atan2((yl - yr) * 4.0, 1)
    pitch = atan2(((yl + yr) * 0.5 - ref) / lenAxe, 1)     clamped to [-30, 90] deg

`lenAxe` is **`springs[11]`** — 0.0894 m on the Overkill, and *not* the mesh
half-track (0.141). The `4.0` is the original's own constant, close to
`1/(2 * half_track)` for this car.

`ref` is `phys+0x5d48`, which is the ani context's **first entry**: the rest
position of `WHEEL_FRONT_LEFT` in body space, put there by `carCreateAniProc1`.
In the port that is the wheel's static rest centre, `mount_y - (len_free - sag)`,
which `gen_rb_data.py` makes exactly 0 by putting the centre of mass on the
wheel-centre plane.

The front and rear clamp *different* quantities — the rear clamps `-pitch` to
`[-90, 30]` and the front clamps `+pitch` to `[-30, 90]` — which is the same
window on the raw angle, so one clamp covers both. The tilt signs really are
opposite, though: the two axle node frames are mirrored.

### The springs

`SpringLength` comes from `Settings/WheelsVisual.crs` (raw **77**, converted by
`carInitAniProc1` as `(raw-1)*0.0009090909 + 0.01` = **0.0791 m**). At init each
spring's aim point is `origin + SpringLength * spring_local_Z`, stored in its
**axle's** frame; `SPRING_REAR_1..4` bind to `REAR_AXLE` and `SPRING_FRONT_1..4`
to `FRONT_AXLE`. Each frame the spring rotates its local +Z onto that point and
scales Z by `reach / SpringLength`, so it visibly extends as the suspension
droops. `carAniProc1` writes the result through `FUN_00503cb0`, which is
`local = world * parent_world^-1` — that is how the world write and the
subsequent local-matrix scale compose instead of clobbering each other.

### The spin integrator — `carWheelSpinUpdate 0x004ef280`

Called once per **frame** from the car's frame update (`0x004f6ea0`), after the
physics, not per substep. Per wheel:

    patch  = wheel centre - body_up * radius        NOT the centre
    target = dot(roll_dir, pointVelocity(patch)) / radius
    rate   = moveTowards(rate, target, grounded ? 200 : 5, dt)
    angle  = wrap(angle + (rate + extra) * dt, -2pi, 2pi)

`grounded` is one flag for the whole car, so a single lifted wheel still decays
at 200 rad/s²; only a fully airborne car keeps its wheel speed. The angle wrap
(`0x0040c6d0`) is `x - floor((x-lo)/range)*range`, which works in both
directions — a reversing wheel does not stick at the low end.

`extra` is wheelspin under power or lock-up under brakes, `±tire[12]` blended out
between 0 and 4 m/s. **`tire[12]` is `SpeedAngMaxREL`, and it is 0 in the retail
game**: `physLoadTires` reads it with the float getter `0x00404950`, which leaves
its target untouched when the key is absent, and no shipped `tires<n>.crs`
carries it. The whole block is dead unless someone edits the config. Transcribed
anyway, and `gen_rb_data.py` emits the zero explicitly rather than by omission.

Two deliberate divergences in the port, both marked in `collide.c`:

- an airborne wheel's contact-patch velocity. The original substitutes a global
  vec3 (`DAT_0070c658`) that nothing on this path ever writes — an engine scratch
  triple shared by a dozen unrelated helpers. The port uses zero, which is
  indistinguishable at the 5 rad/s² airborne rate.
- the port reuses the contacts the substep already gathered instead of running a
  second `carGatherWheelContacts` pass. Same set, same frame, one fewer full
  collision sweep.

### Left and right: the port's wheel 0 is mirrored

`carAniProc1` hardcodes wheels 0/1 and 2/3 as the front and rear pairs and takes
`y[0] - y[1]` as the tilt, which works because **the original's wheel 0 is the
+X (left) one** — `carSetupWheelMounts` reads `WHEEL_FRONT_LEFT` for index 0, and
`carWheelFrame` pushes index 0 toward +X by `cdt[15]`.

`rbcar_init` lays wheel 0 out at **-half_track**, i.e. mirrored. That is
dynamically harmless for a symmetric car and has never shown, with one real
exception worth recording: `rb_gather_contacts` picks the outside wheel for the
friction solve on the sign of `steer`, so on the port it picks the mirror of the
wheel the original would. Left alone for now — it is unrelated to the rig and
changing it would perturb the slope behaviour that is currently settled.

`carani_bind` sidesteps it entirely by resolving the rig from the wheel **mount
positions**, not the wheel index: the sign of `mount.x` names the side and the
sign of `mount.z` the axle. Feeding the axle tilt the indices instead rolls the
axle away from the wheel that dropped — which is what it did on the first run,
and what `rb_test.c`'s part 5 now guards.

### Verified

`rb_test.c` part 5 drives the **real exported rig** out of `assets/car1.vsc`
rather than a synthetic one, because everything interesting here (which local
axis is the axle, which way the axles are mirrored, the 1.05 scale baked into the
node chain) lives in that data:

| check | result |
|-------|--------|
| unmoved rig draws as the identity | worst deviation 1.2e-07 |
| drawn steer angle vs body steer angle, -30..+30 | 0.000° error |
| rear wheels under steer | 0 m |
| quarter turn of spin | 90.00°, tread radius unchanged, axle fixed |
| axle roll under a 50 mm one-sided droop | dropped side -19.8 mm, other +19.8 mm |
| even droop | pure pitch, 0 roll |
| spring at 60 mm droop | stretches to 115%, body anchor fixed |
| wheel rate rolling at 3 m/s | 41.77 rad/s against v/r = 41.77 |
| wheel rate 0.5 s after takeoff | 39.27 rad/s, still turning |

## What porting this actually needs

1. ~~Decode `.bspl` splines~~ — DONE.
2. ~~Recover each loader's scale constant~~ — DONE.
3. ~~Find and transcribe the integrator~~ — **DONE, all of it.** The rigid body
   and integrator are in `rccars_vita/rb.c`, the friction solve, tires, engine
   and surface drag in `contact.c`, and the collision proxy and suspension solve
   in `collide.c`. All verified numerically (see below). The only interface left
   is three world-query callbacks the host supplies.
4. ~~Reconcile x87 vs ARM VFP precision~~ — DONE. The premise was wrong: the
   game does not use 80-bit intermediates.

## Slider -> physical conversions — RECOVERED

`scale_consts.py` recovers them from the decompiled loaders and writes
`scales_table.py`; `dump_physics.py` then prints real quantities instead of
slider integers. **All 46 keys now resolve.** The last two were `kPos`, which is
not a stiffness at all (see above), and `CenterMassOY` — which resolved but then
sat unused for a year because nothing had asked what its target was measured FROM.

Note that three keys are **relative**, not absolute: `lenMin` and `lenMax` are
fractions of `length`, and `kPos` is a fraction of `length - lenMin`. The loader
seeds each slot from the earlier value and multiplies, which is invisible if you
only read the affine part of the expression.

Every conversion is affine:

    value = (raw - base) * scale + offset

but it appears in the loaders in **two different shapes**, and handling only
the first leaves a third of the keys unresolved:

1. **Inline arithmetic** with float globals:
   `fVar1 = (v - _DAT_00554390) * _DAT_005549d4 + _DAT_00554424;`
   Resolve each `_DAT_` by converting its VA to a file offset via the PE
   section table and reading the float.

2. **A linear-remap helper** taking IMMEDIATE float bit patterns:
   `FUN_0040ba70(0x3f800000, 0x40400000, 0x3f800000, 0x42c80000, v)`
   = `(1.0, 3.0, 1.0, 100.0, v)`. Decompiling the helper confirms the argument
   order is `(outLo, outHi, inLo, inHi, x)` returning
   `(x - inLo) * (outHi - outLo) / (inHi - inLo) + outLo`.

Two parsing traps: the decompiler reads the key into one local but refers to a
*different* local in the conversion (the float reinterpretation of the same
stack slot, e.g. `local_5c` vs `local_58`) -- so do not match on the variable
name. And a few keys are pure identity, `(v - base) + base`.

Nearly every scale is `RANGE/99`, i.e. the 1..100 slider span normalised, so
the conversions are really just "map slider onto this physical range".

### Car 1 (Overkill), converted

| key | raw | physical |
|-----|-----|----------|
| speedBaseMax | 27 | 27.0 **km/h** = 7.5 m/s |
| speedBoostMax | 35 | 35.0 **km/h** = 9.7 m/s |
| CoeffFrontTires / Rear | 7 | 0.70 |
| coeffAirResistance | 33 | 0.0323 |
| coeffFrictionBearings | 3 | 0.202 |
| coeffDeepSand | 7 | 0.0606 |
| coeffWater | 20 | 0.192 |
| coeffDriftRear | 25 | 0.218 |
| coeffMomentOZ | 44 | 1.869 |
| coeffMomentOX | 37 | 1.045 |
| lenAxe | 27 | 0.0894 |
| length (spring) | 43 | 0.218 m |
| lenMin | 1 | 0.1 x length = 0.0218 m |
| lenMax | 1 | 1.1 x length = 0.2397 m |
| kPos (static sag) | 64 | 0.609 x (length - lenMin) = 0.1194 m |
| kSpeed | 36 | 4.182 |
| touchPoint | 11 | 0.101 |
| AngleSteer | 30 | 29.29 deg |

These corroborate independently: tyre grip 0.70 is a normal coefficient of
friction; `lenAxe` 0.089 m sits sensibly inside the car's 0.19 m half-width
measured from the mesh; the sag of 0.119 m on a 0.218 m spring is soft but right
for an RC buggy, and the simulation settles to it (0.1186 m measured); and the
27 km/h top speed is what a 1:10 scale model actually does.

## x87 vs ARM precision — RESOLVED (and my earlier premise was wrong)

I had assumed all along that x87's 80-bit intermediates would be the hard
problem. Measurement says otherwise.

### 1. The x87 runs at 53-bit, not 64-bit

The exe loads control word **0x27F** (9 sites, 42 `fldcw`/`fnstcw` total). In
the x87 control word, PC is bits 9:8; `0x27F` gives `10` = **53-bit double
precision**, not the hardware default `0x37F` = `11` = 64-bit extended. This is
the standard MSVC CRT setting.

So intermediates round exactly like IEEE `double`. There are no 80-bit
intermediates to reconcile.

### 2. State is float32 everywhere

Instruction mix over the physics region `0x4f7000`-`0x50f000`:

| instruction | count | meaning |
|-------------|-------|---------|
| `flds` / `fstps` | 1606 / 758 | load/store **float32** |
| `fmuls` `fsubs` `fadds` `fdivs` | 528 / 340 / 229 / 49 | arithmetic with float32 memory operands |
| `fldl` / `fstpl` | 92 / 12 | float64 — rare |

Vehicle state lives in float32 and is rounded back on every store, so error
cannot accumulate across frames beyond float32 rounding. Only within-expression
intermediates are wider.

### 3. Therefore the port rule is simple

> Evaluate intermediates in `double`, keep state in `float`, and preserve
> operation order.

That is bit-identical to x87 at PC=53 storing to float32. Remaining differences
are confined to the exponent range (x87 keeps 15-bit exponents even at PC=53),
which matters only at overflow/underflow — irrelevant at physics magnitudes.

### 4. Measured divergence: negligible

Two simulations at 60 Hz for 180 s (a long race), comparing wide-intermediate
against naive all-float32, using the recovered Overkill constants:

| model | max transient |dy| | final divergence | branch flips |
|-------|------------------|------------------|--------------|
| drag/acceleration | 1.2e-4 m | **0.0000 m** | – |
| spring/damper with contact discontinuity (kPos 1.573, kSpeed 4.182) | – | **0.0000 m** | **0** |

Both systems are dissipative, so error is damped rather than accumulated. Note
these are models of the code's *shape*, not the game's actual integrator, so
treat them as indicative; the structural findings in 1-3 are the solid part.

### 5. The real risk is the compiler, not the FPU

Our Vita build was using `-ffast-math -mfpu=neon`. For physics that is wrong:

- **`-ffast-math` permits reassociation**, which breaks the order-preservation
  the rule above depends on;
- it also enables float auto-vectorisation onto **NEON, whose ARMv7
  single-precision path is flush-to-zero and not IEEE-conformant** (VFP scalar
  is conformant; NEON is not). With `-ffast-math` off, GCC will not auto-
  vectorise float reductions, which sidesteps this.

`CMakeLists.txt` now defines `PHYSICS_C_FLAGS`
(`-O2 -fno-fast-math -ffp-contract=off`) to attach to the physics translation
units when they land. `-ffp-contract=off` additionally prevents fused
multiply-add from silently changing rounding.

## Honest status

The **numbers, curves and unit conversions** are fully in hand. All 46 slider
keys resolve.

**The model is fully transcribed and verified.** Three translation units:

| file | contents |
|------|----------|
| `rccars_vita/rb.c` | rigid body, 32-float Baraff state, explicit Euler, gravity, drag, spring/damper, force/torque summation, rest damper |
| `rccars_vita/contact.c` | 2x2 lateral friction solve, tire grip and drift, engine curves with limiter and upgrades, reverse, engine braking, surface drag |
| `rccars_vita/collide.c` | sphere collision proxy, contact generation, geometric suspension solve |

All three build for the Vita and pass `rb_test.c`, which now drives the **real**
stack — `rb_car_update_suspension` solves the spring lengths against the world and
`rb_collide` produces the contacts, in `carPhysTick`'s order. The only stand-in
left is the world itself: a flat one-sided plane at y = 0.

| check | result |
|-------|--------|
| free fall, 1 s | `vy = -10.0000`, `y = -4.9917` — matches Euler's own truncation exactly |
| drag terminal velocity | `24.7569` vs analytic `24.7576` |
| 3 s free spin | `\|q\| = 1.0000000`, `\|L\|` conserved to 6 digits |
| torque couple round-trip | `tau` recovered exactly, net force zero |
| curve evaluation | clamps and interpolates at the recovered control points |
| tire grip | front/rear 0.6300; drift cuts the rear to 0.1373 |
| engine at rest | 4.200 m/s² = curve(0) 6.0 x resonator_accel 0.70 |
| speed limiter | returns 0 above `speed_base_max * resonator_speed` |
| reverse taper | -4.000 at rest, -2.667 at 3.0 m/s, 0 above 3.33 m/s |
| contact frame | orthonormal; 30° steer rotates the rolling direction by 30° |
| **static ride height** | **settles at y = 0.1703, spring length 0.0984** |
| **10 s full throttle** | **settles at 6.907 m/s against a computed limiter of 6.900 m/s** |
| at rest | speed 0.00000 — the rest damper holds it |
| 25° steer | develops yaw, speed unchanged |

The ride-height result is the strongest single check in the effort. Closed form,
the equilibrium is `len_free + radius - sag` = 0.2179 + 0.0718 - 0.1194 =
**0.1703 m**, with a spring length of 0.0985. The simulation lands there to four
decimals — and it does so with the length **solved geometrically by the collision
code**, not prescribed. Two independently transcribed subsystems, the geometric
solve and the force model, agree on a number neither was fitted to.

The top-speed result exercises the other axis of the model end to end: curve
evaluation, the km/h conversion, resonator scaling, the traction clamp, the
rear-wheel application point, drag cancellation and the limiter.

The model is **wired into the app** and drives it by default; SELECT swaps back
to the `physics.c` placeholder for comparison. `col.c` backs the three `rb_world`
callbacks with the port's own `.col` grid.

Three known limitations remain, each traceable to specific untranscribed code —
the shell query, the extension-rate ramp, and the body-sphere contact solve. See
"Three known limitations" above. A settled or driving car is unaffected; a hard
landing recovers to a lower ride height than it should.

The next step is not more reverse engineering. It is **running it in Vita3K and
looking at it** — a visual check will settle the remaining suspension questions
far faster than more numerical archaeology.

Three original quirks are reproduced deliberately rather than corrected; see
"Quirks reproduced deliberately" above. If the handling ever feels wrong, check
those before suspecting the transcription.

## Reading this binary at all

Enable Ghidra's **Decompiler Parameter ID** analyzer before reading anything
non-trivial. It is off by default in headless runs, and without it call sites
show `FUN_xxxx()` with no arguments and locals are read before assignment —
readable structurally, but not faithful enough to transcribe.

    setAnalysisOption(currentProgram, "Decompiler Parameter ID", "true");
    AutoAnalysisManager.getAnalysisManager(currentProgram).reAnalyzeAll(null);

It costs about 25 minutes for this 1.4 MB / 4,800-function binary. It is already
enabled in `rccars_ghidra_proj`.

Two further traps worth remembering:

- Headless **exits 0 even when a script fails to compile**. Read the log, not
  the exit code.
- Ghidra leaves real code outside any function. Two of the physics entry points
  (`0x4f5400`, and the `0x4fc403` call site) sit in undefined byte ranges, so
  nothing referenced them and no xref search could find them. Scanning raw
  `.text` for `E8` relative calls found them in seconds.
