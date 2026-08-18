#pragma once

#include <telkin/Telkin.h>
#include <math/seadMathCalcCommon.h>

namespace ucology {

// Ease along a function.
class Easer {
public:
    enum class EaseType {
        SineOut,
        SineIn
    };

    void set(EaseType easeType, float start, float end, float stepPercent) {
        mEaseType = easeType;
        mStart = start;
        mEnd = end;
        mInited = true;
        mStep = sead::Mathf::abs(valueFromPercent(stepPercent, start, end) - start);
        mIteration = 0.0f;
        mTotalIterations = abs(mEnd - mStart) / mStep;
    }

    /// @return If the value has reached the target value.
    bool ease(float& out) {
        if (!mInited) {
            tk::fatal("attempted to ease with an easer that has not been inited");
            return true;
        }

        mIteration += 1.0f;
        
        float progress = sead::Mathf::abs(percentFromValue(mStart + mStep * mIteration, mStart, mEnd));

        float (* func)(float) = nullptr;

        switch (mEaseType) {
            case EaseType::SineOut: {
                func = &sineOut;
                break;
            }

            case EaseType::SineIn: {
                func = &sineIn;
                break;
            }
        }
        
        float next = mStart + func(progress) * (mEnd - mStart);

        if (mIteration < mTotalIterations) {
            out = next;
            return false;
        }

        out = mEnd;
        return true;
    }
private:
    static float valueFromPercent(float percent, float min, float max) {
        return (percent * 100.0f * (max - min) / 100.0f) + min;
    }

    static float percentFromValue(float value, float min, float max) {
        return (((value - min) * 100.0f) / (max - min)) / 100.0f;
    }

    static float sineOut(float x) {
        return sead::Mathf::sin((x * sead::Mathf::pi()) / 2.0f);
    }

    static float sineIn(float x) {
        return 1.0f - sead::Mathf::cos((x * sead::Mathf::pi()) / 2.0f);
    }
private:
    EaseType mEaseType;
    bool mInited = false;
    float mStart, mEnd, mStep, mIteration, mTotalIterations;
};

}