#pragma once

class Utils_t
{
public:
    static uint32_t div_ceil_uint(uint32_t a, uint32_t b)
    {
        return (a + b -1) / b;
    }
};
