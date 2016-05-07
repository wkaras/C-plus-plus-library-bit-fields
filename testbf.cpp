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

// NOTE:  When compiling this code with gcc, the option -ftemplate-depth=1100
// is necessary.

#include "bitfield.h"

#include <stdint.h>
#include <iostream>
#include <cstdlib>
#include <cstddef>

#include "testloop.h"

inline bool is_big_endian()
  {
    const uint16_t u = 0x1234;

    return(reinterpret_cast<const uint8_t &>(u) == 0x12);
  }

uint64_t Saw16 = 0x5555;

uint64_t Saw = (((((Saw16 << 16) | Saw16) << 16) | Saw16) << 16) | Saw16;

template <typename S_t, unsigned Num_s>
struct Reverse
  {
    static void x(S_t *dest, const S_t *src)
      {
        *dest = src[Num_s - 1];
        Reverse<S_t, Num_s - 1>::x(dest + 1, src);
      }
  };

template <typename S_t>
struct Reverse<S_t, 0>
  {
    static void x(S_t *, const S_t *) { }
  };

template <typename S_t, bool Ls_first>
class Test_val
  {
  private:

    uint64_t v[3];

    static const unsigned Num_s = sizeof(uint64_t) / sizeof(S_t);

    void set(uint64_t v_)
      {
        v[0] = ~Saw;
        v[2] = Saw;

        if (is_big_endian() != Ls_first)
          v[1] = v_;
        else
          Reverse<S_t, Num_s>::x(
            reinterpret_cast<S_t *>(v + 1), reinterpret_cast<S_t *>(&v_));
      }

  public:

    Test_val() { }

    Test_val(uint64_t v_) { set(v_); }

    void operator = (uint64_t v_) { set(v_); }

    operator uint64_t () const
      {
        uint64_t result;

        if (is_big_endian() != Ls_first)
          result = v[1];
        else
          Reverse<S_t, Num_s>::x(
            reinterpret_cast<S_t *>(&result),
            reinterpret_cast<const S_t *>(v + 1));

        if (v[0] != ~Saw)
          {
            cout << "FAIL corrupt\n";

            exit(1);
          }

        if (v[2] != Saw)
          {
            cout << "FAIL corrupt 2\n";

            exit(1);
          }

        return(result);
      }

    S_t * addr() { return(reinterpret_cast<S_t *>(v)); }

    const S_t * caddr() const { return(reinterpret_cast<const S_t *>(v)); }
  };

template <unsigned Offset, unsigned Width>
struct Format : private Bitfield_format
  {
    F<32> pad[2];
    F<Offset> ofs;
    F<Width> data;
  };

template <unsigned Width>
struct Format<0, Width> : private Bitfield_format
  {
    F<32> pad[2];
    F<Width> data;
  };

inline uint64_t saw(
  unsigned bit_width, uint64_t pattern = Saw)
  {
    if (bit_width < 64)
      pattern >>= (64 - bit_width);
    return(pattern);
  }

inline uint64_t saw2(unsigned bit_width)
  { return(saw(bit_width, ~Saw)); }

inline uint64_t ones(unsigned bit_width)
  { return(saw(bit_width, ~uint64_t(0))); }

