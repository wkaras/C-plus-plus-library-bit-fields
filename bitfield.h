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

#ifndef BITFIELD_H_20160428
#define BITFIELD_H_20160428

#include <limits.h> // For definition of CHAR_BIT.

// PENDING:  noexcept usage.

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

template <typename Value_t, typename Storage_t>
struct Checks
  {
    static bool is_field_too_wide(unsigned field_width)
      { return(field_width > Num_bits<Value_t>::Value); }

    static bool is_value_too_big(Value_t v, unsigned field_width)
      {
        if (field_width == Num_bits<Value_t>::Value)
          return(false);

        return(v > mask<Value_t>(field_width));
      }
  };

template <typename Storage_read_t, typename Storage_t>
inline Storage_t ls_read_s(
  Storage_read_t base, unsigned first_bit, unsigned field_width)
  { return((base.read() >> first_bit) & mask<Storage_t>(field_width)); }

template <class Bitfield_traits, unsigned Depth>
struct Ls_read
  {
    typedef typename Bitfield_traits::Value_t Value_t;

    typedef typename Bitfield_traits::Storage_t Storage_t;

    typedef typename Bitfield_traits::Storage_read_t Storage_read_t;

    static const unsigned Storage_bits = Num_bits<Storage_t>::Value;

    static Value_t x(
      Storage_read_t base, unsigned first_bit, unsigned field_width)
      {
        if ((first_bit + field_width) <= Storage_bits)
          return(
            ls_read_s<Storage_read_t, Storage_t>(
              base, first_bit, field_width));

        Value_t v =
          ls_read_s<Storage_read_t, Storage_t>(
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
      typename Bitfield_traits::Storage_read_t, unsigned, unsigned)
      { return(0); }
  };

template <typename Storage_read_t, typename Storage_t>
inline Storage_t ms_read_s(
  Storage_read_t base, unsigned first_bit, unsigned field_width)
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

    typedef typename Bitfield_traits::Storage_read_t Storage_read_t;

    static const unsigned Storage_bits = Num_bits<Storage_t>::Value;

    static Value_t x(
      Storage_read_t base, unsigned first_bit, unsigned field_width)
      {
        if ((first_bit + field_width) <= Storage_bits)
          return(
            ms_read_s<Storage_read_t, Storage_t>(
              base, first_bit, field_width));

        field_width -= Storage_bits - first_bit;

        Value_t v =
          ms_read_s<Storage_read_t, Storage_t>(
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
      typename Bitfield_traits::Storage_read_t, unsigned, unsigned)
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

template<typename V_t>
struct Err_act_default
  {
    static void field_too_wide(unsigned /* field_width */) { }
    static void value_too_big(V_t /* v */, unsigned /* field_width */) { }
  };

template <typename T, bool Make_const>
struct Constipate;

template <typename T>
struct Constipate<T, true> { typedef const T Type; };

template <typename T>
struct Constipate<T, false> { typedef T Type; };

} // namespace Bitfield_impl

template <
  typename V_t, typename S_t = V_t,
  class Err_act = Bitfield_impl::Err_act_default<V_t> >
class Bitfield_traits_default
  {
  public:

    typedef V_t Value_t;

    typedef S_t Storage_t;

    template <bool Is_const>
    class Storage_access_base
      {
      protected:

        typedef
          typename Bitfield_impl::Constipate<Storage_t, Is_const>::Type CS_t;

      public:

        Storage_access_base(CS_t *p) : ptr(p) { }

        void operator += (unsigned offset) { ptr += offset; }

        Storage_t read() { return(*ptr); }

      protected:

        CS_t *ptr;

      };

    typedef Storage_access_base<true> Storage_read_t;

    class Storage_access_t : public Storage_access_base<false>
      {
      private:

        typedef Storage_access_base<false> Base;

      public:

        Storage_access_t(Storage_t *p) : Base(p) { }

        void write(Storage_t t) { *Base::ptr = t; }

        operator Storage_read_t () { return(Storage_read_t(Base::ptr)); }
      };

    static const bool Storage_ls_bit_first = true;

    static bool check_width(unsigned field_width)
      {
        if (Bf_checks::is_field_too_wide(field_width))
          {
            Err_act::field_too_wide(field_width);

            return(false);
          }

        return(true);
      }

    static bool check_fit(Value_t v, unsigned field_width)
      {
        if (Bf_checks::is_value_too_big(v, field_width))
          {
            Err_act::value_too_big(v, field_width);

            return(false);
          }

        return(true);
      }

    static const bool Fmt_offset_from_start = true;

    static const bool Fmt_align_at_zero_offset = true;

  private:

    typedef Bitfield_impl::Checks<V_t, S_t> Bf_checks;

  }; // class Bitfields_traits_default

struct Bitfield_format
  {
    template <unsigned Width>
    struct F { char x[Width]; };
  };

template <class Bitfield_traits = Bitfield_traits_default<unsigned> >
class Bitfield
  : public
      Bitfield_impl::Checks<
        typename Bitfield_traits::Value_t,
        typename Bitfield_traits::Storage_t>
  {
  public:

    typedef Bitfield_traits Traits;

    typedef typename Traits::Value_t Value_t;

    typedef typename Traits::Storage_t Storage_t;

    typedef typename Traits::Storage_read_t Storage_read_t;

    typedef typename Traits::Storage_access_t Storage_access_t;

    static const bool Storage_ls_bit_first = Traits::Storage_ls_bit_first;

    static const bool Fmt_offset_from_start = Traits::Fmt_offset_from_start;

    static const bool Fmt_align_at_zero_offset =
      Traits::Fmt_align_at_zero_offset;

    static const unsigned Storage_bits =
      Bitfield_impl::Num_bits<Storage_t>::Value;

    static Value_t read(
      Storage_read_t base, unsigned first_bit, unsigned field_width)
      {
        if (!Traits::check_width(field_width))
          return(~(Value_t(0)));

        base += (first_bit / Storage_bits);
        first_bit %= Storage_bits;

        if (Traits::Storage_ls_bit_first)
          return(
            Bitfield_impl::Ls_read<Bitfield_traits, 0>::x(
              base, first_bit, field_width));

        return(
          Bitfield_impl::Ms_read<Bitfield_traits, 0>::x(
            base, first_bit, field_width));
      }

    template <class Modifier>
    static bool modify(
      Storage_access_t base, unsigned first_bit, unsigned field_width,
      Modifier m)
      {
        if (!Traits::check_width(field_width))
          return(false);

        if (!Traits::check_fit(m.value(), field_width))
          return(false);

        base += (first_bit / Storage_bits);
        first_bit %= Storage_bits;

        if (Traits::Storage_ls_bit_first)
          Bitfield_impl::Ls_modify<Bitfield_traits, Modifier, 0>::x(
            base, first_bit, 0, field_width, m);
        else
          Bitfield_impl::Ms_modify<Bitfield_traits, Modifier, 0>::x(
            base, first_bit, field_width, m);

        return(true);
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

    class Mod_zero : public Modifier_base
      {
      public:

        void operator () (
          Storage_access_t s, unsigned storage_shift, unsigned,
          unsigned storage_width)
          { s.write(this->clear(s, storage_shift, storage_width)); }
      };

    static bool zero(
      Storage_access_t base, unsigned first_bit, unsigned field_width)
      { return(modify(base, first_bit, field_width, Mod_zero())); }

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

    static bool write(
      Storage_access_t base, unsigned first_bit, unsigned field_width,
      Value_t val)
      { return(modify(base, first_bit, field_width, Mod_write(val))); }

    template<class Format>
    struct Define
      {
        static const unsigned Storage_bits =
          Bitfield_impl::Num_bits<Storage_t>::Value;

        static const unsigned Dimension =
          (sizeof(Format) + Storage_bits - 1) / Storage_bits;

        typedef Storage_t T[Dimension];
      };

    template<class Format, unsigned Width>
    static unsigned field_width(Bitfield_format::F<Width> Format::*)
      { return(Width); }

    template<class Format, unsigned Width>
    static unsigned field_offset(
      Bitfield_format::F<Width> Format::*field, unsigned base_offset = 0)
      {
        unsigned offset =
          (reinterpret_cast<Format *>(0x100)->*field).x -
           reinterpret_cast<char *>(0x100);

        if (!Bitfield::Fmt_offset_from_start)
          offset = sizeof(Format) - Width - offset;

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

    template<class Format, unsigned Width>
    static Value_t read(
      Storage_read_t base, Bitfield_format::F<Width> Format::*field,
      unsigned offset = 0)
      {
        return(read(base, field_offset(field, offset), field_width(field)));
      }

    template<class Format, unsigned Width, class Modifier>
    static bool modify(
      Storage_access_t base, Bitfield_format::F<Width> Format::*field,
      Modifier m, unsigned offset = 0)
      {
        return(
          modify(base, field_offset(field, offset), field_width(field), m));
      }

    template<class Format, unsigned Width>
    static bool zero(
      Storage_access_t base, Bitfield_format::F<Width> Format::*field,
      unsigned offset = 0)
      {
        return(zero(base, field_offset(field, offset), field_width(field)));
      }

    template<class Format, unsigned Width>
    static bool write(
      Storage_access_t base, Bitfield_format::F<Width> Format::*field,
      Value_t val, unsigned offset = 0)
      {
        return(
          write(base, field_offset(field, offset), field_width(field), val));
      }

  protected:

    static const bool Uh_oh =
      Bitfield_impl::Num_bits<Value_t>::Value >
      (Bitfield_impl::Max_value_to_storage_bits_ratio * Storage_bits);

    // If the line following this comment causes a compile error, the
    // number of bits in Value_t is more than Max_value_to_storage_bits_ratio
    // times the number of bits in Storage_t, which is not allowed.
    static int not_used[1 / (Uh_oh ? 0 : 1)];

  }; // class Bitfield

template <class Bf, class Fmt>
struct Bitfield_w_fmt : public Bf
  {
    static const unsigned Storage_bits = Bf::Storage_bits;

    template <class Base_fmt>
    static unsigned base_offset(unsigned derived_offset = 0)
      {
        const unsigned From_start_unaligned =
          reinterpret_cast<char *>(static_cast<Base_fmt *>(
            reinterpret_cast<Fmt *>(0x100))) - 
          reinterpret_cast<char *>(0x100);

        const unsigned Unaligned =
          Bf::Fmt_offset_from_start ?
            From_start_unaligned :
            sizeof(Fmt) - sizeof(Base_fmt) - From_start_unaligned;

        return(
          Unaligned +
          ((!Bf::Fmt_align_at_zero_offset &&
            ((sizeof(Fmt) % Storage_bits) != 0) &&
            (derived_offset == 0)) ?
             Storage_bits - (sizeof(Fmt) % Storage_bits) : derived_offset));
      }

    typedef Fmt Format;
  };

#define BITF_OFFSET_FROM_START(BWF, FIELD_SPEC) \
  (reinterpret_cast<char *>( \
     &(reinterpret_cast<typename BWF::Format *>(0x100)->FIELD_SPEC)) - \
   reinterpret_cast<char *>(0x100))

#define BITF_WIDTH(BWF, FIELD_SPEC) \
  (sizeof(reinterpret_cast<typename BWF::Format *>(0x100)->FIELD_SPEC))

#define BITF_OFFSET_FROM_END(BWF, FIELD_SPEC) \
  (sizeof(typename BWF::Format) - BITF_WIDTH(BWF, FIELD_SPEC) - \
   BITF_OFFSET_FROM_START(BWF, FIELD_SPEC))

#define BITF_OFFSET_UNALIGNED(BWF, FIELD_SPEC) \
  (BWF::Fmt_offset_from_start ? \
    BITF_OFFSET_FROM_START(BWF, FIELD_SPEC) : \
    BITF_OFFSET_FROM_END(BWF, FIELD_SPEC))

#define BITF_W_OFS(BWF, FIELD_SPEC, OFS) \
  (BITF_OFFSET_UNALIGNED(BWF, FIELD_SPEC) + (OFS)), BITF_WIDTH(BWF, FIELD_SPEC)

#define BITF_OFFSET(BWF, FIELD_SPEC) \
  (BITF_OFFSET_UNALIGNED(BWF, FIELD_SPEC) + \
  (BWF::Fmt_align_at_zero_offset ? \
     0 : ((sizeof(typename BWF::Format) % BWF::Storage_bits) ? \
            (BWF::Storage_bits - \
              (sizeof(typename BWF::Format) % BWF::Storage_bits)) : 0)))

// Because C++ does not accept the syntax &C::x.b (where b is in a structure
// x inside of class C), use BITF(C, x.b) as a more general way to get
// a bit field's offset and width separated by a comma.
//
#define BITF(BWF, FIELD_SPEC) \
  BITF_OFFSET(BWF, FIELD_SPEC), BITF_WIDTH(BWF, FIELD_SPEC)

#endif // Include once.
