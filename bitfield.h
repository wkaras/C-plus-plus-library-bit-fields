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

// Usage documentation in Bitfield.html/Bitfield.pdf .

#ifndef BITFIELD_H_20160428
#define BITFIELD_H_20160428

#include <limits.h> // For definition of CHAR_BIT.

#include <cstddef> // For definition of offsetof.

// noexcept Use -- Since all the functions here are inlined, I'm assuming
// it's not important to conditionally make any of them noexcept based on:
// https://waltsgeekblog.quora.com/G++-inline-and-noexcept

// Names defined in this namespace should not be used outside this
// header file.
namespace Bitfield_impl
{

template< typename T>
struct Num_bits
  {
    // Not using numeric_limits for this because of:
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70767

    static const unsigned Value = sizeof(T) * CHAR_BIT;
  };

// The number of bits in Value_t cannot exceed this value multiplied by
// the number of bits in Storage_t.
const unsigned Max_value_to_storage_bits_ratio = 8;

template<typename U>
inline U mask(unsigned bit_width)
  {
    if (bit_width == Num_bits<U>::Value)
      return(~U(0));

    return((U(1) << bit_width) - 1);
  }

template <typename Storage_access_t, typename Storage_t>
inline Storage_t ls_read_s(
  Storage_access_t base, unsigned first_bit, unsigned field_width)
  { return((base.read() >> first_bit) & mask<Storage_t>(field_width)); }

// Even if you are not David Byrne, you may ask youself, why use the
// recursive class templates Ls_read/Ms_read/Ls_modify/Ms_modify?  Why not
// simply use (non-templeted) recursive functions?  The reason is the GCC
// seems to find it easier to fully optimize (for execution path length)
// when the recursive class templates are used.

template <class Bitfield_traits, unsigned Depth>
struct Ls_read
  {
    typedef typename Bitfield_traits::Value_t Value_t;

    typedef typename Bitfield_traits::Storage_t Storage_t;

    typedef typename Bitfield_traits::Storage_access_t Storage_access_t;

    static const unsigned Storage_bits = Num_bits<Storage_t>::Value;

    static Value_t x(
      Storage_access_t base, unsigned first_bit, unsigned field_width)
      {
        if ((first_bit + field_width) <= Storage_bits)
          return(
            ls_read_s<Storage_access_t, Storage_t>(
              base, first_bit, field_width));

        Value_t v =
          ls_read_s<Storage_access_t, Storage_t>(
            base, first_bit, Storage_bits - first_bit);

        base += 1;
        field_width -= Storage_bits - first_bit;

        return(v | (Ls_read<Bitfield_traits, Depth + 1>::x(base, 0, field_width)
                      << (Storage_bits - first_bit)));
      }
  };

template <class Bitfield_traits>
struct Ls_read<Bitfield_traits, Max_value_to_storage_bits_ratio + 1>
  {
    static typename Bitfield_traits::Value_t x(
      typename Bitfield_traits::Storage_access_t, unsigned, unsigned)
      { return(0); }
  };

template <typename Storage_access_t, typename Storage_t>
inline Storage_t ms_read_s(
  Storage_access_t base, unsigned first_bit, unsigned field_width)
  {
    return(
      (base.read() >> (Num_bits<Storage_t>::Value - first_bit - field_width)) &
      mask<Storage_t>(field_width));
  }

template <class Bitfield_traits, unsigned Depth>
struct Ms_read
  {
    typedef typename Bitfield_traits::Value_t Value_t;

    typedef typename Bitfield_traits::Storage_t Storage_t;

    typedef typename Bitfield_traits::Storage_access_t Storage_access_t;

    static const unsigned Storage_bits = Num_bits<Storage_t>::Value;

    static Value_t x(
      Storage_access_t base, unsigned first_bit, unsigned field_width)
      {
        if ((first_bit + field_width) <= Storage_bits)
          return(
            ms_read_s<Storage_access_t, Storage_t>(
              base, first_bit, field_width));

        field_width -= Storage_bits - first_bit;

        Value_t v =
          ms_read_s<Storage_access_t, Storage_t>(
            base, first_bit, Storage_bits - first_bit) << field_width;

        base += 1;

        return(v |
               Ms_read<Bitfield_traits, Depth + 1>::x(base, 0, field_width));
      }
  };

template <class Bitfield_traits>
struct Ms_read<Bitfield_traits, Max_value_to_storage_bits_ratio + 1>
  {
    static typename Bitfield_traits::Value_t x(
      typename Bitfield_traits::Storage_access_t, unsigned, unsigned)
      { return(0); }
  };

template <class Bitfield_traits, class Modifier, unsigned Depth>
struct Ls_modify
  {
    typedef typename Bitfield_traits::Storage_t Storage_t;

