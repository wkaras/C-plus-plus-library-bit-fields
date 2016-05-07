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

#include "testloop.h"

#include <iostream>
#include <stdint.h>
#include <cstdlib>
#include <cstddef>
#include <vector>

inline bool is_big_endian()
  {
    const uint16_t u = 0x1234;

    return(reinterpret_cast<const uint8_t &>(u) == 0x12);
  }

uint64_t Saw16 = 0x5555;

uint64_t Saw = (((((Saw16 << 16) | Saw16) << 16) | Saw16) << 16) | Saw16;

uint64_t Saw3_16 = 0x3333;

uint64_t Saw3 =
  (((((Saw3_16 << 16) | Saw3_16) << 16) | Saw3_16) << 16) | Saw3_16;

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
struct Format
  {
    BITF_DEF_F

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

#define NO_2F 1
#define QUICK_TEST 0

#if !NO_2F 

class One_test_2f_c : private Test_base
  {
    typedef Bitfield<Bitfield_traits_default<uint32_t, const uint16_t> > Bf;

    typedef uint16_t Mutable_storage_t;

    typedef Bf::Value_t Value_t;

    typedef Format<15, 17> Fmt;

    typedef Bitfield_w_fmt<Bf, Fmt> Bwf;

    static Value_t sign_ext(Value_t v, unsigned field_width)
      {
        if (((v >> (field_width - 1)) & 1) && field_width)
          v |= ~Bf::mask(field_width);

        return(v);
      }

    virtual bool test()
      {
        const unsigned F_w = Bf::field_width(&Fmt::data);

        const unsigned F_shift =
          Bf::Storage_ls_bit_first ?
            Bf::field_offset(&Fmt::data) - 64 :
            128 - Bf::field_offset(&Fmt::data) - F_w;

        Test_val<Mutable_storage_t, Bf::Storage_ls_bit_first> x =
          ~(ones(F_w) << F_shift);

        if (Bf::f(x.caddr(), &Fmt::data) != 0)
          return(false);

        if (Bf::f(x.caddr(), &Fmt::data).read_sign_extend() != 0)
          return(false);

        x = x | saw(F_w) << F_shift;

        #if 0

        // This code seems to trigger a G++ bug when compiled with
        // level 3 optimization.

        if (Bf::f(x.caddr(), &Fmt::data) != saw(F_w))
          return(false);

        if (Bf::f(x.caddr(), &Fmt::data).read_sign_extend() !=
            sign_ext(saw(F_w), F_w))
          return(false);

        x = (saw2(F_w) << F_shift) | ~(ones(F_w) << F_shift);

        if (Bf::f(x.caddr(), &Fmt::data) != saw2(F_w))
          return(false);

        #else

        if (BITF(Bwf, x.caddr(), data) != saw(F_w))
          return(false);

        if (BITF(Bwf, x.caddr(), data).read_sign_extend() !=
            sign_ext(saw(F_w), F_w))
          return(false);

        x = (saw2(F_w) << F_shift) | ~(ones(F_w) << F_shift);

        if (BITF(Bwf, x.caddr(), data) != saw2(F_w))
          return(false);

        #endif

        if (Bf::f(x.caddr(), &Fmt::data).read_sign_extend() !=
            sign_ext(saw2(F_w), F_w))
          return(false);

        return(true);
      }
  };

One_test_2f_c test_2f_c;

template <class Bf, class Fmt>
class One_test_2f : private Test_base
  {
    typedef typename Bf::Value_t Value_t;

    static Value_t sign_ext(Value_t v, unsigned field_width)
      {
        if (((v >> (field_width - 1)) & 1) && field_width)
          v |= ~Bf::mask(field_width);

        return(v);
      }

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

        if (Bf::f(x.addr(), &Fmt::data) != 0)
          return(false);

        if (Bf::f(x.addr(), &Fmt::data).read_sign_extend() != 0)
          return(false);

        x = x | saw(F_w) << F_shift;

        if (Bf::f(x.addr(), &Fmt::data) != saw(F_w))
          return(false);

        if (Bf::f(x.addr(), &Fmt::data).read_sign_extend() !=
            sign_ext(saw(F_w), F_w))
          return(false);

        x = (saw2(F_w) << F_shift) | ~(ones(F_w) << F_shift);

        if (Bf::f(x.addr(), &Fmt::data) != saw2(F_w))
          return(false);

        if (Bf::f(x.addr(), &Fmt::data).read_sign_extend() !=
            sign_ext(saw2(F_w), F_w))
          return(false);

        x = ~uint64_t(0);

        if (!Bf::f(x.addr(), &Fmt::data).zero())
          return(false);
        if (x != ~(ones(F_w) << F_shift))
          return(false);

        const uint64_t y = ~(ones(F_w) << F_shift);
        x = y;
        if (!Bf::f(x.addr(), &Fmt::data).write(saw(F_w)))
          return(false);
        if (x != ((saw(F_w) << F_shift) | y))
          return(false);

        x = y;
        Bf::f(x.addr(), &Fmt::data) = saw(F_w);
        if (x != ((saw(F_w) << F_shift) | y))
          return(false);

        x = y;
        if (!Bf::f(x.addr(), &Fmt::data).write(saw2(F_w)))
          return(false);
        if (x != ((saw2(F_w) << F_shift) | y))
          return(false);

        const unsigned Value_bits = Bitfield_impl::Num_bits<Value_t>::Value;

        if ((F_w != Value_bits) &&
            Bf::f(x.addr(), &Fmt::data).write(uint64_t(1) << F_w))
          return(false);

        x = Saw;
        if (!Bf::f(x.addr(), &Fmt::data).b_comp())
          return(false);
        if (x != (Saw ^ (Bitfield_impl::mask<uint64_t>(F_w) << F_shift)))
          return(false);

        x = Saw3;
        if (!Bf::f(x.addr(), &Fmt::data).b_and(saw(F_w, Saw3)))
          return(false);
        if (x !=
            (((saw(F_w, Saw3) << F_shift) |
              ~(Bitfield_impl::mask<uint64_t>(F_w) << F_shift)) & Saw3))
          return(false);

        x = Saw3;
        Bf::f(x.addr(), &Fmt::data) &= saw(F_w, Saw3);
        if (x !=
            (((saw(F_w, Saw3) << F_shift) |
              ~(Bitfield_impl::mask<uint64_t>(F_w) << F_shift)) & Saw3))
          return(false);

        x = Saw3;
        if (!Bf::f(x.addr(), &Fmt::data).b_or(saw(F_w, Saw3)))
          return(false);
        if (x != ((saw(F_w, Saw3) << F_shift) | Saw3))
          return(false);

        x = Saw3;
        Bf::f(x.addr(), &Fmt::data) |= saw(F_w, Saw3);
        if (x != ((saw(F_w, Saw3) << F_shift) | Saw3))
          return(false);

        x = Saw3;
        if (!Bf::f(x.addr(), &Fmt::data).b_xor(saw(F_w, Saw3)))
          return(false);
        if (x != ((saw(F_w, Saw3) << F_shift) ^ Saw3))
          return(false);

        x = Saw3;
        Bf::f(x.addr(), &Fmt::data) ^= saw(F_w, Saw3);
        if (x != ((saw(F_w, Saw3) << F_shift) ^ Saw3))
          return(false);

        return(true);
      }
  };

