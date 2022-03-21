#pragma once

// Local jumps
// 'Local' means that jumps do not cross stack frame boundaries

// Saved execution context
struct JumpContext {
  // CPU registers?
  void *rbp;
};

// Captures the current execution context into 'ctx'
// 'extern "C"' means "C++ compiler, do not mangle function names"
// https://en.wikipedia.org/wiki/Name_mangling
extern "C" void Capture(JumpContext* ctx);

// Jumps to 'Capture' call that captured provided 'ctx'
// This function does not return
extern "C" void JumpTo(JumpContext* ctx);