    typedef typename Bitfield_traits::Storage_access_t Storage_access_t;

    static const unsigned Storage_bits = Num_bits<Storage_t>::Value;

    static void x(
      Storage_access_t base, unsigned first_storage_bit,
      unsigned first_value_bit, unsigned field_width, Modifier m)
      {
        if ((first_storage_bit + field_width) <= Storage_bits)
          m(base, first_storage_bit, first_value_bit, field_width);
        else
          {
            unsigned storage_width = Storage_bits - first_storage_bit;

            m(base, first_storage_bit, first_value_bit, storage_width);

            base += 1;

            field_width -= storage_width;
            first_value_bit += storage_width;

            Ls_modify<Bitfield_traits, Modifier, Depth + 1>::x(
              base, 0, first_value_bit, field_width, m);
          }
      }
  };

template <class Bitfield_traits, class Modifier>
struct Ls_modify<Bitfield_traits, Modifier, Max_value_to_storage_bits_ratio + 1>
  {
    static void x(
      typename Bitfield_traits::Storage_access_t, unsigned, unsigned, unsigned,
      Modifier)
      { }
  };

template <class Bitfield_traits, class Modifier, unsigned Depth>
struct Ms_modify
  {
    typedef typename Bitfield_traits::Storage_t Storage_t;

    typedef typename Bitfield_traits::Storage_access_t Storage_access_t;

    static const unsigned Storage_bits = Num_bits<Storage_t>::Value;

    static void x(
      Storage_access_t base, unsigned first_storage_bit, unsigned field_width,
      Modifier m)
      {
        if ((first_storage_bit + field_width) <= Storage_bits)
          m(
            base, Storage_bits - field_width - first_storage_bit, 0,
            field_width);
        else
          {
            unsigned storage_width = Storage_bits - first_storage_bit;

            m(base, 0, field_width - storage_width, storage_width);

            base += 1;

            field_width -= storage_width;

            Ms_modify<Bitfield_traits, Modifier, Depth + 1>::x(
              base, 0, field_width, m);
          }
      }
  };

template <class Bitfield_traits, class Modifier>
struct Ms_modify<Bitfield_traits, Modifier, Max_value_to_storage_bits_ratio + 1>
  {
    static void x(
      typename Bitfield_traits::Storage_access_t, unsigned, unsigned,
      Modifier)
      { }
  };

template<typename Value_t>
struct Err_act_default
  {
    static void field_too_wide(unsigned /* field_width */) { }
    static void value_too_big(Value_t /* v */, unsigned /* field_width */) { }
  };

} // namespace Bitfield_impl

template <typename V_t = unsigned, typename S_t = V_t>
class Bitfield_traits_default
  {
  public:

    typedef V_t Value_t;

    typedef S_t Storage_t;

    class Storage_access_t
      {
      public:

        typedef S_t Storage_t;

        Storage_access_t(Storage_t *p) : ptr(p) { }

        void operator += (unsigned offset) { ptr += offset; }

        Storage_t read() { return(*ptr); }

        void write(Storage_t t) { *ptr = t; }

      private:

        Storage_t *ptr;

      };

    static const bool Storage_ls_bit_first = true;

    static const bool Fmt_offset_from_start = true;

    static const bool Fmt_align_at_zero_offset = true;

  }; // class Bitfields_traits_default

#define BITF_DEF_F \
template <unsigned BITF_WIDTH> struct F { char x[BITF_WIDTH]; };

struct Bitfield_format { BITF_DEF_F };

template <
  class Traits = Bitfield_traits_default<unsigned>,
  class Err_act = Bitfield_impl::Err_act_default<typename Traits::Value_t> >
