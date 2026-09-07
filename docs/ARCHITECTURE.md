# SkateGame Architecture

## Core gameplay layers

### 1. Skater
Responsible for player intent, balance state, body orientation and trick input.

### 2. Skateboard
Responsible for board movement, wheel contact, steering response, pop, rotation and landing state.

### 3. Trick System
Interprets player input into trick requests and coordinates animation + board physics.

### 4. Camera
Third-person follow camera with speed-aware distance, anticipation, landing recovery and cinematic smoothing.

### 5. Animation
Animation Blueprint + Control Rig + IK. The body follows the board instead of the board being visually glued to the character.

### 6. Surface Interaction
Classifies ground, ramps, rails, ledges and grindable surfaces. Provides friction and response parameters.

## Initial C++ classes

```text
ASkateCharacter
ASkateboardActor
USkateMovementComponent
USkateTrickComponent
USkateBalanceComponent
USkateCameraComponent
USkateSurfaceComponent
```

## Physics strategy

For the vertical slice, avoid trying to simulate every truck and wheel as a fully independent rigid body from day one. Start with a hybrid model:

- rigid-body board orientation;
- sampled wheel/ground contacts;
- spring-like suspension response;
- velocity-aware steering;
- controlled angular impulses for tricks;
- assisted landing detection;
- animation driven by board state.

This gives us a controllable game feel while preserving believable physical response.

## Vertical slice target

The first playable build should contain one polished test plaza with:

- flat ground;
- bank;
- quarter pipe;
- stair set;
- handrail;
- ledge;
- manual pad.

The player must be able to push, steer, brake, ollie, land, fall and reset reliably before adding a large trick catalog.
