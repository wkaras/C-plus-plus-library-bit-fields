/*
Copyright (c) 2016 Walter William Karas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "bitfield.h"

#include <stdint.h>

// This code is for compiling into assembler to see how well optimization
// is working.

struct Fmt : private Bitfield_format
  {
    F<64> pad;
    F<5> ofs;
    F<64> data;
    F<5> ofs2;
    F<64> pad2;
  };

typedef Bitfield<Bitfield_traits_default<uint64_t, uint8_t> > Bf11;
typedef Bitfield_w_fmt<Bf11, Fmt> Bwf11;
typedef Bitfield<Bitfield_traits_default<uint64_t> > Bf12;
typedef Bitfield_w_fmt<Bf12, Fmt> Bwf12;

Bf11::Value_t r11(Bf11::Storage_t *p)
  { return(Bf11::read(p, BITF(Bwf11, data))); }
void w11(Bf11::Storage_t *p, Bf11::Value_t v)
  { Bf11::write(p, BITF(Bwf11, data), v); }

Bf11::Value_t r11t(Bf11::Storage_t *p) { return(Bf11::read(p, &Fmt::data)); }
void w11t(Bf11::Storage_t *p, Bf11::Value_t v)
  { Bf11::write(p, &Fmt::data, v); }

Bf12::Value_t r12(Bf12::Storage_t *p)
  { return(Bf12::read(p, BITF(Bwf12, data))); }
void w12(Bf12::Storage_t *p, Bf12::Value_t v)
  { Bf12::write(p, BITF(Bwf12, data), v); }

Bf12::Value_t r12t(Bf12::Storage_t *p) { return(Bf12::read(p, &Fmt::data)); }
void w12t(Bf12::Storage_t *p, Bf12::Value_t v)
  { Bf12::write(p, &Fmt::data, v); }

struct Bft21 : Bitfield_traits_default<uint64_t, uint8_t>
  {
    static const bool Fmt_offset_from_start = false;
  };
typedef Bitfield<Bft21> Bf21;
typedef Bitfield_w_fmt<Bf21, Fmt> Bwf21;
struct Bft22 : Bitfield_traits_default<uint64_t>
  {
    static const bool Fmt_offset_from_start = false;
  };
typedef Bitfield<Bft22> Bf22;
typedef Bitfield_w_fmt<Bf22, Fmt> Bwf22;

Bf21::Value_t r21(Bf21::Storage_t *p)
  { return(Bf21::read(p, BITF(Bwf21, data))); }
void w21(Bf21::Storage_t *p, Bf21::Value_t v)
  { Bf21::write(p, BITF(Bwf21, data), v); }

Bf21::Value_t r21t(Bf21::Storage_t *p) { return(Bf21::read(p, &Fmt::data)); }
void w21t(Bf21::Storage_t *p, Bf21::Value_t v)
  { Bf21::write(p, &Fmt::data, v); }

Bf22::Value_t r22(Bf22::Storage_t *p)
  { return(Bf22::read(p, BITF(Bwf22, data))); }
void w22(Bf22::Storage_t *p, Bf22::Value_t v)
  { Bf22::write(p, BITF(Bwf22, data), v); }

Bf22::Value_t r22t(Bf22::Storage_t *p) { return(Bf22::read(p, &Fmt::data)); }
void w22t(Bf22::Storage_t *p, Bf22::Value_t v)
  { Bf22::write(p, &Fmt::data, v); }
