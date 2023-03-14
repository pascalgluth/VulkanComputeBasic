#version 450

struct Calculation
{
    float f1;
    float f2;
    float res;
};

layout(std140, binding = 0) buffer buf
{
    Calculation calc;
};

void main()
{
    calc.res = calc.f1 + calc.f2;
}