class Bitfield
  {
  public:

    typedef typename Traits::Value_t Value_t;

    typedef typename Traits::Storage_t Storage_t;

    typedef typename Traits::Storage_access_t Storage_access_t;

    static const bool Storage_ls_bit_first = Traits::Storage_ls_bit_first;

    static const bool Fmt_offset_from_start = Traits::Fmt_offset_from_start;

    static const bool Fmt_align_at_zero_offset =
      Traits::Fmt_align_at_zero_offset;

    static const unsigned Storage_bits =
      Bitfield_impl::Num_bits<Storage_t>::Value;

    template<class Format>
    struct Define
      {
        static const unsigned Storage_bits =
          Bitfield_impl::Num_bits<Storage_t>::Value;

        static const unsigned Dimension =
          (sizeof(Format) + Storage_bits - 1) / Storage_bits;

        typedef Storage_t T[Dimension];
      };

    static Value_t mask(unsigned field_width)
      {
        return(Bitfield_impl::mask<Value_t>(field_width));
      }

    class Modifier_base
      {
      public:

        static Value_t value() { return(0); }

      protected:

        static Storage_t clear(
          Storage_access_t s, unsigned shift, unsigned width)
          {
            if (width == Storage_bits)
              return(0);

            return(
              s.read() &
              ~(Bitfield_impl::mask<Storage_t>(width) << shift));
          }
      };

    class Value_modifier_base : public Modifier_base
      {
      public:

        Value_t value() const { return(val); }

        Value_modifier_base(Value_t v) : val(v) { }

      private:

        const Value_t val;
      };

    class Bf
      {
      private:

        const Storage_access_t base;

        const unsigned short first_bit, field_width;

      public:

        Bf(
          Storage_access_t base_, unsigned short first_bit_,
          unsigned short field_width_)
          : base(base_), first_bit(first_bit_), field_width(field_width_)
          { }

        unsigned short offset() { return(first_bit); }

        unsigned short width() { return(field_width); }

        bool is_width_invalid()
          {
            return((field_width == 0) ||
                   (field_width > Bitfield_impl::Num_bits<Value_t>::Value));
          }

        Value_t read()
          {
            if (!check_width())
              return(~(Value_t(0)));

            Storage_access_t access(base);
            access += (first_bit / Storage_bits);

            if (Traits::Storage_ls_bit_first)
              return(
                Bitfield_impl::Ls_read<Traits, 0>::x(
                  access, first_bit % Storage_bits, field_width));

            return(
              Bitfield_impl::Ms_read<Traits, 0>::x(
                access, first_bit % Storage_bits, field_width));
          }

        operator Value_t () { return(read()); }

        Value_t read_sign_extend()
          {
            Value_t v = read();

            if (field_width)
              {
                Value_t m = ~mask(field_width - 1);

                if (m & v)
                  v |= m;
              }

            return(v);
          }

        template <class Modifier>
        bool modify_nvc(Modifier m)
          {
            if (!check_width())
              return(false);

            Storage_access_t access(base);
            access += (first_bit / Storage_bits);

            if (Traits::Storage_ls_bit_first)
              Bitfield_impl::Ls_modify<Traits, Modifier, 0>::x(
                access, first_bit % Storage_bits, 0, field_width, m);
            else
              Bitfield_impl::Ms_modify<Traits, Modifier, 0>::x(
                access, first_bit % Storage_bits, field_width, m);

            return(true);
          }

        template <class Modifier>
        bool modify(Modifier m)
          {
            if (!check_fit(m.value()))
              return(false);

            return(modify_nvc(m));
          }

        bool write(Value_t val) { return(modify(Mod_write(val))); }

        Value_t operator = (Value_t val) { write(val); return(val); }

        bool write_nvc(Value_t val) { return(modify_nvc(Mod_write(val))); }

        bool zero() { return(modify_nvc(Mod_zero())); }

        bool b_and(Value_t val) { return(modify(Mod_and(val))); }

        Bf & operator &= (Value_t val) { modify(Mod_and(val)); return(*this); }

        bool b_and_nvc(Value_t val) { return(modify_nvc(Mod_and(val))); }

        bool b_or(Value_t val) { return(modify(Mod_or(val))); }

        Bf & operator |= (Value_t val) { modify(Mod_or(val)); return(*this); }

        bool b_or_nvc(Value_t val) { return(modify_nvc(Mod_or(val))); }

        bool b_xor(Value_t val) { return(modify(Mod_xor(val))); }

        Bf & operator ^= (Value_t val) { modify(Mod_xor(val)); return(*this); }

        bool b_xor_nvc(Value_t val) { return(modify_nvc(Mod_xor(val))); }

        bool b_comp() { return(modify_nvc(Mod_comp())); }

      private:

        bool is_value_too_big(Value_t v)
          {
            if (field_width == Bitfield_impl::Num_bits<Value_t>::Value)
              return(false);

            return(v > mask(field_width));
          }

        bool check_width()
          {
            if (is_width_invalid())
              {
                Err_act::field_too_wide(field_width);

                return(false);
              }

            return(true);
          }

        bool check_fit(Value_t v)
          {
            if (is_value_too_big(v))
              {
                Err_act::value_too_big(v, field_width);

                return(false);
              }

            return(true);
          }

      }; // end class Bf

    static Bf fn(
      Storage_access_t base, unsigned first_bit, unsigned field_width)
      { return(Bf(base, first_bit, field_width)); }

    template<class Format, typename Mbr_type>
    static unsigned field_width(Mbr_type Format::*)
      { return(sizeof(Mbr_type)); }

    template<class Format, typename Mbr_type>
    static unsigned field_offset(
      Mbr_type Format::*field, unsigned base_offset = 0)
      {
        unsigned offset =
          reinterpret_cast<char *>(
            &(reinterpret_cast<Format *>(0x100)->*field)) -
           reinterpret_cast<char *>(0x100);

        if (!Bitfield::Fmt_offset_from_start)
          offset = sizeof(Format) - sizeof(Mbr_type) - offset;

        // If base_offset is not zero, it's assumed that it already
        // incorporates any offset to align the end of the containing
        // structure with a Storage_t boundary.
        //
        if (!Bitfield::Fmt_align_at_zero_offset &&
             ((sizeof(Format) % Storage_bits) != 0) &&
             (base_offset == 0))
          offset += Storage_bits - (sizeof(Format) % Storage_bits);

        return(base_offset + offset);
      }

    template<class Format, typename Mbr_type>
    static Bf f(
      Storage_access_t base, Mbr_type Format::*field, unsigned offset = 0)
      {
        return(Bf(base, field_offset(field, offset), field_width(field)));
      }

  private:

    class Mod_zero : public Modifier_base
      {
      public:

        void operator () (
          Storage_access_t s, unsigned storage_shift, unsigned,
          unsigned storage_width)
          { s.write(this->clear(s, storage_shift, storage_width)); }
      };

    class Mod_write : public Value_modifier_base
      {
      public:

        Mod_write(Value_t v) : Value_modifier_base(v) { }

        void operator () (
          Storage_access_t s, unsigned storage_shift, unsigned value_shift,
          unsigned storage_width)
          {
            Storage_t x =
              (Value_modifier_base::value() >> value_shift) << storage_shift;

            s.write(
              static_cast<Storage_t>(
                x | this->clear(s, storage_shift, storage_width)));
          }
      };

    class Mod_and : public Value_modifier_base
      {
      public:

        Mod_and(Value_t v) : Value_modifier_base(v) { }

        void operator () (
          Storage_access_t s, unsigned storage_shift, unsigned value_shift,
          unsigned storage_width)
          {
            Storage_t x =
              (Value_modifier_base::value() >> value_shift) << storage_shift;

            x |=
              ~(Bitfield_impl::mask<Storage_t>(storage_width) << storage_shift);


            s.write(static_cast<Storage_t>(x & s.read()));
          }
      };

    class Mod_or : public Value_modifier_base
      {
      public:

        Mod_or(Value_t v) : Value_modifier_base(v) { }

        void operator () (
          Storage_access_t s, unsigned storage_shift, unsigned value_shift,
          unsigned)
          {
            Storage_t x =
              (Value_modifier_base::value() >> value_shift) << storage_shift;

            s.write(static_cast<Storage_t>(x | s.read()));
          }
      };

    class Mod_xor : public Value_modifier_base
      {
      public:

        Mod_xor(Value_t v) : Value_modifier_base(v) { }

        void operator () (
          Storage_access_t s, unsigned storage_shift, unsigned value_shift,
          unsigned)
          {
            Storage_t x =
              (Value_modifier_base::value() >> value_shift) << storage_shift;

            s.write(static_cast<Storage_t>(x ^ s.read()));
          }
      };

    class Mod_comp : public Modifier_base
      {
      public:

        void operator () (
          Storage_access_t s, unsigned storage_shift, unsigned,
          unsigned storage_width)
          {
            s.write(
              static_cast<Storage_t>(
                s.read() ^
                (Bitfield_impl::mask<Storage_t>(storage_width)
                   << storage_shift)));
          }
      };

    static const bool Uh_oh =
      Bitfield_impl::Num_bits<Value_t>::Value >
      (Bitfield_impl::Max_value_to_storage_bits_ratio * Storage_bits);

    // If the line following this comment causes a compile error, the
    // number of bits in Value_t is more than Max_value_to_storage_bits_ratio
    // times the number of bits in Storage_t, which is not allowed.
    static int not_used[1 / (Uh_oh ? 0 : 1)];

  }; // class Bitfield

