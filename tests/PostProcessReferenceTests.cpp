//
// Created by chenlong on 2026/10/9.
//

#include <cmath>
#include <iostream>


double ReferenceOutput(const double hdrVal, const double exposure)
{
    double nonNegative = hdrVal * exposure;
    if (nonNegative < 0.0)
        nonNegative = 0.0;

    const double mapped = nonNegative / (1.0 + nonNegative);

    if (mapped <= 0.0031308)
        return mapped * 12.92;

    return 1.055 * std::pow(mapped, 1.0 / 2.4) - 0.055;
}

bool CheckNear(const char* name, double actual, double expected)
{
    constexpr double tolerance = 0.00000001;

    if (std::abs(actual - expected) <= tolerance)
        return true;

    std::cerr << name << ": expected " << expected
              << ", got " << actual << '\n';
    return false;
}

int main()
{
    const bool zeroExposure = CheckNear(
        "zero exposure", ReferenceOutput(3.0, 0.0), 0.0
    );
    const bool middleValue = CheckNear(
        "HDR 1", ReferenceOutput(1.0, 1.0), 0.735356983
    );
    const bool brightValue = CheckNear(
        "HDR 3", ReferenceOutput(3.0, 1.0), 0.880825021
    );
    const bool negativeValue = CheckNear(
        "negative HDR", ReferenceOutput(-0.5, 1.0), 0.0
    );
    const bool breakpoint = CheckNear(
        "sRGB breakpoint",
        ReferenceOutput(0.00314184024364309, 1.0),
        0.040465149
    );

    return zeroExposure && middleValue && brightValue &&
           negativeValue && breakpoint ? 0 : 1;
}