﻿
#pragma once

namespace MP
{
    namespace Internal
    {

        template <typename T> class Flag
        {
            struct Dummy
            {
                constexpr Dummy()
                {
                }
                friend constexpr void adl_flag(Dummy);
            };

            template <bool>
            struct Writer
            {
                friend constexpr void adl_flag(Dummy)
                {
                }
            };

            template <class Dummy, int = (adl_flag(Dummy{}), 0) >
            static constexpr bool Check(int)
            {
                return true;
            }

            template <class Dummy>
            static constexpr bool Check(short)
            {
                return false;
            }

        public:

            template <class Dummy = Dummy, bool Value = Check<Dummy>(0)>
            static constexpr bool ReadSet()
            {
                Writer<Value && 0> tmp{};
                (void)tmp;
                return Value;
            }

            template <class Dummy = Dummy, bool Value = Check<Dummy>(0)>
            static constexpr int Read()
            {
                return Value;
            }
        };

        template <typename T, int I>
        struct Tag
        {

            constexpr int value() const noexcept
            {
                return I;
            }
        };

        template<typename T, int N, int Step, bool B>
        struct Checker
        {
            static constexpr int currentval() noexcept
            {
                return N;
            }
        };

        template<typename T, int N, int Step>
        struct CheckerWrapper
        {
            template<bool B = Flag<Tag<T, N>>{}.Read(), int M = Checker<T, N, Step, B>{}.currentval() >
            static constexpr int currentval()
            {
                return M;
            }
        };

        template<typename T, int N, int Step>
        struct Checker<T, N, Step, true>
        {
            template<int M = CheckerWrapper<T, N + Step, Step>{}.currentval() >
            static constexpr int currentval() noexcept
            {
                return M;
            }
        };

        template<typename T, int N, bool B = Flag<Tag<T, N>>{}.ReadSet() >
        struct Next
        {
            static constexpr int value() noexcept
            {
                return N;
            }
        };
    }

    template<typename Tag = void, int32 Start = 0, int32 Step = 1>
    class TConstexprCounter
    {
    public:
        template <int32 N = Internal::CheckerWrapper<Tag, Start, Step>{}.currentval()>
        static constexpr int32 Next()
        {
            return Internal::Next<Tag, N>{}.value();
        }

        template<int32 N = Internal::CheckerWrapper<Tag, Start, Step>{}.currentval()>
        static constexpr int32 Count()
        {
            return N;
        }
    };
}