template <class Bitfield, class Fmt>
struct Bitfield_w_fmt : public Bitfield
  {
    static const unsigned Storage_bits =
      Bitfield_impl::Num_bits<typename Bitfield::Storage_t>::Value;

    template <class Base_fmt>
    static unsigned base_offset(unsigned derived_offset = 0)
      {
        const unsigned Storage_bits = Bitfield::Storage_bits;

        const unsigned From_start_unaligned =
          reinterpret_cast<char *>(static_cast<Base_fmt *>(
            reinterpret_cast<Fmt *>(0x100))) - 
          reinterpret_cast<char *>(0x100);

        const unsigned Unaligned =
          Bitfield::Fmt_offset_from_start ?
            From_start_unaligned :
            sizeof(Fmt) - sizeof(Base_fmt) - From_start_unaligned;

        return(
          Unaligned +
          ((!Bitfield::Fmt_align_at_zero_offset &&
            ((sizeof(Fmt) % Storage_bits) != 0) &&
            (derived_offset == 0)) ?
             Storage_bits - (sizeof(Fmt) % Storage_bits) : derived_offset));
      }

    typedef Fmt Format;
  };

#if 0

// Standards prior to C++11 do not consider any expression with reintepret_cast
// to be compile-time constants.

#define BITF_OFFSET_FROM_START(BWF, FIELD_SPEC) \
  (reinterpret_cast<char *>( \
     &(reinterpret_cast<typename BWF::Format *>(0x100)->FIELD_SPEC)) - \
   reinterpret_cast<char *>(0x100))

