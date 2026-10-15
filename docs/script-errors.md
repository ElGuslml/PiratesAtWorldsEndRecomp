# The game closes after a script error

**Status: fixed (2026-10-15), confirmed in play.** In Gustavo's next session (64 minutes, three level
ends), the game caught four script errors and went on. Each one would have closed it before.

## Symptom

After 30 minutes of play the game closed (Gustavo, 2026-10-15, with the XenosNative renderer). The log ends
with three faults at the same moment:
- access violations on two game threads, reading near-null addresses;
- a call to the invalid address `0x56657273` on a third thread. That value is the text "Vers".

## Cause

**The faults are a consequence, not the cause.**
- The main thread was inside the C runtime's `exit()`. `sub_82495D10` runs the game's atexit table.
- In that table, the destructor of a global object (`sub_823B26D8`, the object at `0x825BDC60`) releases a
  member. That member's destructor uses another member the same destructor had just cleared.
- The other threads were still running, and used objects that were already destroyed.

**The game called `exit(1)` from Lua.** Its scripts run on Lua 5.1, and `sub_8247A5E0` is `luaD_throw`.
- When a protected call is active, `luaD_throw` jumps back to it with `longjmp`, and the game handles the
  error.
- Otherwise it calls the panic function, and then `exit(EXIT_FAILURE)`.

**The recompiled `longjmp` did not jump.**
- The C runtime's `setjmp` and `longjmp` (`sub_8249A2B0`, `sub_8249A380`) were translated as plain
  functions. `longjmp` restored the registers and then *returned* to `luaD_throw`.
- `luaD_throw` then went on into its path for an uncaught error, and called `exit(1)`.
- So any script error closed the game, even one a protected call catches, which on a console the game would
  have handled and gone on.
- The stack shows it: right above `luaD_throw`'s call to `exit` is the frame of `luaD_pcall`, because the
  stack pointer had been restored to the point of the `setjmp`.

Script errors are rare: this was the first one in several hours of play.

## Fix

- **`setjmp_address` and `longjmp_address` in `potcawe_functions.toml`.** The codegen turns calls to them
  into the host's `setjmp` and `longjmp`, which return to the protected call.
  - Lua uses them for every protected call.
  - Two other libraries in the game use them too (around `0x824AD000` and `0x824C0000`).
- **`src/lua_errors.cpp` writes the game's script errors to the log.** The game prints none of them:
  - an error the game catches is a warning (the first 100):
    ```
    [lua] runtime error caught by the game's protected call (code 2): <script>:<line>: <message>
    ```
  - an error nothing catches is an error, written before the game exits:
    ```
    [lua] runtime error that no script catches (code 2), so the game exits: <script>:<line>: <message>
    ```

## What the game's scripts raise

The four errors of that session, all caught by the game:

```
[lua] runtime error caught by the game's protected call (code 2): ?:0: attempt to call method 'StartCountdownTimer' (a nil value)
[lua] runtime error caught by the game's protected call (code 2): ?:0: attempt to call global 'PreLoad' (a nil value)
```

- **`StartCountdownTimer`** appears three times: twice as a method, once as a global.
- **`PreLoad`** appears once.

The scripts call functions that do not exist at that point. These are most likely the game's own errors,
and on a console they would be just as silent: the game catches them and goes on.

The game's compiled scripts carry no debug information, so the position reads `?:0`. The function name is
what identifies them.

## How to turn it off

There is no option. Without the two addresses, the game would close again at the first script error. The
log lines appear only when there is an error.