template <class Bf, class Fmt>
class One_test_2f : private Test_base
  {
    virtual bool test()
      {
        typedef typename Bf::Storage_t Storage_t;

        const unsigned F_w = Bf::field_width(&Fmt::data);

        const unsigned F_shift =
          Bf::Storage_ls_bit_first ?
            Bf::field_offset(&Fmt::data) - 64 :
            128 - Bf::field_offset(&Fmt::data) - F_w;

        Test_val<Storage_t, Bf::Storage_ls_bit_first> x =
          ~(ones(F_w) << F_shift);

        if (Bf::read(x.caddr(), &Fmt::data) != 0)
          return(false);

        x = x | saw(F_w) << F_shift;

        if (Bf::read(x.caddr(), &Fmt::data) != saw(F_w))
          return(false);

        x = (saw2(F_w) << F_shift) | ~(ones(F_w) << F_shift);

        if (Bf::read(x.caddr(), &Fmt::data) != saw2(F_w))
          return(false);

        x = ~uint64_t(0);

        if (!Bf::zero(x.addr(), &Fmt::data))
          return(false);
        if (x != ~(ones(F_w) << F_shift))
          return(false);

        const uint64_t y = ~(ones(F_w) << F_shift);
        x = y;
        if (!Bf::write(x.addr(), &Fmt::data, saw(F_w)))
          return(false);
        if (x != ((saw(F_w) << F_shift) | y))
          return(false);

        x = y;
        if (!Bf::write(x.addr(), &Fmt::data, saw2(F_w)))
          return(false);
        if (x != ((saw2(F_w) << F_shift) | y))
          return(false);

        if ((F_w != Bitfield_impl::Num_bits<typename Bf::Value_t>::Value) &&
            Bf::write(x.addr(), &Fmt::data, uint64_t(1) << F_w))
          return(false);

        return(true);
      }
  };

template <class Bf, unsigned Ofs, unsigned W>
class Test_2f : private Test_2f<Bf, Ofs - 1, W>
  {
    One_test_2f<Bf, Format<Ofs, W> > t;
  };

template <class Bf, unsigned W>
class Test_2f<Bf, 0, W> : private Test_2f<Bf, 32, W - 1>
  {
    One_test_2f<Bf, Format<0, W> > t;
  };

template <class Bf>
class Test_2f<Bf, 32, 0> { };

#define OFFSET_TEST_ONLY 0
#define QUICK_TEST 0

#if !OFFSET_TEST_ONLY 

typedef Bitfield<Bitfield_traits_default<uint32_t, uint8_t> > Bf8_ls;

Test_2f<Bf8_ls, 32, 32> t_2f_8_ls;

#if !QUICK_TEST

typedef Bitfield<Bitfield_traits_default<uint32_t, uint16_t> > Bf16_ls;

Test_2f<Bf16_ls, 32, 32> t_2f_16_ls;

typedef Bitfield<Bitfield_traits_default<uint32_t> > Bf32_ls;

Test_2f<Bf32_ls, 32, 32> t_2f_32_ls;

#endif

template <typename V_t, typename S_t = V_t>
struct Bitfield_traits_ms : public Bitfield_traits_default<V_t, S_t>
  {
    static const bool Storage_ls_bit_first = false;
  };

typedef Bitfield<Bitfield_traits_ms<uint32_t, uint8_t> > Bf8_ms;

Test_2f<Bf8_ms, 32, 32> t_2f_8_ms;

#if !QUICK_TEST

typedef Bitfield<Bitfield_traits_ms<uint32_t, uint16_t> > Bf16_ms;

Test_2f<Bf16_ms, 32, 32> t_2f_16_ms;

typedef Bitfield<Bitfield_traits_ms<uint32_t, uint32_t> > Bf32_ms;

Test_2f<Bf32_ms, 32, 32> t_2f_32_ms;

#endif

#endif // !OFFSET_TEST_ONLY