#define TEST_2F_1(BF, OFS, W) \
One_test_2f<BF, Format<OFS, W> > test_2f_ ## BF ## OFS ## _ ## W;

#define TEST_2F_2(BF, OFS) \
TEST_2F_1(BF, OFS, 1) \
TEST_2F_1(BF, OFS, 2) \
TEST_2F_1(BF, OFS, 3) \
TEST_2F_1(BF, OFS, 4) \
TEST_2F_1(BF, OFS, 5) \
TEST_2F_1(BF, OFS, 6) \
TEST_2F_1(BF, OFS, 7) \
TEST_2F_1(BF, OFS, 8) \
TEST_2F_1(BF, OFS, 9) \
TEST_2F_1(BF, OFS, 10) \
TEST_2F_1(BF, OFS, 11) \
TEST_2F_1(BF, OFS, 12) \
TEST_2F_1(BF, OFS, 13) \
TEST_2F_1(BF, OFS, 14) \
TEST_2F_1(BF, OFS, 15) \
TEST_2F_1(BF, OFS, 16) \
TEST_2F_1(BF, OFS, 17) \
TEST_2F_1(BF, OFS, 18) \
TEST_2F_1(BF, OFS, 19) \
TEST_2F_1(BF, OFS, 20) \
TEST_2F_1(BF, OFS, 21) \
TEST_2F_1(BF, OFS, 22) \
TEST_2F_1(BF, OFS, 23) \
TEST_2F_1(BF, OFS, 24) \
TEST_2F_1(BF, OFS, 25) \
TEST_2F_1(BF, OFS, 26) \
TEST_2F_1(BF, OFS, 27) \
TEST_2F_1(BF, OFS, 28) \
TEST_2F_1(BF, OFS, 29) \
TEST_2F_1(BF, OFS, 30) \
TEST_2F_1(BF, OFS, 31) \
TEST_2F_1(BF, OFS, 32)

