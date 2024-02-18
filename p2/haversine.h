#ifndef HAVERSINE_H
#define HAVERSINE_H
#include "lib/common.h"
#include <math.h>
#include <stdio.h>


#define EarthRadius 6372.8
u64 convertMe(f64 x);
f64 convertMeBack(u64 x);
f64 referenceHaversine(f64 X0, f64 Y0, f64 X1, f64 Y1);

#endif
