#pragma once

#include <cstdint>
#include <cstdlib>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int32_t i64;

#define LIKELY(x) __builtin_expect(static_cast<bool>(x), 1)
#define UNLIKELY(x) __builtin_expect(static_cast<bool>(x), 0)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define Min(a, b) (((a) < (b)) ? (a) : (b))
#define Max(a, b) (((a) > (b)) ? (a) : (b))
#define Clamp(x, l, u) Min(Max(x, l), u)

template <typename T> class __Deferrer {
public:
  __Deferrer(T f) : f_(f) {}
  ~__Deferrer() { f_(); }

private:
  T f_;
};

#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)
#define defer(x)                                                               \
  __Deferrer CONCAT(__defferer, __COUNTER__) {                                 \
    [&] { x; }                                                                 \
  }

template <u64 kSize = 256> class string {
public:
  constexpr string() { m_v[0] = '\0'; }

  template <u64 N> constexpr string(const char (&v)[N]) {
    static_assert(N <= kSize);
    __builtin_memcpy(m_v, v, N);
  }

  constexpr string(const char *v) { __builtin_snprintf(m_v, kSize, "%s", v); }
  constexpr const char *str() const { return m_v; }
  constexpr char &operator[](const u32 i) { return m_v[i]; }

private:
  char m_v[kSize];
};

#define sprintn(n, fmt, ...)                                                   \
  [](const auto &...args) -> string<n> {                                       \
    string<n> _v;                                                              \
    const i64 _n = __builtin_snprintf(&_v[0], n, fmt, args...);                \
    _v[Clamp(_n, 0, n - 1)] = '\0';                                            \
    return _v;                                                                 \
  }(__VA_ARGS__);
