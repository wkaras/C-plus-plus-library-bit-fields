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

// This code is for compiling into assembler to compare with base language
// bit fields

struct Fmt : private Bitfield_format
  {
    F<14> ofs;
    F<49> data;
  };

typedef Bitfield<Bitfield_traits_default<uint64_t> > Bf;
typedef Bitfield_w_fmt<Bf, Fmt> Bwf;

struct Base_fmt
  {
    uint64_t ofs : 14;
    uint64_t data : 49;
  };

Bf::Value_t r1(Bf::Storage_t *p)
  { return(BITF(Bwf, p, data).read()); }

uint64_t r2(Base_fmt *p)
  { return(p->data); }

void w1(Bf::Storage_t *p, Bf::Value_t v)
  { BITF(Bwf, p, data).write_nvc(v); }

void w2(Base_fmt *p, uint64_t v)
  { p->data = v; }
