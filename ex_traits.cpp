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

#include <stdint.h>

class Example_bitfield_traits
  {
  public:

    typedef uint32_t Value_t;

    typedef uint16_t Storage_t;

    class Storage_access_t
      {
      private:

        static volatile uint8_t & reg_select()
          { return(*reinterpret_cast<volatile uint8_t *>(0xE0000400)); }

        static volatile uint16_t & reg_buffer()
          { return(*reinterpret_cast<volatile uint16_t *>(0xE0000402)); }

        uint16_t reg_offset;

      public:

        Storage_access_t() : reg_offset(0) { }

        void operator += (unsigned offset) { reg_offset += offset; }

        Storage_t read()
          {
            reg_select() = reg_offset;
            Storage_t reg_val = reg_buffer();
            return((reg_val >> 8) | (reg_val << 8));
          }

        void write(uint8_t reg_val)
          {
            reg_select() = reg_offset;
            reg_buffer() = (reg_val >> 8) | (reg_val << 8);
          }
      };

    static const bool Storage_ls_bit_first = true;

    static const bool Fmt_offset_from_start = false;

    static const bool Fmt_align_at_zero_offset = true;

  };
