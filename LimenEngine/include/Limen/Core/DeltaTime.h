//
// Created by chenlong on 2026/8/12.
//

#pragma once

namespace Limen
{
    class DeltaTime
    {
    public:
        DeltaTime(const float time = 0.f)
            : m_Time(time)
        {
        }

        DeltaTime(const double time = 0.f)
            : m_Time(static_cast<float>(time))
        {
        }

        operator float() const { return m_Time; }

        DeltaTime &operator +=(const float time)
        {
            m_Time += time;
            return *this;
        }

        DeltaTime &operator -=(const float time)
        {
            m_Time -= time;
            return *this;
        }

        DeltaTime &operator =(const float time)
        {
            m_Time = time;
            return *this;
        }

        DeltaTime &operator =(const double time)
        {
            m_Time = static_cast<float>(time);
            return *this;
        }


        DeltaTime &operator *=(const float time)
        {
            m_Time *= time;
            return *this;
        }

        DeltaTime &operator /=(const float time)
        {
            m_Time /= time;
            return *this;
        }

        float operator *(const float time) const
        {
            return time * m_Time;
        }

        float GetSeconds() const { return m_Time; }
        float GetMilliseconds() const { return m_Time * 1000.0f; }

    private:
        // 当前时间间隔，单位为秒。
        float m_Time;
    };
}