class Test_field_offset : private Test_base
  {
    template <unsigned Bw>
    struct Format : private Bitfield_format
      {
        F<1> a;
        union
          {
            F<Bw> b;
            struct
              {
                F<11> c;
                F<7> d;
              }
            x;
          };
        F<5> e[3]; 
      };

    template <class Bf, unsigned Bw>
    bool t(unsigned base_ofs, unsigned ss, unsigned oa, unsigned ob,
           unsigned oc, unsigned od, unsigned oe0, unsigned oe2 )
      {
        typedef Format<Bw> Fmt;

        typedef Bitfield_w_fmt<Bf, Fmt> Bwf;

        #define FOFS(BWF, FIELD_SPEC, OFS) \
          ((OFS) ? (BITF_OFFSET_UNALIGNED(BWF, FIELD_SPEC) + (OFS)) : \
                   BITF_OFFSET(BWF, FIELD_SPEC))

        if (sizeof(typename Bf::template Define<Fmt>::T) != ss)
          return(false);

        if (Bf::field_offset(&Fmt::a, base_ofs) != oa)
          return(false);

        if (Bf::field_offset(&Fmt::b, base_ofs) != ob)
          return(false);

        if (FOFS(Bwf, x.c, base_ofs) != oc)
          return(false);

        if (FOFS(Bwf, x.d, base_ofs) != od)
          return(false);

        if (FOFS(Bwf, e[0], base_ofs) != oe0)
          return(false);

        if (FOFS(Bwf, e[2], base_ofs) != oe2)
          return(false);

        return(true);
      }

    struct Bft1 : public Bitfield_traits_default<uint32_t, uint8_t>
      {
        static const bool Fmt_offset_from_start = false;
      };

    struct Bft2 : public Bitfield_traits_default<uint16_t>
      {
        static const bool Fmt_align_at_zero_offset = false;
      };

    struct Bft3 : public Bft2
      {
        static const bool Fmt_offset_from_start = false;
      };

    virtual bool test()
      {
        if (!t<Bitfield<Bitfield_traits_default<uint32_t> >, 37>(
               0, 8, 0, 1, 1, 12, 38, 48))
          return(false);

        if (!t<Bitfield<Bitfield_traits_default<uint32_t, uint8_t> >, 37>(
               0, 7, 0, 1, 1, 12, 38, 48))
          return(false);

        if (!t<Bitfield<Bitfield_traits_default<uint32_t, uint8_t> >, 1>(
               0, 5, 0, 1, 1, 12, 19, 29))
          return(false);

        if (!t<Bitfield<Bft1>, 37>(0, 7, 52, 15, 41, 34, 10, 0))
          return(false);

        if (!t<Bitfield<Bft2>, 37>(0, 8, 11, 12, 12, 23, 49, 59))
          return(false);

        if (!t<Bitfield<Bft2>, 37>(7, 8, 7, 8, 8, 19, 45, 55))
          return(false);

        if (!t<Bitfield<Bft3>, 37>(0, 8, 63, 26, 52, 45, 21, 11))
          return(false);

        return(true);
      }
  };

Test_field_offset test_field_offset;

class Test_base_offset : private Test_base
  {
    class A : public Bitfield_format { F<4> i, j; };

    class B : public Bitfield_format { F<13> x; };

    class C : public Bitfield_format { F<27> x; };

    class D1 { public: A a; B b; C c; Bitfield_format::F<8> y; };

    class D2 : public A, public B, public C
      { Bitfield_format::F<8> y; };

    struct Bft2 : public Bitfield_traits_default<uint16_t>
      {
        static const bool Fmt_offset_from_start = false;
      };

    struct Bft3 : public Bitfield_traits_default<uint16_t>
      {
        static const bool Fmt_align_at_zero_offset = false;
      };

    struct Bft4 : public Bft2
      {
        static const bool Fmt_align_at_zero_offset = false;
      };

    typedef
      Bitfield_w_fmt<Bitfield<Bitfield_traits_default<uint32_t> >, D2> Bwf1;

    typedef Bitfield_w_fmt<Bitfield<Bft2>, D2> Bwf2;

    typedef Bitfield_w_fmt<Bitfield<Bft3>, D2> Bwf3;

    typedef Bitfield_w_fmt<Bitfield<Bft4>, D2> Bwf4;

    virtual bool test()
      {
        #undef X
        #define X(Y, N) \
        if (!((Y(A, a) == Bwf##N::base_offset<A>()) && \
              (Y(B, b) == Bwf##N::base_offset<B>()) && \
              (Y(C, c) == Bwf##N::base_offset<C>()))) \
          return(false);

        #define Z(S, F) offsetof(D1, F)
        X(Z, 1)
        #undef Z

        #define Z(S, F) (sizeof(D1) - sizeof(S) - offsetof(D1, F))
        X(Z, 2)
        #undef Z

        #define Z(S, F) (offsetof(D1, F) + 8)
        X(Z, 3)
        #undef Z

        #define Z(S, F) (sizeof(D1) - sizeof(S) - offsetof(D1, F) + 8)
        X(Z, 4)
        #undef Z

        #undef X

        return(true);
      }
  };

Test_base_offset test_base_offset;
