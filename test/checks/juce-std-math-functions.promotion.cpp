extern "C"
{
double sin(double);
float sinf(float);
double fabs(double);
}

namespace std
{
using ::sin;
}

void promotions(float f, double d)
{
    (void) sin(f);
    (void) fabs(f);

    (void) sinf(f); // EXPECT_WARNING

    (void) sin(d); // EXPECT_WARNING
}