#define TEST_2F(BF) \
TEST_2F_2(BF, 0) \
TEST_2F_2(BF, 1) \
TEST_2F_2(BF, 2) \
TEST_2F_2(BF, 3) \
TEST_2F_2(BF, 4) \
TEST_2F_2(BF, 5) \
TEST_2F_2(BF, 6) \
TEST_2F_2(BF, 7) \
TEST_2F_2(BF, 8) \
TEST_2F_2(BF, 9) \
TEST_2F_2(BF, 10) \
TEST_2F_2(BF, 11) \
TEST_2F_2(BF, 12) \
TEST_2F_2(BF, 13) \
TEST_2F_2(BF, 14) \
TEST_2F_2(BF, 15) \
TEST_2F_2(BF, 16) \
TEST_2F_2(BF, 17) \
TEST_2F_2(BF, 18) \
TEST_2F_2(BF, 19) \
TEST_2F_2(BF, 20) \
TEST_2F_2(BF, 21) \
TEST_2F_2(BF, 22) \
TEST_2F_2(BF, 23) \
TEST_2F_2(BF, 24) \
TEST_2F_2(BF, 25) \
TEST_2F_2(BF, 26) \
TEST_2F_2(BF, 27) \
TEST_2F_2(BF, 28) \
TEST_2F_2(BF, 29) \
TEST_2F_2(BF, 30) \
TEST_2F_2(BF, 31) \
TEST_2F_2(BF, 32)

typedef Bitfield<Bitfield_traits_default<uint32_t, uint8_t> > Bf8_ls;

TEST_2F(Bf8_ls)

#if !QUICK_TEST

typedef Bitfield<Bitfield_traits_default<uint32_t, uint16_t> > Bf16_ls;

typedef Bitfield<Bitfield_traits_default<uint32_t> > Bf32_ls;

TEST_2F(Bf32_ls)

#endif

template <typename V_t, typename S_t = V_t>
struct Bitfield_traits_ms : public Bitfield_traits_default<V_t, S_t>
  {
    static const bool Storage_ls_bit_first = false;
  };

typedef Bitfield<Bitfield_traits_ms<uint32_t, uint8_t> > Bf8_ms;

TEST_2F(Bf8_ms)

#if !QUICK_TEST

typedef Bitfield<Bitfield_traits_ms<uint32_t, uint16_t> > Bf16_ms;

TEST_2F(Bf16_ms)

typedef Bitfield<Bitfield_traits_ms<uint32_t, uint32_t> > Bf32_ms;

TEST_2F(Bf32_ms)

#endif

#endif // !NO_2F

class Test_value_mask : private Test_base
  {
    typedef Bitfield<Bitfield_traits_default<uint32_t> > Bf;

    virtual bool test()
      {
        if (Bf::mask(0) != 0)
          return(false);

        if (Bf::mask(1) != 1)
          return(false);

        if (Bf::mask(19) != ((1 << 19) - 1))
          return(false);

        if (Bf::mask(32) != ~uint32_t(0))
          return(false);

        return(true);
      }
  };

Test_value_mask test_value_mask;

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

        typename Bf::Storage_t base[1];

        if (Bwf::Fmt_offset_from_start && Bwf::Fmt_align_at_zero_offset)
          {
            if (BITF_CAT(Bwf, base, b, e[1]).offset() != (ob - base_ofs))
              return(false);

            if (BITF_CAT(Bwf, base, b, e[1]).width() != (oe2 - ob))
              return(false);
          }
        else if (!Bwf::Fmt_offset_from_start && Bwf::Fmt_align_at_zero_offset)
          {
            if (BITF_CAT(Bwf, base, b, e[0]).offset() != (oe0 - base_ofs))
              return(false);

            if (BITF_CAT(Bwf, base, b, e[0]).width() != (oa - oe0))
              return(false);
          }

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

    class D1 { BITF_DEF_F public: A a; B b; C c; F<8> y; };

    class D2 : public A, public B, public C
      { F<8> y; };

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