#else

#define BITF_OFFSET_FROM_START(BWF, FIELD_SPEC) \
  offsetof(typename BWF::Format, FIELD_SPEC)

#endif

#define BITF_WIDTH(BWF, FIELD_SPEC) \
  (sizeof(reinterpret_cast<typename BWF::Format *>(0x100)->FIELD_SPEC))

#define BITF_OFFSET_FROM_END(BWF, FIELD_SPEC) \
  (sizeof(typename BWF::Format) - BITF_WIDTH(BWF, FIELD_SPEC) - \
   BITF_OFFSET_FROM_START(BWF, FIELD_SPEC))

#define BITF_OFFSET_UNALIGNED(BWF, FIELD_SPEC) \
  (BWF::Fmt_offset_from_start ? \
    BITF_OFFSET_FROM_START(BWF, FIELD_SPEC) : \
    BITF_OFFSET_FROM_END(BWF, FIELD_SPEC))

#define BITF_W_OFS_STD(BWF, BASE, FIELD_SPEC, OFS) \
  BWF::fn((BASE), (BITF_OFFSET_UNALIGNED(BWF, FIELD_SPEC) + (OFS)), \
          BITF_WIDTH(BWF, FIELD_SPEC))

#define BITF_W_OFS_ALT(SEL, FIELD_SPEC, OFS) \
  BITF_W_OFS_STD(BITF_U_##SEL##_BWF, (BITF_U_##SEL##_BASE), FIELD_SPEC, (OFS))

#define BITF_OFFSET(BWF, FIELD_SPEC) \
  (BITF_OFFSET_UNALIGNED(BWF, FIELD_SPEC) + \
  (BWF::Fmt_align_at_zero_offset ? \
     0 : ((sizeof(typename BWF::Format) % BWF::Storage_bits) ? \
            (BWF::Storage_bits - \
              (sizeof(typename BWF::Format) % BWF::Storage_bits)) : 0)))

#define BITF_STD(BWF, BASE, FIELD_SPEC) \
  BWF::fn((BASE), BITF_OFFSET(BWF, FIELD_SPEC), BITF_WIDTH(BWF, FIELD_SPEC))

#define BITF_ALT(SEL, FIELD_SPEC) \
  BITF_STD(BITF_U_##SEL##_BWF, (BITF_U_##SEL##_BASE), FIELD_SPEC)

// This macro is private -- not for direct use.
//
#define BITF_CAT_WIDTH_(LOW_FLD_OFFSET, HIGH_FLD_OFFSET, HIGH_FLD_WIDTH) \
  ((HIGH_FLD_OFFSET) + (HIGH_FLD_WIDTH) - (LOW_FLD_OFFSET))

#define BITF_CAT_WIDTH(BWF, FIELD_SPEC1, FIELD_SPEC2) \
  (BITF_OFFSET_UNALIGNED(BWF, FIELD_SPEC1) > \
   BITF_OFFSET_UNALIGNED(BWF, FIELD_SPEC2) ? \
   BITF_CAT_WIDTH_( \
     BITF_OFFSET_UNALIGNED(BWF, FIELD_SPEC2), \
     BITF_OFFSET_UNALIGNED(BWF, FIELD_SPEC1), \
     BITF_WIDTH(BWF, FIELD_SPEC1)) : \
   BITF_CAT_WIDTH_( \
     BITF_OFFSET_UNALIGNED(BWF, FIELD_SPEC1), \
     BITF_OFFSET_UNALIGNED(BWF, FIELD_SPEC2), \
     BITF_WIDTH(BWF, FIELD_SPEC2)))

#define BITF_CAT_OFFSET(BWF, FIELD_SPEC1, FIELD_SPEC2) \
  (BITF_OFFSET_UNALIGNED(BWF, FIELD_SPEC1) > \
   BITF_OFFSET_UNALIGNED(BWF, FIELD_SPEC2) ? \
   BITF_OFFSET(BWF, FIELD_SPEC2) : BITF_OFFSET(BWF, FIELD_SPEC1))

#define BITF_CAT_STD(BWF, BASE, FIELD_SPEC1, FIELD_SPEC2) \
  BWF::fn((BASE), BITF_CAT_OFFSET(BWF, FIELD_SPEC1, FIELD_SPEC2), \
          BITF_CAT_WIDTH(BWF, FIELD_SPEC1, FIELD_SPEC2))

#define BITF_CAT_ALT(SEL, FIELD_SPEC1, FIELD_SPEC2) \
  BITF_CAT_STD(BITF_U_##SEL##_BWF, BITF_U_##SEL##_BASE, FIELD_SPEC1, \
               FIELD_SPEC2)

#define BITF_CAT_OFFSET_UNALIGNED(BWF, FIELD_SPEC1, FIELD_SPEC2) \
  (BITF_OFFSET_UNALIGNED(BWF, FIELD_SPEC1) > \
   BITF_OFFSET_UNALIGNED(BWF, FIELD_SPEC2) ? \
   BITF_OFFSET_UNALIGNED(BWF, FIELD_SPEC2) : \
   BITF_OFFSET_UNALIGNED(BWF, FIELD_SPEC1))

#define BITF_CAT_W_OFS_STD(BWF, BASE, FIELD_SPEC1, FIELD_SPEC2, OFFSET) \
  BWF::fn((BASE), \
    BITF_CAT_OFFSET_UNALIGNED(BWF, FIELD_SPEC1, FIELD_SPEC2) + (OFFSET), \
    BITF_CAT_WIDTH(BWF, FIELD_SPEC1, FIELD_SPEC2))

#define BITF_CAT_W_OFS_ALT(SEL, FIELD_SPEC1, FIELD_SPEC2, OFS) \
  BITF_CAT_W_OFS_STD(BITF_U_##SEL##_BWF, (BITF_U_##SEL##_BASE), FIELD_SPEC1, \
                     FIELD_SPEC2, OFS)

#if defined(BITF_USE_ALT)

#define BITF BITF_ALT
#define BITF_W_OFS BITF_W_OFS_ALT
#define BITF_CAT BITF_CAT_ALT
#define BITF_CAT_W_OFS BITF_CAT_W_OFS_ALT

#else

#define BITF BITF_STD
#define BITF_W_OFS BITF_W_OFS_STD
#define BITF_CAT BITF_CAT_STD
#define BITF_CAT_W_OFS BITF_CAT_W_OFS_STD

#endif

class Bitfield_seq_storage_write_default_traits
  {
  public:

    static const bool Flush_on_destroy = true;

    static const unsigned First_bit = 0;

    static const unsigned End_bit = ~unsigned(0);

    static void handle_backwards(
      unsigned /* previous_offset */, unsigned /* next_offset */)
      { }
  };

template <
  class Storage_access_t,
  class Traits = Bitfield_seq_storage_write_default_traits>
class Bitfield_seq_storage_write_t;

template <
  class Storage_access_t,
  class Traits = Bitfield_seq_storage_write_default_traits>
class Bitfield_seq_storage_write_buf
  {
  friend class Bitfield_seq_storage_write_t<Storage_access_t, Traits>;

  public:

    typedef typename Storage_access_t::Storage_t Storage_t;

    Bitfield_seq_storage_write_buf(Storage_access_t sa_)
      : sa(sa_), buffer_status(Empty)
      { }

    void flush()
      {
        if (buffer_status == Write_needed)
          {
            // Do this first in case of exception.
            buffer_status = No_read_needed;

            Storage_access_t sa_(sa);

            sa_ += buffer_offset;

            sa_.write(buffer);
          }
      }

    void discard() { buffer_status = Empty; }

    ~Bitfield_seq_storage_write_buf()
      {
        if (Traits::Flush_on_destroy)
          flush();
      }

  private:

    Storage_t read(unsigned offset)
      {
        handle_buffer(offset);

        if (buffer_status == Empty)
          {
            if (((First_storage_offset != ~unsigned(0)) and
                 (offset <= First_storage_offset)) or
                (offset >= Last_storage_offset))
              {
                Storage_access_t sa_(sa);

                sa_ += offset;

                buffer = sa_.read();
              }
            else
              buffer = 0;

            buffer_status = No_read_needed;
            buffer_offset = offset;
          }

        return(buffer);
      }

    void write(unsigned offset, Storage_t t)
      {
        handle_buffer(offset);

        buffer = t;

        buffer_status = Write_needed;
        buffer_offset = offset;
      }

    static const unsigned Storage_bits =
      Bitfield_impl::Num_bits<Storage_t>::Value;

    static const unsigned First_storage_offset =
      (Traits::First_bit == 0) ?
        ~unsigned(0) : (Traits::First_bit / Storage_bits);

    static const unsigned Last_storage_offset =
      (Traits::End_bit == ~unsigned(0)) ?
        ~unsigned(0) : (Traits::End_bit / Storage_bits);

    const Storage_access_t sa;

    enum { Empty, No_read_needed, Write_needed } buffer_status;

    unsigned buffer_offset;

    Storage_t buffer;

    // Common buffer handling for read or write.
    void handle_buffer(unsigned offset)
      {
        if (buffer_status == Empty)
          return;

        if (offset < buffer_offset)
          {
            // Fields are not being written in order.

            // Do this before handle_backwards() in case of exception.
            buffer_status = Empty;

            Traits::handle_backwards(buffer_offset, offset);
          }
        else if (offset > buffer_offset)
          {
            flush();

            buffer_status = Empty;
          }
      }

  }; // end class Bitfield_seq_storage_write_buf

template <class Storage_access_t, class Traits>
class Bitfield_seq_storage_write_t
  {
  public:

    typedef typename Storage_access_t::Storage_t Storage_t;

    Bitfield_seq_storage_write_t(
      Bitfield_seq_storage_write_buf<Storage_access_t, Traits> &ssw_)
      : ssw(ssw_), cum_offset(0)
      { }

    void operator += (unsigned offset) { cum_offset += offset; }

    Storage_t read() { return(ssw.read(cum_offset)); }

    void write(Storage_t t) { ssw.write(cum_offset, t); }

  private:

    Bitfield_seq_storage_write_buf<Storage_access_t, Traits> &ssw;

    unsigned cum_offset;

  }; // end class Bitfield_seq_storage_write_t

#endif // Include once.
