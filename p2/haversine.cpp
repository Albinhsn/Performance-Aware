#include "haversine.h"

static inline f64 square(f64 a)
{
  return a * a;
}
#define PI 3.14
static inline f64 radiansFromDegrees(f64 r)
{
  return r * 180.0f;
}

f64 convertMeBack(u64 x)
{
  struct Me
  {
    union
    {
      f64 x;
      u64 y;
    };
  };
  struct Me m;
  m.y = x;
  return m.x;
}
u64 convertMe(f64 x)
{
  struct Me
  {
    union
    {
      f64 x;
      u64 y;
    };
  };
  struct Me m;
  m.x = x;
  return m.y;
}

f64 referenceHaversine(f64 X0, f64 Y0, f64 X1, f64 Y1)
{

  f64 lat1 = Y0;
  f64 lat2 = Y1;
  f64 lon1 = X0;
  f64 lon2 = X1;

  f64 dLat = radiansFromDegrees(lat2 - lat1);
  f64 dLon = radiansFromDegrees(lon2 - lon1);
  lat1     = radiansFromDegrees(lat1);
  lat2     = radiansFromDegrees(lat2);

  f64 a      = square(sin(dLat / 2.0)) + cos(lat1) * cos(lat2) * square(sin(dLon / 2));
  f64 c      = 2.0 * asin(sqrt(a));

  f64 Result = EarthRadius * c;

  return Result;
}