namespace Test_write_seq
{

const int Op_read = -1;
const int Op_write = -2;
const int Op_offset = -3;

std::vector<int> op;

unsigned check_idx;

void check_op(int v)
  {
    if (op[check_idx] != v)
      {
        cout << "FAIL Test_write_seq::check_op() "
             << "check_idx=" << check_idx << " expected=" << op[check_idx]
             << " actual=" << v << '\n';
        exit(1);
      }

    ++check_idx;
  }

bool reset_op()
  {
    if (check_idx != op.size())
      {
        cout << "FAIL Test_write_seq::reset_op() "
             << "check_idx=" << check_idx << " size=" << op.size() << '\n';
        return(false);
      }

    op.resize(0);
    check_idx = 0;

    return(true);
  }

struct Dummy_sa_t
  {
    typedef uint16_t Storage_t;

    void operator += (unsigned offset)
      { check_op(Op_offset); check_op(int(offset)); }

    uint16_t read() { check_op(Op_read); return(0); }

    void write(uint16_t) { check_op(Op_write); }
  };

Dummy_sa_t dsa_;

template <class Traits = Bitfield_seq_storage_write_default_traits>
struct Bft_ : public Bitfield_traits_default<uint32_t, uint16_t>
  {
    typedef Bitfield_seq_storage_write_t<Dummy_sa_t, Traits> Storage_access_t;
  };

namespace T1
{

class Fmt : private Bitfield_format
  {
  public:

    F<1> f[3 * 16];
  };

typedef
  Bitfield_w_fmt<Bitfield_traits_default<uint32_t, uint16_t>, Fmt> Bwf_def;

typedef Bft_<> Bft;

typedef Bitfield_w_fmt<Bitfield<Bft>, Fmt> Bwf;

class Test : private Test_base
  {
    virtual bool test()
      {
        {
          Bitfield_seq_storage_write_buf<
            Dummy_sa_t, Bitfield_seq_storage_write_default_traits> dsa(dsa_);

          op.push_back(Op_offset);
          op.push_back(0);
          op.push_back(Op_write);

          BITF(Bwf, dsa, f[0]) = 0;
        }
        if (!reset_op())
          return(false);

        {
          Bitfield_seq_storage_write_buf<
            Dummy_sa_t, Bitfield_seq_storage_write_default_traits> dsa(dsa_);

          op.push_back(Op_offset);
          op.push_back(0);
          op.push_back(Op_write);

          BITF_CAT(Bwf, dsa, f[0], f[7]) = 0;
        }
        if (!reset_op())
          return(false);

        {
          Bitfield_seq_storage_write_buf<
            Dummy_sa_t, Bitfield_seq_storage_write_default_traits> dsa(dsa_);

          op.push_back(Op_offset);
          op.push_back(0);
          op.push_back(Op_write);

          BITF_CAT(Bwf, dsa, f[0], f[15]) = 0;
        }
        if (!reset_op())
          return(false);

        {
          Bitfield_seq_storage_write_buf<
            Dummy_sa_t, Bitfield_seq_storage_write_default_traits> dsa(dsa_);

          op.push_back(Op_offset);
          op.push_back(0);
          op.push_back(Op_write);
          op.push_back(Op_offset);
          op.push_back(1);
          op.push_back(Op_write);


          BITF_CAT(Bwf, dsa, f[0], f[16]) = 0;
        }
        if (!reset_op())
          return(false);

        {
          Bitfield_seq_storage_write_buf<
            Dummy_sa_t, Bitfield_seq_storage_write_default_traits> dsa(dsa_);

          op.push_back(Op_offset);
          op.push_back(0);
          op.push_back(Op_write);
          op.push_back(Op_offset);
          op.push_back(1);
          op.push_back(Op_write);


          BITF_CAT(Bwf, dsa, f[0], f[30]) = 0;
        }
        if (!reset_op())
          return(false);

        {
          Bitfield_seq_storage_write_buf<
            Dummy_sa_t, Bitfield_seq_storage_write_default_traits> dsa(dsa_);

          op.push_back(Op_offset);
          op.push_back(0);
          op.push_back(Op_write);
          op.push_back(Op_offset);
          op.push_back(1);
          op.push_back(Op_write);


          BITF_CAT(Bwf, dsa, f[0], f[31]) = 0;
        }
        if (!reset_op())
          return(false);

        {
          Bitfield_seq_storage_write_buf<
            Dummy_sa_t, Bitfield_seq_storage_write_default_traits> dsa(dsa_);

          op.push_back(Op_offset);
          op.push_back(0);
          op.push_back(Op_write);

          BITF(Bwf, dsa, f[1]) = 0;
        }
        if (!reset_op())
          return(false);

        {
          Bitfield_seq_storage_write_buf<
            Dummy_sa_t, Bitfield_seq_storage_write_default_traits> dsa(dsa_);

          op.push_back(Op_offset);
          op.push_back(0);
          op.push_back(Op_write);

          BITF_CAT(Bwf, dsa, f[1], f[7]) = 0;
        }
        if (!reset_op())
          return(false);

        {
          Bitfield_seq_storage_write_buf<
            Dummy_sa_t, Bitfield_seq_storage_write_default_traits> dsa(dsa_);

          op.push_back(Op_offset);
          op.push_back(0);
          op.push_back(Op_write);

          BITF_CAT(Bwf, dsa, f[1], f[15]) = 0;
        }
        if (!reset_op())
          return(false);

        {
          Bitfield_seq_storage_write_buf<
            Dummy_sa_t, Bitfield_seq_storage_write_default_traits> dsa(dsa_);

          op.push_back(Op_offset);
          op.push_back(0);
          op.push_back(Op_write);
          op.push_back(Op_offset);
          op.push_back(1);
          op.push_back(Op_write);


          BITF_CAT(Bwf, dsa, f[1], f[16]) = 0;
        }
        if (!reset_op())
          return(false);

        {
          Bitfield_seq_storage_write_buf<
            Dummy_sa_t, Bitfield_seq_storage_write_default_traits> dsa(dsa_);

          op.push_back(Op_offset);
          op.push_back(0);
          op.push_back(Op_write);
          op.push_back(Op_offset);
          op.push_back(1);
          op.push_back(Op_write);


          BITF_CAT(Bwf, dsa, f[1], f[30]) = 0;
        }
        if (!reset_op())
          return(false);

        {
          Bitfield_seq_storage_write_buf<
            Dummy_sa_t, Bitfield_seq_storage_write_default_traits> dsa(dsa_);

          op.push_back(Op_offset);
          op.push_back(0);
          op.push_back(Op_write);
          op.push_back(Op_offset);
          op.push_back(1);
          op.push_back(Op_write);
          op.push_back(Op_offset);
          op.push_back(2);
          op.push_back(Op_write);

          BITF_CAT(Bwf, dsa, f[1], f[32]) = 0;
        }
        if (!reset_op())
          return(false);

        {
          Bitfield_seq_storage_write_buf<
            Dummy_sa_t, Bitfield_seq_storage_write_default_traits> dsa(dsa_);

          op.push_back(Op_offset);
          op.push_back(0);
          op.push_back(Op_write);

          BITF(Bwf, dsa, f[15]) = 0;
        }
        if (!reset_op())
          return(false);

        {
          Bitfield_seq_storage_write_buf<
            Dummy_sa_t, Bitfield_seq_storage_write_default_traits> dsa(dsa_);

          op.push_back(Op_offset);
          op.push_back(0);
          op.push_back(Op_write);
          op.push_back(Op_offset);
          op.push_back(1);
          op.push_back(Op_write);


          BITF_CAT(Bwf, dsa, f[15], f[16]) = 0;
        }
        if (!reset_op())
          return(false);

        {
          Bitfield_seq_storage_write_buf<
            Dummy_sa_t, Bitfield_seq_storage_write_default_traits> dsa(dsa_);

          op.push_back(Op_offset);
          op.push_back(0);
          op.push_back(Op_write);
          op.push_back(Op_offset);
          op.push_back(1);
          op.push_back(Op_write);


          BITF_CAT(Bwf, dsa, f[15], f[30]) = 0;
        }
        if (!reset_op())
          return(false);

        {
          Bitfield_seq_storage_write_buf<
            Dummy_sa_t, Bitfield_seq_storage_write_default_traits> dsa(dsa_);

          op.push_back(Op_offset);
          op.push_back(0);
          op.push_back(Op_write);
          op.push_back(Op_offset);
          op.push_back(1);
          op.push_back(Op_write);
          op.push_back(Op_offset);
          op.push_back(2);
          op.push_back(Op_write);

          BITF_CAT(Bwf, dsa, f[15], f[32]) = 0;
        }
        if (!reset_op())
          return(false);

        {
          Bitfield_seq_storage_write_buf<
            Dummy_sa_t, Bitfield_seq_storage_write_default_traits> dsa(dsa_);

          op.push_back(Op_offset);
          op.push_back(0);
          op.push_back(Op_write);
          op.push_back(Op_offset);
          op.push_back(1);
          op.push_back(Op_write);
          op.push_back(Op_offset);
          op.push_back(2);
          op.push_back(Op_write);

          BITF_CAT(Bwf, dsa, f[15], f[46]) = 0;
        }
        if (!reset_op())
          return(false);

        return(true);
      }
  };

Test t;

} // end namespce T1

namespace T2
{

class Fmt : private Bitfield_format
  {
  public:

    F<5> f1;
    F<15> f2;
    F<8> f3;
    F<18> f4;
    F<22> f5;
    F<4> f6;
    F<8> f7;
  };

typedef
  Bitfield_w_fmt<Bitfield_traits_default<uint32_t, uint16_t>, Fmt> Bwf_def;

struct Ssw_traits : public Bitfield_seq_storage_write_default_traits
  {
    enum { First_bit = BITF_OFFSET(Bwf_def, f2) };
    enum { End_bit = BITF_OFFSET(Bwf_def, f7) };
  };

static unsigned po = 0xabcd, no = 0xabcd;

struct Ssw_traits2 : public Bitfield_seq_storage_write_default_traits
  {
    static void handle_backwards(
      unsigned previous_offset, unsigned next_offset)
      {
        po = previous_offset;
        no = next_offset;
      }
  };

class Test : private Test_base
  {
    virtual bool test()
      {
        {
          typedef Bft_<> Bft;

          typedef Bitfield_w_fmt<Bitfield<Bft>, Fmt> Bwf;

          Bitfield_seq_storage_write_buf<
            Dummy_sa_t, Bitfield_seq_storage_write_default_traits> dsa(dsa_);

          op.push_back(Op_offset);
          op.push_back(0);
          op.push_back(Op_write);

          BITF(Bwf, dsa, f2) = 0;

          op.push_back(Op_offset);
          op.push_back(1);
          op.push_back(Op_write);

          BITF(Bwf, dsa, f4) = 0;

          op.push_back(Op_offset);
          op.push_back(2);
          op.push_back(Op_write);

          BITF(Bwf, dsa, f6) = 0;

          op.push_back(Op_offset);
          op.push_back(4);
          op.push_back(Op_write);
        }
        if (!reset_op())
          return(false);

        {
          typedef Bft_<Ssw_traits> Bft;

          typedef Bitfield_w_fmt<Bitfield<Bft>, Fmt> Bwf;

          Bitfield_seq_storage_write_buf<Dummy_sa_t, Ssw_traits> dsa(dsa_);

          op.push_back(Op_offset);
          op.push_back(0);
          op.push_back(Op_read);
          op.push_back(Op_offset);
          op.push_back(0);
          op.push_back(Op_write);

          BITF(Bwf, dsa, f2) = 0;

          op.push_back(Op_offset);
          op.push_back(1);
          op.push_back(Op_write);

          BITF(Bwf, dsa, f4) = 0;

          op.push_back(Op_offset);
          op.push_back(2);
          op.push_back(Op_write);
          op.push_back(Op_offset);
          op.push_back(4);
          op.push_back(Op_read);

          BITF(Bwf, dsa, f6) = 0;

          op.push_back(Op_offset);
          op.push_back(4);
          op.push_back(Op_write);
        }
        if (!reset_op())
          return(false);

        {
          typedef Bft_<Ssw_traits2> Bft;

          typedef Bitfield_w_fmt<Bitfield<Bft>, Fmt> Bwf;

          Bitfield_seq_storage_write_buf<Dummy_sa_t, Ssw_traits2> dsa(dsa_);

          op.push_back(Op_offset);
          op.push_back(1);
          op.push_back(Op_write);

          BITF(Bwf, dsa, f4) = 0;

          op.push_back(Op_offset);
          op.push_back(0);
          op.push_back(Op_write);

          BITF(Bwf, dsa, f2) = 0;

          if ((po != 2) or (no != 0))
            return(false);

          op.push_back(Op_offset);
          op.push_back(1);
          op.push_back(Op_write);
        }
        if (!reset_op())
          return(false);

        return(true);
      }
  };

Test t;

} // end namespce T2

namespace T3
{

class Fmt : private Bitfield_format
  {
  public:

    F<21> f1;
    F<15> f2;
    F<8> f3;
    F<18> f4;
    F<22> f5;
    F<4> f6;
    F<24> f7;
  };

typedef
  Bitfield_w_fmt<Bitfield_traits_default<uint32_t, uint16_t>, Fmt> Bwf_def;

struct Ssw_traits : public Bitfield_seq_storage_write_default_traits
  {
    enum { First_bit = BITF_OFFSET(Bwf_def, f2) };
    enum { End_bit = BITF_OFFSET(Bwf_def, f7) };

    static const bool Flush_on_destroy = false;
  };

class Test : private Test_base
  {
    virtual bool test()
      {
        {
          typedef Bft_<> Bft;

          typedef Bitfield_w_fmt<Bitfield<Bft>, Fmt> Bwf;

          Bitfield_seq_storage_write_buf<
            Dummy_sa_t, Bitfield_seq_storage_write_default_traits> dsa(dsa_);

          op.push_back(Op_offset);
          op.push_back(1);
          op.push_back(Op_write);

          BITF(Bwf, dsa, f2) = 0;

          op.push_back(Op_offset);
          op.push_back(2);
          op.push_back(Op_write);

          BITF(Bwf, dsa, f4) = 0;

          op.push_back(Op_offset);
          op.push_back(3);
          op.push_back(Op_write);

          BITF(Bwf, dsa, f6) = 0;

          op.push_back(Op_offset);
          op.push_back(5);
          op.push_back(Op_write);

          if (check_idx >= op.size())
            return(false);

          dsa.flush();

          if (!reset_op())
            return(false);
        }

        {
          typedef Bft_<Ssw_traits> Bft;

          typedef Bitfield_w_fmt<Bitfield<Bft>, Fmt> Bwf;

          Bitfield_seq_storage_write_buf<Dummy_sa_t, Ssw_traits> dsa(dsa_);

          op.push_back(Op_offset);
          op.push_back(1);
          op.push_back(Op_read);
          op.push_back(Op_offset);
          op.push_back(1);
          op.push_back(Op_write);

          BITF(Bwf, dsa, f2) = 0;

          op.push_back(Op_offset);
          op.push_back(2);
          op.push_back(Op_write);

          BITF(Bwf, dsa, f4) = 0;

          op.push_back(Op_offset);
          op.push_back(3);
          op.push_back(Op_write);
          op.push_back(Op_offset);
          op.push_back(5);
          op.push_back(Op_read);

          BITF(Bwf, dsa, f6) = 0;

          op.push_back(Op_offset);
          op.push_back(5);
          op.push_back(Op_write);

          dsa.flush();

          if (!reset_op())
            return(false);
        }

        {
          typedef Bft_<Ssw_traits> Bft;

          typedef Bitfield_w_fmt<Bitfield<Bft>, Fmt> Bwf;

          Bitfield_seq_storage_write_buf<Dummy_sa_t, Ssw_traits> dsa(dsa_);

          op.push_back(Op_offset);
          op.push_back(1);
          op.push_back(Op_read);
          op.push_back(Op_offset);
          op.push_back(1);
          op.push_back(Op_write);

          BITF(Bwf, dsa, f2) = 0;

          op.push_back(Op_offset);
          op.push_back(2);
          op.push_back(Op_write);

          BITF(Bwf, dsa, f4) = 0;

          op.push_back(Op_offset);
          op.push_back(3);
          op.push_back(Op_write);
          op.push_back(Op_offset);
          op.push_back(5);
          op.push_back(Op_read);

          BITF(Bwf, dsa, f6) = 0;

          dsa.discard();

          if (!reset_op())
            return(false);
        }

        return(true);
      }
  };

Test t;

} // end namespce T3

} // end namespace Test_write_seq
